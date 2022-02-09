# FindJemalloc
# -----------
#
# Find the Jemalloc library.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module will set the following variables in your project:
#
# ``JEMALLOC_FOUND``
#   System has the Jemalloc library.
# ``JEMALLOC_INCLUDE_DIR``
#   The Jemalloc include directory.
# ``JEMALLOC_LIBRARY``
#   The Jemalloc jemalloc library.
# ``JEMALLOC_PIC_LIBRARY``
#   The Jemalloc jemalloc_pic library.
# ``JEMALLOC_LIBRARIES``
#   All Jemalloc libraries.
#
# Hints
# ^^^^^
#
# Set ``JEMALLOC_ROOT_DIR`` to the root directory of an Jemalloc installation.
# Set ``JEMALLOC_USE_STATIC_LIBS`` to ``TRUE`` to look for static libraries.

# Support preference of static libs by adjusting CMAKE_FIND_LIBRARY_SUFFIXES
if(JEMALLOC_USE_STATIC_LIBS)
    set(_jemalloc_ORING_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(${CMAKE_FIND_LIBRARY_SUFFIXES} .a)
endif()

find_package(PkgConfig QUIET)
pkg_check_modules(JEMALLOC QUIET "jemalloc")

include(FindPackageHandleStandardArgs)
find_path(JEMALLOC_INCLUDE_DIR 
    NAMES 
        jemalloc/jemalloc.h
    )
find_library(JEMALLOC_LIBRARY 
    NAMES
        jemalloc
    )
find_library(JEMALLOC_PIC_LIBRARY 
    NAMES
        jemalloc_pic
    )
find_package_handle_standard_args(JEMALLOC
    DEFAULT_MSG
    JEMALLOC_INCLUDE_DIR
    JEMALLOC_LIBRARY
    JEMALLOC_PIC_LIBRARY
    )

mark_as_advanced(
    JEMALLOC_INCLUDE_DIR
    JEMALLOC_LIBRARY
    JEMALLOC_PIC_LIBRARY
    )

if (JEMALLOC_FOUND)
    set(JEMALLOC_PIC_LIBRARIES ${JEMALLOC_PIC_LIBRARY})
    set(JEMALLOC_LIBRARIES ${JEMALLOC_LIBRARY} ${JEMALLOC_PIC_LIBRARY})
    mark_as_advanced(JEMALLOC_LIBRARIES JEMALLOC_PIC_LIBRARIES)
endif()

# Restore the original find library ordering
if(JEMALLOC_USE_STATIC_LIBS)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${_jemalloc_ORING_CMAKE_FIND_LIBRARY_SUFFIXES})
endif()


# # Support preference of static libs by adjusting CMAKE_FIND_LIBRARY_SUFFIXES
# if(JEMALLOC_USE_STATIC_LIBS)
#     set(_jemalloc_ORING_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
#     set(${CMAKE_FIND_LIBRARY_SUFFIXES} .a)
# endif()

# set(_JEMALLOC_ROOT_HINTS ${JEMALLOC_ROOT_DIR} ENV JEMALLOC_ROOT_DIR)

# set(_JEMALLOC_ROOT_HINTS_AND_PATHS 
#     HINTS ${_JEMALLOC_ROOT_HINTS}
#     PATHS ${_JEMALLOC_ROOT_PATHS}
#     )

# find_path(JEMALLOC_INCLUDE_DIR
#     NAMES
#         jemalloc/jemalloc.h
#     ${_JEMALLOC_ROOT_HINTS_AND_PATHS}
#     HINTS
#         ${_JEMALLOC_INCLUDEDIR}
#     PATH_SUFFIXES
#         include
#     )

# find_library(JEMALLOC_LIBRARY
#     NAMES
#         jemalloc
#     NAMES_PER_DIR
#     ${_JEMALLOC_ROOT_HINTS_AND_PATHS}
#     HINTS
#         ${_JEMALLOC_LIBDIR}
#     PATH_SUFFIXES
#         lib
#     )

# find_library(JEMALLOC_PIC_LIBRARY
#     NAMES
#         jemalloc_pic
#     NAMES_PER_DIR
#     ${_JEMALLOC_ROOT_HINTS_AND_PATHS}
#     HINTS
#         ${_JEMALLOC_LIBDIR}
#     PATH_SUFFIXES
#         lib
#     )

# mark_as_advanced(JEMALLOC_LIBRARY JEMALLOC_PIC_LIBRARY)

# # compat defines
# set(JEMALLOC_PIC_LIBRARIES ${JEMALLOC_PIC_LIBRARY})
# set(JEMALLOC_LIBRARIES ${JEMALLOC_LIBRARY} ${JEMALLOC_PIC_LIBRARY})

# if(JEMALLOC_INCLUDE_DIR AND JEMALLOC_LIBRARY AND JEMALLOC_PIC_LIBRARY)
#     set(JEMALLOC_FOUND TRUE)
# endif()

# mark_as_advanced(JEMALLOC_INCLUDE_DIR JEMALLOC_LIBRARIES)

# # Restore the original find library ordering
# if(JEMALLOC_USE_STATIC_LIBS)
#   set(CMAKE_FIND_LIBRARY_SUFFIXES ${_jemalloc_ORING_CMAKE_FIND_LIBRARY_SUFFIXES})
# endif()
