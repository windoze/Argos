# Locate Log4cplus library
# This module defines
#  LOG4CPLUS_FOUND, if false, do not try to link to Log4cplus
#  LOG4CPLUS_LIBRARIES
#  LOG4CPLUS_INCLUDE_DIR, where to find log4cplus.hpp

FIND_PATH(LOG4CPLUS_INCLUDE_DIR log4cplus/logger.h
  HINTS
  $ENV{LOG4CPLUS_DIR}
  PATH_SUFFIXES include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

FIND_LIBRARY(LOG4CPLUS_RELEASE_LIBRARY
  NAMES LOG4CPLUS log4cplus
  HINTS
  $ENV{LOG4CPLUS_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)

FIND_LIBRARY(LOG4CPLUS_DEBUG_LIBRARY
  NAMES log4cplusD
  HINTS
  $ENV{LOG4CPLUS_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)

IF(LOG4CPLUS_RELEASE_LIBRARY)
    IF(LOG4CPLUS_DEBUG_LIBRARY)
      SET(LOG4CPLUS_LIBRARIES debug ${LOG4CPLUS_DEBUG_LIBRARY} optimized ${LOG4CPLUS_RELEASE_LIBRARY} CACHE STRING "Log4cplus Libraries")
    ELSE(LOG4CPLUS_DEBUG_LIBRARY)
      SET(LOG4CPLUS_LIBRARIES ${LOG4CPLUS_RELEASE_LIBRARY} CACHE STRING "Log4cplus Libraries")
    ENDIF()
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LOG4CPLUS_FOUND to TRUE if 
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Log4cplus DEFAULT_MSG LOG4CPLUS_LIBRARIES LOG4CPLUS_INCLUDE_DIR)

MARK_AS_ADVANCED(LOG4CPLUS_INCLUDE_DIR LOG4CPLUS_LIBRARIES LOG4CPLUS_DEBUG_LIBRARY LOG4CPLUS_RELEASE_LIBRARY)
