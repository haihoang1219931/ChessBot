#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

// Global variables for UI interaction
cv::Mat img_original, img_display;
std::vector<cv::Point2f> clicked_points;
std::string window_name = "Chessboard Multi-Level Projection";

// Base parameters calculated from your 4 corner clicks
cv::Mat base_rvec, base_tvec;
double base_tilt_deg = 0.0;
bool is_pnp_initialized = false;

// Trackbar variables (scaled to integers for OpenCV)
// These now serve purely as structural adjustments (+/- offsets) relative to the PnP base values
int track_dx = 500;    // Range 0-1000, mapped to -5.0 to 5.0 offset
int track_dy = 500;    // Range 0-1000, mapped to -5.0 to 5.0 offset
int track_dz = 500;    // Range 0-1000, mapped to -5.0 to 5.0 offset
int track_fov = 60;    // Range 1-179 degrees (Absolute field of view)
int track_tilt_offset = 180; // Range 0-360, maps to -90 to +90 degrees tilt change
int track_height = 27;  // Range 0-200, mapped to 0.0 to 2.0 units height above the board

// Callback function for mouse clicks
void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        if (clicked_points.size() < 4) {
            clicked_points.push_back(cv::Point2f(x, y));
            std::cout << "Point " << clicked_points.size() << " selected at: (" << x << ", " << y << ")\n";
        }
    }
}

// Helper function to draw a 9x9 grid layout onto the image frame
void drawGrid(cv::Mat& output_img, const std::vector<cv::Point2f>& points, cv::Scalar color, int thickness) {
    int grid_size = 9;
    for (int i = 0; i < grid_size; ++i) {
        for (int j = 0; j < grid_size; ++j) {
            int current_idx = i * grid_size + j;
            if (j < grid_size - 1) {
                cv::line(output_img, points[current_idx], points[current_idx + 1], color, thickness);
            }
            if (i < grid_size - 1) {
                cv::line(output_img, points[current_idx], points[current_idx + grid_size], color, thickness);
            }
        }
    }
}

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

    for (int i = 0; i <= 8; ++i) {
        for (int j = 0; j <= 8; ++j) {
            float x_coord = j * 0.2f - 0.8f; // Centered at origin O
            float y_coord = i * 0.2f - 0.8f;
            ground_object_points.push_back(cv::Point3f(x_coord, y_coord, 0.0f));
            elevated_object_points.push_back(cv::Point3f(x_coord, y_coord, piece_height));
        }
    }

    // 3. Define the 4 corner ground 3D points corresponding to your 4 clicks
    std::vector<cv::Point3f> board_corners_3d = {
        cv::Point3f(-0.8f, -0.8f, 0.0f),
        cv::Point3f( 0.8f, -0.8f, 0.0f),
        cv::Point3f( 0.8f,  0.8f, 0.0f),
        cv::Point3f(-0.8f,  0.8f, 0.0f)
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

    // 5. Calculate initial camera pose if not done yet
    if (!is_pnp_initialized) {
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

        is_pnp_initialized = true;
    }

    // 6. Apply working modifications over our calculated base values
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

    cv::projectPoints(ground_object_points, working_rvec, working_tvec, camera_matrix, dist_coeffs, projected_ground_points);
    cv::projectPoints(elevated_object_points, working_rvec, working_tvec, camera_matrix, dist_coeffs, projected_elevated_points);

    // 8. Composite screen frames
    img_original.copyTo(img_display);

    std::string text_tilt = "True Tilt Angle (AM^AO): " + std::to_string(base_tilt_deg + tilt_offset_deg) + " deg";
    std::string text_height = "Piece Height: " + std::to_string(piece_height) + " units";
    cv::putText(img_display, text_tilt, cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
    cv::putText(img_display, text_height, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 0, 0), 1, cv::LINE_AA);

    drawGrid(img_display, projected_ground_points, cv::Scalar(0, 255, 0), 1);
    drawGrid(img_display, projected_elevated_points, cv::Scalar(255, 120, 0), 1);

    int corner_indices[] = {0, 8, 80, 72};
    for(int idx : corner_indices) {
        cv::line(img_display, projected_ground_points[idx], projected_elevated_points[idx], cv::Scalar(0, 255, 255), 1);
    }

    cv::imshow(window_name, img_display);
}

void onTrackbar(int, void*) {
    updateProjection();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <Path_to_Chessboard_Image>\n";
        return -1;
    }

    cv::Mat img_input = cv::imread(argv[1]);
    if (img_input.empty()) {
        std::cerr << "Error: Could not open or find the image: " << argv[1] << "\n";
        return -1;
    }

    cv::resize(img_input, img_original, cv::Size(640, 360), 0, 0, cv::INTER_LINEAR);
    img_original.copyTo(img_display);

    cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);
    cv::setMouseCallback(window_name, onMouse, nullptr);

    std::cout << "Click the 4 outer corners clockwise starting from top-left.\n";

    while (clicked_points.size() < 4) {
        updateProjection();
        char key = (char)cv::waitKey(10);
        if (key == 27) return 0;
    }

    cv::setMouseCallback(window_name, nullptr, nullptr);

    // Explicit initialization calculation pass step
    updateProjection();

    // Create sliders matching the base state parameters calculated from the clicks
    cv::createTrackbar("dx Offset", window_name, &track_dx, 1000, onTrackbar);
    cv::createTrackbar("dy Offset", window_name, &track_dy, 1000, onTrackbar);
    cv::createTrackbar("dz Offset", window_name, &track_dz, 1000, onTrackbar);
    cv::createTrackbar("FOV", window_name, &track_fov, 179, onTrackbar);
    cv::createTrackbar("Tilt Offset", window_name, &track_tilt_offset, 360, onTrackbar);
    cv::createTrackbar("Piece Height", window_name, &track_height, 200, onTrackbar);

    while (true) {
        updateProjection();
        char key = (char)cv::waitKey(30);
        if (key == 27) break;
    }

    return 0;
}
