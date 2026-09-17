#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include "chessDetector/ChessImageProcessing.h"

// Global variables for UI interaction
cv::Mat img_input;
cv::Mat img_original, img_display;
std::vector<cv::Point2f> clicked_points;
std::string window_name = "Chessboard Multi-Level Projection";
std::vector<cv::Point2f> dstCorners;
std::vector<cv::Point2f> srcCorners;
// Base parameters calculated from your 4 corner clicks
cv::Mat base_rvec, base_tvec;
double base_tilt_deg = 0.0;
bool is_pnp_initialized = false;
int g_dnnInputSize =  240;
int g_dnnChannel = 3;

// Trackbar variables (scaled to integers for OpenCV)
// These now serve purely as structural adjustments (+/- offsets) relative to the PnP base values
int track_dx = 500;    // Range 0-1000, mapped to -5.0 to 5.0 offset
int track_dy = 500;    // Range 0-1000, mapped to -5.0 to 5.0 offset
int track_dz = 500;    // Range 0-1000, mapped to -5.0 to 5.0 offset
int track_fov = 60;    // Range 1-179 degrees (Absolute field of view)
int track_tilt_offset = 180; // Range 0-360, maps to -90 to +90 degrees tilt change
int track_height = 20;  // Range 0-200, mapped to 0.0 to 2.0 units height above the board

ChessImageProcessing chessDetector;
// Callback function for mouse clicks
void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        if (clicked_points.size() < 4) {
            clicked_points.push_back(cv::Point2f(x, y));
            std::cout << "Point " << clicked_points.size() << " selected at: (" << x << ", " << y << ")\n";
        }
    }
}

// Updated helper function to handle custom grid row and column configurations
void drawGrid(cv::Mat& output_img, const std::vector<cv::Point2f>& points, cv::Scalar color, int thickness) {
    int rows = 9;
    int cols = 15;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            int current_idx = i * cols + j;
            if (j < cols - 1) {
                cv::line(output_img, points[current_idx], points[current_idx + 1], color, thickness);
            }
            if (i < rows - 1) {
                cv::line(output_img, points[current_idx], points[current_idx + cols], color, thickness);
            }
        }
    }
}
bool showElevated = false;
// Function to update the projection render
void updateProjection() {
    if (clicked_points.size() < 4) {
        img_original.copyTo(img_display);
        for (size_t i = 0; i < clicked_points.size(); ++i) {
            cv::circle(img_display, clicked_points[i], 5, cv::Scalar(0, 0, 255), -1);
        }
        cv::imshow(window_name, img_display);
        return;
    }

    // 1. Map trackbars back to actual float values
    float dx_offset = (track_dx - 500) / 100.0f;
    float dy_offset = (track_dy - 500) / 100.0f;
    float dz_offset = (track_dz - 500) / 100.0f;
    float fov = (float)track_fov;

    float tilt_offset_deg = (float)(track_tilt_offset - 180) * 0.5f;
    float tilt_offset_rad = tilt_offset_deg * CV_PI / 180.0f;
    float piece_height = -(float)track_height / 100.0f;

    // 2. Define standard 3D coordinates for Ground Chessboard & Elevated Head Plane
    std::vector<cv::Point3f> ground_object_points;
    std::vector<cv::Point3f> elevated_object_points;

    int total_rows = 9;    // 8 board squares = 9 grid lines
    int total_columns = 15; // 14 board squares = 15 grid lines
    float square_size = 0.2f;

    for (int i = 0; i < total_rows; ++i) {
        for (int j = 0; j < total_columns; ++j) {
            // Center the grid's X and Y coordinates around the local origin
            float x_coord = j * square_size - ((total_columns - 1) * square_size / 2.0f);
            float y_coord = i * square_size - ((total_rows - 1) * square_size / 2.0f);

            ground_object_points.push_back(cv::Point3f(x_coord, y_coord, 0.0f));
            elevated_object_points.push_back(cv::Point3f(x_coord, y_coord, piece_height));
        }
    }

    // 3. Define the 4 corner ground 3D points matching your 4 user mouse clicks
    // Winding Order: Top-Left -> Top-Right -> Bottom-Right -> Bottom-Left
    float half_width = (total_columns - 1) * square_size / 2.0f;  // 14 * 0.2 / 2 = 1.4
    float half_height = (total_rows - 1) * square_size / 2.0f;    //  8 * 0.2 / 2 = 0.8

    std::vector<cv::Point3f> board_corners_3d = {
        cv::Point3f(-half_width, -half_height, 0.0f), // Top-Left
        cv::Point3f( half_width, -half_height, 0.0f), // Top-Right
        cv::Point3f( half_width,  half_height, 0.0f), // Bottom-Right
        cv::Point3f(-half_width,  half_height, 0.0f)  // Bottom-Left
    };

    // 4. Construct camera intrinsics matrix based on FOV
    float width = img_original.cols;
    float height = img_original.rows;
    float f_val = (width / 2.0f) / tan((fov * CV_PI / 180.0f) / 2.0f);

    cv::Mat camera_matrix = (cv::Mat_<double>(3, 3) <<
        f_val, 0, width / 2.0f,
        0, f_val, height / 2.0f,
        0, 0, 1);

    cv::Mat dist_coeffs = cv::Mat::zeros(4, 1, CV_64F);

    // 5. ALWAYS calculate the base camera pose relative to the current FOV
    // to anchor the 3D base board corners perfectly to the static 2D clicked points.
    cv::solvePnP(board_corners_3d, clicked_points, camera_matrix, dist_coeffs, base_rvec, base_tvec);

    // Calculate base tilt angle
    cv::Mat R_initial;
    cv::Rodrigues(base_rvec, R_initial);
    cv::Mat R_T = R_initial.t();
    cv::Mat cam_pos_W = -R_T * base_tvec;

    double ax = cam_pos_W.at<double>(0);
    double ay = cam_pos_W.at<double>(1);
    double az = cam_pos_W.at<double>(2);

    double ao_norm = std::sqrt(ax*ax + ay*ay + az*az);
    double am_x = R_T.at<double>(0, 2);
    double am_y = R_T.at<double>(1, 2);
    double am_z = R_T.at<double>(2, 2);
    double am_norm = std::sqrt(am_x*am_x + am_y*am_y + am_z*am_z);

    double dot_product = (am_x * (-ax)) + (am_y * (-ay)) + (am_z * (-az));
    double cos_theta = std::max(-1.0, std::min(1.0, dot_product / (ao_norm * am_norm)));
    base_tilt_deg = std::acos(cos_theta) * 180.0f / CV_PI;

    // 6. Keep base values completely clean for the ground grid.
    // Apply offsets ONLY to a separate working set for the elevated/transformed grid.
    cv::Mat working_tvec = base_tvec.clone();
    working_tvec.at<double>(0) += dx_offset;
    working_tvec.at<double>(1) += dy_offset;
    working_tvec.at<double>(2) += dz_offset;

    cv::Mat R_working;
    cv::Rodrigues(base_rvec, R_working);

    // Apply local camera tilt rotation matrix
    cv::Mat R_tilt = (cv::Mat_<double>(3, 3) <<
        1, 0,                   0,
        0, cos(tilt_offset_rad), -sin(tilt_offset_rad),
        0, sin(tilt_offset_rad),  cos(tilt_offset_rad));

    R_working = R_working * R_tilt;

    cv::Mat working_rvec;
    cv::Rodrigues(R_working, working_rvec);

    // 7. Project both grids onto the screen space map
    std::vector<cv::Point2f> projected_ground_points;
    std::vector<cv::Point2f> projected_elevated_points;

    // FIX: Ground points use base_rvec/base_tvec so they remain locked to your clicks
    cv::projectPoints(ground_object_points, base_rvec, base_tvec, camera_matrix, dist_coeffs, projected_ground_points);

    // Elevated points use working_rvec/working_tvec to respond to sliders
    cv::projectPoints(elevated_object_points, working_rvec, working_tvec, camera_matrix, dist_coeffs, projected_elevated_points);

    // 8. Composite screen frames
    img_original.copyTo(img_display);

    std::string text_tilt = "True Tilt Angle (AM^AO): " + std::to_string(base_tilt_deg + tilt_offset_deg) + " deg";
    std::string text_height = "Piece Height: " + std::to_string(piece_height) + " units";
    cv::putText(img_display, text_tilt, cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
    cv::putText(img_display, text_height, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 0, 0), 1, cv::LINE_AA);

    drawGrid(img_display, projected_ground_points, cv::Scalar(0, 255, 0), 1);
    drawGrid(img_display, projected_elevated_points, cv::Scalar(255, 120, 0), 1);

    // Dynamic corners based on the 9x15 structural layout
    // Top-Left: 0, Top-Right: 14, Bottom-Right: (9*15)-1 = 134, Bottom-Left: 9*15 - 15 = 120
    int corner_indices[] = {0, total_columns - 1, (total_rows * total_columns) - 1, (total_rows * total_columns) - total_columns};
    for(int idx : corner_indices) {
        cv::line(img_display, projected_ground_points[idx], projected_elevated_points[idx], cv::Scalar(0, 255, 255), 1);
    }

    if(!showElevated) {
        showElevated = true;
        for(int i = 0; i < 4; i++) {
            cv::Point2f elevatedPoint = projected_elevated_points[corner_indices[i]];
            std::cout << "elevatedPoint["<<i <<"] (" << elevatedPoint.x << ", " << elevatedPoint.y << ")\n";
        }
    }
    cv::imshow(window_name, img_display);

    dstCorners = {
        cv::Point2f(0, 0),
        cv::Point2f(WARP_WIDTH - 1, 0),
        cv::Point2f(WARP_WIDTH - 1, WARP_HEIGHT - 1),
        cv::Point2f(0, WARP_HEIGHT - 1)
    };
    srcCorners.clear();
    float scaleFactor = 3.0f;
    for(int idx : corner_indices) {
        srcCorners.push_back(cv::Point2f(scaleFactor*projected_elevated_points[idx].x,
                                         scaleFactor*projected_elevated_points[idx].y));
    }
    for(cv::Point2f corner: srcCorners) {
        std::cout << "elevated corner (" << corner.x << ", " << corner.y << ")\n";
    }
}

void classification() {
    float scaleFactor = 3.0f;
    chessDetector.setCorners(
            scaleFactor*clicked_points[0].x,scaleFactor*clicked_points[0].y,
            scaleFactor*clicked_points[1].x,scaleFactor*clicked_points[1].y,
            scaleFactor*clicked_points[2].x,scaleFactor*clicked_points[2].y,
            scaleFactor*clicked_points[3].x,scaleFactor*clicked_points[3].y);
    cv::Mat homographyMatrix = chessDetector.getFullTranformMatrix();
    cv::Mat warpedBoard;
    cv::warpPerspective(img_input, warpedBoard, homographyMatrix, cv::Size(WARP_WIDTH, WARP_HEIGHT));
//    cv::Mat blurred;
//    cv::GaussianBlur(warpedBoard, blurred, cv::Size(0, 0), 3.0);
//    cv::addWeighted(warpedBoard, 1.5, blurred, -0.5, 0, warpedBoard);
//    chessDetector.classsifyWholeBoardAtOnce((const cv::Mat&)warpedBoard,3360, 1920, 3);
    chessDetector.classifyWholeBoardNativeOpenVINO((const cv::Mat&)warpedBoard);
//    chessDetector.classsifyChessBoardImage((const cv::Mat&)warpedBoard);
}
void onTrackbar(int, void*) {
    updateProjection();
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <onnx_path> <image_path>\n";
        return -1;
    }

    img_input = cv::imread(argv[2]);
    if (img_input.empty()) {
        std::cerr << "Error: Could not open or find the image: " << argv[1] << "\n";
        return -1;
    }
    std::vector<char> print_names = {
        'b', '.', 'k', 'n', 'p', 'q', 'r'
    };
    if(argc >=5) {
        g_dnnInputSize = atoi(argv[3]);
        g_dnnChannel = atoi(argv[4]);
    }
    setenv("OPENCV_DNN_CACHE_DIR", "./dnn_cache", 1);
    chessDetector.setDnnNetAllPieces2(argv[1],print_names,g_dnnInputSize,g_dnnChannel);
    std::vector<cv::Point> listCell {
        cv::Point(0,0),cv::Point(1,0),cv::Point(2,0),cv::Point(11,0),
        cv::Point(0,1),cv::Point(1,1),cv::Point(2,1),cv::Point(11,1),
        cv::Point(0,2),cv::Point(1,2),cv::Point(2,2),cv::Point(11,2),
        cv::Point(2,3),cv::Point(11,3),
        cv::Point(2,4),cv::Point(11,4),
        cv::Point(2,5),cv::Point(11,5),
        cv::Point(2,6),cv::Point(11,6),
        cv::Point(2,7),cv::Point(11,7),
    };
    chessDetector.excludeCellList(listCell);
#ifdef DEBUG_SINGLE_IMAGE
    int row = atoi(argv[3]);
    int col = atoi(argv[4]);
    cv::resize(img_input, img_original, cv::Size(240, 240), 0, 0, cv::INTER_LINEAR);
    ClassificationResult piece = chessDetector.classifyImage(img_original,row,col);
    chessDetector.checkPieceColor(img_original, piece, row, col);
    printf("Gray(%d/%d)Gold => [%s]\r\n",
               piece.grayPixels,piece.goldPixels,piece.className.c_str());
    cv::imshow("img_original",img_original);
    cv::waitKey();
#else
    cv::resize(img_input, img_original, cv::Size(640, 360), 0, 0, cv::INTER_LINEAR);
    img_original.copyTo(img_display);

    cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);
    cv::setMouseCallback(window_name, onMouse, nullptr);

    std::cout << "Click the 4 outer corners clockwise starting from top-left.\n";

    while (clicked_points.size() < 4) {
        char key = (char)cv::waitKey(10);
        if (key == 27) return 0;
        updateProjection();
    }

    cv::setMouseCallback(window_name, nullptr, nullptr);
    // Create sliders matching the base state parameters calculated from the clicks
    cv::createTrackbar("dx Offset", window_name, &track_dx, 1000, onTrackbar);
    cv::createTrackbar("dy Offset", window_name, &track_dy, 1000, onTrackbar);
    cv::createTrackbar("dz Offset", window_name, &track_dz, 1000, onTrackbar);
    cv::createTrackbar("FOV", window_name, &track_fov, 179, onTrackbar);
    cv::createTrackbar("Tilt Offset", window_name, &track_tilt_offset, 360, onTrackbar);
    cv::createTrackbar("Piece Height", window_name, &track_height, 200, onTrackbar);

    while (true) {
        char key = (char)cv::waitKey();
        if (key == 27) break;
        if (key == ' ') classification();
    }
#endif
    return 0;
}
//data-Bishop,data-Empty,data-EmptyTest,data-King,data-Knight,data-Pawn,data-Queen,data-Rook
