#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <Windows.h>
#endif
#include <stdio.h>
#include <inttypes.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <iterator>
#include <cstdio>
#include <filesystem>
#include <chrono>
#include <thread>


using namespace std;

void createSettingsDir();
void registerHotkeys();
void saveSettings();
void loadSettings();
void saveHotkeys(obs_data_t* obj);
void loadHotkeys(obs_data_t* obj);
bool checkMKV();
void cleanupFiles(const string& filename, const string&, const string& metadata);