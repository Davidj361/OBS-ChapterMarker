#pragma once

#include <QMessageBox>

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