//#include <opencv2/opencv.hpp>
//#include <iostream>
//#include <cmath>

//// Global variables
//cv::Mat src_color, src_gray, blurred, edges, edges_resized, display_img;
//const std::string color_window = "1. Drag Circle (Color)";
//const std::string binary_window = "2. Binary Edge Image (Resized)";

//// Circle dimensions and states
//cv::Point circle_center(100, 100);
//int circle_radius = 50;
//bool is_dragging = false;

//// Trackbars
//int blur_slider = 4;           // Default blur kernel: (4 * 2) + 1 = 9
//int canny_threshold = 150;
//int dilate_slider = 0;         // Default: 0 means no dilation applied
//int radius_slider = 50;
//int scale_factor = 3;

//// Matrix to isolate and track the clicked connected component
//cv::Mat selected_component_mask;

//void updateDisplay() {
//    // 1. Calculate safe parameters from trackbars
//    int safe_scale = std::max(1, scale_factor);
//    int blur_ksize = (blur_slider * 2) + 1;

//    // 2. Apply live Gaussian Blur
//    cv::GaussianBlur(src_gray, blurred, cv::Size(blur_ksize, blur_ksize), 2, 2);

//    // 3. Generate raw Canny edges
//    cv::Mat raw_edges;
//    int lower_canny = canny_threshold / 2;
//    cv::Canny(blurred, raw_edges, lower_canny, std::max(1, canny_threshold));

//    // 4. Apply dynamic Morphological Dilation if slider > 0
//    if (dilate_slider > 0) {
//        // Map dilate slider values (1, 2, 3...) to kernel pixel dimensions (3x3, 5x5, 7x7...)
//        int dilate_ksize = (dilate_slider * 2) + 1;
//        cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(dilate_ksize, dilate_ksize));
//        cv::dilate(raw_edges, edges, element);
//    } else {
//        // If 0, bypass dilation entirely to retain original thin edges
//        edges = raw_edges.clone();
//    }

//    // Initialize or reset component tracker to match modified edges size
//    if (selected_component_mask.empty() || selected_component_mask.size() != edges.size()) {
//        selected_component_mask = cv::Mat::zeros(edges.size(), CV_8UC1);
//    }

//    // 5. Build the visual representation for the binary window
//    cv::Mat edges_bgr;
//    cv::cvtColor(edges, edges_bgr, cv::COLOR_GRAY2BGR);

//    // Color the selected connected segment red where the mask is active
//    edges_bgr.setTo(cv::Scalar(0, 0, 255), selected_component_mask > 0);

//    // Resize using INTER_NEAREST to keep pixel structures crisp and blocky
//    cv::resize(edges_bgr, edges_resized, cv::Size(), safe_scale, safe_scale, cv::INTER_NEAREST);
//    cv::imshow(binary_window, edges_resized);

//    // 6. Clone color image for clean drawing
//    display_img = src_color.clone();

//    // 7. Mathematical alignment check
//    int match_count = 0;
//    int total_samples = 36;

//    for (int i = 0; i < total_samples; ++i) {
//        double angle = i * (10.0 * CV_PI / 180.0);
//        int px = cvRound(circle_center.x + circle_radius * cos(angle));
//        int py = cvRound(circle_center.y + circle_radius * sin(angle));

//        if (px >= 0 && px < edges.cols && py >= 0 && py < edges.rows) {
//            if (edges.at<uchar>(py, px) > 0) {
//                match_count++;
//            }
//        }
//    }

//    double match_ratio = (double)match_count / total_samples;
//    cv::Scalar circle_color = (match_ratio > 0.30) ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);

//    // 8. Draw the user-controlled circle
//    cv::circle(display_img, circle_center, circle_radius, circle_color, 3, cv::LINE_AA);
//    cv::circle(display_img, circle_center, 3, circle_color, -1);

//    std::string text = "Edge Match: " + std::to_string((int)(match_ratio * 100)) + "%";
//    cv::putText(display_img, text, cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, circle_color, 2);

//    cv::imshow(color_window, display_img);
//}

//// Trackbar callbacks
//void onTrackbar(int, void*) {
//    circle_radius = std::max(5, radius_slider);
//    updateDisplay();
//}

//// Mouse event callback for the Main Color Window
//void onColorMouse(int event, int x, int y, int flags, void* userdata) {
//    if (event == cv::EVENT_LBUTTONDOWN) {
//        double dist = std::sqrt(std::pow(x - circle_center.x, 2) + std::pow(y - circle_center.y, 2));
//        if (dist < 30.0 || dist < circle_radius) {
//            is_dragging = true;
//        }
//    }
//    else if (event == cv::EVENT_MOUSEMOVE && is_dragging) {
//        circle_center = cv::Point(x, y);
//        updateDisplay();
//    }
//    else if (event == cv::EVENT_LBUTTONUP) {
//        is_dragging = false;
//    }
//}

//// Mouse event callback for the Resized Binary Window
//void onBinaryMouse(int event, int x, int y, int flags, void* userdata) {
//    if (event == cv::EVENT_LBUTTONDOWN) {
//        int safe_scale = std::max(1, scale_factor);

//        int orig_x = x / safe_scale;
//        int orig_y = y / safe_scale;

//        if (orig_x >= 0 && orig_x < edges.cols && orig_y >= 0 && orig_y < edges.rows) {
//            if (edges.at<uchar>(orig_y, orig_x) == 255) {
//                cv::Mat labels;
//                cv::connectedComponents(edges, labels, 8, CV_32S);

//                int clicked_label_id = labels.at<int>(orig_y, orig_x);

//                if (clicked_label_id > 0) {
//                    selected_component_mask = (labels == clicked_label_id);
//                    updateDisplay();
//                }
//            }
//        }
//    }
//}

//int main(int argc, char** argv) {
//    if (argc < 2) {
//        std::cerr << "Usage: " << argv << " <Path_to_Image>" << std::endl;
//        return -1;
//    }

//    std::string image_path(argv[1]);
//    src_color = cv::imread(image_path, cv::IMREAD_COLOR);

//    if (src_color.empty()) {
//        std::cerr << "Error: Could not open image: " << image_path << std::endl;
//        return -1;
//    }

//    cv::cvtColor(src_color, src_gray, cv::COLOR_BGR2GRAY);

//    cv::namedWindow(color_window, cv::WINDOW_AUTOSIZE);
//    cv::namedWindow(binary_window, cv::WINDOW_AUTOSIZE);

//    // Interactive Trackbars assigned to the dashboard interface
//    cv::createTrackbar("Blur Size", color_window, &blur_slider, 15, onTrackbar);
//    cv::createTrackbar("Canny Thresh", color_window, &canny_threshold, 300, onTrackbar);
//    cv::createTrackbar("Dilate Size", color_window, &dilate_slider, 5, onTrackbar); // 0 (Off) to 5 (Heavy dilation)
//    cv::createTrackbar("Circle Radius", color_window, &radius_slider, src_gray.cols / 2, onTrackbar);
//    cv::createTrackbar("Binary Scale", color_window, &scale_factor, 5, onTrackbar);

//    cv::setMouseCallback(color_window, onColorMouse, NULL);
//    cv::setMouseCallback(binary_window, onBinaryMouse, NULL);

//    // Initial render setup
//    updateDisplay();

//    while (true) {
//        char key = (char)cv::waitKey(10);
//        if (key == 27) {
//            break;
//        }
//        else if (key == ' ') {
//            selected_component_mask = cv::Mat::zeros(edges.size(), CV_8UC1);
//            updateDisplay();
//            std::cout << "Cleared red markings." << std::endl;
//        }
//    }

//    return 0;
//}
