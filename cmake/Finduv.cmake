# FindUv
# -----------
#
# Find the Uv library.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module will set the following variables in your project:
#
# ``UV_FOUND``
#   System has the Uv library.
# ``UV_INCLUDE_DIR``
#   The Uv include directory.
# ``UV_LIBRARY``
#   The Uv uv library.
# ``UV_LIBRARIES``
#   All Uv libraries.
#
# Hints
# ^^^^^
#
# Set ``UV_ROOT_DIR`` to the root directory of an Uv installation.
# Set ``UV_USE_STATIC_LIBS`` to ``TRUE`` to look for static libraries.

# Support preference of static libs by adjusting CMAKE_FIND_LIBRARY_SUFFIXES
if(UV_USE_STATIC_LIBS)
    set(_uv_ORING_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

find_package(PkgConfig QUIET)
pkg_check_modules(UV QUIET "libuv")

include(FindPackageHandleStandardArgs)

find_path(UV_INCLUDE_DIR 
    NAMES 
        uv.h
    )
find_library(UV_LIBRARY 
    NAMES
        uv
    )
find_package_handle_standard_args(UV
    DEFAULT_MSG
    UV_INCLUDE_DIR
    UV_LIBRARY
    )

mark_as_advanced(
    UV_INCLUDE_DIR
    UV_LIBRARY
    )

if(UV_FOUND)
    set(UV_LIBRARIES ${UV_LIBRARY})  
    mark_as_advanced(UV_LIBRARIES)
endif()

# Restore the original find library ordering
if(UV_USE_STATIC_LIBS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${_uv_ORING_CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

# Support preference of static libs by adjusting CMAKE_FIND_LIBRARY_SUFFIXES
# if(UV_USE_STATIC_LIBS)
#     set(_uv_ORING_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
#     set(CMAKE_FIND_LIBRARY_SUFFIXES _a.a)
# endif()

# set(_UV_ROOT_HINTS ${UV_ROOT_DIR} ENV UV_ROOT_DIR)

# set(_UV_ROOT_HINTS_AND_PATHS 
#     HINTS ${_UV_ROOT_HINTS}
#     PATHS ${_UV_ROOT_PATHS}
#     )

# find_path(UV_INCLUDE_DIR
#     NAMES
#        uv.h
#     ${_UV_ROOT_HINTS_AND_PATHS}
#     HINTS
#         ${_UV_INCLUDEDIR}
#     PATH_SUFFIXES
#         include
#     )

# find_library(UV_LIBRARY
#     NAMES
#         uv
#     NAMES_PER_DIR
#     ${_UV_ROOT_HINTS_AND_PATHS}
#     HINTS
#         ${_UV_LIBDIR}
#     PATH_SUFFIXES
#         lib
#     )

# mark_as_advanced(UV_LIBRARY)

# # compat defines
# set(UV_LIBRARIES ${UV_LIBRARY})

# if(UV_INCLUDE_DIR AND UV_LIBRARY)
#     set(UV_FOUND TRUE)
# endif()

# mark_as_advanced(UV_INCLUDE_DIR UV_LIBRARIES)

# # Restore the original find library ordering
# if(UV_USE_STATIC_LIBS)
#   set(CMAKE_FIND_LIBRARY_SUFFIXES ${_uv_ORING_CMAKE_FIND_LIBRARY_SUFFIXES})
# endif()
