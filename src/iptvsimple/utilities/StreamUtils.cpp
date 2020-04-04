/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "StreamUtils.h"

#include "../Settings.h"
#include "FileUtils.h"
#include "Logger.h"
#include "WebUtils.h"

#include <p8-platform/util/StringUtils.h>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

void StreamUtils::SetStreamProperty(PVR_NAMED_VALUE* properties, unsigned int* propertiesCount, unsigned int propertiesMax, const std::string& name, const std::string& value)
{
  if (*propertiesCount < propertiesMax)
  {
    strncpy(properties[*propertiesCount].strName, name.c_str(), sizeof(properties[*propertiesCount].strName) - 1);
    strncpy(properties[*propertiesCount].strValue, value.c_str(), sizeof(properties[*propertiesCount].strValue) - 1);
    (*propertiesCount)++;
  }
  else
  {
    Logger::Log(LogLevel::LEVEL_ERROR, "%s - Could not add property as max number reached: %s=%s - count : %u", __FUNCTION__, name.c_str(), value.c_str(), propertiesMax);
  }
}

void StreamUtils::SetAllStreamProperties(PVR_NAMED_VALUE* properties, unsigned int* propertiesCount, unsigned int propertiesMax, const iptvsimple::data::Channel& channel, const std::string& streamURL, std::map<std::string, std::string>& catchupProperties)
{
  if (ChannelSpecifiesInputstream(channel))
  {
    // Channel has an inputstream class set so we only set the stream URL
    StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_STREAMURL, streamURL);
  }
  else
  {
    StreamType streamType = StreamUtils::GetStreamType(streamURL, channel);
    if (streamType == StreamType::OTHER_TYPE)
      streamType = StreamUtils::InspectStreamType(streamURL, channel);

    std::string effectiveUrl = WebUtils::GetEffectiveUrl(streamURL);

    Logger::Log(LogLevel::LEVEL_NOTICE, "%s XXX - \n%s\n%s", __FUNCTION__, streamURL.c_str(), effectiveUrl.c_str());

    // Using kodi's built in inputstreams
    if (StreamUtils::UseKodiInputstreams(streamType))
    {
      std::string ffmpegStreamURL = StreamUtils::GetURLWithFFmpegReconnectOptions(effectiveUrl, streamType, channel);

      StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_STREAMURL, effectiveUrl);

      if (streamType == StreamType::HLS || streamType == StreamType::TS)
      {
        if (channel.IsCatchupSupported())
          StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_INPUTSTREAMCLASS, CATCHUP_INPUTSTREAMCLASS);
        else
          StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_INPUTSTREAMCLASS, PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG);
      }
    }
    else // inputstream.adpative
    {
      StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_STREAMURL, streamURL);
      StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_INPUTSTREAMCLASS, "inputstream.adaptive");
      StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, "inputstream.adaptive.manifest_type", StreamUtils::GetManifestType(streamType));
      if (streamType == StreamType::HLS || streamType == StreamType::DASH)
        StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_MIMETYPE, StreamUtils::GetMimeType(streamType));
      if (streamType == StreamType::DASH)
        StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, "inputstream.adaptive.manifest_update_parameter", "full");
    }
  }

  if (!channel.GetProperties().empty())
  {
    for (auto& prop : channel.GetProperties())
      StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, prop.first, prop.second);
  }

  if (!catchupProperties.empty())
  {
    for (auto& prop : catchupProperties)
      StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, prop.first, prop.second);
  }
}

std::string StreamUtils::GetEffectiveInputStreamClass(const StreamType& streamType, const iptvsimple::data::Channel& channel)
{
  std::string inputStreamClass = channel.GetInputStreamClass();

  if (inputStreamClass.empty())
  {
    if (StreamUtils::UseKodiInputstreams(streamType))
    {
      if (streamType == StreamType::HLS || streamType == StreamType::TS)
      {
        if (channel.IsCatchupSupported() && channel.CatchupSupportsTimeshifting())
          inputStreamClass = CATCHUP_INPUTSTREAMCLASS;
        else
          inputStreamClass = PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG;
      }
    }
    else // inputstream.adpative
    {
      inputStreamClass = "inputstream.adaptive";
    }
  }

  return inputStreamClass;
}

const StreamType StreamUtils::GetStreamType(const std::string& url, const Channel& channel)
{
  std::string mimeType = channel.GetProperty(PVR_STREAM_PROPERTY_MIMETYPE);
  if (mimeType.empty())
    mimeType = channel.GetProperty("inputstream.ffmpegdirect.mime_type");

  if (url.find(".m3u8") != std::string::npos ||
      mimeType == "application/x-mpegURL" ||
      mimeType == "application/vnd.apple.mpegurl")
    return StreamType::HLS;

  if (url.find(".mpd") != std::string::npos || mimeType == "application/xml+dash")
    return StreamType::DASH;

  if (url.find(".ism") != std::string::npos &&
      !(url.find(".ismv") != std::string::npos || url.find(".isma") != std::string::npos))
    return StreamType::SMOOTH_STREAMING;

  if (mimeType == "video/mp2t" || channel.IsCatchupTSStream())
    return StreamType::TS;

  return StreamType::OTHER_TYPE;
}

const StreamType StreamUtils::InspectStreamType(const std::string& url, const Channel& channel)
{
  if (!FileUtils::FileExists(url))
    return StreamType::OTHER_TYPE;

  int httpCode = 0;
  const std::string source = WebUtils::ReadFileContentsStartOnly(url, &httpCode);

  if (httpCode == 200)
  {
    if (StringUtils::StartsWith(source, "#EXTM3U") && (source.find("#EXT-X-STREAM-INF") != std::string::npos || source.find("#EXT-X-VERSION") != std::string::npos))
      return StreamType::HLS;

    if (source.find("<MPD") != std::string::npos)
      return StreamType::DASH;

    if (source.find("<SmoothStreamingMedia") != std::string::npos)
      return StreamType::SMOOTH_STREAMING;
  }

  // If we can't inspect the stream type the only option left for shift mode is TS
  if (channel.GetCatchupMode() == CatchupMode::SHIFT ||
      channel.GetCatchupMode() == CatchupMode::TIMESHIFT)
    return StreamType::TS;

  return StreamType::OTHER_TYPE;
}

const std::string StreamUtils::GetManifestType(const StreamType& streamType)
{
  switch (streamType)
  {
    case StreamType::HLS:
      return "hls";
    case StreamType::DASH:
      return "mpd";
    case StreamType::SMOOTH_STREAMING:
      return "ism";
    default:
      return "";
  }
}

const std::string StreamUtils::GetMimeType(const StreamType& streamType)
{
  switch (streamType)
  {
    case StreamType::HLS:
      return "application/x-mpegURL";
    case StreamType::DASH:
      return "application/xml+dash";
    case StreamType::TS:
      return "video/mp2t";
    default:
      return "";
  }
}

std::string StreamUtils::GetURLWithFFmpegReconnectOptions(const std::string& streamUrl, const StreamType& streamType, const iptvsimple::data::Channel& channel)
{
  std::string newStreamUrl = streamUrl;

  if (WebUtils::IsHttpUrl(streamUrl) && SupportsFFmpegReconnect(streamType, channel) &&
      (channel.GetProperty("http-reconnect") == "true" || Settings::GetInstance().UseFFmpegReconnect()))
  {
    newStreamUrl = AddHeaderToStreamUrl(newStreamUrl, "reconnect", "1");
    if (streamType != StreamType::HLS)
      newStreamUrl = AddHeaderToStreamUrl(newStreamUrl, "reconnect_at_eof", "1");
    newStreamUrl = AddHeaderToStreamUrl(newStreamUrl, "reconnect_streamed", "1");
    newStreamUrl = AddHeaderToStreamUrl(newStreamUrl, "reconnect_delay_max", "4294");

    Logger::Log(LogLevel::LEVEL_DEBUG, "%s - FFmpeg Reconnect Stream URL: %s", __FUNCTION__, newStreamUrl.c_str());
  }

  return newStreamUrl;
}

std::string StreamUtils::AddHeaderToStreamUrl(const std::string& streamUrl, const std::string& headerName, const std::string& headerValue)
{
  std::string newStreamUrl = streamUrl;

  bool hasProtocolOptions = false;
  bool addHeader = true;
  size_t found = newStreamUrl.find("|");

  if (found != std::string::npos)
  {
    hasProtocolOptions = true;
    addHeader = newStreamUrl.find(headerName + "=", found + 1) == std::string::npos;
  }

  if (addHeader)
  {
    if (!hasProtocolOptions)
      newStreamUrl += "|";
    else
      newStreamUrl += "&";

    newStreamUrl += headerName + "=" + headerValue;
  }

  return newStreamUrl;
}

bool StreamUtils::UseKodiInputstreams(const StreamType& streamType)
{
  return streamType == StreamType::OTHER_TYPE || streamType == StreamType::TS ||
        (streamType == StreamType::HLS && !Settings::GetInstance().UseInputstreamAdaptiveforHls());
}

bool StreamUtils::ChannelSpecifiesInputstream(const iptvsimple::data::Channel& channel)
{
  return !channel.GetInputStreamClass().empty();
}

bool StreamUtils::SupportsFFmpegReconnect(const StreamType& streamType, const iptvsimple::data::Channel& channel)
{
  return streamType == StreamType::HLS ||
         channel.GetProperty(PVR_STREAM_PROPERTY_INPUTSTREAMCLASS) == PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG ||
         channel.GetProperty(PVR_STREAM_PROPERTY_INPUTSTREAMADDON) == PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG;
}
