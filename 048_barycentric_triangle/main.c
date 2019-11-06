/*
Barycentric Triangle Rasterisation Algorithm.
Barycentric coords are from Möbius in 1827
Definition:
bounding box (in pixels) of triangle to be drawn is found
within this box, line by line, use barycentric coordinates to calculate if a
pixel is inside the triangle
scanline fill from top to bottom

Implemented by Anton Gerdelan 25 Feb 2015
observation:
more computationally expensive than bresenham triangle fill

basic idea:
http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html#pointintrianglearticle
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
// re-implement old min/max
//
int min (int a, int b) {
	if (a < b) {
		return a;
	}
	return b;
}

//
// re-implement old min/max
//
int max (int a, int b) {
	if (a > b) {
		return a;
	}
	return b;
}

//
// re-implement old min/max
//
float minf (float a, float b) {
	if (a < b) {
		return a;
	}
	return b;
}

//
// re-implement old min/max
//
float maxf (float a, float b) {
	if (a > b) {
		return a;
	}
	return b;
}

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

void cross (int ax, int ay, int az, int bx, int by, int bz, int* rx,
	int* ry, int* rz) {
	*rx = ay * bz - az * by;
	*ry = az * bx - ax * bz;
	*rz = ax * by - ay * bx;
}

int dot (int ax, int ay, int az, int bx, int by, int bz) {
	return ax * bx + ay * by + az * bz;
}

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

	//
	// triangle points
	int ax = 10;
	int ay = 10;
	int bx = 100;
	int by = 200;
	int cx = 200;
	int cy = 120;
	
	//
	// find x,y bounds
	int minx = min (ax, min (bx, cx));
	int miny = min (ay, min (by, cy));
	int maxx = max (ax, max (bx, cx));
	int maxy = max (ay, max (by, cy));
	
	//
	// fill in scanlines inside bbox
	for (int y = miny; y <= maxy; y++) {
		for (int x = minx; x <= maxx; x++) {
			//
			// edge b-a
			//
			int ex = bx - ax;
			int ey = by - ay;
			
			// get vector P-A from vertex a to current point
			int dx = x - ax;
			int dy = y - ay;
			// cross product with an edge
			// if point above the edge = out of screen
			// else = into screen
			//
			// we use another vertex as a reference point to say which cross product
			// result (into or out of screen) is in the triangle
			
			int rx, ry, rz;
			rx = ry = rz = 0;
			cross (ex, ey, 0, dx, dy, 0, &rx, &ry, &rz);
			int fx = cx - ax;
			int fy = cy - ay;
			int sx, sy, sz;
			sx = sy = sz = 0;
			cross (ex, ey, 0, fx, fy, 0, &sx, &sy, &sz);
			//
			// if both pointing into or both pointing out of screen = true for edge 1
			if (dot (rx, ry, rz, sx, sy, sz) < 0) {
				continue; // fail test, outside triangle on this edge
			}
			
			//
			// edge c-b
			//
			ex = cx - bx;
			ey = cy - by;
			dx = x - bx;
			dy = y - by;
			cross (ex, ey, 0, dx, dy, 0, &rx, &ry, &rz);
			fx = ax - bx;
			fy = ay - by;
			cross (ex, ey, 0, fx, fy, 0, &sx, &sy, &sz);
			if (dot (rx, ry, rz, sx, sy, sz) < 0) {
				continue; // fail test, outside triangle on this edge
			}
			
			//
			// edge a-c
			//
			ex = ax - cx;
			ey = ay - cy;
			dx = x - cx;
			dy = y - cy;
			cross (ex, ey, 0, dx, dy, 0, &rx, &ry, &rz);
			fx = bx - cx;
			fy = by - cy;
			cross (ex, ey, 0, fx, fy, 0, &sx, &sy, &sz);
			if (dot (rx, ry, rz, sx, sy, sz) < 0) {
				continue; // fail test, outside triangle on this edge
			}
			
			//
			// passed! colour pixel
			plot (x, y, R, G, B);
		}
	}

	printf ("writing output.png...\n");
	stbi_write_png ("output.png", 256, 256, 3, img, 0);

	return 0;
}
