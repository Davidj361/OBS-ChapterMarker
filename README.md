# Chapter Marker

https://obsproject.com/forum/resources/chapter-marker.1323/

<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-refresh-toc -->
**Table of Contents**

- [Chapter Marker](#chapter-marker)
    - [Disclaimer](#disclaimer)
    - [Building](#building)
    - [Basic Usage](#basic-usage)

<!-- markdown-toc end -->

## Description

This is a plugin for OBS that lets you add chapters to the video file you are recording which are marked via a hotkey. A great alternative to using InfoWriter. Useful for video highlights when recording lengthy videos. Can be used for gameplay analysis like analyzing deaths, or marking parts of a recording for editing purposes, or even just for gameplay highlights that you want in your killmontage.

## Disclaimer

* **Only operates when recording MKV files**. Reason being that MKV containers end up being the least corruptable in my opinion, but I might change that if people request it and test for me.
* **2x the space of a recorded video is REQUIRED**. Meaning if your finished recording is 1 GB, you'll need at least 2 GB of free space on your drive. Reason being is that I cannot intercept the recording/encoding of OBS. Instead the recorded video is duplicated (remuxed) but with added on metadata and where the older recording is deleted and the newer one is renamed to the older one. If you don't have enough space on the drive you're recording onto then no duplication or deletion is made but OBS may bug out.


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
- **obs-websocket users**: To push the hotkey "ChapterMarker"
