/*
Bresenham's Algorithm, Jack Bresenham 1962.
Definition:
An efficient algorithm to render a line with pixels. The long dimension is
incremented for each pixel, and the fractional slope is accumulated.

Implemented by Anton Gerdelan 10 Feb 2015
observation:
generally diagrams use matrices but the INTERSECTIONS of lines represent
pixels, not the big squares. grrr.

basic idea:
http://www.falloutsoftware.com/tutorials/dd/dd4.htm
*/

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <stdio.h>
#include <string.h> // memset

//
// plot colour
#define R 34
#define G 207
#define B 50
// secondary plot colour
#define R2 41
#define G2 184
#define B2 55

// rgb
unsigned char img[256*256*3];

//
// draws a red pixel in the image buffer
//
void plot (int x, int y, int r, int g, int b) {
	int index = y * 256 + x;
	int red = index * 3;
	img[red] = r;
	img[red + 1] = g;
	img[red + 2] = b;
}

//
// 
//
void plot_line (int x_i, int y_i, int x_f, int y_f) {
	// current plot points
	int x = x_i;
	int y = y_i;
	// original deltas between start and end points
	int d_x = x_f - x_i;
	int d_y = y_f - y_i;
	// increase rate on each axis
	int i_x = 1;
	int i_y = 1;
	// remember direction of line on each axis
	if (d_x < 0) {
		printf ("x is negative direction\n");
		i_x = -1;
		d_x = abs (d_x);
	}
	if (d_y < 0) {
		printf ("y is negative direction\n");
		i_y = -1;
		d_y = abs (d_y);
	}
	// scaled deltas (used to allow integer comparison of <0.5)
	int d2_x = d_x * 2;
	int d2_y = d_y * 2;
	printf ("double errors are %i x and %i y\n", d2_x, d2_y);
	
	// identify major axis (remember these have been absoluted)
	if (d_x > d_y) {
		printf ("x axis is major\n");
		// initialise error term
		int err = d2_y  - d_x;
		for (int i = 0; i <= d_x; i++) {
			plot (x, y, R, G, B);
			//
			// uncomment these for line-thickness of 3
			//
			plot (x, y + 1, R2, G2, B2);
			plot (x, y - 1, R2, G2, B2);
			printf ("%i,%i. 2*err is now %i\n", x, y, err);
			if (err >= 0) {
				err -= d2_x;
				y += i_y;
			}
			err += d2_y;
			x += i_x;
		} // endfor
	} else {
		printf ("y axis is major\n");
		// initialise error term
		int err = d2_x  - d_y;
		for (int i= 0; i <= d_y; i++) {
			plot (x, y, R, G, B);
			//
			// uncomment these for line-thickness of 3
			//
			plot (x + 1, y, R2, G2, B2);
			plot (x - 1, y, R2, G2, B2);
			printf ("%i,%i. 2*err is now %i\n", x, y, err);
			if (err >= 0) {
				err -= d2_y;
				x += i_x;
			}
			err += d2_x;
			y += i_y;
		} // endfor
	} // endif
} // endfunc

int main () {
	// grey
	memset (img, 255, 256 * 256 * 3);
	// add some grid lines every 32 pixels
	for (int y = 0; y < 256; y++) {
		for (int x = 0; x < 256; x++) {
			if ((x % 32 == 0) || (y % 32 == 0)) {
				plot (x, y, 200, 200, 200);
			}
		}
	}
	int x_i = -1;
	int y_i = -1;
	int x_f = -1;
	int y_f = -1;
	printf ("domain is positive range 256x256\n");
	printf ("enter line start x,y\n>");
	scanf ("%i,%i", &x_i, &y_i);
	printf ("you entered %i,%i\n", x_i, y_i);
	printf ("enter line end x,y\n>");
	scanf ("%i,%i", &x_f, &y_f);
	printf ("you entered %i,%i\n", x_f, y_f);
	printf ("plotting...\n");
	plot_line (x_i, y_i, x_f, y_f);
	
	// draw a second line
	plot_line (10, 10, 200, 128);
	
	printf ("writing output.png...\n");
	stbi_write_png ("output.png", 256, 256, 3, img, 0);

	return 0;
}
