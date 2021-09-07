#pragma once

#include <QMessageBox>
#include <QProgressDialog>

#define DEBUG
// For AllocConsole
#ifdef DEBUG
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <Windows.h>
#endif
#endif

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <iterator>
#include <filesystem>
#include <chrono>
#include <thread>


using namespace std;

void updateProgress(int64_t i);
void crash(string s);
void convertChapters();
void createSettingsDir();
void registerHotkeys();
void saveSettings();
void loadSettings();
void saveHotkeys(obs_data_t* obj);
void loadHotkeys(obs_data_t* obj);
bool checkMKV();
void cleanupFiles(const string& filename, const string&);
