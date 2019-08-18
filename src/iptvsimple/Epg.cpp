#include "Epg.h"

#include "Settings.h"
#include "../client.h"
#include "utilities/FileUtils.h"
#include "utilities/Logger.h"

#include "p8-platform/util/StringUtils.h"
#include "rapidxml/rapidxml.hpp"

#include <chrono>
#include <regex>
#include <thread>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;
using namespace rapidxml;

template<class Ch>
inline std::string GetNodeValue(const xml_node<Ch>* pRootNode, const char* strTag)
{
  xml_node<Ch>* pChildNode = pRootNode->first_node(strTag);
  if (!pChildNode)
    return "";

  return pChildNode->value();
}

template<class Ch>
inline bool GetAttributeValue(const xml_node<Ch>* pNode, const char* strAttributeName, std::string& strStringValue)
{
  xml_attribute<Ch>* pAttribute = pNode->first_attribute(strAttributeName);
  if (!pAttribute)
  {
    return false;
  }
  strStringValue = pAttribute->value();
  return true;
}

namespace
{

// Adapted from https://stackoverflow.com/a/31533119

// Conversion from UTC date to second, signed 64-bit adjustable epoch version.
// Written by François Grieu, 2015-07-21; public domain.

long long MakeTime(int year, int month, int day)
{
  return static_cast<long long>(year) * 365 + year / 4 - year / 100 * 3 / 4 + (month + 2) * 153 / 5 + day;
}

long long GetUTCTime(int year, int mon, int mday, int hour, int min, int sec)
{
  int m = mon - 1;
  int y = year + 100;

  if (m < 2)
  {
    m += 12;
    --y;
  }

  return (((MakeTime(y, m, mday) - MakeTime(1970 + 99, 12, 1)) * 24 + hour) * 60 + min) * 60 + sec;
}

long long ParseDateTime(const std::string& strDate)
{
  int year = 2000;
  int mon = 1;
  int mday = 1;
  int hour = 0;
  int min = 0;
  int sec = 0;
  char offset_sign = '+';
  int offset_hours = 0;
  int offset_minutes = 0;

  sscanf(strDate.c_str(), "%04d%02d%02d%02d%02d%02d %c%02d%02d", &year, &mon, &mday, &hour, &min, &sec, &offset_sign, &offset_hours, &offset_minutes);

  long offset_of_date = (offset_hours * 60 + offset_minutes) * 60;
  if (offset_sign == '-')
    offset_of_date = -offset_of_date;

  return GetUTCTime(year, mon, mday, hour, min, sec) - offset_of_date;
}

} // unnamed namespace

Epg::Epg(Channels& channels)
      : m_channels(channels)
{
  m_strXMLTVUrl = Settings::GetInstance().GetTvgPath();
  m_iEPGTimeShift = Settings::GetInstance().GetEpgTimeshift();
  m_bTSOverride = Settings::GetInstance().GetTsOverride();
  m_iLastStart = 0;
  m_iLastEnd = 0;
}

void Epg::Clear()
{
  m_channelEpgs.clear();
  m_genres.clear();
}

bool Epg::LoadEPG(time_t iStart, time_t iEnd)
{
  if (m_strXMLTVUrl.empty())
  {
    Logger::Log(LEVEL_NOTICE, "EPG file path is not configured. EPG not loaded.");
    return false;
  }

  std::string data;
  std::string decompressed;
  int iReaded = 0;

  int iCount = 0;
  while (iCount < 3) // max 3 tries
  {
    if ((iReaded = FileUtils::GetCachedFileContents(Settings::GetInstance().GetUserPath(), TVG_FILE_NAME, m_strXMLTVUrl, data, Settings::GetInstance().UseEPGCache())) != 0)
      break;

    Logger::Log(LEVEL_ERROR, "Unable to load EPG file '%s':  file is missing or empty. :%dth try.", m_strXMLTVUrl.c_str(), ++iCount);

    if (iCount < 3)
      std::this_thread::sleep_for(std::chrono::microseconds(2 * 1000 * 1000)); // sleep 2 sec before next try.
  }

  if (iReaded == 0)
  {
    Logger::Log(LEVEL_ERROR, "Unable to load EPG file '%s':  file is missing or empty. After %d tries.", m_strXMLTVUrl.c_str(), iCount);
    return false;
  }

  char* buffer;

  // gzip packed
  if (data[0] == '\x1F' && data[1] == '\x8B' && data[2] == '\x08')
  {
    if (!FileUtils::GzipInflate(data, decompressed))
    {
      Logger::Log(LEVEL_ERROR, "Invalid EPG file '%s': unable to decompress file.", m_strXMLTVUrl.c_str());
      return false;
    }
    buffer = &(decompressed[0]);
  }
  else
    buffer = &(data[0]);

  // xml should starts with '<?xml'
  if (buffer[0] != '\x3C' || buffer[1] != '\x3F' || buffer[2] != '\x78' ||
      buffer[3] != '\x6D' || buffer[4] != '\x6C')
  {
    // check for BOM
    if (buffer[0] != '\xEF' || buffer[1] != '\xBB' || buffer[2] != '\xBF')
    {
      // check for tar archive
      if (strcmp(buffer + 0x101, "ustar") || strcmp(buffer + 0x101, "GNUtar"))
        buffer += 0x200; // RECORDSIZE = 512
      else
      {
        Logger::Log(LEVEL_ERROR, "Invalid EPG file '%s': unable to parse file.", m_strXMLTVUrl.c_str());
        return false;
      }
    }
  }

  xml_document<> xmlDoc;
  try
  {
    xmlDoc.parse<0>(buffer);
  }
  catch (parse_error p)
  {
    Logger::Log(LEVEL_ERROR, "Unable parse EPG XML: %s", p.what());
    return false;
  }

  xml_node<>* pRootElement = xmlDoc.first_node("tv");
  if (!pRootElement)
  {
    Logger::Log(LEVEL_ERROR, "Invalid EPG XML: no <tv> tag found");
    return false;
  }

  // clear previously loaded epg
  if (m_channelEpgs.size() > 0)
    m_channelEpgs.clear();

  int iBroadCastId = 0;
  xml_node<>* pChannelNode = nullptr;
  for (pChannelNode = pRootElement->first_node("channel"); pChannelNode; pChannelNode = pChannelNode->next_sibling("channel"))
  {
    std::string strName;
    std::string strId;
    if (!GetAttributeValue(pChannelNode, "id", strId))
      continue;

    strName = GetNodeValue(pChannelNode, "display-name");
    if (!m_channels.FindChannel(strId, strName))
      continue;

    ChannelEpg channelEpg;
    channelEpg.SetId(strId);
    channelEpg.SetName(strName);

    // get icon if available
    xml_node<>* pIconNode = pChannelNode->first_node("icon");
    std::string icon = channelEpg.GetIcon();
    if (!pIconNode || !GetAttributeValue(pIconNode, "src", icon))
      channelEpg.SetIcon("");
    else
      channelEpg.SetIcon(icon);

    m_channelEpgs.push_back(channelEpg);
  }

  if (m_channelEpgs.size() == 0)
  {
    Logger::Log(LEVEL_ERROR, "EPG channels not found.");
    return false;
  }

  int iMinShiftTime = m_iEPGTimeShift;
  int iMaxShiftTime = m_iEPGTimeShift;
  if (!m_bTSOverride)
  {
    iMinShiftTime = SECONDS_IN_DAY;
    iMaxShiftTime = -SECONDS_IN_DAY;

    for (const auto& channel : m_channels.GetChannelsList())
    {
      if (channel.GetTvgShift() + m_iEPGTimeShift < iMinShiftTime)
        iMinShiftTime = channel.GetTvgShift() + m_iEPGTimeShift;
      if (channel.GetTvgShift() + m_iEPGTimeShift > iMaxShiftTime)
        iMaxShiftTime = channel.GetTvgShift() + m_iEPGTimeShift;
    }
  }

  ChannelEpg* channelEpg = nullptr;
  for (pChannelNode = pRootElement->first_node("programme"); pChannelNode; pChannelNode = pChannelNode->next_sibling("programme"))
  {
    std::string strId;
    if (!GetAttributeValue(pChannelNode, "channel", strId))
      continue;

    if (!channelEpg || StringUtils::CompareNoCase(channelEpg->GetId(), strId) != 0)
    {
      if (!(channelEpg = FindEpgForChannel(strId)))
        continue;
    }

    std::string strStart, strStop;
    if (!GetAttributeValue(pChannelNode, "start", strStart) || !GetAttributeValue(pChannelNode, "stop", strStop))
      continue;

    long long iTmpStart = ParseDateTime(strStart);
    long long iTmpEnd = ParseDateTime(strStop);

    if ((iTmpEnd + iMaxShiftTime < iStart) || (iTmpStart + iMinShiftTime > iEnd))
      continue;

    EpgEntry entry;
    entry.SetBroadcastId(++iBroadCastId);
    entry.SetChannelId(atoi(strId.c_str()));
    entry.SetGenreType(0);
    entry.SetGenreSubType(0);
    entry.SetPlotOutline("");
    entry.SetStartTime(static_cast<time_t>(iTmpStart));
    entry.SetEndTime(static_cast<time_t>(iTmpEnd));

    entry.SetTitle(GetNodeValue(pChannelNode, "title"));
    entry.SetPlot(GetNodeValue(pChannelNode, "desc"));
    entry.SetGenreString(GetNodeValue(pChannelNode, "category"));
    entry.SetEpisodeName(GetNodeValue(pChannelNode, "sub-title"));

    xml_node<> *pCreditsNode = pChannelNode->first_node("credits");
    if (pCreditsNode != NULL) {
        entry.SetCast(GetNodeValue(pCreditsNode, "actor"));
	      entry.SetDirector(GetNodeValue(pCreditsNode, "director"));
	      entry.SetWriter(GetNodeValue(pCreditsNode, "writer"));
    }

    xml_node<>* pIconNode = pChannelNode->first_node("icon");
    std::string iconPath;
    if (!pIconNode || !GetAttributeValue(pIconNode, "src", iconPath))
      entry.SetIconPath("");
    else
      entry.SetIconPath(iconPath);

    channelEpg->GetEpgEntries().push_back(entry);
  }

  xmlDoc.clear();
  LoadGenres();

  Logger::Log(LEVEL_NOTICE, "EPG Loaded.");

  if (Settings::GetInstance().GetEpgLogos() > 0)
    ApplyChannelsLogosFromEPG();

  return true;
}

void Epg::ReloadEPG(const char* strNewPath)
{
  //P8PLATFORM::CLockObject lock(m_mutex);
  //TODO Lock should happen in calling class
  if (strNewPath != m_strXMLTVUrl)
  {
    m_strXMLTVUrl = strNewPath;
    Clear();

    if (LoadEPG(m_iLastStart, m_iLastEnd))
    {
      for (const auto& myChannel : m_channels.GetChannelsList())
      {
        PVR->TriggerEpgUpdate(myChannel.GetUniqueId());
      }
    }
  }
}

PVR_ERROR Epg::GetEPGForChannel(ADDON_HANDLE handle, int iChannelUid, time_t iStart, time_t iEnd)
{
  for (const auto& myChannel : m_channels.GetChannelsList())
  {
    if (myChannel.GetUniqueId() != iChannelUid)
      continue;

    if (iStart > m_iLastStart || iEnd > m_iLastEnd)
    {
      // reload EPG for new time interval only
      LoadEPG(iStart, iEnd);
      {
        // doesn't matter is epg loaded or not we shouldn't try to load it for same interval
        m_iLastStart = static_cast<int>(iStart);
        m_iLastEnd = static_cast<int>(iEnd);
      }
    }

    ChannelEpg* channelEpg = FindEpgForChannel(myChannel);
    if (!channelEpg || channelEpg->GetEpgEntries().size() == 0)
      return PVR_ERROR_NO_ERROR;

    int iShift = m_bTSOverride ? m_iEPGTimeShift : myChannel.GetTvgShift() + m_iEPGTimeShift;

    for (auto& epgEntry : channelEpg->GetEpgEntries())
    {
      if ((epgEntry.GetEndTime() + iShift) < iStart)
        continue;

      EPG_TAG tag;
      memset(&tag, 0, sizeof(EPG_TAG));

      epgEntry.UpdateTo(tag, iChannelUid, iShift, m_genres);

      PVR->TransferEpgEntry(handle, &tag);

      if ((epgEntry.GetStartTime() + iShift) > iEnd)
        break;
    }

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_NO_ERROR;
}

ChannelEpg* Epg::FindEpgForChannel(const std::string& strId)
{
  for (auto& myChannelEpg : m_channelEpgs)
  {
    if (StringUtils::CompareNoCase(myChannelEpg.GetId(), strId) == 0)
      return &myChannelEpg;
  }

  return nullptr;
}

ChannelEpg* Epg::FindEpgForChannel(const Channel& channel)
{
  for (auto& myChannelEpg : m_channelEpgs)
  {
    if (myChannelEpg.GetId() == channel.GetTvgId())
      return &myChannelEpg;

    const std::string strName = std::regex_replace(myChannelEpg.GetName(), std::regex(" "), "_");
    if (strName == channel.GetTvgName() || myChannelEpg.GetName() == channel.GetTvgName())
      return &myChannelEpg;

    if (myChannelEpg.GetName() == channel.GetChannelName())
      return &myChannelEpg;
  }

  return nullptr;
}

void Epg::ApplyChannelsLogosFromEPG()
{
  bool bUpdated = false;

  for (auto& channel : m_channels.GetChannelsList())
  {
    const ChannelEpg* channelEpg = FindEpgForChannel(channel);
    if (!channelEpg || channelEpg->GetIcon().empty())
      continue;

    // 1 - prefer logo from playlist
    if (!channel.GetLogoPath().empty() && Settings::GetInstance().GetEpgLogos() == 1)
      continue;

    // 2 - prefer logo from epg
    if (!channelEpg->GetIcon().empty() && Settings::GetInstance().GetEpgLogos() == 2)
    {
      channel.SetLogoPath(channelEpg->GetIcon());
      bUpdated = true;
    }
  }

  if (bUpdated)
    PVR->TriggerChannelUpdate();
}

bool Epg::LoadGenres()
{
  std::string data;

  // try to load genres from userdata folder
  std::string strFilePath = FileUtils::GetUserFilePath(Settings::GetInstance().GetUserPath(), GENRES_MAP_FILENAME);
  if (!XBMC->FileExists(strFilePath.c_str(), false))
  {
    // try to load file from addom folder
    strFilePath = FileUtils::GetClientFilePath(Settings::GetInstance().GetClientPath(), GENRES_MAP_FILENAME);
    if (!XBMC->FileExists(strFilePath.c_str(), false))
      return false;
  }

  FileUtils::GetFileContents(strFilePath, data);

  if (data.empty())
    return false;

  m_genres.clear();

  char* buffer = &(data[0]);
  xml_document<> xmlDoc;
  try
  {
    xmlDoc.parse<0>(buffer);
  }
  catch (parse_error p)
  {
    return false;
  }

  xml_node<>* pRootElement = xmlDoc.first_node("genres");
  if (!pRootElement)
    return false;

  for (xml_node<>* pGenreNode = pRootElement->first_node("genre"); pGenreNode; pGenreNode = pGenreNode->next_sibling("genre"))
  {
    std::string buff;
    if (!GetAttributeValue(pGenreNode, "type", buff))
      continue;

    if (!StringUtils::IsNaturalNumber(buff))
      continue;

    EpgGenre genre;
    genre.SetGenreString(pGenreNode->value());
    genre.SetGenreType(atoi(buff.c_str()));
    genre.SetGenreSubType(0);

    if (GetAttributeValue(pGenreNode, "subtype", buff) && StringUtils::IsNaturalNumber(buff))
      genre.SetGenreSubType(atoi(buff.c_str()));

    m_genres.push_back(genre);
  }

  xmlDoc.clear();
  return true;
}