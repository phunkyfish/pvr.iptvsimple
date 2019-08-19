#include "ChannelEpg.h"

#include "../utilities/XMLUtils.h"

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace rapidxml;

bool ChannelEpg::UpdateFrom(xml_node<>* pChannelNode, Channels& channels)
{
  std::string strName;
  std::string strId;
  if (!GetAttributeValue(pChannelNode, "id", strId))
    return false;

  strName = GetNodeValue(pChannelNode, "display-name");
  if (!channels.FindChannel(strId, strName))
    return false;

  m_id = strId;
  m_name = strName;

  // get icon if available
  xml_node<>* pIconNode = pChannelNode->first_node("icon");
  std::string icon = m_icon;
  if (!pIconNode || !GetAttributeValue(pIconNode, "src", icon))
    m_icon = "";
  else
    m_icon = icon;

  return true;
}