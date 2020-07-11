#include "apg_rand.h"

static unsigned long int _srand_next = 1;

void apg_srand( unsigned int seed ) { _srand_next = seed; }

int apg_rand( void ) {
  _srand_next = _srand_next * 1103515245 + 12345;
  return (unsigned int)( _srand_next / ( ( APG_RAND_MAX + 1 ) * 2 ) ) % ( APG_RAND_MAX + 1 );
}

float apg_randf( void ) { return (float)apg_rand() / (float)APG_RAND_MAX; }

unsigned int apg_get_srand_next( void ) { return _srand_next; }
