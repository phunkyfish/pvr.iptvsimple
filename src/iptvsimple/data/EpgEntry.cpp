#include "EpgEntry.h"

#include "../Settings.h"
#include "inttypes.h"
#include "p8-platform/util/StringUtils.h"
#include "util/XMLUtils.h"

#include <regex>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

void EpgEntry::UpdateTo(EPG_TAG& left, int iChannelUid, int timeShift, std::vector<EpgGenre>& genres)
{
  left.iUniqueBroadcastId  = m_broadcastId;
  left.strTitle            = m_title.c_str();
  left.iUniqueChannelId    = iChannelUid;
  left.startTime           = m_startTime + timeShift;
  left.endTime             = m_endTime + timeShift;
  left.strPlotOutline      = m_plotOutline.c_str();
  left.strPlot             = m_plot.c_str();
  left.strOriginalTitle    = nullptr;  /* not supported */
  left.strCast             = m_cast.c_str();
  left.strDirector         = m_director.c_str();
  left.strWriter           = m_writer.c_str();
  left.iYear               = 0;     /* not supported */
  left.strIMDBNumber       = nullptr;  /* not supported */
  left.strIconPath         = m_iconPath.c_str();
  if (SetEpgGenre(genres, m_genreString))
  {
    left.iGenreType          = m_genreType;
    left.iGenreSubType       = m_genreSubType;
    left.strGenreDescription = nullptr;
  }
  else
  {
    left.iGenreType          = EPG_GENRE_USE_STRING;
    left.iGenreSubType       = 0;     /* not supported */
    left.strGenreDescription = m_genreString.c_str();
  }
  left.iParentalRating     = 0;     /* not supported */
  left.iStarRating         = 0;     /* not supported */
  left.iSeriesNumber       = 0;     /* not supported */
  left.iEpisodeNumber      = 0;     /* not supported */
  left.iEpisodePartNumber  = 0;     /* not supported */
  left.strEpisodeName      = m_episodeName.c_str();
  left.iFlags              = EPG_TAG_FLAG_UNDEFINED;
}

bool EpgEntry::SetEpgGenre(std::vector<EpgGenre> genres, const std::string& genreToFind)
{
  if (genres.empty())
    return false;

  for (const auto& myGenre : genres)
  {
    if (StringUtils::CompareNoCase(myGenre.GetGenreString(), genreToFind) == 0)
    {
      m_genreType = myGenre.GetGenreType();
      m_genreSubType = myGenre.GetGenreSubType();
      return true;
    }
  }

  return false;
}