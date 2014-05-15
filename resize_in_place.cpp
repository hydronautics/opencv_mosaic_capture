#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

void resizeInPlace( cv::Mat inputArray, cv::Mat outputArray, cv::Point zoomCenter, double scale){
	cv::Size orig_size = inputArray.size();
	int x_delta = zoomCenter.x - orig_size.width/2;
	int y_delta = zoomCenter.y - orig_size.height/2;

	cv::Size ROI_size(orig_size.width - abs(x_delta), orig_size.height - abs(y_delta));

	int ROI_origin_x = (x_delta < 0) ? 0 : x_delta;
	int ROI_origin_y = (y_delta < 0) ? 0 : y_delta;

	cv::Point ROI_origin(ROI_origin_x,ROI_origin_y);
	cv::Rect ROI_rect(ROI_origin,ROI_size);
	cv::Mat zoom_center_ROI(inputArray,ROI_rect);

	cv::Mat temp(orig_size,inputArray.type(),CV_RGB(0,0,0));
	cv::Point temp_ROI_origin(abs(x_delta - ROI_origin_x), abs(y_delta - ROI_origin_y));
	cv::Rect temp_ROI_rect(temp_ROI_origin,ROI_size);
	cv::Mat temp_ROI(temp,temp_ROI_rect);
	zoom_center_ROI.copyTo(temp_ROI);

	outputArray = temp;
}
