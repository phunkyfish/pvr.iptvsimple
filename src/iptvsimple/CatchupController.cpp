/*
 *      Copyright (C) 2005-2019 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "CatchupController.h"

#include "../client.h"
#include "Channels.h"
#include "Epg.h"
#include "Settings.h"
#include "data/Channel.h"
#include "utilities/Logger.h"

#include <regex>

#include <p8-platform/util/StringUtils.h>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

CatchupController::CatchupController(Epg& epg, std::mutex* mutex)
  : m_epg(epg), m_mutex(mutex) {}

void CatchupController::ProcessChannelForPlayback(Channel& channel) //should be a const channel StreamManager
{
  TestAndStoreStreamType(channel);

  // Anything from here is live!
  m_playbackIsVideo = false; // TODO: possible time jitter on UI as this will effect get stream times

  if (!m_fromEpgTag || m_controlsLiveStream)
  {
    EpgEntry* liveEpgEntry = GetLiveEPGEntry(channel);
    if (liveEpgEntry)
    {
      UpdateProgrammeFrom(*liveEpgEntry, channel.GetTvgShift());
      m_catchupStartTime = liveEpgEntry->GetStartTime();
      m_catchupEndTime = liveEpgEntry->GetEndTime();
      Logger::Log(LEVEL_NOTICE, "HAVE live entry %lld", m_programmeStartTime);
    }
    else if (m_controlsLiveStream)
    {
      ClearProgramme();
      m_catchupStartTime = 0;
      m_catchupEndTime = 0;
      Logger::Log(LEVEL_NOTICE, "RESET live entry %lld", m_programmeStartTime);
    }
    m_fromEpgTag = false;
  }

  if (m_controlsLiveStream)
  {
    if (m_resetCatchupState)
    {
      m_resetCatchupState = false;
      if (channel.IsCatchupSupported())
      {
        m_timeshiftBufferOffset = Settings::GetInstance().GetCatchupDaysInSeconds(); //offset from now to start of catchup window
        m_timeshiftBufferStartTime = time(nullptr) - Settings::GetInstance().GetCatchupDaysInSeconds(); // now - the window size
      }
      else
      {
        m_timeshiftBufferOffset = 0;
        m_timeshiftBufferStartTime = 0;
      }
    }
    else
    {
      EpgEntry* currentEpgEntry = GetEPGEntry(channel, m_timeshiftBufferStartTime + m_timeshiftBufferOffset);
      if (currentEpgEntry)
        UpdateProgrammeFrom(*currentEpgEntry, channel.GetTvgShift());
    }

    m_catchupStartTime = m_timeshiftBufferStartTime;
  }
}

void CatchupController::ProcessEPGTagForTimeshiftedPlayback(const EPG_TAG& epgTag, Channel& channel) //should be a const channel StreamManager
{
  TestAndStoreStreamType(channel);

  if (m_controlsLiveStream)
  {
    UpdateProgrammeFrom(epgTag, channel.GetTvgShift());
    m_catchupStartTime = epgTag.startTime;
    m_catchupEndTime = epgTag.endTime;


    Logger::Log(LEVEL_NOTICE, "GetEPGTagStreamProperties - as live");
    time_t timeNow = time(0);
    time_t programmeOffset = timeNow - m_catchupStartTime;
    time_t timeshiftBufferDuration = std::max(programmeOffset, Settings::GetInstance().GetCatchupDaysInSeconds());
    m_timeshiftBufferStartTime = timeNow - timeshiftBufferDuration;
    m_catchupStartTime = m_timeshiftBufferStartTime;
    m_catchupEndTime = timeNow;
    m_timeshiftBufferOffset = timeshiftBufferDuration - programmeOffset;

    m_resetCatchupState = false;

    // if (m_catchupStartTime > 0)
    //   m_playbackIsVideo = true;
  }
  else
  {
    Logger::Log(LEVEL_NOTICE, "GetEPGTagStreamProperties - XXX1t");
    UpdateProgrammeFrom(epgTag, channel.GetTvgShift());
    m_catchupStartTime = epgTag.startTime;
    m_catchupEndTime = epgTag.endTime;

    m_timeshiftBufferStartTime = 0;
    m_timeshiftBufferOffset = 0;
    // m_catchupStartTime = m_catchupStartTime - Settings::GetInstance().GetCatchupWatchEpgBeginBufferSecs();
    // m_catchupEndTime += Settings::GetInstance().GetCatchupWatchEpgEndBufferSecs();

    m_fromEpgTag = true;
  }
}

void CatchupController::ProcessEPGTagForVideoPlayback(const EPG_TAG& epgTag, Channel& channel) //should be a const channel StreamManager
{
  TestAndStoreStreamType(channel);

  if (m_controlsLiveStream)
  {
    if (m_resetCatchupState)
    {
      UpdateProgrammeFrom(epgTag, channel.GetTvgShift());
      m_catchupStartTime = epgTag.startTime;
      m_catchupEndTime = epgTag.endTime;

      Logger::Log(LEVEL_NOTICE, "GetEPGTagStreamProperties - as reset");
      const time_t beginBuffer = Settings::GetInstance().GetCatchupWatchEpgBeginBufferSecs();
      const time_t endBuffer = Settings::GetInstance().GetCatchupWatchEpgEndBufferSecs();
      m_timeshiftBufferStartTime = m_catchupStartTime - beginBuffer;
      m_catchupStartTime = m_timeshiftBufferStartTime;
      m_catchupEndTime += endBuffer;
      m_timeshiftBufferOffset = beginBuffer;

      m_resetCatchupState = false;
    }
  }
  else
  {
    UpdateProgrammeFrom(epgTag, channel.GetTvgShift());
    m_catchupStartTime = epgTag.startTime;
    m_catchupEndTime = epgTag.endTime;

    m_timeshiftBufferStartTime = 0;
    m_timeshiftBufferOffset = 0;
    m_catchupStartTime = m_catchupStartTime - Settings::GetInstance().GetCatchupWatchEpgBeginBufferSecs();
    m_catchupEndTime += Settings::GetInstance().GetCatchupWatchEpgEndBufferSecs();
  }

  if (m_catchupStartTime > 0)
    m_playbackIsVideo = true;
}

void CatchupController::TestAndStoreStreamType(Channel& channel)
{
  // We need to find out what type of stream this is, so let's construct a catchup URL to test with
  std::string streamTestUrl = GetStreamTestUrl(channel);
  StreamType streamType = StreamUtils::GetStreamType(streamTestUrl, channel);
  if (streamType == StreamType::OTHER_TYPE)
    streamType = StreamUtils::InspectStreamType(streamTestUrl);

  // TODO: we really want to store this in a file and load it on any restart
  // using channel doesn't make sense as it's otherwise immutable.

  // If we find a mimetype store it so we don't have to look it up again
  if (!channel.GetProperty("mimetype").empty() && (streamType == StreamType::HLS || streamType == StreamType::DASH))
  {
    std::lock_guard<std::mutex> lock(*m_mutex);
    channel.AddProperty("mimetype", StreamUtils::GetMimeType(streamType));
  }

  if (StreamUtils::GetEffectiveInputStreamClass(streamType, channel) == "inputstream.ffmpegarchive")
    m_controlsLiveStream = true;
  else
    m_controlsLiveStream = false;
}

void CatchupController::UpdateProgrammeFrom(const EPG_TAG& epgTag, int tvgShift)
{
  m_programmeStartTime = epgTag.startTime;
  m_programmeEndTime = epgTag.endTime;
  m_programmeTitle = epgTag.strTitle;
  m_programmeUniqueChannelId = epgTag.iUniqueChannelId;
  m_programmeChannelTvgShift = tvgShift;
}

void CatchupController::UpdateProgrammeFrom(const data::EpgEntry& epgEntry, int tvgShift)
{
  m_programmeStartTime = epgEntry.GetStartTime();
  m_programmeEndTime = epgEntry.GetEndTime();
  m_programmeTitle = epgEntry.GetTitle();
  m_programmeUniqueChannelId = epgEntry.GetChannelId();
  m_programmeChannelTvgShift = tvgShift;
}

void CatchupController::ClearProgramme()
{
  m_programmeStartTime = 0;
  m_programmeEndTime = 0;
  m_programmeTitle.clear();
  m_programmeUniqueChannelId = 0;
  m_programmeChannelTvgShift = 0;
}

PVR_ERROR CatchupController::GetStreamTimes(PVR_STREAM_TIMES *times)
{
  if (!times)
    return PVR_ERROR_INVALID_PARAMETERS;
  if (m_timeshiftBufferStartTime == 0)
    return PVR_ERROR_NOT_IMPLEMENTED;

  *times = {0};
  const time_t dateTimeNow = time(0);

  times->startTime = m_timeshiftBufferStartTime;
  if (m_playbackIsVideo)
    times->ptsEnd = static_cast<int64_t>(std::min(dateTimeNow, m_catchupEndTime) - times->startTime) * DVD_TIME_BASE;
  else // it's live!
    times->ptsEnd = static_cast<int64_t>(dateTimeNow - times->startTime) * DVD_TIME_BASE;

  Logger::Log(LEVEL_NOTICE, "GetStreamTimes - Ch = %u \tTitle = \"%s\" \tepgTag->startTime = %ld \tepgTag->endTime = %ld",
            m_programmeUniqueChannelId, m_programmeTitle.c_str(), m_catchupStartTime, m_catchupEndTime);
  Logger::Log(LEVEL_NOTICE, "GetStreamTimes - startTime = %ld \tptsStart = %lld \tptsBegin = %lld \tptsEnd = %lld",
            times->startTime, times->ptsStart, times->ptsBegin, times->ptsEnd);
  return PVR_ERROR_NO_ERROR;
}

long long CatchupController::LengthLiveStream()
{
  long long ret = -1;
  Logger::Log(LEVEL_NOTICE, "LengthLiveStream");
  if (m_catchupStartTime > 0 && m_catchupEndTime >= m_catchupStartTime)
    ret = (m_catchupEndTime - m_catchupStartTime) * DVD_TIME_BASE;
  return ret;
}

long long CatchupController::SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
  long long ret = -1;
  if (m_catchupStartTime > 0)
  {
    Logger::Log(LEVEL_NOTICE, "SeekLiveStream - iPosition = %lld, iWhence = %d", iPosition, iWhence);
    const time_t timeNow = time(0);
    switch (iWhence)
    {
      case SEEK_SET:
      {
        Logger::Log(LEVEL_NOTICE, "SeekLiveStream - SeekSet");
        iPosition += 500;
        iPosition /= 1000;
        if (m_catchupStartTime + iPosition < timeNow - 10)
        {
          ret = iPosition;
          m_timeshiftBufferOffset = iPosition;
        }
        else
        {
          ret = timeNow - m_catchupStartTime;
          m_timeshiftBufferOffset = ret;
        }
        ret *= DVD_TIME_BASE;
      }
      break;
      case SEEK_CUR:
      {
        long long offset = m_timeshiftBufferOffset;
        Logger::Log(LEVEL_NOTICE, "SeekLiveStream - timeNow = %d, startTime = %d, iTvgShift = %d, offset = %d", timeNow, m_catchupStartTime, m_programmeChannelTvgShift, offset);
        ret = offset * DVD_TIME_BASE;
      }
      break;
      default:
        Logger::Log(LEVEL_NOTICE, "SeekLiveStream - Unsupported SEEK command (%d)", iWhence);
      break;
    }
  }
  return ret;
}

void CatchupController::CloseLiveStream()
{
  m_resetCatchupState = true;
  Logger::Log(LEVEL_NOTICE, "CloseLiveStream - true");
}

namespace
{

void FormatOffset(time_t tTime, std::string &urlFormatString)
{
  const std::string regexStr = ".*(\\{offset:(\\d+)\\}).*";
  std::cmatch mr;
  std::regex rx(regexStr);
  if (std::regex_match(urlFormatString.c_str(), mr, rx) && mr.length() >= 3)
  {
    std::string offsetExp = mr[1].first;
    std::string second = mr[1].second;
    if (second.length() > 0)
      offsetExp = offsetExp.erase(offsetExp.find(second));
    std::string dividerStr = mr[2].first;
    second = mr[2].second;
    if (second.length() > 0)
      dividerStr = dividerStr.erase(dividerStr.find(second));

    const time_t divider = stoi(dividerStr);
    if (divider != 0)
    {
      time_t offset = tTime / divider;
      if (offset < 0)
        offset = 0;
      urlFormatString.replace(urlFormatString.find(offsetExp), offsetExp.length(), std::to_string(offset));
    }
  }
}

void FormatTime(const char ch, const struct tm *pTime, std::string &urlFormatString)
{
  char str[] = { '{', ch, '}', 0 };
  auto pos = urlFormatString.find(str);
  if (pos != std::string::npos)
  {
    char buff[256], timeFmt[3];
    std::snprintf(timeFmt, sizeof(timeFmt), "%%%c", ch);
    std::strftime(buff, sizeof(buff), timeFmt, pTime);
    if (std::strlen(buff) > 0)
      urlFormatString.replace(pos, 3, buff);
  }
}

void FormatUtc(const char *str, time_t tTime, std::string &urlFormatString)
{
  auto pos = urlFormatString.find(str);
  if (pos != std::string::npos)
  {
    char buff[256];
    std::snprintf(buff, sizeof(buff), "%lu", tTime);
    urlFormatString.replace(pos, std::strlen(str), buff);
  }
}

std::string FormatDateTime(time_t dateTimeEpg, time_t duration, const std::string &url, const std::string &postfixQueryString)
{
  std::string urlFormatString;
  if (!postfixQueryString.empty())
  {
    // preserve any kodi protocol options after "|"
    size_t found = url.find_first_of('|');
    if (found != std::string::npos)
      urlFormatString = url.substr(0, found) + postfixQueryString + url.substr(found, url.length());
    else
      urlFormatString = url + postfixQueryString;
  }
  else
  {
    urlFormatString = url;
  }

  const time_t dateTimeNow = std::time(0);
  tm* dateTime = std::localtime(&dateTimeEpg);

  FormatTime('Y', dateTime, urlFormatString);
  FormatTime('m', dateTime, urlFormatString);
  FormatTime('d', dateTime, urlFormatString);
  FormatTime('H', dateTime, urlFormatString);
  FormatTime('M', dateTime, urlFormatString);
  FormatTime('S', dateTime, urlFormatString);
  FormatUtc("{utc}", dateTimeEpg, urlFormatString);
  FormatUtc("${start}", dateTimeEpg, urlFormatString);
  FormatUtc("{utcend}", dateTimeEpg + duration, urlFormatString);
  FormatUtc("${end}", dateTimeEpg + duration, urlFormatString);
  FormatUtc("{lutc}", dateTimeNow, urlFormatString);
  FormatUtc("{duration}", duration, urlFormatString);
  FormatOffset(dateTimeNow - dateTimeEpg, urlFormatString);

  Logger::Log(LEVEL_NOTICE, "CArchiveConfig::FormatDateTime - \"%s\"", urlFormatString.c_str());

  return urlFormatString;
}

std::string BuildEpgTagUrl(time_t startTime, time_t duration, const Channel& channel, long long timeOffset)
{
  std::string startTimeUrl;
  time_t timeNow = time(0);
  time_t offset = startTime + timeOffset;

  if (startTime > 0 && offset < (timeNow - 5))
  {
    if (!channel.GetCatchupSource().empty())
    {
      if (channel.GetCatchupMode() == CatchupMode::DEFAULT)
        startTimeUrl = FormatDateTime(offset - channel.GetTvgShift(), duration, channel.GetCatchupSource(), "");
      else // source is query to be appended
        startTimeUrl = FormatDateTime(offset - channel.GetTvgShift(), duration, channel.GetStreamURL(), channel.GetCatchupSource());
    }
    else
    {
      startTimeUrl = FormatDateTime(offset - channel.GetTvgShift(), duration, channel.GetStreamURL(), Settings::GetInstance().GetCatchupQueryFormat());
    }
  }
  else
  {
    startTimeUrl = channel.GetStreamURL();
  }

  Logger::Log(LEVEL_NOTICE, "BuildEpgTagUrl::startTimeUrl %s", startTimeUrl.c_str());

  return startTimeUrl;
}

} // unnamed namespace

std::string CatchupController::GetCatchupUrl(const Channel& channel) const
{
  Logger::Log(LEVEL_NOTICE, "GetEpgTagUrl::startTime %d", m_catchupStartTime);

  if (m_catchupStartTime > 0)
    return BuildEpgTagUrl(m_catchupStartTime, static_cast<time_t>(m_programmeStartTime - m_programmeEndTime), channel, m_timeshiftBufferOffset);

  return "";
}

std::string CatchupController::GetStreamTestUrl(const Channel& channel) const
{
  // Test URL from 2 hours ago for 1 hour duration.
  return BuildEpgTagUrl(std::time(nullptr) - (2 * 60 * 60),  60 * 60, channel, 0);
}

EpgEntry* CatchupController::GetLiveEPGEntry(const Channel& myChannel)
{
  std::lock_guard<std::mutex> lock(*m_mutex);

  return m_epg.GetLiveEPGEntry(myChannel);
}

EpgEntry* CatchupController::GetEPGEntry(const Channel& myChannel, time_t lookupTime)
{
  std::lock_guard<std::mutex> lock(*m_mutex);

  return m_epg.GetEPGEntry(myChannel, lookupTime);
}
