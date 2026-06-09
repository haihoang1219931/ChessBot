//#include "chessDetector/ChessImageProcessing.h"
//#include <iostream>
//#include <vector>
//#include <map>
//#define DEBUG_SHOW_IMAGE
//using namespace cv;
//using namespace std;

//void scaleUp(const cv::Mat& input, cv::Mat& output, int scale) {
//    // Define your new dimensions
//    cv::Size newSize(input.cols * scale, input.rows * scale);

//    // Step 1: Scale using Nearest Neighbor to prevent blurring
//    cv::resize(input, output, newSize, 0, 0, cv::INTER_NEAREST);
//}
//// --- GLOBAL STORAGE FOR SETUP ---
//Mat img1, img2, warped1, warped2, display1, display2;
//cv::Mat gray1, gray2, edges1, edges2;
//vector<Point2f> corners;
//ChessImageProcessing* m_chessDetector;
//#define WARP_SIZE 320

//// --- GLOBAL: interactive Canny threshold ---
//static int g_canny_threshold = 93;

//// --- GLOBAL: zoom state for Raw edges window ---
//static bool g_zoomEnabled = false;
//static cv::Point g_zoomCenter(0,0);
//static int g_zoomSize = 80; // size of ROI in source pixels
//static int g_zoomScale = 4; // zoom factor for display
//static cv::Mat g_lastEdgesGray;
//static bool g_panActive = false;
//static cv::Point g_lastMousePos(0,0);
//static const int g_zoomScaleMin = 1;
//static const int g_zoomScaleMax = 10;

//void refreshRawEdgesDisplay();

//void showEdges(const cv::Mat imgColor) {
//    cv::Mat edge;
//    cv::cvtColor(imgColor, edge, cv::COLOR_BGR2GRAY);
//    int low = std::max(1, g_canny_threshold);
//    cv::Canny(edge, edge, low, low * 3);
//    // save last edges for zoom interaction
//    g_lastEdgesGray = edge.clone();

//    // prepare color for display (so we can draw ROI rectangle)
//    cv::Mat edgeColor;
//    cv::cvtColor(edge, edgeColor, cv::COLOR_GRAY2BGR);

//    // draw ROI rectangle if zoom enabled
//    if (g_zoomEnabled) {
//        int half = g_zoomSize / 2;
//        cv::Rect roiRect(g_zoomCenter.x - half, g_zoomCenter.y - half, g_zoomSize, g_zoomSize);
//        roiRect &= cv::Rect(0,0,edge.cols, edge.rows);
//        cv::rectangle(edgeColor, roiRect, cv::Scalar(0,255,0), 2);
//    }

//    cv::imshow("Raw edges", edgeColor);

//    // show zoom window if enabled
//    if (g_zoomEnabled) {
//        refreshRawEdgesDisplay();
//    }
//}

//void refreshRawEdgesDisplay() {
//    if (g_lastEdgesGray.empty()) return;
//    int half = g_zoomSize / 2;
//    cv::Rect roiRect(g_zoomCenter.x - half, g_zoomCenter.y - half, g_zoomSize, g_zoomSize);
//    roiRect &= cv::Rect(0,0,g_lastEdgesGray.cols, g_lastEdgesGray.rows);
//    if (roiRect.width <=0 || roiRect.height <=0) return;
//    cv::Mat roi = g_lastEdgesGray(roiRect);
//    cv::Mat zoom;
//    cv::resize(roi, zoom, cv::Size(roi.cols * g_zoomScale, roi.rows * g_zoomScale), 0, 0, cv::INTER_NEAREST);
//    // convert to color and mark center
//    cv::Mat zoomColor; cv::cvtColor(zoom, zoomColor, cv::COLOR_GRAY2BGR);
//    cv::circle(zoomColor, cv::Point(zoom.cols/2, zoom.rows/2), 3, cv::Scalar(0,0,255), -1);
//    cv::imshow("Raw edges zoom", zoomColor);
//}

//// mouse callback for Raw edges zoom interactions
//static void onRawEdgesMouse(int event, int x, int y, int flags, void* userdata) {
//    // map mouse coordinates to image coordinates (Raw edges window shows full image)
//    if (event == cv::EVENT_LBUTTONDOWN) {
//        // start panning if already zoom enabled
//        if (g_zoomEnabled) {
//            g_panActive = true;
//            g_lastMousePos = cv::Point(x, y);
//        }
//    } else if (event == cv::EVENT_LBUTTONUP) {
//        g_panActive = false;
//    } else if (event == cv::EVENT_MBUTTONDOWN) {
//        // toggle zoom on middle click and set center
//        g_zoomEnabled = true;
//        g_zoomCenter = cv::Point(x, y);
//        if (!g_lastEdgesGray.empty()) {
//            cv::Mat edgeColor; cv::cvtColor(g_lastEdgesGray, edgeColor, cv::COLOR_GRAY2BGR);
//            int half = g_zoomSize / 2;
//            cv::Rect roiRect(g_zoomCenter.x - half, g_zoomCenter.y - half, g_zoomSize, g_zoomSize);
//            roiRect &= cv::Rect(0,0,g_lastEdgesGray.cols, g_lastEdgesGray.rows);
//            if (roiRect.width > 0 && roiRect.height > 0) cv::rectangle(edgeColor, roiRect, cv::Scalar(0,255,0), 2);
//            cv::imshow("Raw edges", edgeColor);
//            refreshRawEdgesDisplay();
//        }
//    } else if (event == cv::EVENT_MBUTTONUP) {
//        // keep zoom enabled on release
//        g_panActive = false;
//    } else if (event == cv::EVENT_RBUTTONDOWN) {
//        // right-click disables zoom
//        g_zoomEnabled = false;
//        g_panActive = false;
//        cv::destroyWindow("Raw edges zoom");
//        if (!g_lastEdgesGray.empty()) {
//            cv::Mat edgeColor; cv::cvtColor(g_lastEdgesGray, edgeColor, cv::COLOR_GRAY2BGR);
//            cv::imshow("Raw edges", edgeColor);
//        }
//    } else if (event == cv::EVENT_MOUSEMOVE) {
//        if (g_panActive && g_zoomEnabled) {
//            // pan by difference
//            cv::Point cur(x,y);
//            cv::Point diff = cur - g_lastMousePos;
//            g_lastMousePos = cur;
//            g_zoomCenter += diff;
//            // clamp
//            g_zoomCenter.x = std::min(std::max(g_zoomCenter.x, 0), g_lastEdgesGray.cols-1);
//            g_zoomCenter.y = std::min(std::max(g_zoomCenter.y, 0), g_lastEdgesGray.rows-1);
//            refreshRawEdgesDisplay();
//            if (!g_lastEdgesGray.empty()) {
//                cv::Mat edgeColor; cv::cvtColor(g_lastEdgesGray, edgeColor, cv::COLOR_GRAY2BGR);
//                int half = g_zoomSize/2;
//                cv::Rect roiRect(g_zoomCenter.x - half, g_zoomCenter.y - half, g_zoomSize, g_zoomSize);
//                roiRect &= cv::Rect(0,0,g_lastEdgesGray.cols, g_lastEdgesGray.rows);
//                if (roiRect.width > 0 && roiRect.height > 0) cv::rectangle(edgeColor, roiRect, cv::Scalar(0,255,0), 2);
//                cv::imshow("Raw edges", edgeColor);
//            }
//        }
//    }

//    // handle mouse wheel separately (some OpenCV builds provide EVENT_MOUSEWHEEL)
//    if (event == cv::EVENT_MOUSEWHEEL) {
//        // flags contains wheel delta in high-order word
//        int delta = flags >> 16;
//        if (delta < 0) {
//            // zoom in
//            g_zoomScale = std::min(g_zoomScale + 1, g_zoomScaleMax);
//        } else if (delta > 0) {
//            // zoom out
//            g_zoomScale = std::max(g_zoomScale - 1, g_zoomScaleMin);
//        }
//        refreshRawEdgesDisplay();
//    }
//}

//void processAndDisplay() {
////    unsigned char imgData[64] = {
////        0,  0,  0,  0,  0,  0,  0,  0,
////        0,  0,  0,  0,  0,  0,  0,  0,
////        0,  0,255,  0,  0,  0,  0,  0,
////        0,255,  0,255,  0,  0,  0,  0,
////        0,  0,255,  0,  0,  0,  0,  0,
////        0,  0,  0,  0,  0,  0,  0,  0,
////        0,  0,  0,  0,  0,  0,  0,  0,
////        0,  0,  0,  0,  0,  0,  0,  0,
////    };
////    cv::Mat original = cv::Mat(8,8,CV_8UC1,imgData);
//    vector < Point2f > dst {
//        Point2f(0, 0), Point2f(WARP_SIZE - 1, 0), Point2f(WARP_SIZE - 1, WARP_SIZE - 1), Point2f(0, WARP_SIZE - 1)
//    };
//    showEdges(img1);
//    MoveDetectParams params;
//    Mat M = getPerspectiveTransform(corners, dst);
//    warpPerspective(img1, warped1, M, Size(WARP_SIZE, WARP_SIZE));
//    cv::cvtColor(warped1, gray1, cv::COLOR_BGR2GRAY);
//    int low = std::max(1, g_canny_threshold);
//    cv::Canny(gray1, edges1, low, low * 3);
//    int sq = edges1.cols / 8;
//    std::vector<std::vector<int>> mat1 = m_chessDetector->getPieceMatrix(edges1, sq, params,"warp1");
////    int loop = 7;
////    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(loop*2+1, loop*2+1));
////    cv::morphologyEx(edges1, edges1, cv::MORPH_CLOSE, kernel);

////    cv::Point centerLargestArea;
////    cv::Mat matLargestArea = edges1.clone();
////    m_chessDetector->getCenterOfWhitePixels(matLargestArea,centerLargestArea);
////    cv::Mat scaled;
////    int scaleRatio = 1;
////    scaleUp(edges1,scaled,scaleRatio);
////    cv::Mat vis;
////    cv::cvtColor(scaled, vis, cv::COLOR_GRAY2BGR);

////    cv::circle(vis,
////               cv::Point(centerLargestArea.x*scaleRatio,
////                         centerLargestArea.y*scaleRatio),
////               5,cv::Scalar(0,0,255),2);
////    cv::imshow("scaled",vis);
//}
//void onMouse(int event, int x, int y, int flags, void* userdata) {
//    if (event == EVENT_LBUTTONDOWN && corners.size() < 4) {
//        corners.push_back(Point2f(x, y));
//        circle(display1, Point(x, y), 5, Scalar(0, 255, 0), -1);
//        imshow("Setup", display1);
//        if (corners.size() == 4) {
//            processAndDisplay();
//        }
//    }
//    if(event == EVENT_RBUTTONDOWN) {
//        corners.clear();
//        display1 = img1.clone();
//        imshow("Setup", display1);
//    }
//}

//// callback for Canny trackbar: update value and re-run processing when corners are ready
//static void onCannyTrackbar(int pos, void* userdata) {
//    g_canny_threshold = pos;
//    if (corners.size() == 4) {
//        processAndDisplay();
//    }
//}

//int main(int argc, char** argv) {
//    if (argc < 2) {
//        cerr << "Usage: " << argv[0] << " <image>" << endl;
//        return -1;
//    }

//    img1 = imread(argv[1]);
//    display1 = img1.clone();
//    m_chessDetector = new ChessImageProcessing();

//    // create Controls window and canny trackbar
//    namedWindow("Controls", WINDOW_NORMAL);
//    createTrackbar("Canny", "Controls", &g_canny_threshold, 500, onCannyTrackbar);

//    // ensure Raw edges window exists and has mouse callback
//    namedWindow("Raw edges", WINDOW_NORMAL);
//    setMouseCallback("Raw edges", onRawEdgesMouse);

//    imshow("Setup",img1);
//    setMouseCallback("Setup", onMouse);
//    cv::waitKey(0);
//    return 0;
//}
