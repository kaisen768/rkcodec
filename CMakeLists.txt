cmake_minimum_required(VERSION 3.4)
cmake_policy(SET CMP0048 NEW)

project(rkcodec LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# set(UV_USE_STATIC_LIBS TRUE)
# find_package(uv REQUIRED)

find_package(FFmpeg REQUIRED)

find_package(rockchip_mpp REQUIRED)

file(GLOB_RECURSE
        SOURCE_FILES
        main.c
        rkcodec.c
        )

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

include_directories(
        ${CMAKE_SOURCE_DIR}

        # ${UV_INCLUDE_DIR}
        ${FFMPEG_INCLUDE_DIRS}
        ${ROCKCHIP_MPP_INCLUDE_DIR}
        )

target_link_libraries( ${PROJECT_NAME}
        # ${UV_LIBRARY}
        ${FFMPEG_LIBRARIES}
        ${ROCKCHIP_MPP_LIBRARIES}
        pthread
        dl
        )

