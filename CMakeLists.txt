cmake_minimum_required(VERSION 3.17)

set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

project(cmaketest)

set (CMAKE_CXX_STANDARD 20)

find_package(SDL2 CONFIG REQUIRED)

find_package(libsoundio CONFIG REQUIRED)

find_path( AVCODEC_INCLUDE_DIR libavcodec/avcodec.h )
find_library(AVCODEC_LIBRARY avcodec)

find_path( AVFORMAT_INCLUDE_DIR libavformat/avformat.h )
find_library(AVFORMAT_LIBRARY avformat)

find_path( AVUTIL_INCLUDE_DIR libavutil/avutil.h )
find_library(AVUTIL_LIBRARY avutil)

find_path( AVFILTER_INCLUDE_DIR libavfilter/avfilter.h )
find_library(AVFILTER_LIBRARY avfilter)

find_path( AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h )
find_library(AVDEVICE_LIBRARY avdevice)

find_path( SWRESAMPLE_INCLUDE_DIR libswresample/swresample.h )
find_library(SWRESAMPLE_LIBRARY swresample)

find_path( SWSCALE_INCLUDE_DIR libswscale/swscale.h )
find_library(SWSCALE_LIBRARY swscale)

add_executable(cmaketest src/main.cpp "src/video_reader.hpp" "src/audio_player.hpp")

target_include_directories(cmaketest PRIVATE 
	${AVCODEC_INCLUDE_DIR}
	${AVFORMAT_INCLUDE_DIR}
	${AVUTIL_INCLUDE_DIR}
	${AVFILTER_INCLUDE_DIR}
	${AVDEVICE_INCLUDE_DIR}
	${SWRESAMPLE_INCLUDE_DIR}
	${SWSCALE_INCLUDE_DIR}
)

target_link_libraries(cmaketest PRIVATE 
	SDL2::SDL2 
	SDL2::SDL2main
	${AVCODEC_LIBRARY}
	${AVFORMAT_LIBRARY}
	${AVUTIL_LIBRARY}
	${AVFILTER_LIBRARY}
	${AVDEVICE_LIBRARY}
	${SWRESAMPLE_LIBRARY}
	${SWSCALE_LIBRARY}
	libsoundio::libsoundio 
	libsoundio::libsoundio_shared
)