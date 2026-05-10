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
    delay(500);
    
    /* prep lcd menu*/
    lcd.clear();
    changeMenuScreen(MAIN);
}   


void loop() {
    loopServo();
    draw_lcd_menu();
}