#include "threader.h"
#include "lcd_menu.h"
#include "components.h" // dla funkcji sprzętowych np. obrotu silnika
#include <Arduino.h>

PrintSequence current_print;
bool print_loaded = false;
int print_progress_percent = 0;
bool paused = false;

// STEPPER PARAMS
const int NUM_NAILS = 32; 
const float MICRO_STEP_MODE = 1; // 0.5 for half-step, 0.25 for quarter
const int STEPS_PER_FULL_ROTATION = 200 / MICRO_STEP_MODE; 
const float STEPS_PER_SINGLE_NAIL = STEPS_PER_FULL_ROTATION / NUM_NAILS; 
const int TARGET_STEPPER_RPM = 10;
const int STEP_DELAY_MICROSECONDS = (60 / TARGET_STEPPER_RPM) * 1000 * 1000 / 4096; 

// SERVO PARAMS
const int SERVO_MIDDLE_ANGLE = 90;
const int SERVO_ROTATION_ANGLE_SPAN = 30;
const int SERVO_INSIDE_ANGLE = SERVO_MIDDLE_ANGLE + SERVO_ROTATION_ANGLE_SPAN / 2;
const int SERVO_OUTSIDE_ANGLE = SERVO_MIDDLE_ANGLE - SERVO_ROTATION_ANGLE_SPAN / 2;
const int SERVO_DELAY_MS = 1500;

// Stan maszyny
int current_ring_step_position = 0;
int current_servo_angle = 90;
int next_nail = 0;

// Uchwyt do wątku FreeRTOS
TaskHandle_t printTaskHandle = NULL;

void load_new_print_sequence(PrintSequence seq) {
    // Zakoncz poprzednie zadanie jesli aktywne
    if (printTaskHandle != NULL) {
        Serial.println("Stopping previous task...");
        stop_printing(); 
        vTaskDelay(200 / portTICK_PERIOD_MS); // Daj czas na posprzątanie
    }
    current_print = seq;
    
    // Sprawdz czy sa dane w sekwencji
    if (current_print.nail_sequence.empty()) {
        set_printing_error("ERR Nail sequen-", "-ce is empty!");
        return;
    }

    // sprawdz czy rozklad gwozdzi sie zgadza
    if (current_print.nail_number != NUM_NAILS) {
        // "Nail num mismatch seq:200 set:200"
        set_printing_error("ERR Nail num", "mismatch seq:"+std::to_string(current_print.nail_number) + " set:" + std::to_string(NUM_NAILS));
    }

    // ustaw dane poczatkowe
    print_loaded = true;
    paused = false;
    print_progress_percent = 0;
    Serial.printf("Creating task for print: %s with %d nails\n", current_print.print_name.c_str(), current_print.nail_sequence.size());

    // Tworzymy osobny watek na drukowanie
    BaseType_t result = xTaskCreatePinnedToCore(
        printTask,"PrintTask", 8192, NULL, 2, &printTaskHandle, 0
    );

    if (result != pdPASS) {
        Serial.println("CRITICAL ERROR: Failed to create PrintTask (Out of memory?)");
    }

    set_rgb_specified_colour(COLOUR_PRINTING);
}

int get_current_print_progress_percent() { return print_progress_percent; }
std::string get_current_print_name() { return print_loaded ? current_print.print_name : "none"; }
void pause_printing(bool state) { 
    paused = state;
    if(state) set_rgb_specified_colour(COLOUR_PRINT_PAUSED);
    else      set_rgb_specified_colour(COLOUR_PRINTING); 
}
bool is_printing_paused() { return paused; }
bool is_print_loaded() { return print_loaded; }
int get_next_nail_num() { return print_loaded && !paused ? next_nail : -1; }

void stop_printing() { 
    print_loaded = false;
    paused = false; // odblokowujemy pętle żeby wątek mógł umrzeć
    if (printTaskHandle != NULL) {
        vTaskDelete(printTaskHandle); // Natychmiastowe zabicie wątku
        printTaskHandle = NULL;
    }
    Serial.println("Wydruk przerwany przez uzytkownika (STOP).");
    showAlert("Printing STOP");
    set_rgb_specified_colour(COLOUR_PRINT_STOPPED);
}

void set_printing_error(std::string line1, std::string line2) {
    showAlert(line1,line2);
    Serial.print( ("Printing error thrown: " + line1+line2).c_str() );
}

void rotate_ring(int steps, bool anti_clockwise) {
    return; // debug

    for (int i = 0; i < steps; i++) {
        while (paused && print_loaded) { 
            vTaskDelay(50 / portTICK_PERIOD_MS); // Czekaj w uśpieniu (nie blokuje CPU)
        }
        if (!print_loaded) return; 

        // TODO funkcja ruszajaca silnikiem
        
        current_ring_step_position = (current_ring_step_position + (anti_clockwise ? 1 : (STEPS_PER_FULL_ROTATION-1))) % STEPS_PER_FULL_ROTATION; 
        delayMicroseconds(STEP_DELAY_MICROSECONDS);
        vTaskDelay(2 / portTICK_PERIOD_MS);
    }
}

void rotate_ring_to_nail(int nail_idx) {
    int target_pos = nail_idx * STEPS_PER_SINGLE_NAIL;
    int diff = target_pos - current_ring_step_position;
    
    // Obliczanie najkrótszej drogi obrotu (zgodnie ze wskazówkami lub pod prąd)
    if (diff > STEPS_PER_FULL_ROTATION / 2) diff -= STEPS_PER_FULL_ROTATION;
    if (diff < -STEPS_PER_FULL_ROTATION / 2) diff += STEPS_PER_FULL_ROTATION;

    rotate_ring(abs(diff), diff > 0);
}

void plot_around_nail (int nail_idx) {
    rotate_ring_to_nail(nail_idx);
    if(!print_loaded) return;
        
    servoWrite(SERVO_OUTSIDE_ANGLE);
    vTaskDelay(SERVO_DELAY_MS / portTICK_PERIOD_MS);
    if(!print_loaded) return;
    
    rotate_ring_to_nail((nail_idx + 1) % NUM_NAILS);
    if(!print_loaded) return;
    
    servoWrite(SERVO_INSIDE_ANGLE);
    vTaskDelay(SERVO_DELAY_MS / portTICK_PERIOD_MS);
}

void printTask(void *pvParameters) {
    Serial.println(">>>> PRINT TASK STARTED <<<<");
    
    servoWrite(SERVO_INSIDE_ANGLE);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    int total_nails = current_print.nail_sequence.size();
    
    // Obsługa pierwszego gwoździa
    next_nail = current_print.nail_sequence[0];
    Serial.printf("Moving to start nail: %d\n", next_nail);
    rotate_ring_to_nail(next_nail);

    // glowna petla
    for (int i = 1; i < total_nails; i++) {
        while (paused && print_loaded) {
            static unsigned long last_pause_msg = 0;
            if (millis() - last_pause_msg > 2000) {
                Serial.println("Print is paused. Waiting...");
                last_pause_msg = millis();
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        
        if (!print_loaded) {
            Serial.println("Print flag cleared. Exiting loop.");
            break;
        }
        
        next_nail = current_print.nail_sequence[i];
        Serial.printf("Step %d/%d: Plotting nail %d\n", i, total_nails-1, next_nail);
        
        plot_around_nail(next_nail);
        
        print_progress_percent = (i * 100) / (total_nails - 1);
        
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }

    Serial.println(">>>> PRINT TASK FINISHED <<<<");
    printTaskHandle = NULL;
    vTaskDelete(NULL); 
    set_rgb_specified_colour(COLOUR_PRINT_COMPLETE);
}