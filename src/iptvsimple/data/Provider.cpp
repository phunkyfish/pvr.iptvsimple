/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Provider.h"

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace kodi::tools;

bool Provider::Like(const Provider& right) const
{
  bool isLike = (m_providerName == right.m_providerName);

  return isLike;
}

bool Provider::operator==(const Provider& right) const
{
  bool isEqual = (m_providerName == right.m_providerName);
  isEqual &= (m_providerType == right.m_providerType);
  isEqual &= (m_iconPath == right.m_iconPath);
  isEqual &= (m_country == right.m_country);
  isEqual &= (m_language == right.m_language);

  return isEqual;
}

bool Provider::operator!=(const Provider& right) const
{
  return !(*this == right);
}

void Provider::UpdateTo(kodi::addon::PVRProvider& left) const
{
  left.SetUniqueId(m_uniqueId);
  left.SetProviderName(m_providerName);
  left.SetProviderType(m_providerType);
  left.SetIconPath(m_iconPath);
  left.SetCountry(m_country);
  left.SetLanguage(m_language);
}
