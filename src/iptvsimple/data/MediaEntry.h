/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "BaseEntry.h"
#include "Channel.h"
#include "EpgEntry.h"

#include <string>

#include <kodi/addon-instance/pvr/Media.h>

namespace iptvsimple
{
  namespace data
  {
    class ATTRIBUTE_HIDDEN MediaEntry : public BaseEntry
    {
    public:
      const std::string& GetMediaEntryId() const { return m_mediaEntryId; }
      void SetMediaEntryId(const std::string& value) { m_mediaEntryId = value; }

      int GetDuration() const { return m_duration; }
      void SetDuration(int value) { m_duration = value; }

      int GetPlayCount() const { return m_playCount; }
      void SetPlayCount(int value) { m_playCount = value; }

      int GetLastPlayedPosition() const { return m_lastPlayedPosition; }
      void SetLastPlayedPosition(int value) { m_lastPlayedPosition = value; }

      time_t GetNextSyncTime() const { return m_nextSyncTime; }
      void SetNextSyncTime(time_t value) { m_nextSyncTime = value; }

      const std::string& GetStreamURL() const { return m_streamURL; }
      void SetStreamURL(const std::string& value) { m_streamURL = value; }

      const std::string& GetEdlURL() const { return m_edlURL; }
      void SetEdlURL(const std::string& value) { m_edlURL = value; }

      const std::string& GetProviderName() const { return m_providerName; }
      void SetProviderName(const std::string& value) { m_providerName = value; }

      int GetProviderUniqueId() const { return m_providerUniqueId; }
      void SetProviderUniqueId(int value) { m_providerUniqueId = value; }

      const std::string& GetDirectory() const { return m_directory; }
      void SetDirectory(const std::string& value) { m_directory = value; }

      PVR_MEDIA_TAG_CLASS GetMediaClass() const { return m_mediaClass; }
      void SetMediaClass(PVR_MEDIA_TAG_CLASS value) { m_mediaClass = value; }

      PVR_MEDIA_TAG_TYPE GetMediaType() const { return m_mediaType; }
      void SetMediaType(PVR_MEDIA_TAG_TYPE value) { m_mediaType = value; }

      int64_t GetSizeInBytes() const { return m_sizeInBytes; }
      void SetSizeInBytes(int64_t value) { m_sizeInBytes = value; }

      const std::string& GetM3UName() const { return m_m3uName; }
      const std::string& GetTvgId() const { return m_tvgId; }
      const std::string& GetTvgName() const { return m_tvgName; }

      const std::map<std::string, std::string>& GetProperties() const { return m_properties; }
      void SetProperties(std::map<std::string, std::string>& value) { m_properties = value; }
      void AddProperty(const std::string& prop, const std::string& value) { m_properties.insert({prop, value}); }
      std::string GetProperty(const std::string& propName) const;
      bool HasMimeType() const { return !GetProperty(PVR_STREAM_PROPERTY_MIMETYPE).empty(); }
      std::string GetMimeType() const { return GetProperty(PVR_STREAM_PROPERTY_MIMETYPE); }

      const std::string& GetInputStreamName() const { return m_inputStreamName; };
      void SetInputStreamName(const std::string& value) { m_inputStreamName = value; }

      void Reset();

      void UpdateFrom(iptvsimple::data::Channel channel);
      void UpdateFrom(iptvsimple::data::EpgEntry epgEntry);
      void UpdateTo(kodi::addon::PVRMediaTag& left, bool isInVirtualMediaEntryFolder, bool haveMediaTypes);

    private:
      std::string m_mediaEntryId;
      int m_duration;
      int m_playCount = 0;
      int m_lastPlayedPosition = 0;
      time_t m_nextSyncTime = 0;
      std::string m_streamURL;
      std::string m_edlURL;
      std::string m_providerName;
      int m_providerUniqueId = PVR_PROVIDER_INVALID_UID;
      std::string m_directory;
      PVR_MEDIA_TAG_CLASS m_mediaClass = PVR_MEDIA_TAG_CLASS_UNKNOWN;
      PVR_MEDIA_TAG_TYPE m_mediaType = PVR_MEDIA_TAG_TYPE_UNKNOWN;
      int64_t m_sizeInBytes = 0;

      // EPG lookup
      std::string m_m3uName;
      std::string m_tvgId;
      std::string m_tvgName;

      // Props
      std::map<std::string, std::string> m_properties;
      std::string m_inputStreamName;
    };
  } //namespace data
} //namespace iptvsimple
