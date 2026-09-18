find_package(PkgConfig REQUIRED)

pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswresample
    libswscale
)

if(NOT TARGET SDL3::SDL3)
    message(FATAL_ERROR "The vendored SDL3 project did not provide SDL3::SDL3")
endif()

add_library(tutorial_dependencies INTERFACE)

target_link_libraries(tutorial_dependencies INTERFACE
    PkgConfig::FFMPEG
    SDL3::SDL3
)