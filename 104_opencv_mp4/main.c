#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp> // don't need for this
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>  // OpenCV window I/O
#include <stdio.h>

/* Notes
- VideoCapture class does all video manip.
- This is a wrapper over ffmpeg.
- ffmpeg builds in with regular cv libs so don't need to explicitly link it.
*/

int main( int argc, char** argv  ) {
	const char* title_str = "Vid";

	if ( argc < 2 ) {	
		printf("usage: ./a.out VIDEOFILE\n");
		return 0;
	}
	printf("opening `%s`\n", argv[1] );

	cv::VideoCapture vid;
	vid.open( argv[1] );
	if (! vid.isOpened() ) {
		fprintf( stderr, "ERROR opening video `%s`\n", argv[1] );
		return 1;
	}
	cv::Size dims = cv::Size( (int)vid.get( cv::CAP_PROP_FRAME_WIDTH ),
		(int)vid.get( cv::CAP_PROP_FRAME_HEIGHT ) );

	cv::namedWindow( title_str, cv::WINDOW_AUTOSIZE );
	printf( "video frame res: w=%i, h=%i, n_frames=%i\n", (int)dims.width, (int)dims.height, (int)vid.get( cv::CAP_PROP_FRAME_COUNT ) );

	cv::Mat frame_img;

	vid >> frame_img;
	int frame_idx = 0;
	
	// NOTE(Anton) OpenCV uses BGR image data, and stb expects RGB.
	// so output channels are wrong here. PNG incidentally uses BGR natively.
	stbi_write_png("frame0000.png",dims.width,dims.height,3,frame_img.data, 3*dims.width);

	while ( !frame_img.empty() ) {
		cv::imshow( title_str, frame_img );


		char c = (char)cv::waitKey(24);
		vid >> frame_img;
		frame_idx++;
	}
	printf("processed %i frames.\n", frame_idx);

	return 0;
}
