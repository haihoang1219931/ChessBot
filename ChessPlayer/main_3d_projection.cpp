#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cmath>

// Global variables for UI interaction
cv::Mat img_original, img_display;
std::vector<cv::Point2f> clicked_points;
std::string window_name = "Chessboard Multi-Level Projection";

// Trackbar variables (scaled to integers for OpenCV)
int track_dx = 500;    // Range 0-1000, mapped to -5.0 to 5.0
int track_dy = 500;    // Range 0-1000, mapped to -5.0 to 5.0
int track_dz = 0;    // Range 0-1000, mapped to 0.1 to 10.1
int track_fov = 56;    // Range 1-179 degrees
int track_tilt_offset = 180; // Range 0-360, maps to -90 to +90 degrees tilt change
int track_height = 25;  // Range 0-200, mapped to 0.0 to 2.0 units height above the board

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
            // Horizontal lines
            if (j < grid_size - 1) {
                cv::line(output_img, points[current_idx], points[current_idx + 1], color, thickness);
            }
            // Vertical lines
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
    float dx = (track_dx - 500) / 100.0f;
    float dy = (track_dy - 500) / 100.0f;
    float dz = (track_dz) / 100.0f;
    float fov = (float)track_fov;

    float tilt_offset_deg = (float)(track_tilt_offset - 180) * 0.5f;
    float tilt_offset_rad = tilt_offset_deg * CV_PI / 180.0f;

    // Piece height parameter: maps 0-200 slider value to 0.0 to 2.0 metric units
    float piece_height = -((float)track_height / 100.0f);

    // 2. Define standard 3D coordinates for the Ground Chessboard (Z = 0)
    std::vector<cv::Point3f> ground_object_points;
    // 3. Define standard 3D coordinates for the Elevated Head Plane (Z = piece_height)
    std::vector<cv::Point3f> elevated_object_points;

    for (int i = 0; i <= 8; ++i) {
        for (int j = 0; j <= 8; ++j) {
            float x_coord = j * 0.2f - 0.8f; // Centered at origin O
            float y_coord = i * 0.2f - 0.8f;

            ground_object_points.push_back(cv::Point3f(x_coord, y_coord, 0.0f));
            elevated_object_points.push_back(cv::Point3f(x_coord, y_coord, piece_height));
        }
    }

    // 4. Define the 4 corner ground 3D points corresponding to your 4 clicks
    std::vector<cv::Point3f> board_corners_3d = {
        cv::Point3f(-0.8f, -0.8f, 0.0f),
        cv::Point3f( 0.8f, -0.8f, 0.0f),
        cv::Point3f( 0.8f,  0.8f, 0.0f),
        cv::Point3f(-0.8f,  0.8f, 0.0f)
    };

    // 5. Estimate base Camera Pose using PnP
    float width = img_original.cols;
    float height = img_original.rows;
    float f_val = (width / 2.0f) / tan((fov * CV_PI / 180.0f) / 2.0f);

    cv::Mat camera_matrix = (cv::Mat_<double>(3, 3) <<
        f_val, 0, width / 2.0f,
        0, f_val, height / 2.0f,
        0, 0, 1);

    cv::Mat dist_coeffs = cv::Mat::zeros(4, 1, CV_64F);
    cv::Mat rvec, tvec;

    cv::solvePnP(board_corners_3d, clicked_points, camera_matrix, dist_coeffs, rvec, tvec);

    tvec.at<double>(0) += dx;
    tvec.at<double>(1) += dy;
    tvec.at<double>(2) += dz;

    // 6. Calculate the baseline tilt angle between AM and AO
    cv::Mat R;
    cv::Rodrigues(rvec, R);
    cv::Mat R_T = R.t();

    cv::Mat cam_pos_W = -R_T * tvec;
    double ax = cam_pos_W.at<double>(0);
    double ay = cam_pos_W.at<double>(1);
    double az = cam_pos_W.at<double>(2);

    double ao_x = -ax;
    double ao_y = -ay;
    double ao_z = -az;
    double ao_norm = std::sqrt(ao_x*ao_x + ao_y*ao_y + ao_z*ao_z);

    double am_x = R_T.at<double>(0, 2);
    double am_y = R_T.at<double>(1, 2);
    double am_z = R_T.at<double>(2, 2);
    double am_norm = std::sqrt(am_x*am_x + am_y*am_y + am_z*am_z);

    double dot_product = (am_x * ao_x) + (am_y * ao_y) + (am_z * ao_z);
    double cos_theta = dot_product / (ao_norm * am_norm);

    cos_theta = std::max(-1.0, std::min(1.0, cos_theta));
    double current_tilt_rad = std::acos(cos_theta);
    double current_tilt_deg = current_tilt_rad * 180.0f / CV_PI;

    // 7. Apply Tilt adjustment around the camera's local X-axis (Pitch adjustment)
    cv::Mat R_tilt = (cv::Mat_<double>(3, 3) <<
        1, 0,                   0,
        0, cos(tilt_offset_rad), -sin(tilt_offset_rad),
        0, sin(tilt_offset_rad),  cos(tilt_offset_rad));

    R = R * R_tilt;
    cv::Rodrigues(R, rvec);

    // 8. Project both Ground and Elevated grids back onto the 2D image plane
    std::vector<cv::Point2f> projected_ground_points;
    std::vector<cv::Point2f> projected_elevated_points;

    cv::projectPoints(ground_object_points, rvec, tvec, camera_matrix, dist_coeffs, projected_ground_points);
    cv::projectPoints(elevated_object_points, rvec, tvec, camera_matrix, dist_coeffs, projected_elevated_points);

    // 9. Draw UI text and overlay both grids
    img_original.copyTo(img_display);

    // Status text overlay
    std::string text_tilt = "True Tilt Angle (AM^AO): " + std::to_string(current_tilt_deg + tilt_offset_deg) + " deg";
    std::string text_height = "Piece Height: " + std::to_string(piece_height) + " units";
    cv::putText(img_display, text_tilt, cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
    cv::putText(img_display, text_height, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 0, 0), 1, cv::LINE_AA);

    // Draw base board grid in Green
    drawGrid(img_display, projected_ground_points, cv::Scalar(0, 255, 0), 1);

    // Draw elevated piece-head grid in Blue
    drawGrid(img_display, projected_elevated_points, cv::Scalar(255, 120, 0), 1);

    // Optional: Draw vertical pillars connecting corner points to visualize 3D bounds
    int corner_indices[4] = {0, 8, 80, 72}; // 4 outermost index positions of the 9x9 matrix grid
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

    cv::resize(img_input, img_original, cv::Size(1280, 720), 0, 0, cv::INTER_LINEAR);
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

    cv::createTrackbar("dx", window_name, &track_dx, 1000, onTrackbar);
    cv::createTrackbar("dy", window_name, &track_dy, 1000, onTrackbar);
    cv::createTrackbar("dz", window_name, &track_dz, 1000, onTrackbar);
    cv::createTrackbar("FOV", window_name, &track_fov, 179, onTrackbar);
    cv::createTrackbar("Tilt Offset", window_name, &track_tilt_offset, 360, onTrackbar);
    cv::createTrackbar("Piece Height", window_name, &track_height, 200, onTrackbar); // New height trackbar

    while (true) {
        updateProjection();
        char key = (char)cv::waitKey(30);
        if (key == 27) break;
    }

    return 0;
}
