//#include "chessDetector/ChessImageProcessing.h"
//#include <iostream>
//#include <vector>
//#include <map>

//using namespace cv;
//using namespace std;

//// --- GLOBAL STORAGE FOR SETUP ---
//Mat img1, img2, warped1, warped2;
//vector<Point2f> corners;
//ChessImageProcessing* m_chessDetector;
//// --- ADJUSTABLE PARAMETERS (Controlled by Trackbars) ---
//int g_threshold_val = 500;
//int g_roi_percent = 50;
//int g_canny_low = 50;
//int g_diff_thresh = 30;

//// --- NEW PARAMETERS FOR PIECE DETECTION ---
//int g_piece_min_points = 400; // Default threshold for number of edge points
//int g_piece_roi_percent = 80; // Default percent of square area to check (from center)
//int g_piece_max_bbox_percent = 60; // Maximum percent of ROI area for bounding box to be considered clustered

//void processAndDisplay(const string& playerSide = "white") {
//    if (img1.empty() || img2.empty()) return;
//    m_chessDetector->findPossibleMoves(img1,img2,
//                                       g_threshold_val,g_roi_percent,g_canny_low,g_diff_thresh,
//                                       g_piece_min_points,g_piece_roi_percent,
//                                       "white");
//}

//void onMouse(int event, int x, int y, int flags, void* userdata) {
//    if (event == EVENT_LBUTTONDOWN && corners.size() < 4) {
//        corners.push_back(Point2f(x, y));
//        circle(img1, Point(x, y), 5, Scalar(0, 255, 0), -1);
//        imshow("Setup", img1);
//        if (corners.size() == 4) {
//            m_chessDetector->setCorners(corners[0].x,corners[0].y,
//                    corners[1].x,corners[1].y,
//                    corners[2].x,corners[2].y,
//                    corners[3].x,corners[3].y);
//            processAndDisplay();
//        }
//    }
//}

//int main(int argc, char** argv) {
//    if (argc < 3) return -1;
//    img1 = imread(argv[1]); img2 = imread(argv[2]);
//    if (img1.empty() || img2.empty()) return -1;
//    m_chessDetector = new ChessImageProcessing();
//    namedWindow("Setup"); namedWindow("Controls");
//    createTrackbar("Min Pixels", "Controls", &g_threshold_val, 2000, [](int, void*){ processAndDisplay(); });
//    createTrackbar("ROI %", "Controls", &g_roi_percent, 100, [](int, void*){ processAndDisplay(); });
//    createTrackbar("Canny Low", "Controls", &g_canny_low, 255, [](int, void*){ processAndDisplay(); });
//    createTrackbar("Diff Thresh", "Controls", &g_diff_thresh, 255, [](int, void*){ processAndDisplay(); });
//    // --- Add trackbars for piece detection ---
//    createTrackbar("Piece MinPts", "Controls", &g_piece_min_points, 2000, [](int, void*){ processAndDisplay(); });
//    createTrackbar("Piece ROI%", "Controls", &g_piece_roi_percent, 100, [](int, void*){ processAndDisplay(); });
//    createTrackbar("Piece MaxBBox%", "Controls", &g_piece_max_bbox_percent, 100, [](int, void*){ processAndDisplay(); });

//    setMouseCallback("Setup", onMouse);
//    imshow("Setup", img1);
//    while(waitKey(1) != 27);
//    return 0;
//}
