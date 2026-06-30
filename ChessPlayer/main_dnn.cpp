//#include <opencv2/opencv.hpp>
//#include <iostream>
//#include <chrono>
//#include <iomanip>
//#include <sstream>
//#include <algorithm>

//// Trackbar callback placeholder
//void on_trackbar(int, void*) {}

//// Function to generate a timestamped filename
//std::string getTimestampFilename() {
//    auto now = std::chrono::system_clock::now();
//    auto in_time_t = std::chrono::system_clock::to_time_t(now);

//    std::stringstream ss;
//    // Format: cap_YYYYMMDD_HHMMSS.jpg
//    ss << "cap_" << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S") << ".jpg";
//    return ss.str();
//}

//int main() {
//    // Open camera using platform-optimized backends
//#if defined(__linux__)
//    cv::VideoCapture cap(1, cv::CAP_V4L2);
//#elif defined(_WIN32)
//    cv::VideoCapture cap(0, cv::CAP_DSHOW);
//#else
//    cv::VideoCapture cap(0);
//#endif

//    if (!cap.isOpened()) {
//        std::cerr << "Error: Could not open camera." << std::endl;
//        return -1;
//    }

//    std::string winName = "Camera Controls";
//    cv::namedWindow(winName, cv::WINDOW_AUTOSIZE);

//    // --- INITIAL VALUE SETTINGS ---
//    // OpenCV normalized properties typically expect values mapped from 0 to 100 or 0 to 255.
//    int auto_exposure = 1;
//    int exposure_val  = 50;
//    int auto_wb       = 1;
//    int wb_temp       = 5000;
//    int brightness    = 50;  // Default halfway midtone
//    int contrast      = 50;  // Default halfway contrast
//    int hue           = 50;  // Default halfway hue tint

//    // --- CREATE ALL TRACKBARS ---
//    cv::createTrackbar("Auto Exposure (0=Off, 1=On)", winName, &auto_exposure, 1, on_trackbar);
//    cv::createTrackbar("Exposure Value", winName, &exposure_val, 100, on_trackbar);
//    cv::createTrackbar("Auto White Balance", winName, &auto_wb, 1, on_trackbar);
//    cv::createTrackbar("WB Temp (Kelvin/Steps)", winName, &wb_temp, 10000, on_trackbar);

//    // New hardware trackbars
//    cv::createTrackbar("Brightness", winName, &brightness, 255, on_trackbar);
//    cv::createTrackbar("Contrast", winName, &contrast, 255, on_trackbar);
//    cv::createTrackbar("Hue", winName, &hue, 128, on_trackbar);

//    cv::Mat frame;
//    std::cout << "==========================================" << std::endl;
//    std::cout << " Controls:" << std::endl;
//    std::cout << "  Press 'S' or 's' to Save a timestamped JPG" << std::endl;
//    std::cout << "  Press 'ESC' to Exit" << std::endl;
//    std::cout << "==========================================" << std::endl;

//    while (true) {
//        cap >> frame;
//        if (frame.empty()) {
//            std::cerr << "Blank frame grabbed." << std::endl;
//            break;
//        }

//        // --- 1. EXPOSURE CONTROLS ---
//        if (auto_exposure == 0) {
//#if defined(__linux__)
//            cap.set(cv::CAP_PROP_AUTO_EXPOSURE, 1);
//            cap.set(cv::CAP_PROP_EXPOSURE, exposure_val * 10);
//#else
//            cap.set(cv::CAP_PROP_AUTO_EXPOSURE, 0);
//            cap.set(cv::CAP_PROP_EXPOSURE, -1 * (exposure_val / 8));
//#endif
//        } else {
//#if defined(__linux__)
//            cap.set(cv::CAP_PROP_AUTO_EXPOSURE, 3);
//#else
//            cap.set(cv::CAP_PROP_AUTO_EXPOSURE, 1);
//#endif
//        }

//        // --- 2. WHITE BALANCE CONTROLS ---
//        if (auto_wb == 0) {
//            cap.set(cv::CAP_PROP_AUTO_WB, 0);
//            cap.set(cv::CAP_PROP_WB_TEMPERATURE, std::max(2000, wb_temp));
//        } else {
//            cap.set(cv::CAP_PROP_AUTO_WB, 1);
//        }

//        // --- 3. BRIGHTNESS, CONTRAST, & HUE CONTROLS ---
//        // Scale slider values (0-100) to typical driver ranges (maps nicely via fractions)
//        cap.set(cv::CAP_PROP_BRIGHTNESS, brightness / 100.0);
//        cap.set(cv::CAP_PROP_CONTRAST, contrast / 100.0);
//        cap.set(cv::CAP_PROP_HUE, hue / 100.0);

//        // Show live feed frame
//        cv::imshow(winName, frame);

//        // Check for keyboard triggers
//        int key = cv::waitKey(30);

//        if (key == 27) { // ESC key to exit
//            break;
//        }
//        else if (key == 's' || key == 'S') { // S key to save frame
//            std::string filename = getTimestampFilename();

//            // Save the frame with 95% JPEG quality
//            bool saved = cv::imwrite(filename, frame, {cv::IMWRITE_JPEG_QUALITY, 95});

//            if (saved) {
//                std::cout << "[SUCCESS] Saved frame as: " << filename << std::endl;
//            } else {
//                std::cerr << "[ERROR] Failed to save image file." << std::endl;
//            }
//        }
//    }

//    cap.release();
//    cv::destroyAllWindows();
//    return 0;
//}
