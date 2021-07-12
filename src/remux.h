#pragma once

// a trick to get C++ functions to be called in C
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>

AVChapter* avpriv_new_chapter(int64_t id, AVRational time_base,
    int64_t start, int64_t end, const char* title);
int startRemux(const char* filename, const char* newFilename);
int finishRemux();
int end(int ret);

EXTERNC void errorPopup(const char* s);