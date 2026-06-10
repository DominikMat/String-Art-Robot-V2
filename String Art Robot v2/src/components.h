#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <SD.h> 
#include <AccelStepper.h>
#include <vector>

// Deklaracje zmiennych globalnych (używamy extern, by były widoczne w main)
extern LiquidCrystal_I2C lcd;
extern const int STEPS_PER_FULL_STEPPER_ROTATION;

enum LCDCustomChars {
    CHAR_INVERTED_LESS = 0, 
    CHAR_INVERTED_DASH = 1,
    CHAR_DOT = 2,           
    CHAR_INVERTED_DOT = 3, 
    CHAR_PAUSE = 4, 
    CHAR_INVERTED_PAUSE = 5
};

enum RGBColour {
    COLOUR_PRINTING,       // Druk w toku
    COLOUR_PRINT_COMPLETE, // Zielony
    COLOUR_PRINT_PAUSED,   // Pomarańczowy
    COLOUR_PRINT_STOPPED,  // Czerwono-pomarańczowy
    COLOUR_READY,          // Cyjan / Jasny niebieski
    COLOUR_ERROR,          // Czerwony
    COLOUR_CLEAR,          // Wyłączony
    COLOUR_ALERT           // fiolet taki
};

enum MicrostepMode {
    FULL_STEP = 1,
    HALF_STEP = 2,
    QUARTER_STEP = 4,
    EIGHTS_STEP = 8
};

struct PrintSequence {
    int nail_number = 0;
    int sequence_length = 0;
    std::string print_name;
    std::vector<int> nail_sequence;
};

// Prototypy funkcji
void initLcd();
void initServo();
void loopServo();
void servoWrite(int angleDeg);
void initPotentiometer();
float readPotentiometer();
int readPotentiometerRaw();
void initLed();
void setRGBColor(int r, int g, int b);
void initButton();
bool checkButtonPressed();
void initSDReader();
void initHallSensor();
int readHallSensor();
void initStepper();
void setRGBColor(int r, int g, int b);
void set_rgb_specified_colour(RGBColour colour);
void move_stepper_steps(int steps, bool dir);
MicrostepMode get_current_microstep_mode();

std::vector<std::string> readPrintableFileNames();
PrintSequence generatePrintSequenceFromFile(std::string filename);

#endif