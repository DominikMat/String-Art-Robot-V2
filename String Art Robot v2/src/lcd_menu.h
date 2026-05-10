#ifndef LCD_MENU_H
#define LCD_MENU_H

#include "components.h"
#include <string>
#include <vector>

// enum for screen selection
enum Screen {
    MAIN, PRINT_SELECT, PROGRESS, SENSORS, SETTINGS, CREDITS
};

const int DEFAULT_SCREEN_REFRESH_INTERVAL_MS = 1000;

// menu option struct
using MenuCallback = std::function<void()>;
struct MenuOption {
    std::string name;
    MenuCallback onClicked = nullptr;
    MenuCallback onUpdate = nullptr;
    int updateIntervalMs = DEFAULT_SCREEN_REFRESH_INTERVAL_MS;

    MenuOption(std::string n, 
        MenuCallback click = nullptr, 
        MenuCallback update = nullptr, 
        int interval = DEFAULT_SCREEN_REFRESH_INTERVAL_MS)
    : name(n), onClicked(click), onUpdate(update), updateIntervalMs(interval)
    {}
};

/* callbacki na klikniecie */
void actionStartPrint(std::string filename);
void changeMenuScreen(Screen new_screen);
std::string get_menu_title(Screen screen);
void goBack();
void showAlert(std::string msg, int durationMs = 1000);

// glowna funckja
void draw_lcd_menu();

#endif
