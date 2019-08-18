#include "ChannelGroups.h"

#include "../client.h"
#include "utilities/Logger.h"

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

ChannelGroups::ChannelGroups(Channels& channels)
      : m_channels(channels)
{
}

void ChannelGroups::Clear()
{
  m_channelGroups.clear();
}

int ChannelGroups::GetChannelGroupsAmount()
{
  return m_channelGroups.size();
}

void ChannelGroups::GetChannelGroups(std::vector<PVR_CHANNEL_GROUP>& kodiChannelGroups, bool radio) const
{
  Logger::Log(LEVEL_DEBUG, "%s - Starting to get ChannelGroups for PVR", __FUNCTION__);

  for (const auto& channelGroup : m_channelGroups)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Transfer channelGroup '%s', ChannelGroupIndex '%d'", __FUNCTION__, channelGroup.GetGroupName().c_str(), channelGroup.GetGroupId());

    if (channelGroup.IsRadio() == radio)
    {
      PVR_CHANNEL_GROUP kodiChannelGroup = {0};

      channelGroup.UpdateTo(kodiChannelGroup);

      kodiChannelGroups.emplace_back(kodiChannelGroup);
    }
  }

  Logger::Log(LEVEL_DEBUG, "%s - Finished getting ChannelGroups for PVR", __FUNCTION__);
}

PVR_ERROR ChannelGroups::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& group)
{
  ChannelGroup* myGroup = FindChannelGroup(group.strGroupName);
  if (myGroup)
  {
    for (int memberId : myGroup->GetMemberChannels())
    {
      if ((memberId) < 0 || (memberId) >= static_cast<int>(m_channels.GetChannelsAmount()))
        continue;

      Channel& channel = m_channels.GetChannelsList().at(memberId);
      PVR_CHANNEL_GROUP_MEMBER xbmcGroupMember;
      memset(&xbmcGroupMember, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));

      strncpy(xbmcGroupMember.strGroupName, group.strGroupName, sizeof(xbmcGroupMember.strGroupName) - 1);
      xbmcGroupMember.iChannelUniqueId = channel.GetUniqueId();
      xbmcGroupMember.iChannelNumber = channel.GetChannelNumber();

      PVR->TransferChannelGroupMember(handle, &xbmcGroupMember);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

ChannelGroup* ChannelGroups::FindChannelGroup(const std::string& strName)
{
  for (auto& myGroup : m_channelGroups)
  {
    if (myGroup.GetGroupName() == strName)
      return &myGroup;
  }

  return nullptr;
}