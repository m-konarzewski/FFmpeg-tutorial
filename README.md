# FFmpeg Tutorial

This repository contains lessons for FFmpeg. Each lesson is a separate executable under `src/`.

## Platform Support

Linux is currently the only supported and tested platform. The build uses
`pkg-config` to find the system FFmpeg development libraries.

## Dependencies

- CMake 3.24 or newer
- A C++23 compiler
- FFmpeg development packages: `libavcodec`, `libavformat`, `libavutil`, `libavfilter`,
  `libswresample`, and `libswscale`
- The SDL3 Git submodule under `vendored/SDL`

FFmpeg is discovered through `pkg-config`. SDL3 is added to the build from the
submodule source tree with `add_subdirectory`.

## Get the Source

Because of SDL existing as a submodule, you can use:

```sh
git clone --recurse-submodules https://github.com/m-konarzewski/FFmpeg-tutorial.git
```

Otherwise, first clone the tutorial repository itself:

```sh
git clone https://github.com/m-konarzewski/FFmpeg-tutorial.git
cd FFmpeg-tutorial
```

Then fetch and initialize the SDL3 submodule:

```sh
git submodule update --init --recursive
```

## Build All Lessons

Use the build script with either `debug` or `release`:

```sh
./build.sh debug
./build.sh release
```

## Run Lessons

Run lessons with e.g.:

```sh
./build/debug/Debug/lesson-1
./build/release/Release/lesson-1
```
