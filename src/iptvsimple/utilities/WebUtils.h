/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>

namespace iptvsimple
{
  namespace utilities
  {
    static const std::string HTTP_PREFIX = "http://";
    static const std::string HTTPS_PREFIX = "https://";
    static const std::string UDP_MULTICAST_PREFIX = "udp://@";
    static const std::string RTP_MULTICAST_PREFIX = "rtp://@";

    class WebUtils
    {
    public:
      static const std::string UrlEncode(const std::string& value);
      static std::string ReadFileContentsStartOnly(const std::string& url, int* httpCode);
      static std::string ReadFileContentsStartOnly(const std::string& url, std::string& effectiveUrl,int* httpCode);
      static std::string GetEffectiveUrl(const std::string &url);
      static bool IsHttpUrl(const std::string& url);
    };
  } // namespace utilities
} // namespace iptvsimple
