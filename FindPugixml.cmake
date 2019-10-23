# - Try to find pugixml
# Once done this will define
#
# PUGIXML_FOUND - system has pugixml
# PUGIXML_INCLUDE_DIRS - the pugixml include directory
# PUGIXML_LIBRARIES - the pugixml library

find_path(PUGIXML_INCLUDE_DIRS pugixml/pugixml.hpp)
find_library(PUGIXML_LIBRARIES pugixml)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Pugixml DEFAULT_MSG PUGIXML_LIBRARIES PUGIXML_INCLUDE_DIRS)

mark_as_advanced(PUGIXML_INCLUDE_DIRS)
