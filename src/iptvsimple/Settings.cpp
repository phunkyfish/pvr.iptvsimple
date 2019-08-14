#include "Settings.h"

#include "../client.h"
#include "p8-platform/util/StringUtils.h"

using namespace ADDON;
using namespace iptvsimple;
using namespace iptvsimple::utilities;

/***************************************************************************
 * PVR settings
 **************************************************************************/
void Settings::ReadFromAddon(const std::string& userPath, const std::string clientPath)
{
  m_userPath = userPath;
  m_clientPath = clientPath;

  char buffer[1024];
  int pathType = 0;
  if (!XBMC->GetSetting("m3uPathType", &pathType))
    pathType = 1;

  if (pathType)
  {
    if (XBMC->GetSetting("m3uUrl", &buffer))
      m_m3uPath = buffer;
    if (!XBMC->GetSetting("m3uCache", &m_cacheM3U))
      m_cacheM3U = true;
  }
  else
  {
    if (XBMC->GetSetting("m3uPath", &buffer))
      m_m3uPath = buffer;

    m_cacheM3U = false;
  }

  if (!XBMC->GetSetting("startNum", &m_startNumber))
    m_startNumber = 1;

  if (!XBMC->GetSetting("epgPathType", &pathType))
    pathType = 1;

  if (pathType)
  {
    if (XBMC->GetSetting("epgUrl", &buffer))
      m_tvgPath = buffer;
    if (!XBMC->GetSetting("epgCache", &m_cacheEPG))
      m_cacheEPG = true;
  }
  else
  {
    if (XBMC->GetSetting("epgPath", &buffer))
      m_tvgPath = buffer;

    m_cacheEPG = false;
  }

  float fShift;
  if (XBMC->GetSetting("epgTimeShift", &fShift))
    m_epgTimeShift = static_cast<int>(fShift * 3600.0); // hours to seconds

  if (!XBMC->GetSetting("epgTSOverride", &m_tsOverride))
    m_tsOverride = true;

  if (!XBMC->GetSetting("logoPathType", &pathType))
    pathType = 1;

  if (XBMC->GetSetting(pathType ? "logoBaseUrl" : "logoPath", &buffer))
    m_logoPath = buffer;

  // Logos from EPG
  if (!XBMC->GetSetting("logoFromEpg", &m_epgLogos))
    m_epgLogos = 0;
}

ADDON_STATUS Settings::SetValue(const std::string& settingName, const void* settingValue)
{
  // reset cache and restart addon

  std::string strFile = GetUserFilePath(M3U_FILE_NAME);
  if (XBMC->FileExists(strFile.c_str(), false))
  {
    XBMC->DeleteFile(strFile.c_str());
  }

  strFile = GetUserFilePath(TVG_FILE_NAME);
  if (XBMC->FileExists(strFile.c_str(), false))
  {
    XBMC->DeleteFile(strFile.c_str());
  }

  return ADDON_STATUS_NEED_RESTART;
}
