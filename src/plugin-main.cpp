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

extern "C" {
#include "remux.h"
}

#include "plugin-macros.generated.h"
#include "plugin-main.hpp"


OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

const static char* configFile = "ChapterMarker.json";
obs_output_t* recording = nullptr;
vector<uint64_t> chapters;
string filename = "";
bool isMKV = false;
chrono::steady_clock::time_point start;
uint64_t elapsed;

obs_hotkey_id chapterMarkerHotkey = OBS_INVALID_HOTKEY_ID;
bool hotkeysRegistered = false;

thread _t;
bool running = false; // a way to check if the thread is done

QProgressDialog* progress = nullptr;


void crash(string s) {
	errorPopup(s.c_str());
	string str = "Chapter Marker Plugin : " + s;
	throw runtime_error(str);
}


void errorPopup(const char* s) {
	string str = "Chapter Marker Plugin: " + string(s);
	QMessageBox::critical(0, QString("ERROR!"), QString::fromStdString(str), QMessageBox::Ok);
}


void resetProgress() {
	progress = nullptr;
}


void createProgressBar() {
	if (progress != nullptr)
		errorPopup("createProgressBar: progress != nullptr");
	string message = filesystem::path(filename).filename().string() + "\nDuplicating video file with chapter metadata";
	progress = new QProgressDialog(message.c_str(), "Cancel", 0, 100);
	progress->setWindowTitle("Chapter Marker");
	progress->setWindowFlags(progress->windowFlags() & ~Qt::WindowCloseButtonHint & ~Qt::WindowContextHelpButtonHint);
	progress->setAttribute(Qt::WA_DeleteOnClose);
	QObject::connect(progress, &QProgressDialog::canceled, resetProgress);
	QObject::connect(progress, &QProgressDialog::finished, resetProgress);
	progress->show(); progress->raise();
}


// Input: Percentage of progress for remuxing
void updateProgress(int64_t i) {
	if (progress == nullptr) {
		//errorPopup("QProgressDialog is null");
		return;
	}
	QMetaObject::invokeMethod(progress, "setValue", Qt::QueuedConnection, Q_ARG(int, static_cast<int>(i)));
}


bool cancelledProgress() {
	if (progress == nullptr)
		return true;
	return progress->wasCanceled();
}


void convertChapters() {
	int i = 1;
	for (auto t : chapters) {
		AVRational r;
		r.num = 1;
		r.den = 1000; // milliseconds
		avpriv_new_chapter(i, r, t, t, to_string(i++).c_str());
	}
}


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


uint64_t getOutputRunningTime() {
	auto finish = chrono::steady_clock::now();
	uint64_t dur = chrono::duration_cast<chrono::milliseconds>(finish - start).count() + elapsed;
	return dur;
}


// Lambda function for hotkey
auto HotkeyFunc = [](void* data, obs_hotkey_id id, obs_hotkey_t* hotkey, bool pressed) {
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(hotkey);

	if (pressed && obs_frontend_recording_active() && isMKV) {
		chapters.push_back(getOutputRunningTime());
	}
};


void createSettingsDir() {
	char* file = obs_module_config_path("");
	filesystem::create_directory(file);
}


void saveSettings() {
	if (!filesystem::exists(obs_module_config_path("")))
		createSettingsDir();
	obs_data_t* obj = obs_data_create();
	saveHotkeys(obj);
	char* file = obs_module_config_path(configFile);
	obs_data_save_json(obj, file);
	obs_data_release(obj);
}

void loadSettings() {
	char* file = obs_module_config_path(configFile);
	obs_data_t* obj = obs_data_create_from_json_file(file);
	loadHotkeys(obj);
}


void registerHotkeys() {
	chapterMarkerHotkey = obs_hotkey_register_frontend("ChapterMarker", "Create chapter marker", HotkeyFunc, NULL);
	hotkeysRegistered = true;
}


void saveHotkeys(obs_data_t* obj) {
	obs_data_array_t* chapterMarkerHotkeyArrray = obs_hotkey_save(chapterMarkerHotkey);
	obs_data_set_array(obj, "chapterMarkerHotkey", chapterMarkerHotkeyArrray);
	obs_data_array_release(chapterMarkerHotkeyArrray);
}


void loadHotkeys(obs_data_t* obj) {
	if (!hotkeysRegistered) {
		registerHotkeys();
	}

	obs_data_array_t* chapterMarkerHotkeyArrray =
		obs_data_get_array(obj, "chapterMarkerHotkey");
	obs_hotkey_load(chapterMarkerHotkey, chapterMarkerHotkeyArrray);
	obs_data_array_release(chapterMarkerHotkeyArrray);
}


bool checkMKV() {
	isMKV = false; // reset
	printf("before regex\n");
	printf("filename: %s\n", filename.c_str());
	regex re(".mkv$");
	smatch m;
	regex_search(filename, m, re);
	if (m.size() == 1)
		isMKV = true;
	return isMKV;
}


// Delete redundant video files so 2x space isn't taken
void cleanupFiles(const string& f, const string& f2) {
	// Don't delete anything if the chapter marker video wasn't created
	if (!filesystem::exists(f2))
		return;
	remove(f.c_str());
	rename(f2.c_str(), f.c_str());
}


void startThread(string f) {
	running = true;
	// copy the encoding but give it metadata of chapters
	regex re("(.*)\\.mkv$");
	smatch m;
	regex_search(f, m, re);
	if (m.size() < 2)
		crash("Didn't find the filename of the recording!");

	string newFilename = m[1].str() + " - ChapterMarker.mkv";

	// TODO Remove after testing
	// For testing long videos
	f = "F:/Videos/test.mkv";
	newFilename = "F:/Videos/test2.mkv";

	printf("%s \n", f.c_str());
	printf("%s \n", newFilename.c_str());

	startRemux(f.c_str(), newFilename.c_str());
	convertChapters();
	finishRemux();
	if (progress != nullptr) {
		progress->setValue(100);
		progress->close();
		progress = nullptr;
	}

	// TODO uncomment
	//cleanupFiles(f, newFilename);
	running = false;
}


// Callback for when a recording stops
// Remaking the outputted video file with the chapters metadata
auto EvenHandler = [](enum obs_frontend_event event, void* private_data) {
	switch (event) {
	case OBS_FRONTEND_EVENT_RECORDING_STARTED: {
		chapters.clear();
		recording = obs_frontend_get_recording_output();
		filename = GetCurrentRecordingFilename();
		if (checkMKV()) {
			start = chrono::steady_clock::now();
			elapsed = 0;
		}
		break;
	}

	case OBS_FRONTEND_EVENT_RECORDING_PAUSED: {
		auto finish = chrono::steady_clock::now();
		elapsed += chrono::duration_cast<chrono::milliseconds>(finish - start).count();
		break;
	}

	case OBS_FRONTEND_EVENT_RECORDING_UNPAUSED: {
		start = chrono::steady_clock::now();
		break;
	}

	case OBS_FRONTEND_EVENT_RECORDING_STOPPED: {
		if (chapters.size() == 0)
			break;
		if (filesystem::file_size(filename) > filesystem::space(filesystem::path(filename)).free) {
			errorPopup("No space left for duplicate video! Aborting...");
			break;
		}

		if (running) {
			errorPopup("Still duplicating previous recording, aborting chapter creation for current recording!");
			break;
		}
		if (_t.joinable()) // wait for previous thread to finish 
			_t.join();
		createProgressBar(); // Freezes if created within worker thread
		_t = thread(startThread, filename);
		break;
	}

	case OBS_FRONTEND_EVENT_EXIT:
		saveSettings();
		break;
	}
};


bool obs_module_load(void) {
	blog(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);


	
	// For easy debugging
#ifdef DEBUG
	if (AllocConsole())
		blog(LOG_INFO, "alloc console succeeded");
	else {
		blog(LOG_INFO, "alloc console FAILED");
		return false;
	}
	FILE* fDummy = NULL;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	printf("Hello console\n");
#endif
	


	loadSettings();
	obs_frontend_add_event_callback(EvenHandler, NULL);
	return true;
}


void obs_module_unload() {
	blog(LOG_INFO, "plugin unloaded");
}
