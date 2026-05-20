
#ifndef THREADER_H
#define THREADER_H

#include "components.h"
#include <string>

bool load_new_print_sequence (PrintSequence seq);
int get_current_print_progress_percent();
std::string get_current_print_name();

void pause_printing(bool state);
void stop_printing();
bool is_printing_paused();
bool is_print_loaded();
int get_next_nail_num();

int get_current_ring_position();
void set_current_ring_position(int pos);

void rotate_ring_to_nail(int nail_idx);
void plot_around_nail(int nail_idx);

void set_printing_error(std::string line1, std::string line2 = "");
void printTask(void *params);

#endif