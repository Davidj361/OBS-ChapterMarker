/*
Chapter Marker
Copyright (C) <2021> <David J> <david.j.361@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>
#include "plugin-macros.generated.h"

#include <Windows.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <iterator>
#include <cstdio>

using namespace std;

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

obs_output_t* recording = nullptr;
vector<uint64_t> chapters;
uint32_t num=0, den=0;
string filename = "";


// Get filename
const char* GetCurrentRecordingFilename()
{
    if (!recording)
        return nullptr;
    auto settings = obs_output_get_settings(recording);

    // mimicks the behavior of BasicOutputHandler::GetRecordingFilename :
    // try to fetch the path from the "url" property, then try "path" if the first one
    // didn't yield any result
    auto item = obs_data_item_byname(settings, "url");
    if (!item) {
        item = obs_data_item_byname(settings, "path");
        if (!item) {
            return nullptr;
        }
    }

    return obs_data_item_get_string(item);
}


// Gets how many nanoseconds currently recording
uint64_t getOutputRunningTime(obs_output_t* output) {
    if (!output || !obs_output_active(output)) {
        return 0;
    }

    video_t* video = obs_output_video(output);
    uint64_t frameTimeNs = video_output_get_frame_time(video);
    int totalFrames = obs_output_get_total_frames(output);

    // return (((uint64_t)totalFrames) * frameTimeNs);
    uint64_t ns = (((uint64_t)totalFrames) * frameTimeNs);
    uint64_t ms = ns / 1000000ULL;
    //uint64_t secs = ms / 1000ULL;
    return ms;
}


void writeChapter(ofstream& f, const uint64_t& frame, int i) {
    f << "[CHAPTER]" << endl;
    f << "TIMEBASE=1/1000" << endl;
    f << "START=" + to_string(frame) << endl;
    f << "END=" + to_string(frame) << endl;
    f << "title=" << to_string(i) << endl << endl;
}


/*
void saveHotkeys(obs_data_t* obj)
{
    obs_data_array_t* startHotkeyArrray = obs_hotkey_save(startHotkey);
    obs_data_set_array(obj, "startHotkey", startHotkeyArrray);
    obs_data_array_release(startHotkeyArrray);

    obs_data_array_t* stopHotkeyArrray = obs_hotkey_save(stopHotkey);

    obs_data_set_array(obj, "stopHotkey", stopHotkeyArrray);
    obs_data_array_release(stopHotkeyArrray);

    obs_data_array_t* toggleHotkeyArrray = obs_hotkey_save(toggleHotkey);
    obs_data_set_array(obj, "toggleHotkey", toggleHotkeyArrray);
    obs_data_array_release(toggleHotkeyArrray);
}

void loadHotkeys(obs_data_t* obj)
{
    if (!hotkeysRegistered) {
        registerHotkeys();
    }

    obs_data_array_t* startHotkeyArrray =
        obs_data_get_array(obj, "startHotkey");
    obs_hotkey_load(startHotkey, startHotkeyArrray);
    obs_data_array_release(startHotkeyArrray);

    obs_data_array_t* stopHotkeyArrray =
        obs_data_get_array(obj, "stopHotkey");
    obs_hotkey_load(stopHotkey, stopHotkeyArrray);
    obs_data_array_release(stopHotkeyArrray);

    obs_data_array_t* toggleHotkeyArrray =
        obs_data_get_array(obj, "toggleHotkey");
    obs_hotkey_load(toggleHotkey, toggleHotkeyArrray);
    obs_data_array_release(toggleHotkeyArrray);
}
*/


// Callback for when a recording stops
// Remaking the outputted video file with the chapters metadata
auto EvenHandler = [](enum obs_frontend_event event, void* private_data) {
    if (event == OBS_FRONTEND_EVENT_RECORDING_STARTED) {
        chapters.clear();
        recording = obs_frontend_get_recording_output();
        filename = GetCurrentRecordingFilename();
        /*
        auto info = video_output_get_info(obs_output_video(recording));
        // den/num, perfect for use with metadata timebase and inputting frames
        num = info->fps_num;
        den = info->fps_den;
        */
    } else if (event == OBS_FRONTEND_EVENT_RECORDING_STOPPED) {
        if (chapters.size() == 0)
            return;

        // copy the encoding but give it metadata of chapters
        regex re("(.*)\\.mkv$");
        smatch m;
        regex_search(filename, m, re);
        if (m.size() < 2) {
            string err = "ChapterMarker Plugin didn't find filename of recording";
            // TODO Do a popup message box before throwing
            throw runtime_error(err);
        }
        string newFilename = m[1].str() + " - ChapterMarker.mkv";

        // create chapters for metadata file
        string metadata = filename + ".metadata";
        ofstream file;
        file.open(metadata);
        file << ";FFMETADATA1\n\n";
        int i = 1;
        for (uint64_t& frame : chapters)
            writeChapter(file, frame, i++);
        file.close();

        stringstream ss("");
        ss << "ffmpeg -i \"" + filename + "\" -i \"" + metadata + "\" -map_metadata 1 -c copy \"" + newFilename + "\" -y";

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        //define something for Windows (32-bit and 64-bit, this part is common)
        WinExec(ss.str().c_str(), SW_HIDE);
        //system(ss.str().c_str());
//#elif __linux__ ||  __unix__ || defined(_POSIX_VERSION)
//#elif __APPLE__
#else
        system(ss.str().c_str());
#endif

        // TODO Maybe delete files later?
        // Tried deleting but it breaks WinExec from race condition
        //remove(metadata.c_str());

    }
};


// Lambda function for hotkey
auto HotkeyFunc = [](void* data, obs_hotkey_id id , obs_hotkey_t* hotkey, bool pressed) {
    UNUSED_PARAMETER(id);
    UNUSED_PARAMETER(data);
    UNUSED_PARAMETER(hotkey);

    if (pressed && obs_frontend_recording_active()) {
        chapters.push_back(getOutputRunningTime(recording));
    }
};


/*
auto LoadHotkeyData = [&](const char* name) -> OBSData {
    const char* info =
        config_get_string(basicConfig, "Hotkeys", name);
    if (!info)
        return {};

    obs_data_t* data = obs_data_create_from_json(info);
    if (!data)
        return {};

    OBSData res = data;
    obs_data_release(data);
    return res;
};


auto LoadHotkey = [&](obs_hotkey_id id, const char* name) {
    obs_data_array_t* array =
        obs_data_get_array(LoadHotkeyData(name), "bindings");

    obs_hotkey_load(id, array);
    obs_data_array_release(array);
};
*/


bool obs_module_load(void)
{
    blog(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
    
    /*
    if (AllocConsole())
        blog(LOG_INFO, "alloc console succeeded");
    else {
        blog(LOG_INFO, "alloc console FAILED");
        return false;
    }
    FILE* fDummy = NULL;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    printf("Hello console\n");
    */

    auto hotkey = obs_hotkey_register_frontend("ChapterMarker", "Create chapter marker", HotkeyFunc, NULL);
    obs_frontend_add_event_callback(EvenHandler, NULL);
    //LoadHotkey(hotkey, "ChapterMarker");
    return true;
}


void obs_module_unload()
{
    blog(LOG_INFO, "plugin unloaded");
    /*
    obs_data_t* obj = obs_data_create();
    saveSettings(obj);
    obs_data_save_json(obj, file.fileName().toUtf8().constData());
    obs_data_release(obj);
    */
}
