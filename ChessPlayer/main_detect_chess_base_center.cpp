//#include <opencv2/opencv.hpp>
//#include <iostream>
//#include <vector>
//#include <cmath>

//// Global structure to pass data into the mouse callback
//struct WarpCallbackData {
//    cv::Mat displayImage;
//    std::vector<cv::Point2f> clickedPoints;
//};

//// Mouse callback to record exactly 4 clicks for the warp step
//void onMouseClick(int event, int x, int y, int flags, void* userdata) {
//    WarpCallbackData* data = reinterpret_cast<WarpCallbackData*>(userdata);
//    if (event == cv::EVENT_LBUTTONDOWN) {
//        if (data->clickedPoints.size() < 4) {
//            cv::Point2f pt(static_cast<float>(x), static_cast<float>(y));
//            data->clickedPoints.push_back(pt);

//            cv::circle(data->displayImage, pt, 5, cv::Scalar(0, 0, 255), -1);
//            cv::putText(data->displayImage, std::to_string(data->clickedPoints.size()),
//                        cv::Point(x + 10, y + 10), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 255), 2);
//            cv::imshow("Click 4 Corners", data->displayImage);

//            if (data->clickedPoints.size() == 4) {
//                std::cout << "\n[INFO] 4 corners selected! Press any key to continue..." << std::endl;
//            }
//        }
//    }
//}

//// =========================================================================
//// UPDATED FUNCTION: Uses Adaptive Thresholding Natively
//// =========================================================================
//void traditionalThinning(const cv::Mat& src, cv::Mat& dst) {
//    // If input is already color or has channel variance, convert safely to single channel
//    cv::Mat gray;
//    if (src.channels() == 3) {
//        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
//    } else {
//        src.copyTo(gray);
//    }

//    // Smooth out high-frequency sensor noise before computing local blocks
//    cv::Mat blurred;
//    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);

//    // Apply Adaptive Thresholding natively inside the function
//    cv::Mat img;
//    int blockSize = 11; // Must be odd
//    double C = 2.0;
//    cv::adaptiveThreshold(
//        blurred,
//        img,
//        255,
//        cv::ADAPTIVE_THRESH_GAUSSIAN_C,
//        cv::THRESH_BINARY_INV, // Keeps edges white (255) on black background (0)
//        blockSize,
//        C
//    );

//    // Run structural cleanup to minimize pixel fragmentation
//    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
//    cv::morphologyEx(img, img, cv::MORPH_CLOSE, kernel);

//    // Morphological skeletonization loop
//    dst = cv::Mat::zeros(img.size(), CV_8UC1);
//    cv::Mat skel(img.size(), CV_8UC1, cv::Scalar(0));
//    cv::Mat temp;
//    cv::Mat eroded;
//    cv::Mat element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));

//    bool done;
//    do {
//        cv::erode(img, eroded, element);
//        cv::dilate(eroded, temp, element);
//        cv::subtract(img, temp, temp);
//        cv::bitwise_or(skel, temp, skel);
//        eroded.copyTo(img);

//        done = (cv::countNonZero(img) == 0);
//    } while (!done);

//    skel.copyTo(dst);
//}

//// Function to find cells that exceed the white pixel count threshold
//std::vector<cv::Point> findCellsExceedingThreshold(const cv::Mat& perfectEdges, int pixelThreshold) {
//    std::vector<cv::Point> activeCells;

//    int cellWidth = 64;
//    int cellHeight = 64;
//    int cols = 14;
//    int rows = 8;
//    int radius = 28;

//    for (int r = 0; r < rows; ++r) {
//        for (int c = 0; c < cols; ++c) {
//            int centerX = (c * cellWidth) + (cellWidth / 2);
//            int centerY = (r * cellHeight) + (cellHeight / 2);
//            int whitePixelCount = 0;

//            for (int y = r * cellHeight; y < (r + 1) * cellHeight; ++y) {
//                for (int x = c * cellWidth; x < (c + 1) * cellWidth; ++x) {
//                    if (perfectEdges.at<uchar>(y, x) == 255) {
//                        double dx = x - centerX;
//                        double dy = y - centerY;
//                        double distance = std::sqrt(dx * dx + dy * dy);

//                        if (distance <= radius) {
//                            whitePixelCount++;
//                        }
//                    }
//                }
//            }

//            if (whitePixelCount > pixelThreshold) {
//                activeCells.push_back(cv::Point(c, r)); // Store as (column, row)
//            }
//        }
//    }

//    return activeCells;
//}

//int main(int argc, char** argv) {
//    if (argc < 2) {
//        std::cout << "Usage: " << argv[0] << " <Path_to_Image>" << std::endl;
//        return -1;
//    }

//    // 1. Load source image
//    cv::Mat srcImage = cv::imread(argv[1]);
//    if (srcImage.empty()) {
//        std::cout << "Error: Could not load image!" << std::endl;
//        return -1;
//    }

//    // 2. Define target dimensions for warping (896x512)
//    float targetWidth = 896.0f;
//    float targetHeight = 512.0f;
//    cv::resize(srcImage, srcImage, cv::Size(targetWidth, targetHeight));

//    // 3. Setup Interactive Corner Selection Window
//    WarpCallbackData mouseData;
//    mouseData.displayImage = srcImage.clone();

//    cv::namedWindow("Click 4 Corners");
//    cv::setMouseCallback("Click 4 Corners", onMouseClick, &mouseData);

//    std::cout << "Instructions: Click the 4 corners of your target region." << std::endl;
//    std::cout << "Sequence: Top-Left -> Top-Right -> Bottom-Right -> Bottom-Left (Clockwise)" << std::endl;

//    cv::imshow("Click 4 Corners", mouseData.displayImage);
//    cv::waitKey(0);

//    if (mouseData.clickedPoints.size() < 4) {
//        std::cout << "Error: You did not select 4 points!" << std::endl;
//        return -1;
//    }

//    std::vector<cv::Point2f> targetPoints2D = {
//        cv::Point2f(0.0f, 0.0f),
//        cv::Point2f(targetWidth, 0.0f),
//        cv::Point2f(targetWidth, targetHeight),
//        cv::Point2f(0.0f, targetHeight)
//    };

//    // Calculate perspective matrix and warp the image
//    cv::Mat perspectiveMatrix = cv::getPerspectiveTransform(mouseData.clickedPoints, targetPoints2D);
//    cv::Mat warpedColor;
//    cv::warpPerspective(srcImage, warpedColor, perspectiveMatrix, cv::Size(targetWidth, targetHeight));
//    cv::destroyWindow("Click 4 Corners");

//    // =========================================================================
//    // 4. Pass the Warped Image Directly to Thinning
//    // The adaptive thresholding code is now fully encapsulated inside the function.
//    // =========================================================================
//    cv::Mat perfectEdges;
//    traditionalThinning(warpedColor, perfectEdges);

//    // 5. Execute Exceeded Threshold Detection Function
//    int myThresholdValue = 150;
//    std::vector<cv::Point> matchedCells = findCellsExceedingThreshold(perfectEdges, myThresholdValue);

//    // 6. Generate display overlay highlighting matches
//    cv::Mat outputDisplay = warpedColor.clone();
//    int cellWidth = 64;
//    int cellHeight = 64;
//    int radius = 28;

//    // Draw normal grid circles first in green
//    for (int r = 0; r < 8; ++r) {
//        for (int c = 0; c < 14; ++c) {
//            int cx = (c * cellWidth) + (cellWidth / 2);
//            int cy = (r * cellHeight) + (cellHeight / 2);
//            cv::circle(outputDisplay, cv::Point(cx, cy), radius, cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
//        }
//    }

//    // Overlay matched threshold cells in bold RED to visually confirm the list contents
//    std::cout << "\n--- Cells with White Pixels > " << myThresholdValue << " ---" << std::endl;
//    for (const auto& cell : matchedCells) {
//        int col = cell.x;
//        int row = cell.y;
//        std::cout << "Exceeded match at: Row " << row << ", Col " << col << std::endl;

//        int cx = (col * cellWidth) + (cellWidth / 2);
//        int cy = (row * cellHeight) + (cellHeight / 2);

//        cv::circle(outputDisplay, cv::Point(cx, cy), radius, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
//    }

//    // 7. Render Final Output Windows
//    cv::namedWindow("Threshold Highlight Result", cv::WINDOW_AUTOSIZE);
//    cv::imshow("Threshold Highlight Result", outputDisplay);
//    cv::imshow("Thinned Adaptive Edge Reference Map", perfectEdges);

//    cv::waitKey(0);
//    return 0;
//}
