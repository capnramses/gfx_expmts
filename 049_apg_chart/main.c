/*
Copyright Anton Gerdelan 2015

2019 NOTE
I found this code on my old laptop - I think it was an experiment I wrote around 2015 to try manually making a chart-plotter.
I eventually wrote something similar as a WebGL routine for creating plots as textures for use on 3D GUI panels.
Don't use wchar types/functions in production software - use UTF-8 Unicode - but it's fine for quick demos.
ENDNOTE

Ideas

* render text to separate memory image
	that way total width etc. can be retained,
	and the whole thing is very easily centred and re-aligned

* error bar renderer

* point renderer
  - point styles could be encoded like the font thing

*/

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdio.h>
#include <string.h> // memset
#define _USE_MATH_DEFINES
#include <math.h>
#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif
#define HALF_PI M_PI / 2
#include <wchar.h>
typedef wchar_t wchar;
#include <locale.h>
#include <stdbool.h>
#include <stdlib.h>

#define RES 512
#define XOFF 12
#define CHARTMARGL 32 + 12
#define CHARTMARGR RES - 32 + 12
#define CHARTMARGT 32
#define CHARTMARGB RES - 32

// rgb working image
unsigned char img[RES * RES * 3];

// rgb font image
unsigned char* font_img;
int font_w;

#define N 100
double datapoints[N * 3];
int n;

void ginput () {
	FILE* fp = fopen ("test.xy", "r");

	char line[1024];
	double y_sum = 0.0;
	while (fgets (line, 1024, fp)) {
		double x, y, y_err;
		sscanf (line, "%lf %lf %lf\n", &x, &y, &y_err);
		datapoints[n * 3] = x;
		datapoints[n * 3 + 1] = y;
		datapoints[n * 3 + 2] = y_err;
		n++;
	}
	fclose (fp);
}

//
// draws a red pixel in the image buffer
//
void plot (int x, int y, int r, int g, int b) {
	if (x < 0 || y < 0 || x >= RES || y >= RES) {
		return;
	}
	int index = y * RES + x;
	int red = index * 3;
	img[red] = r;
	img[red + 1] = g;
	img[red + 2] = b;
}

int glyph_widths[] = {
	1, 3, 5, 3, 4, 4, 1, 2, // !"#$%&'(
	2, 3, 3, 2, 3, 1, 4, // )*+,-./
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, // 0-9
	2, 2, 3, 4, 3, 4, 4, // :;<=>?@
	4, 4, 4, 4, // ABCD
	4, 4, 4, 4, // EFGH
	3, 3, 4, 4, // IJKL
	5, 4, 4, 4, // MNOP
	4, 4, 4, 3, // QRST
	4, 5, 5, 5, // UVWX
	5, 4, // YZ
	2, 4, 2, 3, 4, 2, // [\]^_`
	3, 3, 3, 3, // abcd
	3, 3, 3, 3, // efgh
	1, 2, 3, 1, // ijkl
	4, 3, 3, 3, // mnop
	3, 3, 3, 3, // qrst
	3, 3, 4, 3, // uvwx
	3, 3, // yz
	3, 1, 3, 4, // {|}~
	4, 5, 4, 5, // Gamma, Delta, Theta, Lamda
	4, 4, 4, 5, // Xi, Pi, Sigma, Upsilon
	5, 5, 5, // Phi, Psi, Omega
	4, 3, 3, 3, // alpha, beta, gamma, delta,
	2, 3, 4, 3, // epsilon, digamma, zeta, eta,
	3, 2, 3, 5, // theta, iota, kappa, lamda
	3, 3, 3, 5, // mu, nu, xi, pi
	4, 4, 4, 3, // rho, sigma, tau, upsilon
	5, 4, 5, 5 // phi, chi, psi, omega
};

//
// x,y is the bottom-left pixel of the first glyph
// code is the ascii code
//
void plot_glyph (int x, int y, int code, int r, int g, int b, int size_fac,
	bool alt) {
	int code_index = code - 33;
	assert (code_index > 0 && code_index < font_w / 5);
	int w = glyph_widths[code_index];
	for (int yi = 0; yi < 5 * size_fac; yi++) {
		for (int xi = 0; xi < w * size_fac; xi++) {
			unsigned char val_red = font_img[(yi / size_fac * font_w +
				xi / size_fac + code_index * 5)];
			if (val_red) {
				if (alt) {
					plot (x + yi - 5 * size_fac + 1, y - xi, r, g, b);
				} else {
					plot (x + xi, y + yi - 5 * size_fac + 1, r, g, b);
				}
			}
		} // endfor
	} // endfor
}

void plot_str (int x, int y, const wchar* str, int r, int g, int b,
	int size_fac, bool alt) {
	assert (str);
	int len = wcslen (str);
	int curr_x = x;
	int curr_y = y;
	for (int i = 0; i < len; i++) {
		if (str[i] == ' ') {
			if (alt) {
				curr_y -= 3 * size_fac;
			} else {
				curr_x += 3 * size_fac;
			}
		} else if (str[i] == '\t') {
			if (alt) {
				curr_y -= 8 * size_fac;
			} else {
				curr_x += 8 * size_fac;
			}
		} else if (str[i] == '\n') {
			if (alt) {
				curr_x += 6 * size_fac;
				curr_y = y;
			} else {
				curr_y += 6 * size_fac;
				curr_x = x;
			}
		} else if (str[i] >= 33 && str[i] <= 126) {
			plot_glyph (curr_x, curr_y, str[i], r, g, b, size_fac, alt);
			int w = glyph_widths[str[i] - 33];
			if (alt) {
				curr_y -= (w * size_fac + 1);
			} else {
				curr_x += (w * size_fac + 1);
			}
		// lower-case greek
		} else {
			int code = 0;
			switch (str[i]) {
				case 0x0393: code = '~' + 1; break; // Gamma
				case 0x0394: code = '~' + 2; break; // Delta
				case 0x0398: code = '~' + 3; break; // Theta
				case 0x039B: code = '~' + 4; break; // Lambda
				case 0x039E: code = '~' + 5; break; // Xi
				case 0x03A0: code = '~' + 6; break; // Pi
				case 0x03A3: code = '~' + 7; break; // Sigma
				case 0x03A5: code = '~' + 8; break; // Upsilon
				case 0x03A6: code = '~' + 9; break; // Phi
				case 0x03A8: code = '~' + 10; break; // Psi
				case 0x03A9: code = '~' + 11; break; // Omega
				case 0x03B1: code = '~' + 12; break; // alpha
				case 0x03B2: code = '~' + 13; break; // beta
				case 0x03B3: code = '~' + 14; break; // gamma
				case 0x03B4: code = '~' + 15; break; // delta
				case 0x03B5: code = '~' + 16; break; // epsilon
				case 0x03DD: code = '~' + 17; break; // digamma
				case 0x03B6: code = '~' + 18; break; // zeta
				case 0x03B7: code = '~' + 19; break; // eta
				case 0x03B8: code = '~' + 20; break; // theta
				case 0x03B9: code = '~' + 21; break; // iota
				case 0x03BA: code = '~' + 22; break; // kappa
				case 0x03BB: code = '~' + 23; break; // lamda
				case 0x03BC: code = '~' + 24; break; // mu
				case 0x03BD: code = '~' + 25; break; // nu
				case 0x03BE: code = '~' + 26; break; // xi
				case 0x03C0: code = '~' + 27; break; // pi
				case 0x03C1: code = '~' + 28; break; // rho
				case 0x03C2: code = '~' + 29; break; // sigma
				case 0x03C4: code = '~' + 30; break; // tau
				case 0x03C5: code = '~' + 31; break; // upsilon
				case 0x03C6: code = '~' + 32; break; // phi
				case 0x03C7: code = '~' + 33; break; // chi
				case 0x03C8: code = '~' + 34; break; // psi
				case 0x03C9: code = '~' + 35; break; // omega
				default:
					printf ("unhandled char, code dec %i hex %#x\n", (int)str[i], (unsigned int)str[i]);
					return;
			} // endswitch
			plot_glyph (curr_x, curr_y, code, r, g, b, size_fac, alt);
			int w = glyph_widths[code - 33];
			if (alt) {
				curr_y -= (w * size_fac + 1);
			} else {
				curr_x += (w * size_fac + 1);
			}
		}
	}
}

void plot_cross (int x, int y, int r, int g, int b) {
	for (int i = 0; i < 7; i++) {
		plot (x + i - 3, y, r, g, b);
	}
	for (int i = 0; i < 7; i++) {
		plot (x, y + i - 3, r, g, b);
	}
}
void plot_error (int x, int y, int h, int r, int g, int b) {
	// top crossbar
	for (int i = 0; i < 7; i++) {
		plot (x + i - 3, y + h, r, g, b);
	}
	// bottom crossbar
	for (int i = 0; i < 7; i++) {
		plot (x + i - 3, y - h, r, g, b);
	}
	// vertical bar
	for (int i = -h; i <= h; i++) {
		plot (x, y + i, r, g, b);
	}
}

//
// 
//
void plot_line (int x_i, int y_i, int x_f, int y_f, int r, int g, int b) {
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
		i_x = -1;
		d_x = abs (d_x);
	}
	if (d_y < 0) {
		i_y = -1;
		d_y = abs (d_y);
	}
	// scaled deltas (used to allow integer comparison of <0.5)
	int d2_x = d_x * 2;
	int d2_y = d_y * 2;
	
	// identify major axis (remember these have been absoluted)
	if (d_x > d_y) {
		// initialise error term
		int err = d2_y  - d_x;
		for (int i = 0; i <= d_x; i++) {
			plot (x, y, r, g, b);
			//
			// uncomment these for line-thickness of 3
			//
			//plot (x, y + 1, r, g, b);
			//plot (x, y - 1, r, g, b);
			if (err >= 0) {
				err -= d2_x;
				y += i_y;
			}
			err += d2_y;
			x += i_x;
		} // endfor
	} else {
		// initialise error term
		int err = d2_x  - d_y;
		for (int i= 0; i <= d_y; i++) {
			plot (x, y, r, g, b);
			//
			// uncomment these for line-thickness of 3
			//
			//plot (x + 1, y, r, g, b);
			//plot (x - 1, y, r, g, b);
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
	setlocale (LC_ALL, "");

	ginput ();

	// load font
	int iy, in;
	font_img = stbi_load ("font.png", &font_w, &iy, &in, 1);

	// grey
	memset (img, 255, RES * RES * 3);
	// add some grid lines every 32 pixels
	for (int y = 32; y <= RES - 32; y++) {
		for (int x = 32; x <= RES - 32; x++) {
			if (((x + 32) % 64 == 0) || ((y + 32) % 64 == 0)) {
				plot (x + XOFF, y, 200, 200, 200);
			}
		}
	}
	
	// plot some data points
	float range = 448.0f;
	float pixels = 32.0f * 7.0f;
	for (int i = 0; i < n; i++) {
		int x = CHARTMARGL + (int)(datapoints[i *3] / range * pixels);
		int y = RES - (32 + (int)(datapoints[i * 3 + 1] / range * pixels));
		int h = (int)(datapoints[i * 3 + 2] / range * pixels);
		plot_cross (x, y, 0, 0, 0);
		plot_error (x, y, h, 0, 0, 0);
	}
	
	// draw a main plot line
	plot_line (CHARTMARGL, RES - 32, CHARTMARGL + 384, RES - (32 + 320), 255, 0, 0);
	
	plot_line (CHARTMARGL, 32, CHARTMARGL, RES - 32, 0, 0 ,0);
	plot_line (CHARTMARGL, RES - 32, RES - CHARTMARGL, RES - 32, 0, 0 ,0);
	for (int y = 32; y < RES; y += 64) {
		wchar tmp[32];
		swprintf (tmp, 32, L"%3i", 512 - y - 32);
		plot_str (12 + XOFF, y + 2, tmp, 0,0,0,1, false);
	}
	for (int x = 32; x < RES; x += 64) {
		wchar tmp[32];
		swprintf (tmp, 32, L"%3i", x - 32);
		plot_str (x - 7 + XOFF, RES - 32 + 11, tmp, 0,0,0,1, false);
	}
	plot_str (RES / 2 - 60 + XOFF, RES - 4, L"ΔResistance (Ω)", 0,0,0,2, false);
	plot_str (13, 315, L" Capacitance (μF)", 0,0,0,2, true);
	plot_str (RES / 2 - 130 + XOFF, 18, L"Observations of λΨ Waves ", 0,0,0,3, false);
	
	// plot some labels
	//plot_str (0, 5, " !\"#$%&'()*+,-./\n0123456789:;<=>?\n@ABCDEFGHIJKLMNO\nPQRSTUVWXYZ[\\]^_\n`abcdefghijklmno\npqrstuvwxyz{|}~", 0, 0, 0);
	wprintf (L"writing output.png...\n");
	stbi_write_png ("output.png", RES, RES, 3, img, 0);

	return 0;
}
