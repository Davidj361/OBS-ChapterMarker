# Chapter Marker

## Building

Use CMake to build the project, change the some of the variable in the `CMakeLists.txt`, i.e.
```cmake
# Hackish CMake variables that were added to finally get this working
set(LIBOBS_LIB "C:/Users/DJ/source/repos/obs-studio/build/libobs/Release")
set(LIBOBS_INCLUDE_DIR "C:/Users/DJ/source/repos/obs-studio/libobs")
set(LibObs_DIR "C:/Users/DJ/source/repos/obs-studio/build/libobs")
set(Qt5_DIR "C:/Users/DJ/source/repos/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5")
set(OBS_FRONTEND_LIB "C:/Users/DJ/source/repos/obs-studio/build/UI/obs-frontend-api/Release/obs-frontend-api.lib")
```

**Note**: uses C++17
