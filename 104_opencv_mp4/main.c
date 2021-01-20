// https://docs.opencv.org/3.4/da/d0a/group__imgcodecs__c.html
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

	cv::namedWindow( "OpenCV Vid", cv::WINDOW_AUTOSIZE );
	printf( "video frame res: w=%i, h=%i, n_frames=%i\n", (int)dims.width, (int)dims.height, (int)vid.get( cv::CAP_PROP_FRAME_COUNT ) );

	cv::Mat frame_img;




	return 0;
}
