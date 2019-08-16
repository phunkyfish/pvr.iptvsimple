#include "Channel.h"

#include "../Settings.h"
#include "inttypes.h"
#include "p8-platform/util/StringUtils.h"
#include "util/XMLUtils.h"

#include <regex>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

void Channel::UpdateTo(Channel& left) const
{
  left.m_uniqueId         = m_uniqueId;
  left.m_radio            = m_radio;
  left.m_channelNumber    = m_channelNumber;
  left.m_encryptionSystem = m_encryptionSystem;
  left.m_channelName      = m_channelName;
  left.m_logoPath         = m_logoPath;
  left.m_streamURL        = m_streamURL;
  left.m_properties       = m_properties;
}