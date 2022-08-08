cmake_minimum_required(VERSION 3.5)
project(sei)

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

    set(PROCESS x86_64)
    IF(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm")
        message(STATUS "this is arm cpu")
        set(PROCESS arm)
    ELSEIF(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
        message(STATUS "this is x86_64 cpu")
        set(PROCESS x86_64)
    ENDIF(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm")

    link_directories(third_party/ffmpeg/lib/${PROCESS})
    link_directories(third_party/glog/lib/${PROCESS})
    link_directories(third_party/x264/lib/${PROCESS})

    set(COMMON_LIB
            -lx264 -lz -lbz2 -liconv
            -lavformat -lavutil -lavcodec -lswresample -lavfilter -lswscale -lpostproc
            -lglog
            )
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

aux_source_directory(src/add_sei ADD_SEI_SRC)
aux_source_directory(src/read_sei READ_SEI_SRC)

add_executable(sei main.cpp ${ADD_SEI_SRC} ${READ_SEI_SRC})
target_link_libraries(sei ${COMMON_LIB})
