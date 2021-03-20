/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Media.h"

#include "../PVRIptvData.h"
#include "utilities/Logger.h"

#include <kodi/tools/StringUtils.h>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;
using namespace kodi::tools;

Media::Media()
{
}

void Media::GetMedia(std::vector<kodi::addon::PVRMediaTag>& kodiMedia)
{
  for (auto& mediaEntry : m_media)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Transfer mediaEntry '%s', MediaEntry Id '%s'", __func__, mediaEntry.GetTitle().c_str(), mediaEntry.GetMediaEntryId().c_str());
    kodi::addon::PVRMediaTag kodiMediaEntry;

    mediaEntry.UpdateTo(kodiMediaEntry, IsInVirtualMediaEntryFolder(mediaEntry), m_haveMediaTypes);

    kodiMedia.emplace_back(kodiMediaEntry);
  }
}

int Media::GetNumMedia() const
{
  return m_media.size();
}

void Media::Clear()
{
  m_media.clear();
  m_mediaIdMap.clear();
  m_haveMediaTypes = false;
}

namespace
{

int GenerateMediaEntryId(const char* providerName, const char* streamUrl)
{
  std::string concat(providerName);
  concat.append(streamUrl);

  const char* calcString = concat.c_str();
  int iId = 0;
  int c;
  while ((c = *calcString++))
    iId = ((iId << 5) + iId) + c; /* iId * 33 + c */

  return abs(iId);
}

} // unamed namespace

bool Media::AddMediaEntry(MediaEntry& mediaEntry)
{
  std::string mediaEntryId = std::to_string(GenerateMediaEntryId(mediaEntry.GetProviderName().c_str(),
                                                                 mediaEntry.GetStreamURL().c_str()));
  mediaEntryId.append("-" + mediaEntry.GetDirectory() + mediaEntry.GetTitle());
  mediaEntry.SetMediaEntryId(mediaEntryId);

  if (m_mediaIdMap.find(mediaEntryId) != m_mediaIdMap.end())
    return false;

  m_media.emplace_back(mediaEntry);
  m_mediaIdMap.insert({mediaEntryId, mediaEntry});

  if (mediaEntry.GetMediaType() != PVR_MEDIA_TAG_TYPE_UNKNOWN)
    m_haveMediaTypes = true;

  return true;
}

MediaEntry Media::GetMediaEntry(const std::string& mediaEntryId) const
{
  MediaEntry entry;

  auto mediaEntryPair = m_mediaIdMap.find(mediaEntryId);
  if (mediaEntryPair != m_mediaIdMap.end())
  {
    entry = mediaEntryPair->second;
  }

  return entry;
}

bool Media::IsInVirtualMediaEntryFolder(const MediaEntry& mediaEntryToCheck) const
{
  const std::string& mediaEntryFolderToCheck = mediaEntryToCheck.GetTitle();

  int iMatches = 0;
  for (const auto& mediaEntry : m_media)
  {
    if (mediaEntryFolderToCheck == mediaEntry.GetTitle())
    {
      iMatches++;
      Logger::Log(LEVEL_DEBUG, "%s Found MediaEntry title '%s' in media vector!", __func__, mediaEntryFolderToCheck.c_str());
      if (iMatches > 1)
      {
        Logger::Log(LEVEL_DEBUG, "%s Found MediaEntry title twice '%s' in media vector!", __func__, mediaEntryFolderToCheck.c_str());
        return true;
      }
    }
  }

  return false;
}

const std::string Media::GetMediaEntryURL(const kodi::addon::PVRMediaTag& mediaTag)
{
  Logger::Log(LEVEL_INFO, "%s", __func__);

  auto mediaEntry = GetMediaEntry(mediaTag.GetMediaTagId());

  if (!mediaEntry.GetMediaEntryId().empty())
    return mediaEntry.GetStreamURL();

  return "";
}
