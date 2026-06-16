
#include "components.h"
#include "lcd_menu.h"
#include "threader.h"
#include <string>
#include <vector>

#define max(a,b) (a > b ? a : b)

#define MENU_SELECTION_DEAD_ZONE_PERCENT 0.2f // extra padding on menu option potentionmeter ranges so on transition we dont get flickers

extern const int SERVO_INSIDE_ANGLE;
extern const int SERVO_OUTSIDE_ANGLE;
extern const int STEPS_PER_FULL_STEPPER_ROTATION;

// zmienne do sekwencji testowych
int rgbTestStep = -1;
int stepperTestStep = -1;
int servoTestStep = -1;
unsigned long nextTestStepMs = 0;
int homeingTestStep = -1;
int nailSpacingTestStep = -1;

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
void showAlert(std::string msg_line_1, std::string msg_line_2, unsigned long durationMs) {
    alertMessage = msg_line_1+msg_line_2;
    alertEndTime = millis() + durationMs;
    isAlertActive = true;
    lcd.clear();
    lcd.setCursor(max(0,(16-msg_line_1.size())/2), 0); // wysrodkowanie
    lcd.print(msg_line_1.c_str());
    lcd.setCursor(max(0,(16-msg_line_2.size())/2),1); // wysrodkowanie
    lcd.print(msg_line_2.c_str());
    set_rgb_specified_colour(RGBColour::COLOUR_ALERT);

    Serial.printf("Displaying alert msg: \n - %s - %s \n", msg_line_1.c_str(), msg_line_2.c_str());
}

void actionStartPrint(std::string filename) {
    PrintSequence seq = generatePrintSequenceFromFile(filename);
    if (load_new_print_sequence(seq)){
        Serial.printf("Rozpoczynam wydruk: %s. Kroków: %d (%d) dla Gwozdzi:%d\n", seq.print_name.c_str(), seq.nail_sequence.size(), seq.sequence_length, seq.nail_number);
        changeMenuScreen(PROGRESS);
    } else {
        Serial.println("Nie udalo sie rozpoczac wydruku");
        changeMenuScreen(PRINT_SELECT);
    }
}
void prepareScreenData(Screen screen) {
    currentMenuOptions.clear();

    switch (screen) {
        case MAIN:
            // Dodajemy jawne rzutowanie na wektor przed klamrą
            currentMenuOptions = std::vector<MenuOption>{
                MenuOption("Choose Print", []() { changeMenuScreen(PRINT_SELECT); }),
                MenuOption("Components",   []() { changeMenuScreen(COMPONENTS); }),
                MenuOption("Sensors",      []() { changeMenuScreen(SENSORS); }),
                MenuOption("Settings",     []() { changeMenuScreen(SETTINGS); }),
                MenuOption("Credits",      []() { changeMenuScreen(CREDITS); })
            };
            break;

        case COMPONENTS: // <-- NOWY EKRAN TESTOWY
            currentMenuOptions = std::vector<MenuOption>{
                MenuOption("back", []() { 
                    // Resetuj testy przy wyjściu
                    rgbTestStep = -1; stepperTestStep = -1; servoTestStep = -1;
                    setRGBColor(0,0,0);
                    goBack(); 
                }),
                // dioda RGB
                MenuOption("Run Test  ", []() { rgbTestStep = 0; nextTestStepMs = millis(); }, []() {
                    if (rgbTestStep == -1) { currentMenuOptions[1].name = "Run Test  "; return; }
                    if (millis() < nextTestStepMs) return;

                    switch(rgbTestStep) {
                        case 0: setRGBColor(255, 0, 0);     currentMenuOptions[1].name = "Red     ";        nextTestStepMs = millis() + 800;  rgbTestStep++; break;
                        case 1: setRGBColor(0, 255, 0);     currentMenuOptions[1].name = "Green     ";      nextTestStepMs = millis() + 800;  rgbTestStep++; break;
                        case 2: setRGBColor(0, 0, 255);     currentMenuOptions[1].name = "Blue     ";       nextTestStepMs = millis() + 800;  rgbTestStep++; break;
                        case 3: setRGBColor(255, 255, 0);   currentMenuOptions[1].name = "RG Yellow     ";  nextTestStepMs = millis() + 800;  rgbTestStep++; break;
                        case 4: setRGBColor(0, 255, 255);   currentMenuOptions[1].name = "GB Cyan     ";    nextTestStepMs = millis() + 800;  rgbTestStep++; break;
                        case 5: setRGBColor(255, 0, 255);   currentMenuOptions[1].name = "RB Magenta     "; nextTestStepMs = millis() + 800;  rgbTestStep++; break;
                        case 6: setRGBColor(0, 0, 0);       currentMenuOptions[1].name = "Test Done     ";  nextTestStepMs = millis() + 1200; rgbTestStep++; break;
                        default: rgbTestStep = -1; break;
                    }
                }, 50),
                // STEPPER
                MenuOption("Run Test  ", []() { stepperTestStep = 0; nextTestStepMs = millis(); }, []() {
                    if (stepperTestStep == -1) { currentMenuOptions[2].name = "Run Test  "; return; }
                    if (millis() < nextTestStepMs) return;

                    switch(stepperTestStep) {
                        case 0:  currentMenuOptions[2].name = "Find home...      "; find_stepper_home_position(); nextTestStepMs = millis() + 1000; stepperTestStep++; break;
                        case 1:  currentMenuOptions[2].name = "360 CW      ";       move_stepper_steps(STEPS_PER_FULL_STEPPER_ROTATION, false, true);     nextTestStepMs = millis() + 400; stepperTestStep++; break;
                        case 2:  currentMenuOptions[2].name = "180 CCW      ";      move_stepper_steps(STEPS_PER_FULL_STEPPER_ROTATION / 2, true, true);  nextTestStepMs = millis() + 400; stepperTestStep++; break;
                        case 3:  currentMenuOptions[2].name = "90 CW      ";        move_stepper_steps(STEPS_PER_FULL_STEPPER_ROTATION / 4, false, true); nextTestStepMs = millis() + 400; stepperTestStep++; break;
                        case 4:  currentMenuOptions[2].name = "45 CCW      ";       move_stepper_steps(STEPS_PER_FULL_STEPPER_ROTATION / 8, true, true);  nextTestStepMs = millis() + 400; stepperTestStep++; break;
                        case 5:  currentMenuOptions[2].name = "1 step x1      ";    move_stepper_steps(1, false, true); nextTestStepMs = millis() + 300; stepperTestStep++; break;
                        case 6:  currentMenuOptions[2].name = "1 step x2      ";    move_stepper_steps(1, false, true); nextTestStepMs = millis() + 600; stepperTestStep++; break;
                        case 7:  currentMenuOptions[2].name = "Half step x1      "; change_microstepping_mode(STEP_MODE_HALF); move_stepper_steps(1, false, true); nextTestStepMs = millis() + 300; stepperTestStep++; break;
                        case 8:  currentMenuOptions[2].name = "Half step x2      "; move_stepper_steps(1, false, true); nextTestStepMs = millis() + 600; stepperTestStep++; break;
                        case 9:  currentMenuOptions[2].name = "Quarter x1      ";   change_microstepping_mode(STEP_MODE_QUARTER); move_stepper_steps(1, false, true); nextTestStepMs = millis() + 300; stepperTestStep++; break;
                        case 10: currentMenuOptions[2].name = "Quarter x2      ";   move_stepper_steps(1, false, true); nextTestStepMs = millis() + 600; stepperTestStep++; break;
                        case 11: currentMenuOptions[2].name = "1/8 step x1      ";  change_microstepping_mode(STEP_MODE_EIGHTS); move_stepper_steps(1, false, true); nextTestStepMs = millis() + 300; stepperTestStep++; break;
                        case 12: currentMenuOptions[2].name = "1/8 step x2      ";  move_stepper_steps(1, false, true); nextTestStepMs = millis() + 600; stepperTestStep++; break;
                        case 13: currentMenuOptions[2].name = "Test Done      ";    nextTestStepMs = millis() + 1200; stepperTestStep++; change_microstepping_mode(STEP_MODE_FULL);  break;
                        default: stepperTestStep = -1; break;
                    }
                }, 50),
                // SERVO
                MenuOption("Run Test  ", []() { servoTestStep = 0; nextTestStepMs = millis(); }, []() {
                    if (servoTestStep == -1) { currentMenuOptions[3].name = "Run Test  "; return; }
                    if (millis() < nextTestStepMs) return;

                    switch(servoTestStep) {
                        case 0: servoWrite(90);                  currentMenuOptions[3].name = "Middle (90)     ";  nextTestStepMs = millis() + 800;  servoTestStep++; break;
                        case 1: servoWrite(0);                   currentMenuOptions[3].name = "0 Degrees       ";  nextTestStepMs = millis() + 800;  servoTestStep++; break;
                        case 2: servoWrite(180);                 currentMenuOptions[3].name = "180 Degrees     ";  nextTestStepMs = millis() + 800;  servoTestStep++; break;
                        case 3: servoWrite(90);                  currentMenuOptions[3].name = "Middle (90)     ";  nextTestStepMs = millis() + 800;  servoTestStep++; break;
                        case 4: servoWrite(SERVO_INSIDE_ANGLE);  currentMenuOptions[3].name = "Inner Pos      ";   nextTestStepMs = millis() + 800;  servoTestStep++; break;
                        case 5: servoWrite(SERVO_OUTSIDE_ANGLE); currentMenuOptions[3].name = "Outer Pos      ";   nextTestStepMs = millis() + 800;  servoTestStep++; break;
                        case 6: servoWrite(90);                  currentMenuOptions[3].name = "Test Done      ";   nextTestStepMs = millis() + 1200; servoTestStep++; break;
                        default: servoTestStep = -1; break;
                    }
                }, 50),
                // HOMEING TEST
                MenuOption("Find home pos  ", []() { find_stepper_home_position(); }),

                // NAIL SPACING TEST
                MenuOption("Run Test  ", []() { nailSpacingTestStep = 0; nextTestStepMs = millis(); }, []() {
                    if (nailSpacingTestStep == -1) { currentMenuOptions[5].name = "Run Test  "; return; }
                    if (isAlertActive) return;
                    if (nailSpacingTestStep >= 120) { nailSpacingTestStep = -1; return; } 
                    
                    // start and end conditions
                    if (nailSpacingTestStep == 0) {
                        servoWrite(SERVO_INSIDE_ANGLE);
                        find_stepper_home_position(); // go to nail 0 position
                        showAlert("At nail zero", "(home pos)", 1000000);
                    } 
                    
                    // nail logic
                    else {
                        rotate_ring_to_nail(nailSpacingTestStep, true);
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        servoWrite(SERVO_OUTSIDE_ANGLE);
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        servoWrite(SERVO_INSIDE_ANGLE);

                        std::string line2 = "Nail: " + std::to_string(nailSpacingTestStep) + "/120";
                        showAlert("Testing gap...", line2, 1000000);
                    }

                    // progress step
                    nailSpacingTestStep++;            
                }, 50),

                // NAIL SPACING TEST 2
                MenuOption("Run Test  ", []() { nailSpacingTestStep = 0; nextTestStepMs = millis(); }, []() {
                    
                    if (nailSpacingTestStep == -1) { currentMenuOptions[6].name = "Run Test  "; return; }
                    if (isAlertActive) return;
                    if (nailSpacingTestStep >= 24) { nailSpacingTestStep = -1; return; } 
                    
                    // start and end conditions
                    if (nailSpacingTestStep == 0) {
                        servoWrite(SERVO_INSIDE_ANGLE);
                        find_stepper_home_position(); // go to nail 0 position
                        showAlert("At nail zero", "(home pos)", 1000000);
                    } 
                    
                    // nail logic
                    else {
                        int nail_idx = (nailSpacingTestStep * 25) % 120;
                        rotate_ring_to_nail(nail_idx, true);
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        servoWrite(SERVO_OUTSIDE_ANGLE);
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        servoWrite(SERVO_INSIDE_ANGLE);

                        std::string line2 = "Nail: " + std::to_string(nail_idx) + "/120";
                        showAlert("Testing gap...", line2, 1000000);
                    }

                    // progress step
                    nailSpacingTestStep++;            
                }, 50)
            };
            break;

        case SENSORS:
            currentMenuOptions = std::vector<MenuOption>{
                MenuOption("back", []() { goBack(); }),
                MenuOption("Pot: 0", nullptr, []() {
                    int val = (int)(readPotentiometerRaw());
                    int val2 = (int)(readPotentiometer()*100);
                    currentMenuOptions[1].name = "Pot:" + std::to_string(val) + " (" + std::to_string(val2) + "%) ";
                }, 200),
                MenuOption("Hall: 0", nullptr, []() {
                    int val = (int)(readHallSensor());
                    currentMenuOptions[2].name = "Hall: " + std::to_string(val) + "   ";
                }, 200),
                MenuOption("Pot: 0", nullptr, []() {
                    int val = (int)(readPotentiometerRaw());
                    int val2 = (int)(readPotentiometer()*100);
                    currentMenuOptions[4].name = "Pot:" + std::to_string(val) + " (" + std::to_string(val2) + "%) ";
                }, 200),
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
                MenuOption("stop",  []() { stop_printing(); changeMenuScreen(PRINT_SELECT); }),

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
        case COMPONENTS: {
            switch (prevSelected) {
                case 1: return "RGB LED";
                case 2: return "Stepper";
                case 3: return "Servo";
                case 4: return "Homeing";
                case 5: return "Nail Space (1)";
                case 6: return "Nail Space (25)";
                default: return "Components";
            }
        }
        default: return "Menu";
    }
}


int update_menu_selection() {
    int optionsCount = currentMenuOptions.size();

    float potVal = readPotentiometer();
    float optionWidth = 1.0f / optionsCount;

    float currentLowerLimit = (prevSelected * optionWidth) - (optionWidth * MENU_SELECTION_DEAD_ZONE_PERCENT);
    float currentUpperLimit = ((prevSelected + 1) * optionWidth) + (optionWidth * MENU_SELECTION_DEAD_ZONE_PERCENT);

    if (potVal < currentLowerLimit || potVal > currentUpperLimit || prevSelected == -1) {
        int selected = (int)(potVal * optionsCount * 0.999f);
        selected = constrain(selected, 0, optionsCount - 1);
        return selected;
    }
    return prevSelected;
}

void draw_lcd_menu() {
    unsigned long currentMs = millis();

    // Obsługa Alertów
    if (isAlertActive) {
        if (currentMs > alertEndTime || checkButtonPressed()) {
            Serial.println("skipping alert with button press") ;
            isAlertActive = false;
            refreshScreenNextCycle = true;
            set_rgb_specified_colour(RGBColour::COLOUR_CLEAR);
        } 
        return;
    }
    
    // mapowanie obrotu potencjometru na opcje menu
    int optionsCount = currentMenuOptions.size();
    if (optionsCount == 0) return;

    int selected = update_menu_selection();
    if (selected != prevSelected) {
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
    if (isAlertActive) return; // early exit if alert thrown
    
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

bool is_showing_alert() {
    return isAlertActive;
}