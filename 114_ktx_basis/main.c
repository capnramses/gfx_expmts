#include "gfx.h"
#include <stdio.h>


int main( int argc, char** argv ) {
	if ( argc < 2 ) {

	printf("Usage: %s IMAGE.BASIS\n", argv[0] );
return 0;
}

	if ( !gfx_start( "ktx/basis demo", NULL, false ) ) { return 1; }
	printf( "renderer:\n%s\n", gfx_renderer_str() );


	const char* input_str = argv[1];
	printf( "loading `%s`\n", input_str);


	gfx_stop();

	return 0;
}
