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

#include "zlib.h"

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

/*
 * This method uses zlib to decompress a gzipped file in memory.
 * Author: Andrew Lim Chong Liang
 * http://windrealm.org
 */
bool FileUtils::GzipInflate(const std::string& compressedBytes, std::string& uncompressedBytes)
{

#define HANDLE_CALL_ZLIB(status) {   \
  if(status != Z_OK) {        \
    free(uncomp);             \
    return false;             \
  }                           \
}

  if (compressedBytes.size() == 0)
  {
    uncompressedBytes = compressedBytes;
    return true;
  }

  uncompressedBytes.clear();

  unsigned full_length = compressedBytes.size();
  unsigned half_length = compressedBytes.size() / 2;

  unsigned uncompLength = full_length;
  char* uncomp = static_cast<char*>(calloc(sizeof(char), uncompLength));

  z_stream strm;
  strm.next_in = (Bytef*)compressedBytes.c_str();
  strm.avail_in = compressedBytes.size();
  strm.total_out = 0;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;

  bool done = false;

  HANDLE_CALL_ZLIB(inflateInit2(&strm, (16 + MAX_WBITS)));

  while (!done)
  {
    // If our output buffer is too small
    if (strm.total_out >= uncompLength)
    {
      // Increase size of output buffer
      uncomp = static_cast<char*>(realloc(uncomp, uncompLength + half_length));
      if (!uncomp)
        return false;
      uncompLength += half_length;
    }

    strm.next_out = (Bytef*)(uncomp + strm.total_out);
    strm.avail_out = uncompLength - strm.total_out;

    // Inflate another chunk.
    int err = inflate(&strm, Z_SYNC_FLUSH);
    if (err == Z_STREAM_END)
      done = true;
    else if (err != Z_OK)
    {
      break;
    }
  }

  HANDLE_CALL_ZLIB(inflateEnd(&strm));

  for (size_t i = 0; i < strm.total_out; ++i)
  {
    uncompressedBytes += uncomp[i];
  }

  free(uncomp);
  return true;
}

int FileUtils::GetCachedFileContents(const std::string& strCachedName, const std::string& filePath,
                                       std::string& strContents, const bool bUseCache /* false */)
{
  bool bNeedReload = false;
  const std::string strCachedPath = FileUtils::GetUserFilePath(strCachedName);
  const std::string strFilePath = filePath;

  // check cached file is exists
  if (bUseCache && XBMC->FileExists(strCachedPath.c_str(), false))
  {
    struct __stat64 statCached;
    struct __stat64 statOrig;

    XBMC->StatFile(strCachedPath.c_str(), &statCached);
    XBMC->StatFile(strFilePath.c_str(), &statOrig);

    bNeedReload = statCached.st_mtime < statOrig.st_mtime || statOrig.st_mtime == 0;
  }
  else
    bNeedReload = true;

  if (bNeedReload)
  {
    FileUtils::GetFileContents(strFilePath, strContents);

    // write to cache
    if (bUseCache && strContents.length() > 0)
    {
      void* fileHandle = XBMC->OpenFileForWrite(strCachedPath.c_str(), true);
      if (fileHandle)
      {
        XBMC->WriteFile(fileHandle, strContents.c_str(), strContents.length());
        XBMC->CloseFile(fileHandle);
      }
    }
    return strContents.length();
  }

  return FileUtils::GetFileContents(strCachedPath, strContents);
}