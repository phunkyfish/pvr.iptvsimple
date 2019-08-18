#include "Channels.h"

#include "../client.h"
#include "Settings.h"
#include "utilities/FileUtils.h"
#include "utilities/Logger.h"

#include <regex>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

Channels::Channels()
{
  m_strLogoPath = Settings::GetInstance().GetLogoPath();
}

void Channels::Clear()
{
  m_channels.clear();
}

int Channels::GetChannelsAmount()
{
  return m_channels.size();
}

void Channels::GetChannels(std::vector<PVR_CHANNEL>& kodiChannels, bool bRadio) const
{
  for (const auto& channel : m_channels)
  {
    if (channel.IsRadio() == bRadio)
    {
      Logger::Log(LEVEL_DEBUG, "%s - Transfer channel '%s', ChannelIndex '%d'", __FUNCTION__, channel.GetChannelName().c_str(),
                  channel.GetUniqueId());
      PVR_CHANNEL kodiChannel = {0};

      channel.UpdateTo(kodiChannel);

      kodiChannels.emplace_back(kodiChannel);
    }
  }
}

bool Channels::GetChannel(const PVR_CHANNEL& channel, Channel& myChannel)
{
  for (const auto& thisChannel : m_channels)
  {
    if (thisChannel.GetUniqueId() == static_cast<int>(channel.iUniqueId))
    {
      thisChannel.UpdateTo(myChannel);

      return true;
    }
  }

  return false;
}

const Channel* Channels::FindChannel(const std::string& strId, const std::string& strName) const
{
  const std::string strTvgName = std::regex_replace(strName, std::regex(" "), "_");

  for (const auto& myChannel : m_channels)
  {
    if (myChannel.GetTvgId() == strId)
      return &myChannel;

    if (strTvgName == "")
      continue;

    if (myChannel.GetTvgName() == strTvgName)
      return &myChannel;

    if (myChannel.GetChannelName() == strName)
      return &myChannel;
  }

  return nullptr;
}

void Channels::ApplyChannelLogos()
{
  for (auto& channel : m_channels)
  {
    if (!channel.GetTvgLogo().empty())
    {
      if (!m_strLogoPath.empty() && channel.GetTvgLogo().find("://") == std::string::npos) // special proto
        channel.SetLogoPath(FileUtils::PathCombine(m_strLogoPath, channel.GetTvgLogo()));
      else
        channel.SetLogoPath(channel.GetTvgLogo());
    }
  }
}

void Channels::ReapplyChannelLogos(const char* strNewPath)
{
  //P8PLATFORM::CLockObject lock(m_mutex);
  //TODO Lock should happen in calling class
  if (strlen(strNewPath) > 0)
  {
    m_strLogoPath = strNewPath;
    ApplyChannelLogos();

    PVR->TriggerChannelUpdate();
    PVR->TriggerChannelGroupsUpdate();
  }
}