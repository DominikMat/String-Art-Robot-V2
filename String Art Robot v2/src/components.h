#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <SD.h> 
#include <AccelStepper.h>

// Deklaracje zmiennych globalnych (używamy extern, by były widoczne w main)
extern LiquidCrystal_I2C lcd;
extern volatile bool isButtonPressed;
extern float potentiometerValue;
extern int hallValue;
extern float currentStepperSpeed;
extern AccelStepper stepper;
extern unsigned long lastDisplayUpdate;
extern int colorState;

// Prototypy funkcji
void initLcd();
void initServo();
void loopServo();
void servoWrite(int angleDeg);
void initPotentiometer();
void readPotentiometer();
void initLed();
void setRGBColor(int r, int g, int b);
void initButton();
void initSDReader();
void initHallSensor();
void readHallSensor();
void initStepper();

#endif