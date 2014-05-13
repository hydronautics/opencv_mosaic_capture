#include <opencv2/opencv.hpp>
#include <string>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <windows.h>
using namespace std;

const string winName = "camera capture";
const string outputWindow = "Panorama";
const string fileBegin = "Image";
const string fileEnd = ".jpg";
const char ESC = 27;
const char ENTER = 13;
const char SPACE = 32;
const int DEFAULT_CAMERA = 0;
const int EXTERNAL_CAMERA = 1;

const unsigned NUMBER_OF_CAPTURES = 3;

unsigned counter;

cv::Point left_point, right_point;

bool volatile waitingForMouseEvents = false;

struct CapturedImage { 

	cv::Mat img;
	int img_number;
	cv::Point left_point;
	cv::Point right_point;

	CapturedImage(const cv::Mat& i, unsigned n,cv::Point lp, cv::Point rp):
		img(i.clone()),
		img_number(n),
		left_point(lp),
		right_point(rp)
	{}

	CapturedImage(const CapturedImage& src):
		img(src.img.clone()),
		img_number(src.img_number),
		left_point(src.left_point),
		right_point(src.right_point)
	{}

};

vector<CapturedImage> caps;
// snapshot location is 60x100 centimeters
const double ROI_ratio = 3.0/5;

typedef enum Target {
	LEFT_TARGET,
	RIGHT_TARGET,
};

void drawTarget(cv::Mat& img, int x, int y, int radius, Target t){

		
		cv::Scalar target_color;

		switch (t){
		case LEFT_TARGET:
			target_color = CV_RGB(255,0,0);
			break;
		case RIGHT_TARGET:
			target_color = CV_RGB(0,0,255);
			break;
		default:
			throw runtime_error("Unexpected target type");
			return;
		}
		cv::circle(img,cv::Point(x, y),radius,target_color);
        cv::line(img, cv::Point(x-radius/2, y-radius/2), cv::Point(x+radius/2, y+radius/2),target_color);
        cv::line(img, cv::Point(x-radius/2, y+radius/2), cv::Point(x+radius/2, y-radius/2),target_color);
}

// обработчик событий от мышки
void myMouseCallback( int event, int x, int y, int, void* param )
{
	static int click_count; // default to 0
	if (waitingForMouseEvents){
		cv::Mat& img = *(cv::Mat*) param;
		string window_name;

		switch( event ){
		case CV_EVENT_LBUTTONDOWN:
			cout << x << " " << y << endl;
			switch (click_count)
			{
			case 0:
				drawTarget(img, x, y, 10,LEFT_TARGET);
				left_point = cv::Point(x,y);
				break;
			case 1:
				drawTarget(img, x, y, 10,RIGHT_TARGET);
				right_point = cv::Point(x,y);
				click_count = 0;
				waitingForMouseEvents = false;
				return;
			default:
				throw runtime_error("Unexpected click count");
				return;
			}			
			++click_count;	
			return;
		default:
			return;
		}
	}
}


void stitching(cv::Size cap_size, int cap_type){
	cout << "Started stitching..." << endl;

	// resulting image should be big enough to contain all images, shifted by Y-axis
	cv::Size result_size(cap_size.width*NUMBER_OF_CAPTURES,cap_size.height*2);
	
	cv::Mat panorama(result_size,cap_type);
	cv::Point pan_ROI_origin(0,cap_size.height/2);
	cv::Rect pan_ROI_rect(pan_ROI_origin,cap_size);
	cv::Mat pan_ROI(panorama,pan_ROI_rect);
	caps.at(0).img.copyTo(pan_ROI);
	cv::imshow(outputWindow,panorama);
	cv::waitKey();

	int right_edge_x_of_prev_img = cap_size.width;

	for (decltype(caps.size()) i = 1; i < caps.size(); ++i){
		pan_ROI_origin.x = right_edge_x_of_prev_img - caps.at(i).left_point.x - (cap_size.width - caps.at(i-1).right_point.x);
		pan_ROI_origin.y += caps.at(i-1).right_point.y - caps.at(i).left_point.y ;
		pan_ROI_rect = cv::Rect(pan_ROI_origin,cap_size);
		pan_ROI = cv::Mat(panorama,pan_ROI_rect);
		caps.at(i).img.copyTo(pan_ROI);
		cv::imshow(outputWindow,panorama);
		right_edge_x_of_prev_img = pan_ROI_origin.x + cap_size.width;
		cv::waitKey();
	}

	cout << "Done stitching..." << endl;
}

int main()
try {

	cout << "Select a camera for capturing" << endl;
	cout << DEFAULT_CAMERA << ": Default camera (usually a webcam laptop webcam)" << endl;
	cout << EXTERNAL_CAMERA << ": Additional camera (usually an externally connected device)" << endl;
	int cam_index = DEFAULT_CAMERA;
	cin >> cam_index;
	cout << "Selected device # " << cam_index << endl;
	cv::VideoCapture capture(cam_index);
	if (!capture.isOpened()){
		throw runtime_error("Can't access selected device.");
	}

	double width = capture.get(CV_CAP_PROP_FRAME_WIDTH);
	double height = capture.get(CV_CAP_PROP_FRAME_HEIGHT);

	cout << "width: " << width << endl;
	cout << "height: " << height << endl;

    int ROI_height = int(height);
    int ROI_width = int(ROI_ratio*width);
    int ROI_x_offset = int((width - ROI_width)/ 2.0); // ROI is in the middle
    int ROI_y_offset = 0;

	cv::Rect frame_ROI(ROI_x_offset,ROI_y_offset,ROI_width,ROI_height);

	cout << "Press Enter for frame capture and Escape for exit" << endl;

	int counter = 0;

	cv::namedWindow(winName);

	// size of the all captures, stored in the vector
	// Is determined during startup
	cv::Size cap_size;

	// type of all captures, stored in the vector
	int cap_type;

	for (;;){
		cv::Mat original_frame;
		capture >> original_frame;
		//if (!capture) throw runtime_error("Failed to query frame");
		
		// setting ROI for the original frame. All work will be then done on this ROI.
		cv::Mat frame(original_frame,frame_ROI);
        // Entire image won't be shown
        // Only ROI will be shown

		cap_size = frame.size();
		cap_type = frame.type();

		cv::Mat shown_frame(frame.clone());
		cv::Point text_origin(10,60);
		ostringstream ost;
		ost << counter;
		cv::putText(shown_frame,ost.str(),text_origin,cv::FONT_HERSHEY_PLAIN,5,CV_RGB(255,0,0),10);

		cv::imshow(winName,shown_frame);
        
        int c = cv::waitKey(1);

		// now we want our window to be topmost and have the focus
		// this cannot be done by opencv, we have to use WinAPI
		HWND winhandle = (HWND) cvGetWindowHandle(winName.c_str());
		SetForegroundWindow(winhandle);

		bool ready_for_stitching = false;
		string filename;

		switch (c) {
		case ESC:
				return 0;
		case ENTER:
				waitingForMouseEvents = true;
				cv::setMouseCallback(winName,myMouseCallback, (void *) &frame);
				while (waitingForMouseEvents){
					cv::imshow(winName,frame);
					cv::waitKey(1);
				}

				filename = fileBegin + ost.str() + fileEnd;
				cv::imwrite(filename,frame);

				if (caps.size() < NUMBER_OF_CAPTURES) caps.push_back(CapturedImage(frame,counter,left_point,right_point));
				else caps.at(counter) = CapturedImage(frame,counter,left_point,right_point);

				cout << "captured: " << filename << endl;
				for (vector<CapturedImage>::size_type i = 0; i < caps.size(); ++i){
					CapturedImage& this_img = caps.at(i);
					cv::imshow(ost.str(),this_img.img);
				}
				cv::waitKey(1);
				++counter;
				if (counter >= NUMBER_OF_CAPTURES) counter = 0;
				cout << "Size of vector of captures: " << caps.size() << endl;
				break;
		case SPACE:
			if (caps.size() == NUMBER_OF_CAPTURES) ready_for_stitching = true;
			if (caps.size() > NUMBER_OF_CAPTURES) throw runtime_error("Too many captures in the vector");
			if (caps.size() < NUMBER_OF_CAPTURES) cout << "Not enough images was captured" << endl;
			break;
		default:
			break;
			
		}
		if (ready_for_stitching) break;
	}
	cout << caps.size() << endl;
	stitching(cap_size,cap_type);
	return 0;
}
catch (exception& e){
	cerr << e.what() << endl;
	cerr << "Press Enter to quit" << endl;
	cin.ignore(1);
	return 1;
}
