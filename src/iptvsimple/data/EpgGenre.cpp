#include "EpgGenre.h"

#include "../utilities/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace rapidxml;

bool EpgGenre::UpdateFrom(rapidxml::xml_node<>* pGenreNode)
{
  std::string buff;
  if (!GetAttributeValue(pGenreNode, "type", buff))
    return false;

  if (!StringUtils::IsNaturalNumber(buff))
    return false;

  m_genreString = pGenreNode->value();
  m_genreType = atoi(buff.c_str());
  m_genreSubType = 0;

  if (GetAttributeValue(pGenreNode, "subtype", buff) && StringUtils::IsNaturalNumber(buff))
    m_genreSubType = atoi(buff.c_str());

  return true;
}
