// clang -o test main.c -std=c99 -lX11

// problems:
// * load system fonts?
// * tearing vsyc refresh rate
// * window close does not quit / server times out

// notes:
// * fixed font char width 6px height 9px

#include <X11/Xlib.h>
#include <stdio.h>
#include <unistd.h> // sleep
#include <stdbool.h>
#include <string.h>

XColor green_col, text_col, ruler_col, charcoal_col, lcharcoal_col, lineno_col;

void draw_frame (Display* display, Window window, GC graphics_context) {
	if (!display) {
		fprintf (stderr, "ERROR: lost display\n");
		return;
	}
	int black_colour = BlackPixel (display, DefaultScreen (display));
	int white_colour = WhitePixel (display, DefaultScreen (display));
	
	XClearWindow (display, window);
	
	XSetForeground (display, graphics_context, black_colour);
	int x = 0, y = 0;
	int width = 36, height = 1024;
	XFillRectangle (display, window, graphics_context, x, y, width, height);
	
	XSetForeground (display, graphics_context, lcharcoal_col.pixel);
	x = 36 + 6 * 80;
	width = 1024 - 36 + 6 * 80;
	height = 768;
	XFillRectangle (display, window, graphics_context, x, y, width, height);
	width = 36, height = 1024;
	
	XSetForeground (display, graphics_context, lineno_col.pixel);
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

	XSetForeground (display, graphics_context, text_col.pixel);
	x = 3 + width, y = 12 + 12;
	char txt[] = "Hello X11!";
	int len = strlen (txt);
	XDrawString (display, window, graphics_context, x, y, txt, len);
	y += 12;
	char txtb[] = "Here is a lengthy treatise on fonts.";
	len = strlen (txtb);
	XDrawString (display, window, graphics_context, x, y, txtb, len);
	
	//XFlush (display); // dispatches the command queue
}

void event_loop (Display* display, Window window, GC graphics_context) {
	while (true) {
		if (!display) {
			fprintf (stderr, "ERROR: lost display\n");
			return;
		}
	
		XEvent event;
		XNextEvent (display, &event);
		if (event.type == Expose) { // was MapNotify
			draw_frame (display, window, graphics_context);
		}
	}
}

int main () {
	printf ("X11 demo\n");

	Display* display = NULL;
	GC graphics_context;
	Window window;
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
			1024, 768, 0, charcoal_col.pixel, charcoal_col.pixel);
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

		event_loop (display, window, graphics_context);
	} // enddraw

	return 0;
}
