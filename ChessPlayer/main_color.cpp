//#include <opencv2/opencv.hpp>
//#include <iostream>
//#include <vector>

//// 1. Struct to hold a single color paired with its specific tolerances
//struct TargetColor {
//    cv::Scalar hsvValue;
//    int hTolerance;
//    int sTolerance;
//    int vTolerance;
//};

//// 2. Master configuration struct to hold application state
//struct ColorFilterConfig {
//    cv::Mat imgOriginal;
//    cv::Mat imgHSV;
//    cv::Mat imgResult;

//    // Dynamic array tracking both colors and their paired individual tolerances
//    std::vector<TargetColor> targetColors;

//    // Trackbar variables linked directly to the UI (tracks the active color layer)
//    int currentHTolerance = 10;
//    int currentSTolerance = 40;
//    int currentVTolerance = 40;

//    const int MAX_H = 180;
//    const int MAX_SV = 255;
//};

//// Function declarations
//void updateFilter(ColorFilterConfig* config);
//void onMouseClick(int event, int x, int y, int flags, void* userdata);
//void onTrackbarChange(int, void* userdata);

//int main(int argc, char** argv) {
//    if (argc < 2) {
//        std::cout << "Usage: " << argv[0] << " <Path_to_Image>" << std::endl;
//        return -1;
//    }

//    cv::Mat imageOrigin = cv::imread(argv[1]);
//    if (imageOrigin.empty()) {
//        std::cerr << "Error: Could not open or find the image at: " << argv[1] << std::endl;
//        return -1;
//    }
//    ColorFilterConfig config;
//    cv::resize(imageOrigin,config.imgOriginal,cv::Size(240,240));
//    cv::cvtColor(config.imgOriginal, config.imgHSV, cv::COLOR_BGR2HSV);
//    config.imgResult = cv::Mat::zeros(config.imgOriginal.size(), config.imgOriginal.type());

//    cv::namedWindow("Original Image", cv::WINDOW_AUTOSIZE);
//    cv::namedWindow("Filtered Result", cv::WINDOW_AUTOSIZE);
//    cv::namedWindow("Controls", cv::WINDOW_AUTOSIZE);

//    cv::setMouseCallback("Original Image", onMouseClick, &config);

//    // Trackbars bind to current variables; changes propagate to the active target color
//    cv::createTrackbar("Active Hue Range", "Controls", &config.currentHTolerance, config.MAX_H, onTrackbarChange, &config);
//    cv::createTrackbar("Active Sat Range", "Controls", &config.currentSTolerance, config.MAX_SV, onTrackbarChange, &config);
//    cv::createTrackbar("Active Val Range", "Controls", &config.currentVTolerance, config.MAX_SV, onTrackbarChange, &config);

//    cv::imshow("Original Image", config.imgOriginal);
//    cv::imshow("Filtered Result", config.imgResult);

//    std::cout << "Instructions:" << std::endl;
//    std::cout << "- Click on 'Original Image' to add a new color target." << std::endl;
//    std::cout << "- Move trackbars to tweak tolerances *only* for the last color clicked." << std::endl;
//    std::cout << "- Press 'U' to undo the last clicked color layer." << std::endl;
//    std::cout << "- Press 'C' to clear all layers entirely." << std::endl;
//    std::cout << "- Press 'ESC' or 'Q' to quit." << std::endl;

//    while (true) {
//        char key = (char)cv::waitKey(10);
//        if (key == 27 || key == 'q' || key == 'Q') {
//            break;
//        }
//        // 'C' to clear everything
//        if (key == 'c' || key == 'C') {
//            config.targetColors.clear();
//            config.imgResult = cv::Mat::zeros(config.imgOriginal.size(), config.imgOriginal.type());
//            cv::imshow("Filtered Result", config.imgResult);
//            std::cout << "Cleared all color layers." << std::endl;
//        }
//        // 'U' to undo the last color layer added
//        if (key == 'u' || key == 'U') {
//            if (!config.targetColors.empty()) {
//                config.targetColors.pop_back();
//                std::cout << "Removed last layer. Remaining color layers: " << config.targetColors.size() << std::endl;

//                // If a previous layer exists, restore its specific sliders to the UI view
//                if (!config.targetColors.empty()) {
//                    TargetColor lastRemaining = config.targetColors.back();
//                    config.currentHTolerance = lastRemaining.hTolerance;
//                    config.currentSTolerance = lastRemaining.sTolerance;
//                    config.currentVTolerance = lastRemaining.vTolerance;

//                    cv::setTrackbarPos("Active Hue Range", "Controls", config.currentHTolerance);
//                    cv::setTrackbarPos("Active Sat Range", "Controls", config.currentSTolerance);
//                    cv::setTrackbarPos("Active Val Range", "Controls", config.currentVTolerance);
//                }
//                updateFilter(&config);
//            } else {
//                std::cout << "No color layers left to remove!" << std::endl;
//            }
//        }
//    }

//    cv::destroyAllWindows();
//    return 0;
//}
//int countMatchPixelColor(const cv::Mat& imageHSV, const std::vector<TargetColor>& targetColors, int maxH, int maxSV) {
//    cv::Mat finalMask = cv::Mat::zeros(imageHSV.size(), CV_8UC1);
//    // Loop through every standalone paired color configuration context block
//    for (const auto& target : targetColors) {
//        int lowerH = std::max(0, (int)target.hsvValue[0] - target.hTolerance);
//        int upperH = std::min(maxH, (int)target.hsvValue[0] + target.hTolerance);

//        int lowerS = std::max(0, (int)target.hsvValue[1] - target.sTolerance);
//        int upperS = std::min(maxSV, (int)target.hsvValue[1] + target.sTolerance);

//        int lowerV = std::max(0, (int)target.hsvValue[2] - target.vTolerance);
//        int upperV = std::min(maxSV, (int)target.hsvValue[2] + target.vTolerance);

//        cv::Scalar lowerBound(lowerH, lowerS, lowerV);
//        cv::Scalar upperBound(upperH, upperS, upperV);

//        cv::Mat singleMask;
//        cv::inRange(imageHSV, lowerBound, upperBound, singleMask);

//        // Merge mask arrays using logical bitwise operations
//        cv::bitwise_or(finalMask, singleMask, finalMask);
//    }
//    return cv::countNonZero(finalMask);
//}
//#define DEBUG_FILTER_COLOR
//// Process and isolate independent color masks using their specific individual tolerances
//void updateFilter(ColorFilterConfig* config) {
//    if (config->targetColors.empty()) {
//        config->imgResult = cv::Mat::zeros(config->imgOriginal.size(), config->imgOriginal.type());
//        cv::imshow("Filtered Result", config->imgResult);
//        return;
//    }
//#ifdef DEBUG_FILTER_COLOR
//    cv::Mat finalMask = cv::Mat::zeros(config->imgOriginal.size(), CV_8UC1);

//    // Loop through every standalone paired color configuration context block
//    for (const auto& target : config->targetColors) {
//        int lowerH = std::max(0, (int)target.hsvValue[0] - target.hTolerance);
//        int upperH = std::min(config->MAX_H, (int)target.hsvValue[0] + target.hTolerance);

//        int lowerS = std::max(0, (int)target.hsvValue[1] - target.sTolerance);
//        int upperS = std::min(config->MAX_SV, (int)target.hsvValue[1] + target.sTolerance);

//        int lowerV = std::max(0, (int)target.hsvValue[2] - target.vTolerance);
//        int upperV = std::min(config->MAX_SV, (int)target.hsvValue[2] + target.vTolerance);

//        cv::Scalar lowerBound(lowerH, lowerS, lowerV);
//        cv::Scalar upperBound(upperH, upperS, upperV);

//        cv::Mat singleMask;
//        cv::inRange(config->imgHSV, lowerBound, upperBound, singleMask);

//        // Merge mask arrays using logical bitwise operations
//        cv::bitwise_or(finalMask, singleMask, finalMask);
//        cv::imshow("singleMask",singleMask);
//        cv::imshow("finalMask",finalMask);
//    }

//    config->imgResult = cv::Mat::zeros(config->imgOriginal.size(), config->imgOriginal.type());
//    config->imgOriginal.copyTo(config->imgResult, finalMask);

//    cv::imshow("Filtered Result", config->imgResult);
//#else
//    // Gray
//    std::vector<TargetColor> configGray;
//    configGray.push_back({cv::Scalar(20, 8, 91),50,40,40});
//    configGray.push_back({cv::Scalar(0, 0, 156),50,40,40});

//    // Gold
//    std::vector<TargetColor> configGold;
//    configGold.push_back({cv::Scalar(18, 190, 185),50,40,40});
//    configGold.push_back({cv::Scalar(21, 98, 243),50,40,40});
//    cv::Size originImageSize = config->imgHSV.size();
//    cv::Mat bottomHSV = config->imgHSV(cv::Rect(0,originImageSize.height/2,
//                                                originImageSize.width,originImageSize.height/2));
//    int grayPixels = countMatchPixelColor(bottomHSV,configGray,180,255);
//    int goldPixels = countMatchPixelColor(bottomHSV,configGold,180,255);
//    std::string pieceColor;

//    if(grayPixels > 3 * goldPixels) pieceColor = "black";
//    else if(goldPixels > 3 * grayPixels) pieceColor = "white";
//    else pieceColor = "unknown";
//    printf("Gray(%d/%d)Gold => [%s]\r\n",
//           grayPixels,goldPixels,pieceColor.c_str());
//#endif
//}

//// Mouse parser capturing unique values and embedding them into standalone structs
//void onMouseClick(int event, int x, int y, int flags, void* userdata) {
//    if (event == cv::EVENT_LBUTTONDOWN) {
//        ColorFilterConfig* config = static_cast<ColorFilterConfig*>(userdata);

//        cv::Vec3b hsvPixel = config->imgHSV.at<cv::Vec3b>(cv::Point(x, y));
//        cv::Scalar newColorValue(hsvPixel[0], hsvPixel[1], hsvPixel[2]);

//        // Construct a unique color package matching current slider adjustments
//        TargetColor newTarget;
//        newTarget.hsvValue = newColorValue;
//        newTarget.hTolerance = config->currentHTolerance;
//        newTarget.sTolerance = config->currentSTolerance;
//        newTarget.vTolerance = config->currentVTolerance;

//        config->targetColors.push_back(newTarget);

//        std::cout << "\nAdded Layer #" << config->targetColors.size() << std::endl;
//        std::cout << "-> HSV Base: [" << (int)newColorValue[0] << ", " << (int)newColorValue[1] << ", " << (int)newColorValue[2] << "]" << std::endl;
//        std::cout << "-> Frozen Tolerances: [H=" << newTarget.hTolerance << ", S=" << newTarget.sTolerance << ", V=" << newTarget.vTolerance << "]" << std::endl;

//        updateFilter(config);
//    }
//}

//// Slider routine updating the active color layer configuration profile
//void onTrackbarChange(int, void* userdata) {
//    ColorFilterConfig* config = static_cast<ColorFilterConfig*>(userdata);

//    // If a layer is active, update its isolated settings using the adjusted sliders
//    if (!config->targetColors.empty()) {
//        TargetColor& activeTarget = config->targetColors.back();
//        activeTarget.hTolerance = config->currentHTolerance;
//        activeTarget.sTolerance = config->currentSTolerance;
//        activeTarget.vTolerance = config->currentVTolerance;
//    }

//    updateFilter(config);
//}

///**
// * Gray
// * Hue: 83, Sat: 100, Val: 40
// * Gray HSV: [40, 17, 135]
// * Gray HSV: [15, 65, 94]
// * Gray HSV: [86, 17, 104]
// *
// * Gray HSV: [30, 8, 194] Hue: 92, Sat: 47, Val: 127
// *
// * Hue: 6, Sat: 24, Val: 254
// * Gray HSV: [100, 47, 211]
// * Yellow HSV: [20, 157, 210]
// * Yellow HSV: [19, 178, 191]
// *
// * 92, 85, 67
// * Yellow HSV: [20, 155, 188]
//*/
