/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>

#include <kodi/AddonBase.h>
#include <kodi/addon-instance/pvr/Providers.h>

namespace iptvsimple
{
  namespace data
  {
    class ATTRIBUTE_HIDDEN Provider
    {
    public:
      Provider() = default;
      Provider(const Provider& b) : m_uniqueId(b.GetUniqueId()), m_providerName(b.GetProviderName()),
      m_providerType(b.GetProviderType()), m_iconPath(b.GetIconPath()), m_country(b.GetCountry()),
      m_language(b.GetLanguage()) {};
      ~Provider() = default;

      int GetUniqueId() const { return m_uniqueId; }
      void SetUniqueId(int value) { m_uniqueId = value; }

      const std::string& GetProviderName() const { return m_providerName; }
      void SetProviderName(const std::string& value) { m_providerName = value; }

      PVR_PROVIDER_TYPE GetProviderType() const { return m_providerType; }
      void SetProviderType(PVR_PROVIDER_TYPE value) { m_providerType = value; }

      const std::string& GetIconPath() const { return m_iconPath; }
      void SetIconPath(const std::string& value) { m_iconPath = value; }

      const std::string& GetCountry() const { return m_country; }
      void SetCountry(const std::string& value) { m_country = value; }

      const std::string& GetLanguage() const { return m_language; }
      void SetLanguage(const std::string& value) { m_language = value; }

      void UpdateTo(kodi::addon::PVRProvider& left) const;

      bool Like(const Provider& right) const;
      bool operator==(const Provider& right) const;
      bool operator!=(const Provider& right) const;

    protected:
      int m_uniqueId = -1;
      std::string m_providerName;
      PVR_PROVIDER_TYPE m_providerType;
      std::string m_iconPath;
      std::string m_country;
      std::string m_language;
    };
  } //namespace data
} //namespace enigma2
