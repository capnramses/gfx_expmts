// tool for converting incorrectly exported normal maps from unreal
// author: anton gerdelan 30 nov 2015 <gerdela@scss.tcd.ie>
// written in C99

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char** argv) {
	if (argc < 3) {
		printf ("usage: ./channel_swap INPUT_FILE.* OUTPUT_FILE.png\n");
		return 0;
	}
	printf ("reading file %s\n", argv[1]);
	int x = 0, y = 0, n = 0;
	unsigned char* input_buff = stbi_load (argv[1], &x, &y, &n, 3);
	printf ("image %ix%i %i chans\n", x, y, n);
	unsigned char* output_buff = (unsigned char*)malloc (x * y * n);
	int pix_count = x * y;
	for (int i = 0; i < pix_count; i++) {
		output_buff[i * n + 2] = input_buff[i * n]; // red -> blue
		output_buff[i * n + 1] = input_buff[i * n + 1]; // green -> red
		output_buff[i * n] = input_buff[i * n + 2]; // blue -> green
	}
	printf ("writing PNG file %s\n", argv[2]);
	stbi_write_png (argv[2], x, y, n, output_buff, 3 * x);
	return 0;
}