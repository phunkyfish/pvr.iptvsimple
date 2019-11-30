#pragma once
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

#include <string>

#include "data/Channel.h"
#include "data/EpgEntry.h"
#include "utilities/StreamUtils.h"

#include <mutex>

#include <kodi/libXBMC_pvr.h>

namespace iptvsimple
{
  class Epg;

  class CatchupController
  {
  public:
    CatchupController(iptvsimple::Epg& epg, std::mutex* mutex);

    void ProcessChannelForPlayback(data::Channel& channel); //should be const channel
    void ProcessEPGTagForTimeshiftedPlayback(const EPG_TAG& epgTag, data::Channel& channel); //should be const channel
    void ProcessEPGTagForVideoPlayback(const EPG_TAG& epgTag, data::Channel& channel); //should be const channel

    std::string GetTimeshiftInputStreamClass(const utilities::StreamType& streamType);

    std::string GetCatchupUrl(const data::Channel& channel) const;
    std::string GetStreamTestUrl(const data::Channel& channel) const;

    bool ControlsLiveStream() const { return m_controlsLiveStream; }

    //InputStream API
    long long LengthLiveStream();
    long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */);
    void CloseLiveStream();
    PVR_ERROR GetStreamTimes(PVR_STREAM_TIMES *times);

  private:
    data::EpgEntry* GetLiveEPGEntry(const iptvsimple::data::Channel& myChannel);
    data::EpgEntry* GetEPGEntry(const iptvsimple::data::Channel& myChannel, time_t lookupTime);
    void TestAndStoreStreamType(data::Channel& channel);

    // Programme helpers
    void UpdateProgrammeFrom(const EPG_TAG& epgTag, int tvgShift);
    void UpdateProgrammeFrom(const data::EpgEntry& epgEntry, int tvgShift);
    void ClearProgramme();

    // State of current stream
    time_t m_catchupStartTime = 0;
    time_t m_catchupEndTime = 0;
    time_t m_timeshiftBufferStartTime = 0;
    long long m_timeshiftBufferOffset = 0;
    bool m_resetCatchupState = false;
    bool m_playbackIsVideo = false;
    bool m_fromEpgTag = false;

    // Current programme details
    time_t m_programmeStartTime;
    time_t m_programmeEndTime;
    std::string m_programmeTitle;
    unsigned int m_programmeUniqueChannelId = 0;
    int m_programmeChannelTvgShift = 0;

    bool m_controlsLiveStream = false;
    iptvsimple::Epg& m_epg;
    std::mutex* m_mutex;
  };
} //namespace iptvsimple