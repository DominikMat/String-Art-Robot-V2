#include "threader.h"
#include "components.h" // dla funkcji sprzętowych np. obrotu silnika
#include <Arduino.h>

PrintSequence current_print;
bool print_loaded = false;
int print_progress_percent = 0;
bool paused = false;

// --- Parametry mechaniki z V1 ---
const int num_nails = 32; 
const int stepper_steps_per_ring_rotation = 4096 * 4; 
const int steps_per_nail = stepper_steps_per_ring_rotation / num_nails; 
const int target_stepper_rpm = 10;
const int step_delay_microseconds = (60 / target_stepper_rpm) * 1000 * 1000 / 4096; 

const int servo_middle_angle = 90;
const int servo_rotation_angle_span = 30;
const int servo_inside_angle = servo_middle_angle + servo_rotation_angle_span / 2;
const int servo_outside_angle = servo_middle_angle - servo_rotation_angle_span / 2;
const int SERVO_DELAY_MS = 1500;

// Stan maszyny
int current_ring_step_position = 0;
int current_servo_angle = 90;
int next_nail = 0;

// Uchwyt do wątku FreeRTOS
TaskHandle_t printTaskHandle = NULL;

void load_new_print_sequence(PrintSequence seq) {
    // 1. Zabezpieczenie przed wielokrotnym tworzeniem zadań
    if (printTaskHandle != NULL) {
        Serial.println("Stopping previous task...");
        stop_printing(); 
        vTaskDelay(200 / portTICK_PERIOD_MS); // Daj czas na posprzątanie
    }

    current_print = seq;
    
    // 2. Sprawdzenie czy sekwencja w ogóle ma dane
    if (current_print.nail_sequence.empty()) {
        Serial.println("ERROR: Nail sequence is EMPTY! Task not started.");
        return;
    }

    print_loaded = true;
    paused = false;
    print_progress_percent = 0;
    
    Serial.printf("Creating task for print: %s with %d nails\n", current_print.print_name.c_str(), current_print.nail_sequence.size());

    // 3. Tworzymy zadanie z wyższym priorytetem (np. 2), żeby na pewno ruszyło
    BaseType_t result = xTaskCreatePinnedToCore(
        printTask,
        "PrintTask",
        8192,               // Zwiększony stos dla bezpieczeństwa
        NULL,
        2,                  // Wyższy priorytet
        &printTaskHandle,
        0                   // Przypisanie do Core 0 (pętla loop() zwykle działa na Core 1)
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
    set_rgb_specified_colour(COLOUR_PRINT_STOPPED);
}

void rotate_ring(int steps, bool anti_clockwise) {
    return; // debug

    for (int i = 0; i < steps; i++) {
        while (paused && print_loaded) { 
            vTaskDelay(50 / portTICK_PERIOD_MS); // Czekaj w uśpieniu (nie blokuje CPU)
        }
        if (!print_loaded) return; 

        // ------------------------------------------------------------------
        // TODO: TUTAJ WSTAW FUNKCJĘ Z TWOJEGO NOWEGO components.h 
        // np. stepStepper(anti_clockwise); albo customowy zapis do pinów
        // ------------------------------------------------------------------
        
        current_ring_step_position = (current_ring_step_position + (anti_clockwise ? 1 : (stepper_steps_per_ring_rotation-1))) % stepper_steps_per_ring_rotation; 
        delayMicroseconds(step_delay_microseconds);
        vTaskDelay(2 / portTICK_PERIOD_MS);
    }
}

void rotate_ring_to_nail(int nail_idx) {
    int target_pos = nail_idx * steps_per_nail;
    int diff = target_pos - current_ring_step_position;
    
    // Obliczanie najkrótszej drogi obrotu (zgodnie ze wskazówkami lub pod prąd)
    if (diff > stepper_steps_per_ring_rotation / 2) diff -= stepper_steps_per_ring_rotation;
    if (diff < -stepper_steps_per_ring_rotation / 2) diff += stepper_steps_per_ring_rotation;

    rotate_ring(abs(diff), diff > 0);
}

void plot_around_nail (int nail_idx) {
    rotate_ring_to_nail(nail_idx);
    if(!print_loaded) return; // Jeśli wcisnęliśmy stop w trakcie obrotu pierścienia
        
    servoWrite(servo_outside_angle);
    vTaskDelay(SERVO_DELAY_MS / portTICK_PERIOD_MS);
    if(!print_loaded) return;
    
    rotate_ring_to_nail((nail_idx + 1) % num_nails);
    if(!print_loaded) return;
    
    servoWrite(servo_inside_angle);
    vTaskDelay(SERVO_DELAY_MS / portTICK_PERIOD_MS);
}

void printTask(void *pvParameters) {
    Serial.println(">>>> PRINT TASK STARTED <<<<");
    
    servoWrite(servo_inside_angle);
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