#include "PlaylistLoader.h"

#include "Settings.h"
#include "../client.h"
#include "utilities/FileUtils.h"
#include "utilities/Logger.h"

#include "p8-platform/util/StringUtils.h"

#include <map>
#include <regex>
#include <sstream>
#include <vector>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

PlaylistLoader::PlaylistLoader(Channels& channels, ChannelGroups& channelGroups)
      : m_channels(channels), m_channelGroups(channelGroups)
{
  m_strM3uUrl = Settings::GetInstance().GetM3UPath();
}

bool PlaylistLoader::LoadPlayList(void)
{
  if (m_strM3uUrl.empty())
  {
    Logger::Log(LEVEL_NOTICE, "Playlist file path is not configured. Channels not loaded.");
    return false;
  }

  std::string strPlaylistContent;
  if (!FileUtils::GetCachedFileContents(Settings::GetInstance().GetUserPath(), M3U_FILE_NAME, m_strM3uUrl, strPlaylistContent, Settings::GetInstance().UseM3UCache()))
  {
    Logger::Log(LEVEL_ERROR, "Unable to load playlist file '%s':  file is missing or empty.", m_strM3uUrl.c_str());
    return false;
  }

  std::stringstream stream(strPlaylistContent);

  /* load channels */
  bool bFirst        = true;
  bool bIsRealTime   = true;
  int iChannelIndex  = 0;
  int iUniqueGroupId = 0;
  int iChannelNum    = Settings::GetInstance().GetStartNumber();
  int iEPGTimeShift  = 0;
  std::vector<int> iCurrentGroupId;

  Channel tmpChannel;
  tmpChannel.SetTvgId("");
  tmpChannel.SetChannelName("");
  tmpChannel.SetTvgName("");
  tmpChannel.SetTvgLogo("");
  tmpChannel.SetTvgShift(0);

  std::string strLine;
  while (std::getline(stream, strLine))
  {
    strLine = StringUtils::TrimRight(strLine, " \t\r\n");
    strLine = StringUtils::TrimLeft(strLine, " \t");

    Logger::Log(LEVEL_DEBUG, "Read line: '%s'", strLine.c_str());

    if (strLine.empty())
    {
      continue;
    }

    if (bFirst)
    {
      bFirst = false;

      if (StringUtils::Left(strLine, 3) == "\xEF\xBB\xBF")
        strLine.erase(0, 3);

      if (StringUtils::StartsWith(strLine, M3U_START_MARKER))
      {
        double fTvgShift = atof(ReadMarkerValue(strLine, TVG_INFO_SHIFT_MARKER).c_str());
        iEPGTimeShift = (int)(fTvgShift * 3600.0);
        continue;
      }
      else
      {
        Logger::Log(LEVEL_ERROR,
                  "URL '%s' missing %s descriptor on line 1, attempting to "
                  "parse it anyway.",
                  m_strM3uUrl.c_str(), M3U_START_MARKER.c_str());
      }
    }

    if (StringUtils::StartsWith(strLine, M3U_INFO_MARKER))
    {
      bool        bRadio       = false;
      double      fTvgShift    = 0;
      std::string strChnlNo    = "";
      std::string strChnlName  = "";
      std::string strTvgId     = "";
      std::string strTvgName   = "";
      std::string strTvgLogo   = "";
      std::string strTvgShift  = "";
      std::string strGroupName = "";
      std::string strRadio     = "";

      // parse line
      int iColon = static_cast<int>(strLine.find(':'));
      int iComma = static_cast<int>(strLine.rfind(','));
      if (iColon >= 0 && iComma >= 0 && iComma > iColon)
      {
        // parse name
        iComma++;
        strChnlName = StringUtils::Right(strLine, static_cast<int>(strLine.size() - iComma));
        strChnlName = StringUtils::Trim(strChnlName);
        tmpChannel.SetChannelName(XBMC->UnknownToUTF8(strChnlName.c_str()));

        // parse info
        iColon++;
        iComma--;
        const std::string strInfoLine = StringUtils::Mid(strLine, iColon, iComma - iColon);

        strTvgId      = ReadMarkerValue(strInfoLine, TVG_INFO_ID_MARKER);
        strTvgName    = ReadMarkerValue(strInfoLine, TVG_INFO_NAME_MARKER);
        strTvgLogo    = ReadMarkerValue(strInfoLine, TVG_INFO_LOGO_MARKER);
        strChnlNo     = ReadMarkerValue(strInfoLine, TVG_INFO_CHNO_MARKER);
        strGroupName  = ReadMarkerValue(strInfoLine, GROUP_NAME_MARKER);
        strRadio      = ReadMarkerValue(strInfoLine, RADIO_MARKER);
        strTvgShift   = ReadMarkerValue(strInfoLine, TVG_INFO_SHIFT_MARKER);

        if (strTvgId.empty())
        {
          char buff[255];
          sprintf(buff, "%d", atoi(strInfoLine.c_str()));
          strTvgId.append(buff);
        }

        if (strTvgLogo.empty())
          strTvgLogo = strChnlName;

        if (!strChnlNo.empty())
          iChannelNum = atoi(strChnlNo.c_str());

        fTvgShift = atof(strTvgShift.c_str());

        bRadio                = !StringUtils::CompareNoCase(strRadio, "true");
        tmpChannel.SetTvgId(strTvgId);
        tmpChannel.SetTvgName(XBMC->UnknownToUTF8(strTvgName.c_str()));
        tmpChannel.SetTvgLogo(XBMC->UnknownToUTF8(strTvgLogo.c_str()));
        tmpChannel.SetTvgShift(static_cast<int>(fTvgShift * 3600.0));
        tmpChannel.SetRadio(bRadio);

        if (strTvgShift.empty())
          tmpChannel.SetTvgShift(iEPGTimeShift);

        if (!strGroupName.empty())
        {
          std::stringstream streamGroups(strGroupName);
          iCurrentGroupId.clear();

          while (std::getline(streamGroups, strGroupName, ';'))
          {
            strGroupName = XBMC->UnknownToUTF8(strGroupName.c_str());
            const ChannelGroup* pGroup = m_channelGroups.FindChannelGroup(strGroupName);

            if (!pGroup)
            {
              ChannelGroup group;
              group.SetGroupName(strGroupName);
              group.SetGroupId(++iUniqueGroupId);
              group.SetRadio(bRadio);

              m_channelGroups.GetChannelGroupsList().push_back(group);
              iCurrentGroupId.push_back(iUniqueGroupId);
            }
            else
            {
              iCurrentGroupId.push_back(pGroup->GetGroupId());
            }
          }
        }
      }
    }
    else if (StringUtils::StartsWith(strLine, KODIPROP_MARKER))
    {
      const std::string value = ReadMarkerValue(strLine, KODIPROP_MARKER);
      auto pos = value.find('=');
      if (pos != std::string::npos)
      {
        const std::string prop = value.substr(0, pos);
        const std::string propValue = value.substr(pos + 1);
        tmpChannel.GetProperties().insert({prop, propValue});
      }
    }
    else if (StringUtils::StartsWith(strLine, EXTVLCOPT_MARKER))
    {
      const std::string value = ReadMarkerValue(strLine, EXTVLCOPT_MARKER);
      auto pos = value.find('=');
      if (pos != std::string::npos)
      {
        const std::string prop = value.substr(0, pos);
        const std::string propValue = value.substr(pos + 1);
        tmpChannel.GetProperties().insert({prop, propValue});

        Logger::Log(LEVEL_DEBUG, "Found #EXTVLCOPT property: '%s' value: '%s'", prop.c_str(), propValue.c_str());
      }
    }
    else if (StringUtils::StartsWith(strLine, PLAYLIST_TYPE_MARKER))
    {
      if (ReadMarkerValue(strLine, PLAYLIST_TYPE_MARKER) == "VOD")
        bIsRealTime = false;
    }
    else if (strLine[0] != '#')
    {
      Logger::Log(LEVEL_DEBUG, "Found URL: '%s' (current channel name: '%s')", strLine.c_str(), tmpChannel.GetChannelName().c_str());

      if (bIsRealTime)
        tmpChannel.GetProperties().insert({PVR_STREAM_PROPERTY_ISREALTIMESTREAM, "true"});

      Channel channel;
      channel.SetUniqueId(GetChannelId(tmpChannel.GetChannelName().c_str(), strLine.c_str()));
      channel.SetChannelNumber(iChannelNum);
      channel.SetTvgId(tmpChannel.GetTvgId());
      channel.SetChannelName(tmpChannel.GetChannelName());
      channel.SetTvgName(tmpChannel.GetTvgName());
      channel.SetTvgLogo(tmpChannel.GetTvgLogo());
      channel.SetTvgShift(tmpChannel.GetTvgShift());
      channel.SetRadio(tmpChannel.IsRadio());
      channel.SetProperties(tmpChannel.GetProperties());
      channel.SetStreamURL(strLine);
      channel.SetEncryptionSystem(0);

      iChannelNum++;

      for (int myGroupId : iCurrentGroupId)
      {
        channel.SetRadio(m_channelGroups.GetChannelGroupsList().at(myGroupId - 1).IsRadio());
        m_channelGroups.GetChannelGroupsList().at(myGroupId - 1).GetMemberChannels().push_back(iChannelIndex);
      }

      m_channels.GetChannelsList().push_back(channel);
      iChannelIndex++;

      tmpChannel.SetTvgId("");
      tmpChannel.SetChannelName("");
      tmpChannel.SetTvgName("");
      tmpChannel.SetTvgLogo("");
      tmpChannel.SetTvgShift(0);
      tmpChannel.SetRadio(false);
      tmpChannel.GetProperties().clear();
      bIsRealTime = true;
    }
  }

  stream.clear();

  if (m_channels.GetChannelsAmount() == 0)
  {
    Logger::Log(LEVEL_ERROR, "Unable to load channels from file '%s':  file is corrupted.", m_strM3uUrl.c_str());
    return false;
  }

  m_channels.ApplyChannelLogos();

  Logger::Log(LEVEL_NOTICE, "Loaded %d channels.", m_channels.GetChannelsAmount());
  return true;
}

void PlaylistLoader::ReloadPlayList(const char* strNewPath)
{
  //P8PLATFORM::CLockObject lock(m_mutex);
  //TODO Lock should happe in calling class
  if (strNewPath != m_strM3uUrl)
  {
    m_strM3uUrl = strNewPath;
    m_channels.Clear();
    m_channelGroups.Clear();

    if (LoadPlayList())
    {
      PVR->TriggerChannelUpdate();
      PVR->TriggerChannelGroupsUpdate();
    }
  }
}

std::string PlaylistLoader::ReadMarkerValue(const std::string& strLine, const std::string& strMarkerName)
{
  int iMarkerStart = static_cast<int>(strLine.find(strMarkerName));
  if (iMarkerStart >= 0)
  {
    const std::string strMarker = strMarkerName;
    iMarkerStart += strMarker.length();
    if (iMarkerStart < static_cast<int>(strLine.length()))
    {
      char cFind = ' ';
      if (strLine[iMarkerStart] == '"')
      {
        cFind = '"';
        iMarkerStart++;
      }
      int iMarkerEnd = static_cast<int>(strLine.find(cFind, iMarkerStart));
      if (iMarkerEnd < 0)
      {
        iMarkerEnd = strLine.length();
      }
      return strLine.substr(iMarkerStart, iMarkerEnd - iMarkerStart);
    }
  }

  return std::string("");
}

int PlaylistLoader::GetChannelId(const char* strChannelName, const char* strStreamUrl)
{
  std::string concat(strChannelName);
  concat.append(strStreamUrl);

  const char* strString = concat.c_str();
  int iId = 0;
  int c;
  while ((c = *strString++))
    iId = ((iId << 5) + iId) + c; /* iId * 33 + c */

  return abs(iId);
}
