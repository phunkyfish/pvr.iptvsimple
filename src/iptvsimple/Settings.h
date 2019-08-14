#pragma once

#include "utilities/Logger.h"
#include "xbmc_addon_types.h"

#include <string>

#include <p8-platform/util/StringUtils.h>

class PVRIptvData;

namespace iptvsimple
{
  static const std::string M3U_FILE_NAME = "iptv.m3u.cache";
  static const std::string TVG_FILE_NAME = "xmltv.xml.cache";

  enum class PathType
    : int // same type as addon settings
  {
    LOCAL_PATH = 0,
    REMOTE_PATH
  };

  enum class XmltvLogosType
    : int // same type as addon settings
  {
    IGNORE = 0,
    PREFER_M3U,
    PREFER_XMLTV
  };

  class Settings
  {
  public:
    /**
     * Singleton getter for the instance
     */
    static Settings& GetInstance()
    {
      static Settings settings;
      return settings;
    }

    void ReadFromAddon(const std::string& userPath, const std::string clientPath);
    ADDON_STATUS SetValue(const std::string& settingName, const void* settingValue);

    const std::string& GetUserPath() const { return m_userPath; }
    const std::string& GetClientPath() const { return m_clientPath; }
    const std::string& GetM3UPath() const { return m_m3uPath; }
    const std::string& GetTvgPath() const { return m_tvgPath; }
    const std::string& GetLogoPath() const { return m_logoPath; }
    int GetEpgTimeshift() const { return m_epgTimeShift; }
    int GetStartNumber() const { return m_startNumber; }
    bool GetTsOverride() const { return m_tsOverride; }
    bool UseM3UCache() const { return m_cacheM3U; }
    bool UseEPGCache() const { return m_cacheEPG; }
    int GetEpgLogos() const { return m_epgLogos; }

  private:
    Settings() = default;

    Settings(Settings const&) = delete;
    void operator=(Settings const&) = delete;

    std::string m_userPath = "";
    std::string m_clientPath = "";
    std::string m_m3uPath = "";
    std::string m_tvgPath = "";
    std::string m_logoPath = "";
    int m_epgTimeShift = 0;
    int m_startNumber = 1;
    bool m_tsOverride = true;
    bool m_cacheM3U = false;
    bool m_cacheEPG = false;
    int m_epgLogos = 0;
  };
} //namespace iptvsimple
