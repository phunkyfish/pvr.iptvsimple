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

#include "rapidxml/rapidxml.hpp"
#include <string>

template<class Ch>
inline std::string GetNodeValue(const rapidxml::xml_node<Ch>* pRootNode, const char* strTag)
{
  rapidxml::xml_node<Ch>* pChildNode = pRootNode->first_node(strTag);
  if (!pChildNode)
    return "";

  return pChildNode->value();
}

template<class Ch>
inline bool GetAttributeValue(const rapidxml::xml_node<Ch>* pNode, const char* strAttributeName, std::string& strStringValue)
{
  rapidxml::xml_attribute<Ch>* pAttribute = pNode->first_attribute(strAttributeName);
  if (!pAttribute)
  {
    return false;
  }
  strStringValue = pAttribute->value();
  return true;
}
