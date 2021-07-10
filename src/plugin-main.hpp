#pragma once

void createSettingsDir();
void registerHotkeys();
void saveSettings();
void loadSettings();
void saveHotkeys(obs_data_t* obj);
void loadHotkeys(obs_data_t* obj);
bool checkMKV();