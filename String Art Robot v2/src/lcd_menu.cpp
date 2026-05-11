
#include "components.h"
#include "lcd_menu.h"
#include "threader.h"
#include <string>
#include <vector>

#define max(a,b) (a > b ? a : b)

// variables
bool refreshScreenNextCycle = true;
bool skipLcdClear = false;
int prevSelected = -1;
Screen currentScreen = Screen::MAIN;
std::vector<Screen> screenHistory;
unsigned long lastUpdateMs = 0;

// Alerty
bool isAlertActive = false;
unsigned long alertEndTime = 0;
std::string alertMessage = "";

// Zmienne dla PROGRESS
std::string currentPrintFilename = "";
int printProgressPercent = 0;

// listy menu
std::vector<MenuOption> currentMenuOptions;
extern std::vector<MenuOption> sensorMenu;

/* callbacki na klikniecie */
void showAlert(std::string msg_line_1, std::string msg_line_2, int durationMs) {
    alertMessage = msg_line_1+msg_line_2;
    alertEndTime = millis() + durationMs;
    isAlertActive = true;
    lcd.clear();
    lcd.setCursor(max(0,(16-msg_line_1.size())/2), 0); // wysrodkowanie
    lcd.print(msg_line_1.c_str());
    lcd.setCursor(max(0,(16-msg_line_2.size())/2),1); // wysrodkowanie
    lcd.print(msg_line_2.c_str());
    set_rgb_specified_colour(RGBColour::COLOUR_ALERT);
}

void actionStartPrint(std::string filename) {
    PrintSequence seq = generatePrintSequenceFromFile(filename);
    load_new_print_sequence(seq);
    Serial.printf("Rozpoczynam wydruk: %s. Kroków: %d\n", seq.print_name.c_str(), seq.nail_sequence.size());

    changeMenuScreen(PROGRESS);
}
void prepareScreenData(Screen screen) {
    currentMenuOptions.clear();

    switch (screen) {
        case MAIN:
            // Dodajemy jawne rzutowanie na wektor przed klamrą
            currentMenuOptions = std::vector<MenuOption>{
                MenuOption("Choose Print", []() { changeMenuScreen(PRINT_SELECT); }),
                MenuOption("Sensors",      []() { changeMenuScreen(SENSORS); }),
                MenuOption("Settings",     []() { changeMenuScreen(SETTINGS); }),
                MenuOption("Credits",      []() { changeMenuScreen(CREDITS); })
            };
            break;

        case SENSORS:
            currentMenuOptions = std::vector<MenuOption>{
                MenuOption("back", []() { goBack(); }),
                MenuOption("Pot: 0%", nullptr, []() {
                    int val = (int)(readPotentiometer() * 100);
                    currentMenuOptions[1].name = "Pot: " + std::to_string(val) + "   ";
                }, 200),
                MenuOption("Hall: 0", nullptr, []() {
                    int val = (int)(readHallSensor());
                    currentMenuOptions[2].name = "Hall: " + std::to_string(val) + "   ";
                }, 200)
            };
            break;

        case PRINT_SELECT: {
            currentMenuOptions.push_back(MenuOption("back", []() { goBack(); }));
            auto printableFiles = readPrintableFileNames();
            
            if (printableFiles.size() == 0) {
                currentMenuOptions.push_back(MenuOption("No files found!", nullptr));
            } else {
                for (auto& filename : printableFiles) {
                    currentMenuOptions.push_back(MenuOption(filename, [filename]() { actionStartPrint(filename); }));
                }
            }
            break;
        }
        case PROGRESS:
            currentMenuOptions = std::vector<MenuOption>{
                MenuOption("back", []() { goBack(); } ),
                MenuOption("pause",  []() { pause_printing(!is_printing_paused()); } ),
                MenuOption("stop",  []() { stop_printing(); }),

                MenuOption("[----]---% N:---", nullptr, []() {
                    int print_percent = get_current_print_progress_percent();
                    int next_nail = get_next_nail_num();
                    std::string next_nail_str = " N:" + (next_nail >= 0 ? std::to_string(next_nail) : "-");

                    int spare_chars = (print_percent < 100 ? (print_percent < 10 ? 2 : 1) : 0) + (next_nail < 100 ? (next_nail < 10 ? 2 : 1) : 0 ); // aim to use all 16 chars
                    const int block_num = 4 + spare_chars; 
                    int filled_blocks = (int)(print_percent * block_num / 100.f);
                    std::string progress_bar_str = "[";

                    for (int i=0; i<filled_blocks; i++) progress_bar_str += "#";
                    for (int i=0; i<block_num-filled_blocks; i++) progress_bar_str += "-";
                    progress_bar_str += "]" + std::to_string(print_percent) + "%";


                    currentMenuOptions[3].name = progress_bar_str + next_nail_str;
                }, 500)
            };
            break;


        case CREDITS: {
            currentMenuOptions = std::vector<MenuOption>{
                MenuOption("back", []() { goBack(); }),
                MenuOption("autor: Dominik",     nullptr),
                MenuOption("Matuszczyk",      nullptr),
                MenuOption("studia: AGH",      nullptr),
                MenuOption("Informatyka",      nullptr),
                MenuOption("2026",      nullptr),
                MenuOption("przedmiot:",      nullptr),
                MenuOption("Zlozone Sys-",      nullptr),
                MenuOption("-temy Cyfrowe",      nullptr)
            };
            break;
        }
        case SETTINGS: {
            currentMenuOptions = std::vector<MenuOption>{
                MenuOption("back", []() { goBack(); }),
                MenuOption("Gwozdzie: 100",     nullptr),
                MenuOption("Srednica: 366mm",      nullptr)
            };
            break;
        }
    }
}

void changeMenuScreen(Screen new_screen) {
    if (currentScreen != new_screen) screenHistory.push_back(currentScreen);
    currentScreen = new_screen;
    prepareScreenData(currentScreen);
    refreshScreenNextCycle = true;
    prevSelected = -1;

    Serial.println("New screen loaded with options:");
    Serial.println(("> Title: " + get_menu_title(currentScreen)).c_str());
    for (auto opt : currentMenuOptions) {
        Serial.println(("> " + opt.name).c_str());
    }
}

void goBack() {
    if (!screenHistory.empty()) {
        currentScreen = screenHistory.back();
        screenHistory.pop_back();
        prepareScreenData(currentScreen);
        refreshScreenNextCycle = true;
        prevSelected = -1;
    }
}

std::string get_menu_title(Screen scr) {
    switch (scr) {
        case MAIN: return "Menu";
        case SETTINGS: return "Settings";
        case CREDITS: return "Credits";
        case PROGRESS: return get_current_print_name();
        case SENSORS: return "Sensors"; 
        case PRINT_SELECT: return "Select print";
        default: return "Menu";
    }
}

void draw_lcd_menu() {
    unsigned long currentMs = millis();

    // Obsługa Alertów
    if (isAlertActive) {
        if (currentMs > alertEndTime) {
            isAlertActive = false;
            refreshScreenNextCycle = true;
            set_rgb_specified_colour(RGBColour::COLOUR_CLEAR);
        } else {
            return; 
        }
    }
    
    // mapowanie obrotu potencjometru na opcje menu
    int optionsCount = currentMenuOptions.size();
    if (optionsCount == 0) return;
    int selected = (int)(readPotentiometer() * optionsCount * 0.999f);
    selected = constrain(selected, 0, optionsCount - 1);
    if (selected != prevSelected) {
        Serial.println(("new option selected: " + std::to_string(selected)).c_str());
        refreshScreenNextCycle = true;
        prevSelected = selected;
    }
    
    // sprawdz extra opcje menu
    bool menuWithBackArrow = (currentMenuOptions[0].name == "back"); // sprawdz czy ten ekran ma miec strzalke do cofania
    bool menuWithPauseButton = currentMenuOptions.size() > 1 && currentMenuOptions[1].name == "pause" && is_print_loaded(); // sprawdz czy ten ekran ma miec opcje pauzy druku
    bool menuWithStopButton = currentMenuOptions.size() > 2 && currentMenuOptions[2].name == "stop" && is_print_loaded(); // sprawdz czy ten ekran ma miec opcje zatzymania druku
    
    bool is_back_arrow_selected = menuWithBackArrow && selected == 0;
    bool is_pause_selected = menuWithPauseButton && selected == 1;
    bool is_stop_selected = menuWithStopButton && selected == 2;
    
    int first_non_control_menu_option = menuWithBackArrow&&menuWithPauseButton&&menuWithStopButton ? 3 : !menuWithPauseButton&&menuWithBackArrow ? 1 : 0;
    
    // Sprawdzanie Update-Loop (np. dla Sensorów)
    int update_menu_option_index = is_back_arrow_selected||is_pause_selected||is_stop_selected ? first_non_control_menu_option : selected;
    if (currentMenuOptions[update_menu_option_index].onUpdate != nullptr) {
        if (currentMs - lastUpdateMs > currentMenuOptions[update_menu_option_index].updateIntervalMs) {
            currentMenuOptions[update_menu_option_index].onUpdate();
            lastUpdateMs = currentMs;
            refreshScreenNextCycle = true; 
            skipLcdClear = true; // prevent flickering for frequent updates
        }
    }
    
    // Obsługa Kliknięć
    if (checkButtonPressed()) {
        if (currentMenuOptions[selected].onClicked != nullptr) {
            currentMenuOptions[selected].onClicked();
        }
        refreshScreenNextCycle = true;
        return; 
    }
    
    // Rysowanie Normalnego Menu
    if (refreshScreenNextCycle) {
        refreshScreenNextCycle = false;
        
        if (!skipLcdClear) lcd.clear();
        skipLcdClear = false;
        
        // Tytul menu
        lcd.setCursor(0, 0);
        std::string menu_title = get_menu_title(currentScreen);
        lcd.print( menu_title.c_str() );
        
        // Rysowanie strzałki "back" (jeśli występuje na liscie)
        if (menuWithBackArrow) {
            if (is_back_arrow_selected) { 
                lcd.setCursor(13, 0);
                lcd.print(" ");
                lcd.write(CHAR_INVERTED_LESS); 
                lcd.write(CHAR_INVERTED_DASH);
                selected = first_non_control_menu_option; // still draw the first option on other line 
            } else {
                lcd.setCursor(14, 0);
                lcd.print("<-");
            }
        }
        
        // rysowanie ikony do pauzy druku
        if (menuWithPauseButton) {
            if (is_pause_selected) {
                lcd.setCursor(11,0);
                lcd.print(" ");
                lcd.write(CHAR_INVERTED_PAUSE);
                lcd.print(" ");
                selected = first_non_control_menu_option; // still draw the first option on other line 
            } else {
                lcd.setCursor(11,0);
                lcd.print(" ");
                lcd.write(CHAR_PAUSE);
                lcd.print(" ");
            }
        }

        // rysowanie ikony do stopu druku
        if (menuWithStopButton) {
            if (is_stop_selected) {
                lcd.setCursor(9,0);
                lcd.print(" ");
                lcd.write(CHAR_INVERTED_DOT);
                lcd.print(" ");
                selected = first_non_control_menu_option; // still draw the first option on other line 
            } else {
                lcd.setCursor(9,0);
                lcd.print(" ");
                lcd.write(CHAR_DOT);
                lcd.print(" ");
            }
        }

        // rysowanie tekstu do funckji kontrolych
        if (is_back_arrow_selected) {
            lcd.setCursor(9, 0);
            lcd.print(" back"); 
        }
        if (is_pause_selected) {
            if (is_printing_paused()) {
                lcd.setCursor(4,0);
                lcd.print(" unpause");
            } else {
                lcd.setCursor(6,0);
                lcd.print(" pause");
            }
        }
        if (is_stop_selected) {
            lcd.setCursor(5,0);
            lcd.print(" stop");
        }
        
        // Wypisz nazwe zaznaczonej opcji menu
        lcd.setCursor(0, 1);
        bool isClickable = (currentMenuOptions[selected].onClicked != nullptr);
        
        if (selected > first_non_control_menu_option) lcd.print("< "); // jesli nie pierwszy element - strzalka w lewo
        if (isClickable) {
            if (is_back_arrow_selected || is_pause_selected || is_stop_selected) lcd.write(CHAR_DOT); // kropka ale nie zaznaczona
            else lcd.write(CHAR_INVERTED_DOT); // jesli element jest klikalny - kropka 
        }
        lcd.print(currentMenuOptions[selected].name.c_str()); // nazwa

        // Serial.println(("Drawing menu option: " + currentMenuOptions[selected].name).c_str());
        
        // jesli nie ostatni element - strzalka w prawo
        if (selected < optionsCount - 1) { 
            lcd.setCursor(15, 1); 
            lcd.print(">"); 
        }
    }
}
