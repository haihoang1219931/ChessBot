//#include <opencv2/opencv.hpp>
//#include <iostream>

//int main() {
//    std::cout << "OpenCV Version: " << CV_VERSION << std::endl;
//    // Open the default USB webcam (/dev/video0) using the V4L2 backend
//    cv::VideoCapture cap(0, cv::CAP_V4L);

//    if (!cap.isOpened()) {
//        std::cerr << "Error: Could not open the webcam." << std::endl;
//        return -1;
//    }

//    // Force the camera to use MJPEG compression to save USB bandwidth
//    cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));

//    // Set your target resolution
//    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
//    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);

//    // Set your target frame rate
//    cap.set(cv::CAP_PROP_FPS, 30);

//    // Verify what the hardware actually set (some cameras fallback if unsupported)
//    double actual_width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
//    double actual_height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
//    double actual_fps = cap.get(cv::CAP_PROP_FPS);

//    std::cout << "Capture initialized: " << actual_width << "x" << actual_height
//              << " @ " << actual_fps << " FPS" << std::endl;

//    cv::Mat frame;
//    cv::namedWindow("Webcam Stream", cv::WINDOW_GUI_NORMAL);

//    while (true) {
//        // Grab a frame from the webcam
//        cap >> frame;

//        if (frame.empty()) {
//            std::cerr << "Error: Blank frame grabbed." << std::endl;
//            break;
//        }

//        // Display the frame
//        cv::imshow("Webcam Stream", frame);

//        // Break loop if 'q' or 'ESC' (27) is pressed
//        char key = (char)cv::waitKey(1);
//        if (key == 'q' || key == 27) {
//            break;
//        }
//    }

//    // Release hardware resources
//    cap.release();
//    cv::destroyAllWindows();
//    return 0;
//}
