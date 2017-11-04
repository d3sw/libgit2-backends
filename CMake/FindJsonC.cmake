# Try to find json-c
# Once done, this will define
#
# JSONC_FOUND         - system has jsonc
# JSONC__INCLUDE_DIRS - the jsonc include directories
# JSONC__LIBRARIES    - jsonc libraries directories

SET(JSONC_DEFINITIONS ${PC_JSONC_CFLAGS_OTHER})
SET(PC_JSONC_INCLUDE_DIRS ${PC_JSONC_INCLUDE_DIRS} /usr/include/json-c/ /usr/local/include/json-c/)
SET(PC_JSONC_LIBRARY_DIRS ${PC_JSONC_LIBRARY_DIRS} /usr/lib/ /usr/local/lib/)

FIND_PATH(JSONC_INCLUDE_DIR json.h
    HINTS
    ${PC_JSONC_INCLUDEDIR}
    ${PC_JSONC_INCLUDE_DIRS}
)

FIND_LIBRARY(JSONC_LIBRARY json-c
    HINTS
    ${PC_JSONC_LIBDIR}
    ${PC_JSONC_LIBRARY_DIRS}
)

SET(JSONC_INCLUDE_DIRS ${JSONC_INCLUDE_DIR})
SET(JSONC_LIBRARIES ${JSONC_LIBRARY})

# handle the QUIETLY and REQUIRED arguments and set JSONC_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(jsonc DEFAULT_MSG JSONC_INCLUDE_DIRS JSONC_LIBRARIES)

MARK_AS_ADVANCED(JSONC_INCLUDE_DIRS JSONC_LIBRARIES)
