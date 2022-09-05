# Chapter Marker

This is a plugin for OBS that lets you add chapters to the video file you are recording which are marked via a hotkey. This is useful when recording videos and you want to mark and highlight an important part of the video thus making it easier to find those moments. Useful for gameplay highlights when recording lengthy videos.

## Building

Use CMake to generate a project in your preferred toolchain (I used Visual Studio 2022) and compile with your toolchain.

Set your CMAKE_PREFIX_PATH properly, an example is below.

`F:/code/Visual Studio/Visual Studio Community 2019/obs-studio/build/libobs;F:/code/Visual Studio/Visual Studio Community 2019/obs-studio/build/deps/w32-pthreads;F:/code/Visual Studio/Visual Studio Community 2019/obs-studio/build/UI/obs-frontend-api;F:/code/Visual Studio/Visual Studio Community 2019/obs-deps/lib;F:/code/Visual Studio/Visual Studio Community 2019/obs-deps/include;F:/code/Visual Studio/Visual Studio Community 2019/obs-deps` 

**Note**: The project uses C++17 and the project was forked off [https://github.com/obsproject/obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate).

## Basic Usage

- After install this plugin, a new option "Create chapter marker" will be available (File > Preferences > Hotkeys). Map it to something meaningful to you:
  - ![OBS option](imgs/obs_option.jpg)
- Do some recording, invoking this hotkey.
- Open the mkv file created, and search the chapters marks, e.g.:
  - Each chapter will have a number as identification (starting with 1)
  - ![list of chapters in VLC-player](imgs/vlc_chapters_example.gif)
- Markers are displayed differently according to the video player, so, an alternative is to list chapters using `ffprobe -show_chapters <your_video_name.mkv>`.
