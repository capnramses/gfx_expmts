// https://docs.opencv.org/3.4/da/d0a/group__imgcodecs__c.html
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <stdio.h>

int main( int argc, char** argv  ) {
	if ( argc < 2 ) {	
		printf("usage: ./a.out IMAGEFILE\n");
		return 0;
	}
	printf("opening `%s`\n", argv[1] );
	cv::Mat img = imread( argv[1], cv::IMREAD_COLOR );
	if ( img.empty() ) {
		fprintf( stderr, "ERROR opening `%s`\n", argv[1] );
		return 1;
	}
	cv::imshow( "Display Window", img );
	int k = cv::waitKey( 0 ); // wait for keystroke in window
	if ( k == 's' ) {
		cv::imwrite( "output.png", img );
	}
	return 0;
}
