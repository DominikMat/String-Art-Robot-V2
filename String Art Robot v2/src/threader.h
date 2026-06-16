
#ifndef THREADER_H
#define THREADER_H

#include "components.h"
#include <string>

extern const int SERVO_INSIDE_ANGLE;
extern const int SERVO_OUTSIDE_ANGLE;
extern const int STEPS_PER_FULL_STEPPER_ROTATION;

bool load_new_print_sequence (PrintSequence seq);
int get_current_print_progress_percent();
std::string get_current_print_name();

void pause_printing(bool state);
void stop_printing();
bool is_printing_paused();
bool is_print_loaded();
int get_next_nail_num();
float calculate_total_string_length_meters(PrintSequence seq, float ring_diameter_mm = 250.0f);
void wait_ms(int ms);

float get_current_ring_position();
void set_current_ring_position(float pos);
void change_current_ring_position(float steps);

void rotate_ring_to_nail(int nail_idx, bool bypass_print_checks = false);
void plot_around_nail(int nail_idx);

void set_printing_error(std::string line1, std::string line2 = "");
void printTask(void *params);

#endif