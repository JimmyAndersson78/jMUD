/*---------------------------------------------------------------------------*\
|* File: Properties.cpp                              Part of jMUD-GameEngine *|
|*                                                                           *|
|* Description:                                                              *|
|*                                                                           *|
|* TODO:  Make the list of properties sorted, makes for better output and we *|
|*       can possibly interrupt the searching through the list before the    *|
|*       end. [HIGH PRIORITY]                                                *|
|*        Save the initial comments from the input file, we could possibly   *|
|*       even save ALL comments and lines (including empty ones) to make the *|
|*       output as similar to the input as possible. Comments and formating  *|
|*       can be very beneficial even in a settings file. [LOW PRIORITY]      *|
|*                                                                           *|
|* Authors: Jimmy Andersson <jimmy.andersson.1571@student.uu.se>             *|
|* Created: 2006-09-30 by Jimmy Andersson                                    *|
|* Version: 0.1.00                                                           *|
\*---------------------------------------------------------------------------*/
#include "Settings.h"

#include <fstream>
#include <cstring>



Settings::Settings(const char *filename) : _settings(), lastIndex(0) {
    _settings.reserve(40);

    if (filename == NULL) {
        fprintf(stderr, "Settings: ERROR: NULL string given as filename.");
        return;
    }
    if (filename[0] == '\0') {
        fprintf(stderr, "Settings: ERROR: Empty string given as filename.");
        return;
    }

    // Load the given settings from the file.
    loadSettings(filename);
}



Settings::~Settings() {
    for (std::size_t i = 0; i < _settings.size(); i++) {
        delete[] _settings[i].key;
        delete[] _settings[i].value;
    }
}
	

const char* Settings::getSetting(const char *key) {
    int i = findSetting(key);
    if (i == -1)
        return NULL;
    return static_cast<const char*>(_settings[i].value);
}


bool Settings::hasSetting(const char *key) {
    if (findSetting(key) != -1)
        return true;
    return false;
}


void Settings::setSetting(const char *key, const char *value) {
    int i = findSetting(key);

    // If the key existed replace the value, otherwise add the key and value.
    if (i == -1) {
        SettingsElement e;
        e.key = new char[strlen(key)+1];
        e.value = new char[strlen(value)+1];
        strcpy(e.key, key);
        strcpy(e.value, value);

        _settings.push_back(e);
    } else {
        delete[] _settings[i].value;
        _settings[i].value = new char[strlen(value)+1];
        strcpy(_settings[i].value, value);
    }
}


bool Settings::isEnabled(const char *key) {
    int i = findSetting(key);
    if (i == -1) return false;

    if (strcmp(_settings[i].value, "true") == 0)
        return true;
    if (strcmp(_settings[i].value, "yes") == 0)
        return true;
    if (strcmp(_settings[i].value, "on") == 0)
        return true;

    return false;
}


// FIXME: Improve logic flow.
bool Settings::isNumeric(const char *key) {
    int i = findSetting(key);
    if (i == -1) return false;

    std::size_t size = strlen(_settings[i].value);
    std::size_t n = 0;
    while (n < size && isdigit(_settings[i].value[n])) {
        ++n;
    }

    if (n == size)
        return true;
    return false;
}


int Settings::getNumber(const char *key) {
    if (!isNumeric(key))
        return 0;

    int i = findSetting(key);
    return atoi(_settings[i].value);
}



void Settings::loadSettings(const char *filename) {
    std::fstream file(filename, std::ios::in);

    if (file.is_open() == 0) {
        fprintf(stderr, "Properties: Could not open input file '%s'.\n", filename);
        return;
    }

    char read_buf[1024], key[1024], value[1024];
    std::size_t line = 0;

    while (!file.eof()) {
        file >> std::ws;
        file.getline(read_buf, 1023, '\n');
        line++;

        // Ignore comment lines.
        if (read_buf[0] == '#' || read_buf[0] == '/' || read_buf[0] == '!')
            continue;

        if (sscanf(read_buf,"%s = %s", key, value) != 2) {
            if (sscanf(read_buf,"%s : %s", key, value) != 2) {
                fprintf(stderr, "Settings: Invalid format of line %lu in file '%s'.\n (%s)\n", line, filename, read_buf);
                continue;
            }
        }
        setSetting(key, value);
        file >> std::ws;
    }
    file.close();
}


void Settings::saveSettings(const char *filename) {
    std::fstream file(filename, std::ios::out);

    if (file.is_open() == 0) {
        fprintf(stderr, "Settings: Could not open output file '%s'.\n", filename);
        return;
    }
    std::size_t size = _settings.size();

    time_t t = time(NULL);
    struct tm *l = localtime(&t);
    file << "# " << asctime(l) << std::endl;

    for (size_t i = 0; i < size; i++) {
        file << _settings[i].key << ": " << _settings[i].value << std::endl;
    }
    file.close();
}


int Settings::findSetting(const char *key) {
    if (_settings.size() > lastIndex) {
#if defined(_WIN32)
        if (strnicmp(key, _settings[lastIndex].key, strlen(key)) == 0) {
#else
        if (strncasecmp(key, _settings[lastIndex].key, strlen(key)) == 0) {
#endif
            return static_cast<int>(lastIndex);
        }
    }

    for (std::size_t i = 0; i < _settings.size(); i++) {
#if defined(_WIN32)
        if (strnicmp(key, _settings[i].key, strlen(key)) == 0) {
#else
        if (strncasecmp(key, _settings[i].key, strlen(key)) == 0) {
#endif
            lastIndex = i;
            return static_cast<int>(lastIndex);
        }
    }
    return -1;
}
