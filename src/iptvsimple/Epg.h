#pragma once
/*
 *      Copyright (C) 2005-2015 Team XBMC
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

#include "libXBMC_pvr.h"

#include "Channels.h"
#include "data/ChannelEpg.h"

#include <string>
#include <vector>

namespace iptvsimple
{
  static const int SECONDS_IN_DAY = 86400;
  static const std::string GENRES_MAP_FILENAME = "genres.xml";

  class Epg
  {
  public:
    Epg(iptvsimple::Channels& channels);

    PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, int iChannelUid, time_t iStart, time_t iEnd);
    void Clear();

  private:
    bool LoadEPG(time_t iStart, time_t iEnd);
    void ReloadEPG(const char* strNewPath);
    data::ChannelEpg* FindEpgForChannel(const std::string& strId);
    data::ChannelEpg* FindEpgForChannel(const data::Channel& channel);
    void ApplyChannelsLogosFromEPG();
    bool LoadGenres();

    std::string m_strXMLTVUrl;
    int m_iEPGTimeShift;
    bool m_bTSOverride;
    int m_iLastStart;
    int m_iLastEnd;

    iptvsimple::Channels& m_channels;
    std::vector<data::ChannelEpg> m_channelEpgs;
    std::vector<iptvsimple::data::EpgGenre> m_genres;
  };
} //namespace iptvsimple