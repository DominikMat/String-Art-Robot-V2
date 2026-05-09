// autor Dominik Matuszczyk 
// na przedmiot Zlozone Systemy Cyfrowe - Informatyka AGH
// 2026

#include "components.h"
#include "lcd_menu.h"

/* ====================================================================================================== */
/*                                        MAIN APPLICATION LOGIC                                          */
/* ====================================================================================================== */

void setup() {
    Serial.begin(115200);
    Serial.println("Initializing system...");
    
    /* Init compoennts */
    initLcd();
    initServo();
    initPotentiometer();
    initLed();
    initButton(); 
    initHallSensor();
    initSDReader(); 
    initStepper();
    
    /* Init seuqence */
    Serial.println("Ready!");
    delay(1500);
    lcd.clear();

    lcd.print("Waiting...");
}   


void loop() {
    loopServo();
    
    /* Button logic */
    if (isButtonPressed) {
        isButtonPressed = false;

        /* change motor dir */
        // currentStepperSpeed = -currentStepperSpeed; 
        // stepper.setSpeed(currentStepperSpeed);
        // Serial.println("Zmieniono kierunek silnika krokowego.");

        /* set rgb colour */
        colorState++;
        if (colorState > 3) colorState = 0;
        
        lcd.setCursor(0, 0);
        lcd.print("Kolor:          ");
        lcd.setCursor(7, 0);
        
        switch (colorState) {
            case 0: setRGBColor(0, 0, 0); lcd.print("Brak"); break;
            case 1: setRGBColor(255, 0, 0); lcd.print("Czerwony"); break;
            case 2: setRGBColor(0, 255, 0); lcd.print("Zielony"); break;
            case 3: setRGBColor(0, 0, 255); lcd.print("Niebieski"); break;
        } 
    }

    draw_lcd_menu();
}