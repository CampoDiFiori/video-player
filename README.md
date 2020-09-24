# Video Player

This is a cute little video player that *should* play any video roughly at the speed of the encoded fps,
but it's more of an educational project, so it's rough around the edges for now. It only plays it from beginning to end, cannot stop the video or seek (yet).
It uses `ffmpeg` to decode the video and `SDL2` to present it.

## Setup

First you'd need to download `ffmpeg` and `SDL2` via `vcpkg` (CMakeLists.txt is set up to support `vcpkg`, but you can change it if you know how to):

- `vcpkg install ffmpeg SDL2`

- Then make sure that your system has a `VCPKG_ROOT` environmental variable set up (it should point to `vcpkg` installation folder).
- Then you should be fine if you run the regular `cmake` and `make` commands from some `build` directory, but I never actually did that myself. Because I built it on Windows
I was always using some IDE, it should work out of the box if you run it in Visual Studio or VS Code with CMake Tools extension or CLion.

## TODO
- Decoding a frame from the video stream takes a lot of time (10 ms or so) - investigate
- Possibly decode streams on a separate thread (should be fun and more efficient)
- Add the video's audio stream decoding and actual sound via `libsoundio` probably
- Maybe some UI to support seeking and pausing?
