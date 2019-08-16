#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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

#include "p8-platform/os.h"

#include <string>
#include <vector>

namespace iptvsimple
{
  namespace utilities
  {
    class FileUtils
    {
    public:
      static std::string PathCombine(const std::string& strPath, const std::string& strFileName);
      static std::string GetClientFilePath(const std::string& strFileName);
      static std::string GetUserFilePath(const std::string& strFileName);
      static int GetFileContents(const std::string& url, std::string& strContent);
      static bool GzipInflate(const std::string& compressedBytes, std::string& uncompressedBytes);
      static int GetCachedFileContents(const std::string& strCachedName, const std::string& strFilePath,
                                        std::string& strContent, const bool bUseCache = false);

    };
  } // namespace utilities
} // namespace iptvsimple
