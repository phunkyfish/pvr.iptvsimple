#pragma once
/*
 *      Copyright (C) 2013-2015 Anton Fedchin
 *      http://github.com/afedchin/xbmc-addon-iptvsimple/
 *
 *      Copyright (C) 2011 Pulse-Eight
 *      http://www.pulse-eight.com/
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
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "iptvsimple/Settings.h"

#include "p8-platform/os.h"
#include "libXBMC_pvr.h"
#include "p8-platform/threads/threads.h"

#include "iptvsimple/data/Channel.h"
#include "iptvsimple/data/ChannelEpg.h"
#include "iptvsimple/data/ChannelGroup.h"
#include "iptvsimple/data/EpgEntry.h"
#include "iptvsimple/data/EpgGenre.h"

#include <map>
#include <vector>

class PVRIptvData : public P8PLATFORM::CThread
{
public:
  PVRIptvData(void);
  ~PVRIptvData(void);

  int GetChannelsAmount(void);
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  bool GetChannel(const PVR_CHANNEL& channel, iptvsimple::data::Channel& myChannel);
  int GetChannelGroupsAmount(void);
  PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);
  PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& group);
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, int iChannelUid, time_t iStart, time_t iEnd);
  void ReaplyChannelsLogos(const char* strNewPath);
  void ReloadPlayList(const char* strNewPath);
  void ReloadEPG(const char* strNewPath);

protected:
  bool LoadPlayList(void);
  bool LoadEPG(time_t iStart, time_t iEnd);
  bool LoadGenres(void);
  const iptvsimple::data::Channel* FindChannel(const std::string& strId, const std::string& strName) const;
  iptvsimple::data::ChannelGroup* FindGroup(const std::string& strName);
  iptvsimple::data::ChannelEpg* FindEpgForChannel(const std::string& strId);
  iptvsimple::data::ChannelEpg* FindEpgForChannel(const iptvsimple::data::Channel& channel);
  bool GzipInflate(const std::string& compressedBytes, std::string& uncompressedBytes);
  int GetCachedFileContents(const std::string& strCachedName, const std::string& strFilePath,
                                    std::string& strContent, const bool bUseCache = false);
  void ApplyChannelsLogos();
  void ApplyChannelsLogosFromEPG();
  std::string ReadMarkerValue(const std::string& strLine, const char* strMarkerName);
  int GetChannelId(const char* strChannelName, const char* strStreamUrl);

protected:
  void* Process(void) override;

private:

  iptvsimple::Settings& m_settings = iptvsimple::Settings::GetInstance();

  bool m_bTSOverride;
  int m_iEPGTimeShift;
  int m_iLastStart;
  int m_iLastEnd;
  std::string m_strXMLTVUrl;
  std::string m_strM3uUrl;
  std::string m_strLogoPath;
  std::vector<iptvsimple::data::ChannelGroup> m_groups;
  std::vector<iptvsimple::data::Channel> m_channels;
  std::vector<iptvsimple::data::ChannelEpg> m_epg;
  std::vector<iptvsimple::data::EpgGenre> m_genres;
  P8PLATFORM::CMutex m_mutex;
};
