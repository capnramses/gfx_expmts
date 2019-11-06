// gcc -o test main.c -std=c99 -lX11 -lm

// problems:
// * load system fonts?
// * tearing vsyc refresh rate
// * window close does not quit / server times out

// notes:
// * fixed font char width 6px height 9px

#include "apg_maths.h"
#include <X11/Xlib.h>
#include <stdio.h>
#define __USE_XOPEN_EXTENDED // what the heck
#include <unistd.h> // sleep
#include <stdbool.h>
#include <string.h>
#define __USE_POSIX199309 // sigh
#include <time.h>
#include <assert.h>

#define WIDTH 800
#define HEIGHT 600

float geom[] = {
	1.0, 1.0, 1.0,
	-1.0, 1.0, 1.0,
	-1.0, -1.0, 1.0,
	-1.0, -1.0, 1.0,
	1.0, -1.0, 1.0,
	1.0, 1.0, 1.0,
	
	1.0, 1.0, 1.0,
	1.0, 1.0, -1.0,
	1.0, -1.0, -1.0,
	1.0, -1.0, -1.0,
	1.0, -1.0, 1.0,
	1.0, 1.0, 1.0,
	
	-1.0, 1.0, 1.0,
	-1.0, 1.0, -1.0,
	-1.0, -1.0, -1.0,
	-1.0, -1.0, -1.0,
	-1.0, -1.0, 1.0,
	-1.0, 1.0, 1.0,
	
	1.0, 1.0, -1.0,
	-1.0, 1.0, -1.0,
	-1.0, -1.0, -1.0,
	-1.0, -1.0, -1.0,
	1.0, -1.0, -1.0,
	1.0, 1.0, -1.0,
	
	-1.0, 1.0, 1.0,
	1.0, 1.0, 1.0,
	1.0, 1.0, -1.0,
	1.0, 1.0, -1.0,
	-1.0, 1.0, -1.0,
	-1.0, 1.0, 1.0,
	
	-1.0, -1.0, -1.0,
	1.0, -1.0, -1.0,
	1.0, -1.0, 1.0,
	1.0, -1.0, 1.0,
	-1.0, -1.0, 1.0,
	-1.0, -1.0, -1.0,
};

Display* display;
GC graphics_context;
Window window;
XColor green_col, text_col, ruler_col, charcoal_col, lcharcoal_col, lineno_col;

typedef struct timespec timespec;
timespec prev_ts;
double delta_s;
double frame_accum;
long frame_count;
char fps_txt[32];

vec4 persp_div (vec4 v) {
	vec4 r;
	r.v[0] = v.v[0] / v.v[3]; // TODO WHYYYYY did i need 1.0 - ?
	r.v[1] = v.v[1] / v.v[3];
	r.v[2] = v.v[2] / v.v[3];
	r.v[3] = v.v[3];
	return r;
}

void draw_cube () {
	// HACK fake timer
	// draw triangle geom
	for (int i = 0; i < 12; i++) {
		float ax = geom[i * 9];
		float ay = geom[i * 9 + 1];
		float az = geom[i * 9 + 2];
		float bx = geom[i * 9 + 3];
		float by = geom[i * 9 + 4];
		float bz = geom[i * 9 + 5];
		float cx = geom[i * 9 + 6];
		float cy = geom[i * 9 + 7];
		float cz = geom[i * 9 + 8];
		
		vec4 va = vec4_from_4f (ax, ay, az, 1.0);
		vec4 vb = vec4_from_4f (bx, by, bz, 1.0);
		vec4 vc = vec4_from_4f (cx, cy, cz, 1.0);
		//mat4 S = scale_mat4 (vec3_from_3f (0.5, 0.5, 0.5));
		mat4 Rx = rot_x_deg_mat4 (45.0);
		static double ra = 45.0;
		ra += delta_s * 10.0;
		mat4 Ry = rot_y_deg_mat4 (ra);
		// TODO - think my maths lib has args backwards
		// correction: nope, seems right
		mat4 R = mult_mat4_mat4 (Ry, Rx);
		mat4 T = translate_mat4 (vec3_from_3f (-4,2,-1));
		//mat4 M = mult_mat4_mat4 (R, S);
		mat4 M = mult_mat4_mat4 (T, R);
		
		mat4 V = look_at (vec3_from_3f (0.0f, 0.0f, 10.0f),
			vec3_from_3f (0.0f, 0.0f, 0.0f), vec3_from_3f (0.0f, 1.0f, 0.0f));
		mat4 P = perspective (45.0f, (float)WIDTH / (float)HEIGHT, 0.01f, 100.0f);
		mat4 PV = mult_mat4_mat4 (P, V);
		mat4 PVM = mult_mat4_mat4 (PV, M);
		
		va = mult_mat4_vec4 (PVM, va);
		vb = mult_mat4_vec4 (PVM, vb);
		vc = mult_mat4_vec4 (PVM, vc);
		va = persp_div (va);
		vb = persp_div (vb);
		vc = persp_div (vc);
		
		// 0 to RES
		// TODO vectors/mats should just be float arrays
		va.v[0] = (va.v[0] + 1.0) * 0.5 * WIDTH;
		va.v[1] = HEIGHT - (va.v[1] + 1.0) * 0.5 * HEIGHT;
		vb.v[0] = (vb.v[0] + 1.0) * 0.5 * WIDTH;
		vb.v[1] = HEIGHT - (vb.v[1] + 1.0) * 0.5 * HEIGHT;
		vc.v[0] = (vc.v[0] + 1.0) * 0.5 * WIDTH;
		vc.v[1] = HEIGHT - (vc.v[1] + 1.0) * 0.5 * HEIGHT;
		
		int r = 0;
		int g = 0;
		int b = 0;
		if (i >= 6) {
			r = 255;
			b = 255;
		} else if (i >= 4) {
			b = 255;
		} else if (i >= 2) {
			g = 255;
		} else {
			r = 255;
		}
		
		// TODO triangle filling algorithm
		/*tri (Number ((va[0]).toFixed (0)), Number ((va[1]).toFixed (0)),
			Number ((vb[0]).toFixed (0)), Number ((vb[1]).toFixed (0)),
			Number ((vc[0]).toFixed (0)), Number ((vc[1]).toFixed (0)),
			r, g, b);*/
			
		/*XDrawLine (display, window, graphics_context, va.v[0], va.v[1],
			vb.v[0], vb.v[1]);
		XDrawLine (display, window, graphics_context, vb.v[0], vb.v[1],
			vc.v[0], vc.v[1]);
		XDrawLine (display, window, graphics_context, vc.v[0], vc.v[1],
			va.v[0], va.v[1]);*/
			
		XPoint pts[3];
		pts[0].x = va.v[0];
		pts[0].y = va.v[1];
		pts[1].x = vb.v[0];
		pts[1].y = vb.v[1];
		pts[2].x = vc.v[0];
		pts[2].y = vc.v[1];
			
		XFillPolygon (display, window, graphics_context,pts, 3,  Convex, CoordModeOrigin); 
	}
}

void draw_frame (Display* display, Window window, GC graphics_context) {
	if (!display) {
		fprintf (stderr, "ERROR: lost display\n");
		return;
	}
	int black_colour = BlackPixel (display, DefaultScreen (display));
	int white_colour = WhitePixel (display, DefaultScreen (display));
	
	XClearWindow (display, window);
	
	XSetForeground (display, graphics_context, lcharcoal_col.pixel);
	int x = 0, y = 0;
	int width = WIDTH;
	int height = HEIGHT;
	XFillRectangle (display, window, graphics_context, x, y, width, height);
	width = 36, height = HEIGHT;
	
	/*XSetForeground (display, graphics_context, lineno_col.pixel);
	x = 3, y = 12;
	for (int i = 0; i < 100; i++) {
		char tmp[16];
		sprintf (tmp, "%4i", i + 98);
		int len = strlen (tmp);
		XDrawString (display, window, graphics_context, x, y, tmp, len);
		y += 12;
	}
	
	XSetForeground (display, graphics_context, ruler_col.pixel);
	XDrawLine (display, window, graphics_context, 36 + 6 * 80, 10,
		36 + 6 * 80, 758);
	XDrawLine (display, window, graphics_context, 1000, 10, 1000, 758);
*/
	XSetForeground (display, graphics_context, text_col.pixel);
	x = 3 + width, y = 12 + 12;
	frame_accum += delta_s;
	if (frame_accum >= 0.5 && frame_count > 0) {
		double fps = (double)frame_count / frame_accum;
		sprintf (fps_txt, "FPS: %lf", fps);
		frame_accum = 0.0;
		frame_count = 0;
	}
	int len = strlen (fps_txt);
	XDrawString (display, window, graphics_context, x, y, fps_txt, len);
	//y += 12;
	//char txtb[] = "Here is a lengthy treatise on fonts.";
	//len = strlen (txtb);
	//XDrawString (display, window, graphics_context, x, y, txtb, len);
	
	draw_cube ();
}

void event_loop (Display* display, Window window, GC graphics_context) {
	while (true) {
		if (!display) {
			fprintf (stderr, "ERROR: lost display\n");
			return;
		}
		timespec curr_ts;
		assert (0 == clock_gettime(CLOCK_REALTIME, &curr_ts));
		long delta_ns = curr_ts.tv_nsec - prev_ts.tv_nsec;
		//printf ("delta ns = %ld\n", delta_ns);
		double delta_ms = (double)delta_ns / 1000000.0;
		if (delta_ms < 0.0) {
			delta_ms = 0.0;
		}
		delta_s = delta_ms / 1000.0;
		memcpy (&prev_ts, &curr_ts, sizeof (timespec));
		//XEvent event;
		//XNextEvent (display, &event);
		//if (event.type == Expose) { // was MapNotify
		//	;
		//}
		draw_frame (display, window, graphics_context);
		//XFlush (display); // dispatches the command queue
		XSync (display, true); // dispatches and waits
		frame_count++;
		usleep (100);
	}
}

int main () {
	printf ("X11 demo\n");

	int black_colour, white_colour;

	{ // create window and display combo
		printf ("init window...\n");
		display = XOpenDisplay (NULL);
		if (!display) {
			fprintf (stderr, "ERROR: opening display\n");
			return 1;
		}
		black_colour = BlackPixel (display, DefaultScreen (display));
		white_colour = WhitePixel (display, DefaultScreen (display));
		// colour map
		Colormap colour_map;
		char green[] = "#00FF00";
		char charcoal[] ="RGBi:0.01/0.02/0.01";
		char lcharcoal[] = "RGBi:0.02/0.03/0.02";//"#445544";
		char textc[] = "#DDEEDD";
		char linenoc[] = "#667766";
		char rulerc[] = "#556655";
		colour_map = DefaultColormap (display, 0);
		// add colours
		XParseColor (display, colour_map, green, &green_col);
		XAllocColor (display, colour_map, &green_col);
		XParseColor (display, colour_map, charcoal, &charcoal_col);
		XAllocColor (display, colour_map, &charcoal_col);
		XParseColor (display, colour_map, lcharcoal, &lcharcoal_col);
		XAllocColor (display, colour_map, &lcharcoal_col);
		XParseColor (display, colour_map, textc, &text_col);
		XAllocColor (display, colour_map, &text_col);
		XParseColor (display, colour_map, rulerc, &ruler_col);
		XAllocColor (display, colour_map, &ruler_col);
		XParseColor (display, colour_map, linenoc, &lineno_col);
		XAllocColor (display, colour_map, &lineno_col);
		// note: can use CopyFromParent instead of colour index
		// note: there is also XCreateWindow with more params
		// simplewindow also clears the window to background colour
		// 0,0 is request position, other 0 is border (not used)
		window = XCreateSimpleWindow (display, DefaultRootWindow (display), 0, 0,
			WIDTH, HEIGHT, 0, charcoal_col.pixel, charcoal_col.pixel);
		XStoreName (display, window, "Anton's X11 demo");
		// get MapNotify events etc.
		XSelectInput (display, window, ExposureMask); // was StructureNotifyMask
		// put window on screen command
		XMapWindow (display, window);
		// create graphics context
		graphics_context = XCreateGC (display, window, 0, 0);
	} // endinitwindow

	{ // draw
		// note: can also unload a font
		const char* font_name = "-*-Monospace-*-10-*";
		XFontStruct* font = XLoadQueryFont (display, font_name);
		if (!font) {
			fprintf (stderr, "ERROR: could not load font %s\n", font_name);
			font = XLoadQueryFont (display, "fixed");
			if (!font) {
				fprintf (stderr, "ERROR: default font fixed did not load\n");
				return 1;
			}
		}
		XSetFont (display, graphics_context, font->fid);
		sprintf (fps_txt, "FPS:");
		assert (0 == clock_gettime(CLOCK_REALTIME, &prev_ts));
//		printf ("start time is %llds and %ldns\n", (long long)prev_ts.tv_sec, (long)prev_ts.tv_nsec);
		event_loop (display, window, graphics_context);
	} // enddraw

	return 0;
}
