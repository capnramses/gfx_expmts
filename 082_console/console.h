#pragma once
#include <stdbool.h>

#define C_STR_MAX 128

bool c_create_var( const char* str, float val );
bool c_set_var( const char* str, float val );
bool c_get_var( const char* str, float* val );
bool c_autocomplete( const char* str, char* completed );

void c_hist_add( const char* str );
void c_hist_print();
