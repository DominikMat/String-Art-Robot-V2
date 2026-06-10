#include "threader.h"
#include "lcd_menu.h"
#include "components.h"
#include <Arduino.h>

PrintSequence current_print;
bool print_loaded = false;
int print_progress_percent = 0;
bool paused = false;

int nail_number = 120; 
float steps_per_single_nail = (float)STEPS_PER_FULL_STEPPER_ROTATION / (float) nail_number; 

// SERVO PARAMS
const int SERVO_MIDDLE_ANGLE = 90;
const int SERVO_ROTATION_ANGLE_SPAN = 30;
const int SERVO_INSIDE_ANGLE = SERVO_MIDDLE_ANGLE - SERVO_ROTATION_ANGLE_SPAN / 2;
const int SERVO_OUTSIDE_ANGLE = SERVO_MIDDLE_ANGLE + SERVO_ROTATION_ANGLE_SPAN / 2;
const int SERVO_DELAY_MS = 250;

// Stan maszyny
int current_ring_step_position = 0;
int current_servo_angle = 90;
int next_nail = 0;

// Uchwyt do wątku FreeRTOS
TaskHandle_t printTaskHandle = NULL;

bool load_new_print_sequence(PrintSequence seq) {
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
        return false;
    }

    // sprawdz czy rozklad gwozdzi sie zgadza
    // if (current_print.nail_number != NUM_NAILS) {
    //     // "Nail num mismatch seq:200 set:200"
    //     set_printing_error("NailNum mismatch", "seq:"+std::to_string(current_print.nail_number) + " set:" + std::to_string(NUM_NAILS));
    //     return false;
    // }

    // ustaw dane poczatkowe
    print_loaded = true;
    paused = false;
    print_progress_percent = 0;
    Serial.printf("Creating task for print: %s with %d nails\n", current_print.print_name.c_str(), current_print.nail_sequence.size());
    change_microstepping_mode(STEP_MODE_QUARTER); // default microstep mode
    find_stepper_home_position();
    // set_stepper_motor_max_speed();
    // set_stepper_motor_speed();

    // dostosuj rozklad gwozdzi
    if (current_print.nail_number != nail_number) {
        nail_number = current_print.nail_number;
        steps_per_single_nail = STEPS_PER_FULL_STEPPER_ROTATION * (int)get_current_microstep_mode() / (float)nail_number;
        Serial.printf("Set new nail number for print: %d\n", nail_number);
    }

    // Tworzymy osobny watek na drukowanie
    BaseType_t result = xTaskCreatePinnedToCore(
        printTask,"PrintTask", 8192, NULL, 2, &printTaskHandle, 0
    );

    if (result != pdPASS) {
        Serial.println("CRITICAL ERROR: Failed to create PrintTask (Out of memory?)");
        return false;
    }

    set_rgb_specified_colour(COLOUR_PRINTING);
    return true;
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

int get_current_ring_position() {
    return current_ring_step_position;
}
void set_current_ring_position(int step_position) {
    int steps_per_rotation = STEPS_PER_FULL_STEPPER_ROTATION * (int)get_current_microstep_mode();
    current_ring_step_position = step_position % steps_per_rotation;
    if (current_ring_step_position < 0) current_ring_step_position += steps_per_rotation;
}

void rotate_ring_to_nail(int nail_idx) {
    int steps_per_rotation = STEPS_PER_FULL_STEPPER_ROTATION * (int)get_current_microstep_mode();
    int target_pos = nail_idx * steps_per_single_nail;
    int diff = target_pos - current_ring_step_position;
    
    // Obliczanie najkrótszej drogi obrotu (zgodnie ze wskazówkami lub pod prąd)
    if (diff > steps_per_rotation / 2) diff -= steps_per_rotation;
    if (diff < -steps_per_rotation / 2) diff += steps_per_rotation;

    move_stepper_steps(abs(diff), diff > 0);
}

void plot_around_nail (int nail_idx) {
    rotate_ring_to_nail(nail_idx);
    if(!print_loaded) return;
        
    servoWrite(SERVO_OUTSIDE_ANGLE);
    vTaskDelay(SERVO_DELAY_MS / portTICK_PERIOD_MS);
    if(!print_loaded) return;
    
    rotate_ring_to_nail((nail_idx + 1) % nail_number);
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