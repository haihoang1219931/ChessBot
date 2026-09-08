//#include <opencv2/opencv.hpp>

//int main(int argc, char** argv) {
//    cv::Mat frame = cv::imread(argv[1]);
//    cv::Mat hsv, gray_mask;

//    // Convert BGR to HSV
//    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

//    // Strict thresholds to separate gray from black
//    // Increase the last value (50) if black pixels still leak in
//    cv::Scalar lower_gray(0, 0, 50);
//    cv::Scalar upper_gray(180, 40, 200);

//    // Filter the image
//    cv::inRange(hsv, lower_gray, upper_gray, gray_mask);

//    cv::imshow("Detected Gray Pieces", gray_mask);
//    cv::waitKey(0);
//    return 0;
//}
