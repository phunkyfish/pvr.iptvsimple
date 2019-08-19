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
  m_m3uUrl = Settings::GetInstance().GetM3UPath();
}

bool PlaylistLoader::LoadPlayList(void)
{
  if (m_m3uUrl.empty())
  {
    Logger::Log(LEVEL_NOTICE, "Playlist file path is not configured. Channels not loaded.");
    return false;
  }

  std::string playlistContent;
  if (!FileUtils::GetCachedFileContents(Settings::GetInstance().GetUserPath(), M3U_FILE_NAME, m_m3uUrl, playlistContent, Settings::GetInstance().UseM3UCache()))
  {
    Logger::Log(LEVEL_ERROR, "Unable to load playlist file '%s':  file is missing or empty.", m_m3uUrl.c_str());
    return false;
  }

  std::stringstream stream(playlistContent);

  /* load channels */
  bool isFirstLine        = true;
  bool isRealTime   = true;
  int channelIndex  = 0;
  int uniqueGroupId = 0;
  int channelNumber    = Settings::GetInstance().GetStartNumber();
  int epgTimeShift  = 0;
  std::vector<int> currentGroupIdList;

  Channel tmpChannel;

  std::string line;
  while (std::getline(stream, line))
  {
    line = StringUtils::TrimRight(line, " \t\r\n");
    line = StringUtils::TrimLeft(line, " \t");

    Logger::Log(LEVEL_DEBUG, "Read line: '%s'", line.c_str());

    if (line.empty())
    {
      continue;
    }

    if (isFirstLine)
    {
      isFirstLine = false;

      if (StringUtils::Left(line, 3) == "\xEF\xBB\xBF")
        line.erase(0, 3);

      if (StringUtils::StartsWith(line, M3U_START_MARKER))
      {
        double tvgShiftDecimal = atof(ReadMarkerValue(line, TVG_INFO_SHIFT_MARKER).c_str());
        epgTimeShift = static_cast<int>(tvgShiftDecimal * 3600.0);
        continue;
      }
      else
      {
        Logger::Log(LEVEL_ERROR,
                  "URL '%s' missing %s descriptor on line 1, attempting to "
                  "parse it anyway.",
                  m_m3uUrl.c_str(), M3U_START_MARKER.c_str());
      }
    }

    if (StringUtils::StartsWith(line, M3U_INFO_MARKER))
    {
      bool isRadio             = false;
      double tvgShiftDecimal   = 0;
      std::string strChnlNo    = "";
      std::string strChnlName  = "";
      std::string strTvgId     = "";
      std::string strTvgName   = "";
      std::string strTvgLogo   = "";
      std::string strTvgShift  = "";
      std::string strGroupName = "";
      std::string strRadio     = "";

      // parse line
      int colonIndex = static_cast<int>(line.find(':'));
      int commaIndex = static_cast<int>(line.rfind(','));
      if (colonIndex >= 0 && commaIndex >= 0 && commaIndex > colonIndex)
      {
        // parse name
        commaIndex++;
        strChnlName = StringUtils::Right(line, static_cast<int>(line.size() - commaIndex));
        strChnlName = StringUtils::Trim(strChnlName);
        tmpChannel.SetChannelName(XBMC->UnknownToUTF8(strChnlName.c_str()));

        // parse info
        colonIndex++;
        commaIndex--;
        const std::string strInfoLine = StringUtils::Mid(line, colonIndex, commaIndex - colonIndex);

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
          channelNumber = atoi(strChnlNo.c_str());

        tvgShiftDecimal = atof(strTvgShift.c_str());

        isRadio = !StringUtils::CompareNoCase(strRadio, "true");
        tmpChannel.SetTvgId(strTvgId);
        tmpChannel.SetTvgName(XBMC->UnknownToUTF8(strTvgName.c_str()));
        tmpChannel.SetTvgLogo(XBMC->UnknownToUTF8(strTvgLogo.c_str()));
        tmpChannel.SetTvgShift(static_cast<int>(tvgShiftDecimal * 3600.0));
        tmpChannel.SetRadio(isRadio);

        if (strTvgShift.empty())
          tmpChannel.SetTvgShift(epgTimeShift);

        if (!strGroupName.empty())
        {
          std::stringstream streamGroups(strGroupName);
          currentGroupIdList.clear();

          while (std::getline(streamGroups, strGroupName, ';'))
          {
            strGroupName = XBMC->UnknownToUTF8(strGroupName.c_str());
            const ChannelGroup* pGroup = m_channelGroups.FindChannelGroup(strGroupName);

            if (!pGroup)
            {
              ChannelGroup group;
              group.SetGroupName(strGroupName);
              group.SetGroupId(++uniqueGroupId);
              group.SetRadio(isRadio);

              m_channelGroups.GetChannelGroupsList().push_back(group);
              currentGroupIdList.push_back(uniqueGroupId);
            }
            else
            {
              currentGroupIdList.push_back(pGroup->GetGroupId());
            }
          }
        }
      }
    }
    else if (StringUtils::StartsWith(line, KODIPROP_MARKER))
    {
      const std::string value = ReadMarkerValue(line, KODIPROP_MARKER);
      auto pos = value.find('=');
      if (pos != std::string::npos)
      {
        const std::string prop = value.substr(0, pos);
        const std::string propValue = value.substr(pos + 1);
        tmpChannel.GetProperties().insert({prop, propValue});
      }
    }
    else if (StringUtils::StartsWith(line, EXTVLCOPT_MARKER))
    {
      const std::string value = ReadMarkerValue(line, EXTVLCOPT_MARKER);
      auto pos = value.find('=');
      if (pos != std::string::npos)
      {
        const std::string prop = value.substr(0, pos);
        const std::string propValue = value.substr(pos + 1);
        tmpChannel.GetProperties().insert({prop, propValue});

        Logger::Log(LEVEL_DEBUG, "Found #EXTVLCOPT property: '%s' value: '%s'", prop.c_str(), propValue.c_str());
      }
    }
    else if (StringUtils::StartsWith(line, PLAYLIST_TYPE_MARKER))
    {
      if (ReadMarkerValue(line, PLAYLIST_TYPE_MARKER) == "VOD")
        isRealTime = false;
    }
    else if (line[0] != '#')
    {
      Logger::Log(LEVEL_DEBUG, "Found URL: '%s' (current channel name: '%s')", line.c_str(), tmpChannel.GetChannelName().c_str());

      if (isRealTime)
        tmpChannel.GetProperties().insert({PVR_STREAM_PROPERTY_ISREALTIMESTREAM, "true"});

      Channel channel;
      tmpChannel.UpdateTo(channel);
      channel.SetUniqueId(GetChannelId(tmpChannel.GetChannelName().c_str(), line.c_str()));
      channel.SetChannelNumber(channelNumber++);
      channel.SetStreamURL(line);

      channelNumber++;

      for (int myGroupId : currentGroupIdList)
      {
        channel.SetRadio(m_channelGroups.GetChannelGroupsList().at(myGroupId - 1).IsRadio());
        m_channelGroups.GetChannelGroupsList().at(myGroupId - 1).GetMemberChannels().push_back(channelIndex);
      }

      m_channels.GetChannelsList().push_back(channel);
      channelIndex++;

      tmpChannel.Reset();
      isRealTime = true;
    }
  }

  stream.clear();

  if (m_channels.GetChannelsAmount() == 0)
  {
    Logger::Log(LEVEL_ERROR, "Unable to load channels from file '%s':  file is corrupted.", m_m3uUrl.c_str());
    return false;
  }

  m_channels.ApplyChannelLogos();

  Logger::Log(LEVEL_NOTICE, "Loaded %d channels.", m_channels.GetChannelsAmount());
  return true;
}

void PlaylistLoader::ReloadPlayList(const char* newPath)
{
  //P8PLATFORM::CLockObject lock(m_mutex);
  //TODO Lock should happe in calling class
  if (newPath != m_m3uUrl)
  {
    m_m3uUrl = newPath;
    m_channels.Clear();
    m_channelGroups.Clear();

    if (LoadPlayList())
    {
      PVR->TriggerChannelUpdate();
      PVR->TriggerChannelGroupsUpdate();
    }
  }
}

std::string PlaylistLoader::ReadMarkerValue(const std::string& line, const std::string& markerName)
{
  int markerStart = static_cast<int>(line.find(markerName));
  if (markerStart >= 0)
  {
    const std::string marker = markerName;
    markerStart += marker.length();
    if (markerStart < static_cast<int>(line.length()))
    {
      char find = ' ';
      if (line[markerStart] == '"')
      {
        find = '"';
        markerStart++;
      }
      int markerEnd = static_cast<int>(line.find(find, markerStart));
      if (markerEnd < 0)
      {
        markerEnd = line.length();
      }
      return line.substr(markerStart, markerEnd - markerStart);
    }
  }

  return std::string("");
}

int PlaylistLoader::GetChannelId(const char* channelName, const char* streamUrl)
{
  std::string concat(channelName);
  concat.append(streamUrl);

  const char* calcString = concat.c_str();
  int iId = 0;
  int c;
  while ((c = *calcString++))
    iId = ((iId << 5) + iId) + c; /* iId * 33 + c */

  return abs(iId);
}
