#include "threader.h"
#include "lcd_menu.h"
#include "components.h"
#include <Arduino.h>
#include <math.h>

PrintSequence current_print;
bool print_loaded = false;
int print_progress_percent = 0;
bool paused = false;

int nail_number = 120; 
float steps_per_single_nail = (float)STEPS_PER_FULL_STEPPER_ROTATION / (float) nail_number; 

// SERVO PARAMS
const int SERVO_MIDDLE_ANGLE = 90;
const int SERVO_ROTATION_ANGLE_SPAN = 30;
const int SERVO_INSIDE_ANGLE = SERVO_MIDDLE_ANGLE + SERVO_ROTATION_ANGLE_SPAN / 2;
const int SERVO_OUTSIDE_ANGLE = SERVO_MIDDLE_ANGLE - SERVO_ROTATION_ANGLE_SPAN / 2;
const int SERVO_DELAY_MS = 250;

// stepper params
const int STEPPER_DELAY_MS = 1200;
const MicrostepMode DEFAULT_MICROSTEP_MODE = MicrostepMode::STEP_MODE_EIGHTS;

// Stan maszyny
float current_ring_step_position = 0;
int current_servo_angle = 90;
int next_nail = 0;

// Uchwyt do wątku FreeRTOS
TaskHandle_t printTaskHandle = NULL;

/* ====================================================================================================== */
/*                                            RING MOVEMENT                                               */
/* ====================================================================================================== */

float get_current_ring_position() {
    return current_ring_step_position;
}
void set_current_ring_position(float step_position) {
    if (step_position >= STEPS_PER_FULL_STEPPER_ROTATION) step_position -= STEPS_PER_FULL_STEPPER_ROTATION;
    if (step_position < 0) step_position += STEPS_PER_FULL_STEPPER_ROTATION;
    current_ring_step_position = step_position;
}
void change_current_ring_position(float steps) {
    set_current_ring_position( current_ring_step_position + steps );
}

void rotate_ring_to_nail(int nail_idx, bool bypass_print_checks) {
    float target_pos = nail_idx * steps_per_single_nail;
    float diff = target_pos - current_ring_step_position;
    
    // Obliczanie najkrótszej drogi obrotu (zgodnie ze wskazówkami lub pod prąd)
    if (diff > STEPS_PER_FULL_STEPPER_ROTATION / 2) diff -= STEPS_PER_FULL_STEPPER_ROTATION;
    if (diff < -STEPS_PER_FULL_STEPPER_ROTATION / 2) diff += STEPS_PER_FULL_STEPPER_ROTATION;

    change_microstepping_mode(DEFAULT_MICROSTEP_MODE);
    move_stepper_steps(abs(diff), diff < 0, bypass_print_checks);
}

void plot_around_nail (int nail_idx) {
    if (!print_loaded) return;
    
    float target_pos = nail_idx * steps_per_single_nail;
    float diff = target_pos - current_ring_step_position;
    if (diff > STEPS_PER_FULL_STEPPER_ROTATION / 2) diff -= STEPS_PER_FULL_STEPPER_ROTATION;
    if (diff < -STEPS_PER_FULL_STEPPER_ROTATION / 2) diff += STEPS_PER_FULL_STEPPER_ROTATION;

    bool movement_dir = diff < 0;
    const int nail_overshoot_amount = 4;
    int nail_idx_overshoot = movement_dir ? (nail_idx + nail_number - nail_overshoot_amount) % nail_number : (nail_idx + nail_overshoot_amount) % nail_number;
    int nail_idx_first = movement_dir ? nail_idx : (nail_idx + 1) % nail_number;
    int nail_idx_last = movement_dir ? (nail_idx + 1) % nail_number : nail_idx;

    rotate_ring_to_nail(nail_idx_overshoot);
    vTaskDelay(STEPPER_DELAY_MS*0.2 / portTICK_PERIOD_MS);
    rotate_ring_to_nail(nail_idx_first);
    vTaskDelay(STEPPER_DELAY_MS / portTICK_PERIOD_MS);
        
    servoWrite(SERVO_OUTSIDE_ANGLE);
    vTaskDelay(SERVO_DELAY_MS / portTICK_PERIOD_MS);
    
    rotate_ring_to_nail(nail_idx_last);
    vTaskDelay(STEPPER_DELAY_MS / portTICK_PERIOD_MS);
    
    servoWrite(SERVO_INSIDE_ANGLE);
    vTaskDelay(SERVO_DELAY_MS / portTICK_PERIOD_MS);
}


/* ====================================================================================================== */
/*                                           THREADING LOGIC                                              */
/* ====================================================================================================== */

// prepare the robot for printing 
// setups stepper configs, servo position, 
// runs init sequence for stirng to wrap around first nail
void setup_printing_mode(){

    // move servo inside 
    servoWrite(SERVO_INSIDE_ANGLE);

    // find home pos for nail ring
    find_stepper_home_position();
    
    // user sequence for tieing first nail
    rotate_ring_to_nail((current_print.nail_sequence[0] + nail_number/2) % nail_number); // move ring so first nail is directly accross ring
    // pause_printing(true);
    showAlert("Tie string" , "accross ring", 600000);
}

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

    // dostosuj rozklad gwozdzi
    nail_number = current_print.nail_number;
    steps_per_single_nail = STEPS_PER_FULL_STEPPER_ROTATION / (float)nail_number;
    Serial.printf("Set new nail number for print: %d\n", nail_number);

    // obliczaniie wymaganej dlugosci nici
    float required_string_m = calculate_total_string_length_meters(current_print);
    Serial.printf("Creating task for print: %s with %d nails\n", current_print.print_name.c_str(), current_print.nail_sequence.size());
    Serial.printf("===> POTRZEBNA DLUGOSC NICI: %.2f metrow <===\n", required_string_m);

    // setup robot
    setup_printing_mode();

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

void printTask(void *pvParameters) {
    Serial.println(">>>> PRINT TASK STARTED <<<<");
    
    servoWrite(SERVO_INSIDE_ANGLE);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    int total_nails = current_print.nail_sequence.size();
    
    // wait to clear alert msg
    while (is_showing_alert()) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    find_stepper_home_position(); // in case user moves ring while attaching string

    // Obsługa pierwszego gwoździa NIEPOTRZEBNA JUZ
    next_nail = -1; //current_print.nail_sequence[0] % nail_number; // assume nails are 0 indexed
    // Serial.printf("Moving to start nail: %d\n", next_nail);
    // rotate_ring_to_nail(next_nail);

    // glowna petla
    for (int i = 1; i < total_nails; i++) {
        while ((paused && print_loaded) || is_showing_alert()) {
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

        // // reset position couter to account for over-time errors 
        // if (i % 50 == 0) { // every 50 nails
        //     Serial.printf("Periodical position reset - finding home ... \n");
        //     find_stepper_home_position();
        //     vTaskDelay( 2500 / portTICK_PERIOD_MS );
        // }
        
        next_nail = current_print.nail_sequence[i] % nail_number; // assume nails are 0 indexed
        Serial.printf("Step %d/%d: Plotting nail %d\n", i+1, total_nails, next_nail+1);
        
        plot_around_nail(next_nail);
        
        print_progress_percent = (i * 100) / (total_nails - 1);
        
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }

    Serial.println(">>>> PRINT TASK FINISHED <<<<");
    printTaskHandle = NULL;
    vTaskDelete(NULL); 
    set_rgb_specified_colour(COLOUR_PRINT_COMPLETE);
}

/* ====================================================================================================== */
/*                                              HELPERS                                                   */
/* ====================================================================================================== */

int get_current_print_progress_percent() { return print_progress_percent; }
std::string get_current_print_name() { return print_loaded ? current_print.print_name : "none"; }
void pause_printing(bool state) { 
    paused = state;
    if(state) set_rgb_specified_colour(COLOUR_PRINT_PAUSED);
    else      set_rgb_specified_colour(COLOUR_PRINTING); 

    // Serial.println("Printing paused, returning to home pos in 5s");
    // vTaskDelay( (2*(STEPPER_DELAY_MS+SERVO_DELAY_MS) + 1000) / portTICK_PERIOD_MS); // wait for threader task to finish 

    // servoWrite(SERVO_INSIDE_ANGLE);
    // find_stepper_home_position();
}
bool is_printing_paused() { return paused; }
bool is_print_loaded() { return print_loaded; }
int get_next_nail_num() { return print_loaded && !paused ? next_nail : -1; }

void set_printing_error(std::string line1, std::string line2) {
    showAlert(line1,line2);
    Serial.print( ("Printing error thrown: " + line1+line2).c_str() );
}

float calculate_total_string_length_meters(PrintSequence seq, float ring_diameter_mm) {
    if (seq.nail_sequence.size() < 2) return 0.0f;

    float radius_mm = ring_diameter_mm / 2.0f;
    float total_length_mm = 0.0f;
    int n = seq.nail_number;

    for (size_t i = 1; i < seq.nail_sequence.size(); i++) {
        int n1 = seq.nail_sequence[i - 1];
        int n2 = seq.nail_sequence[i];

        // Szukamy najkrótszej drogi między gwoździami po okręgu
        int diff = abs(n1 - n2);
        if (diff > n / 2) diff = n - diff; 

        // Kąt w radianach i długość cięciwy
        float angle_rad = diff * (2.0f * M_PI / n);
        float distance = 2.0f * radius_mm * sin(angle_rad / 2.0f);
        
        total_length_mm += distance;

        // Dodajemy 10mm marginesu per gwóźdź na samo owinięcie wokół metalu
        total_length_mm += 10.0f; 
    }

    return total_length_mm / 1000.0f; // Zamiana mm na metry
}

void wait_ms(int wait_time) {
    unsigned long current_time = millis();
    unsigned long end_time = current_time + wait_time;
    unsigned long last_watchdog_yield = current_time;
    
    while (current_time < end_time){
        if (current_time - last_watchdog_yield > 15) {
            taskYIELD();
            last_watchdog_yield = current_time;        
        }
        vTaskDelay(5 / portTICK_PERIOD_MS);
        current_time = millis();
    }
}