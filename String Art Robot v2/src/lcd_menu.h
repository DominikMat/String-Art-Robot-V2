#ifndef LCD_MENU_H
#define LCD_MENU_H

#include "components.h"

enum Screen {
    MAIN, PRINT_SELECT, PROGRESS
};

void draw_lcd_menu() {
    // Print values (co 200ms)
    if (millis() - lastDisplayUpdate > 200) { 
        readPotentiometer();
        readHallSensor();
        
        lcd.setCursor(0, 1);
        lcd.print("P:");
        lcd.print((int)(potentiometerValue * 100)); 
        lcd.print(" H:");
        lcd.print(hallValue);
        lcd.print("   "); 
        
        lastDisplayUpdate = millis();
    }
}

#endif
