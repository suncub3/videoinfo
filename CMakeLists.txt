cmake_minimum_required(VERSION 3.5)
project(test)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_BUILD_TYPE "Debug")

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    message(STATUS "current platform is LINUX")
    add_definitions(-DADDSEI_LINUX)

    set(CMAKE_SHARED_LINKER_FLAGS "-fPIC")
    add_compile_options(-fPIC)

    set(COMMON_LIB
            -lavformat -lavutil -lavcodec -lswresample -lavfilter -lswscale -lpostproc
            -lglog
            -lstdc++
            )
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    message(STATUS "current platform is MAC")
    add_definitions(-DADDSEI_MAC)
    set(CMAKE_EXE_LINKER_FLAGS " -framework Security -framework AudioToolbox -framework CoreVideo -framework VideoToolbox -framework CoreMedia -framework Cocoa")

    include_directories(third_party/ffmpeg/include)
    include_directories(third_party/x264/include)
    include_directories(third_party/glog/include)
    include_directories(third_party/gflags/include)

    IF(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm")
        message(STATUS "this is aarch64 cpu")
        link_directories(third_party/ffmpeg/lib/aarch64)
        link_directories(third_party/glog/lib/aarch64)
        link_directories(third_party/x264/lib/aarch64)
    ELSEIF(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
        message(STATUS "this is x86 cpu")
        link_directories(third_party/ffmpeg/lib/x86)
        link_directories(third_party/glog/lib/x86)
        link_directories(third_party/x264/lib/x86)
    ENDIF(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")

    set(COMMON_LIB
            -lx264 -lz -lbz2 -liconv
            -lavformat -lavutil -lavcodec -lswresample -lavfilter -lswscale -lpostproc
            -lglog
            #-lmp3lame -lopenjp2
            #-lstdc++ -lpthread -lm -llzma
            )
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

aux_source_directory(src/add_sei ADD_SEI_SRC)
aux_source_directory(src/read_sei READ_SEI_SRC)

add_executable(test main.cpp ${ADD_SEI_SRC} ${READ_SEI_SRC})
target_link_libraries(test ${COMMON_LIB})
