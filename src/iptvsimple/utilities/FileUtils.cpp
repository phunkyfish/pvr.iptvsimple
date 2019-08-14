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

#include "FileUtils.h"

#include "../../client.h"
#include "../Settings.h"
#include "Logger.h"

using namespace iptvsimple;
using namespace iptvsimple::utilities;

std::string FileUtils::PathCombine(const std::string& strPath, const std::string& strFileName)
{
  std::string strResult = strPath;
  if (strResult.at(strResult.size() - 1) == '\\' ||
      strResult.at(strResult.size() - 1) == '/')
  {
    strResult.append(strFileName);
  }
  else
  {
    strResult.append("/");
    strResult.append(strFileName);
  }

  return strResult;
}

std::string FileUtils::GetClientFilePath(const std::string& strFileName)
{
  return PathCombine(Settings::GetInstance().GetClientPath(), strFileName);
}

std::string FileUtils::GetUserFilePath(const std::string& strFileName)
{
  return PathCombine(Settings::GetInstance().GetUserPath(), strFileName);
}

int FileUtils::GetFileContents(const std::string& url, std::string& strContent)
{
  strContent.clear();
  void* fileHandle = XBMC->OpenFile(url.c_str(), 0);
  if (fileHandle)
  {
    char buffer[1024];
    while (int bytesRead = XBMC->ReadFile(fileHandle, buffer, 1024))
      strContent.append(buffer, bytesRead);
    XBMC->CloseFile(fileHandle);
  }

  return strContent.length();
}
