// platform-consistent rand() and srand()
// based on http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf pg 312
#pragma once

#define APG_RAND_MAX 32767 // Must be at least 32767 (0x7fff). Windows uses this value.

void apg_srand( unsigned int seed );
int apg_rand( void );
// same as apg_rand() except returns a value between 0.0 and 1.0
float apg_randf( void );
// useful to re-seed apg_srand later with whatever the pseudo-random sequence is up to now i.e. for saved games.
unsigned int apg_get_srand_next( void );
