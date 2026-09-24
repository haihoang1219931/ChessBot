//#include <opencv2/opencv.hpp>
//#include <iostream>
//#include <vector>
//#include <cmath>

//// Global structures to share data across trackbar callbacks
//struct AppState {
//    cv::Mat srcImage;
//    cv::Mat cameraMatrix;
//    cv::Mat distCoeffs;

//    // PnP Pose output calculated from user clicks
//    cv::Mat baseRvec;
//    cv::Mat baseTvec;
//    bool poseValid = false;

//    // Trackbar values
//    int heightSlider = 150;  // 150 cm = 1.5 meters
//    int yawSlider = 0;       // 0 to 360 degrees
//    int pitchSlider = 0;     // 0 to 360 degrees
//    int rollSlider = 0;      // 0 to 360 degrees
//};

//struct MouseCallbackData {
//    cv::Mat displayImage;
//    std::vector<cv::Point2f> clickedPoints;
//};

//// Global AppState reference for the trackbar loop
//AppState gState;

//// Forward declaration of the render function
//void draw3DScene();

//// Trackbar callback wrapper
//void onTrackbarChange(int, void*) {
//    draw3DScene();
//}

//void onMouseClick(int event, int x, int y, int flags, void* userdata) {
//    MouseCallbackData* data = reinterpret_cast<MouseCallbackData*>(userdata);
//    if (event == cv::EVENT_LBUTTONDOWN) {
//        if (data->clickedPoints.size() < 4) {
//            cv::Point2f pt(static_cast<float>(x), static_cast<float>(y));
//            data->clickedPoints.push_back(pt);

//            cv::circle(data->displayImage, pt, 5, cv::Scalar(0, 0, 255), -1);
//            cv::putText(data->displayImage, std::to_string(data->clickedPoints.size()),
//                        cv::Point(x + 5, y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
//            cv::imshow("Select 4 Bottom Points", data->displayImage);

//            if (data->clickedPoints.size() == 4) {
//                std::cout << "\n[INFO] 4 points selected! Press any key to activate trackbars..." << std::endl;
//            }
//        }
//    }
//}

//// Function to calculate and draw everything dynamically
//void draw3DScene() {
//    if (!gState.poseValid) return;

//    cv::Mat outputImage = gState.srcImage.clone();

//    // 1. Parse dimension conversions from sliders
//    float boxSize = 1.0f;
//    float boxHeight = static_cast<float>(gState.heightSlider) / 100.0f; // cm to meters

//    // Convert degrees to radians
//    double yaw = (gState.yawSlider - 180) * CV_PI / 180.0;
//    double pitch = (gState.pitchSlider - 180) * CV_PI / 180.0;
//    double roll = (gState.rollSlider - 180) * CV_PI / 180.0;

//    // 2. Generate Object-Relative Rotation Matrix (Euler angles)
//    cv::Mat zRot = (cv::Mat_<double>(3, 3) <<
//        cos(yaw), -sin(yaw), 0,
//        sin(yaw),  cos(yaw), 0,
//        0,        0,        1);

//    cv::Mat yRot = (cv::Mat_<double>(3, 3) <<
//        cos(pitch), 0, sin(pitch),
//        0,          1, 0,
//       -sin(pitch), 0, cos(pitch));

//    cv::Mat xRot = (cv::Mat_<double>(3, 3) <<
//        1, 0,        0,
//        0, cos(roll), -sin(roll),
//        0, sin(roll),  cos(roll));

//    cv::Mat deltaRMat = zRot * yRot * xRot;

//    // Combine original camera pose rotation with local trackbar rotation adjustments
//    cv::Mat baseRMat;
//    cv::Rodrigues(gState.baseRvec, baseRMat);
//    cv::Mat finalRMat = baseRMat * deltaRMat;

//    cv::Mat finalRvec;
//    cv::Rodrigues(finalRMat, finalRvec);

//    // 3. Define 3D Box vertices centered around the rotation origin
//    float half = boxSize / 2.0f;
//    std::vector<cv::Point3f> fullBoxPoints3D = {
//        // Base corners centered on Z=0
//        cv::Point3f(-half, -half, 0.0f),
//        cv::Point3f( half, -half, 0.0f),
//        cv::Point3f( half,  half, 0.0f),
//        cv::Point3f(-half,  half, 0.0f),
//        // Roof corners extruded upwards (-Z)
//        cv::Point3f(-half, -half, -boxHeight),
//        cv::Point3f( half, -half, -boxHeight),
//        cv::Point3f( half,  half, -boxHeight),
//        cv::Point3f(-half,  half, -boxHeight)
//    };

//    // 4. Project Box Points
//    std::vector<cv::Point2f> projectedBoxPoints2D;
//    cv::projectPoints(fullBoxPoints3D, finalRvec, gState.baseTvec, gState.cameraMatrix, gState.distCoeffs, projectedBoxPoints2D);

//    // 5. Generate Cylinder Points
//    int segments = 32;
//    std::vector<cv::Point3f> cylinderPoints3D;
//    for (int i = 0; i < segments; ++i) {
//        float angle = static_cast<float>(i) * 2.0f * CV_PI / static_cast<float>(segments);
//        cylinderPoints3D.push_back(cv::Point3f(half * cos(angle), half * sin(angle), 0.0f));
//    }
//    for (int i = 0; i < segments; ++i) {
//        float angle = static_cast<float>(i) * 2.0f * CV_PI / static_cast<float>(segments);
//        cylinderPoints3D.push_back(cv::Point3f(half * cos(angle), half * sin(angle), -boxHeight));
//    }

//    // 6. Project Cylinder Points
//    std::vector<cv::Point2f> projectedCylinderPoints2D;
//    cv::projectPoints(cylinderPoints3D, finalRvec, gState.baseTvec, gState.cameraMatrix, gState.distCoeffs, projectedCylinderPoints2D);

//    // =========================================================================
//    // RENDERING
//    // =========================================================================
//    cv::Scalar boxColor(100, 100, 100);
//    cv::Scalar cylColor(255, 0, 255);
//    int thickness = 2;

//    // Draw Box
//    for (int i = 0; i < 4; i++) {
//        cv::line(outputImage, projectedBoxPoints2D[i], projectedBoxPoints2D[(i + 1) % 4], boxColor, 1);
//        cv::line(outputImage, projectedBoxPoints2D[i + 4], projectedBoxPoints2D[((i + 1) % 4) + 4], boxColor, 1);
//        cv::line(outputImage, projectedBoxPoints2D[i], projectedBoxPoints2D[i + 4], boxColor, 1);
//    }

//    // Draw Cylinder Rings
//    for (int i = 0; i < segments; ++i) {
//        cv::line(outputImage, projectedCylinderPoints2D[i], projectedCylinderPoints2D[(i + 1) % segments], cylColor, thickness, cv::LINE_AA);
//        cv::line(outputImage, projectedCylinderPoints2D[i + segments], projectedCylinderPoints2D[((i + 1) % segments) + segments], cylColor, thickness, cv::LINE_AA);
//    }
//    // Draw Cylinder Pillars
//    for (int i = 0; i < segments; i += segments / 4) {
//        cv::line(outputImage, projectedCylinderPoints2D[i], projectedCylinderPoints2D[i + segments], cylColor, thickness, cv::LINE_AA);
//    }

//    cv::imshow("Estimated 3D Box & Cylinder Result", outputImage);
//}

//int main(int argc, char** argv) {
//    if (argc < 2) {
//        std::cout << "Usage: " << argv[0] << " <Path_to_Image>" << std::endl;
//        return -1;
//    }

//    gState.srcImage = cv::imread(argv[1]);
//    if (gState.srcImage.empty()) {
//        std::cout << "Error: Could not load image!" << std::endl;
//        return -1;
//    }

//    // Camera Intrinsic setup
//    double focal_length = 800.0;
//    cv::Point2d center(gState.srcImage.cols / 2.0, gState.srcImage.rows / 2.0);
//    gState.cameraMatrix = (cv::Mat_<double>(3, 3) <<
//        focal_length, 0,            center.x,
//        0,            focal_length, center.y,
//        0,            0,            1);
//    gState.distCoeffs = cv::Mat::zeros(4, 1, CV_64F);

//    // Initial click step
//    MouseCallbackData mouseData;
//    mouseData.displayImage = gState.srcImage.clone();
//    cv::namedWindow("Select 4 Bottom Points");
//    cv::setMouseCallback("Select 4 Bottom Points", onMouseClick, &mouseData);
//    cv::imshow("Select 4 Bottom Points", mouseData.displayImage);
//    cv::waitKey(0);

//    if (mouseData.clickedPoints.size() < 4) {
//        std::cout << "Error: Selection incomplete." << std::endl;
//        return -1;
//    }

//    // Define 3D matching points centered around the system base origin
//    float half = 0.5f;
//    std::vector<cv::Point3f> objectPoints3D = {
//        cv::Point3f(-half, -half, 0.0f),
//        cv::Point3f( half, -half, 0.0f),
//        cv::Point3f( half,  half, 0.0f),
//        cv::Point3f(-half,  half, 0.0f)
//    };

//    // Calculate initial pose matrix reference
//    gState.poseValid = cv::solvePnP(objectPoints3D, mouseData.clickedPoints, gState.cameraMatrix, gState.distCoeffs, gState.baseRvec, gState.baseTvec);
//    cv::destroyWindow("Select 4 Bottom Points");

//    if (!gState.poseValid) {
//        std::cout << "Error: Initial PnP failed." << std::endl;
//        return -1;
//    }

//    // Create Main Interactive Adjustment Window
//    cv::namedWindow("Estimated 3D Box & Cylinder Result", cv::WINDOW_AUTOSIZE);

//    // Initialize slider midpoints to allow left/right rotation offsets (180 = no offset)
//    gState.yawSlider = 180;
//    gState.pitchSlider = 180;
//    gState.rollSlider = 180;

//    // Instantiate Trackbars
//    cv::createTrackbar("Height (cm)", "Estimated 3D Box & Cylinder Result", &gState.heightSlider, 400, onTrackbarChange);
//    cv::createTrackbar("Yaw (Angle Z)", "Estimated 3D Box & Cylinder Result", &gState.yawSlider, 360, onTrackbarChange);
//    cv::createTrackbar("Pitch (Angle Y)", "Estimated 3D Box & Cylinder Result", &gState.pitchSlider, 360, onTrackbarChange);
//    cv::createTrackbar("Roll (Angle X)", "Estimated 3D Box & Cylinder Result", &gState.rollSlider, 360, onTrackbarChange);

//    // Initial draw call
//    draw3DScene();
//    cv::waitKey(0);

//    return 0;
//}
