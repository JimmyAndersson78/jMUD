/*---------------------------------------------------------------------------*\
|* File: Properties.h                                Part of jMUD-GameEngine *|
|*                                                                           *|
|* Description:                                                              *|
|*                                                                           *|
|* Authors: Jimmy Andersson <jimmy.andersson.1571@student.uu.se>             *|
|* Created: 2006-09-30 by Jimmy Andersson                                    *|
|* Version: 0.1.00                                                           *|
\*---------------------------------------------------------------------------*/
#ifndef SETTINGS_H
#define SETTINGS_H

#include <vector>
#include <cstdio>


class Settings;

void print(const Settings& p, FILE* file);


struct SettingsElement {
    char* key;
    char* value;
};



class Settings {
  friend void print(const Settings& p, FILE* file);

  public:
    Settings(const char *filename);
    ~Settings();

    const char* getSetting(const char* key);
    bool        hasSetting(const char* key);
    void        setSetting(const char* key, const char* value);

    bool isEnabled(const char* key);
    bool isNumeric(const char* key);
    int  getNumber(const char* key);

    void loadSettings(const char* filename);
    void saveSettings(const char* filename);

  private:
    Settings();
    Settings(const Settings&);

    int findSetting(const char *key);

    std::vector<SettingsElement> _settings;
    std::size_t lastIndex;
};


inline void print(const Settings& p, FILE* file = NULL) {
    if (file == NULL)
        file = stdout;

    std::size_t size = p._settings.size();

    fprintf(file, "Settings: size=%lu\n", size);
    fprintf(file, "========================================\n");
    for (size_t i = 0; i < size; i++) {
        fprintf(file, "'%s' : '%s'\n", p._settings[i].key, p._settings[i].value);
    }
    fprintf(file, "========================================\n\n");
}

#endif // SETTINGS_H
