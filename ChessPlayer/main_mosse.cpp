//#include "chessDetector/ChessImageProcessing.h"
//#include <opencv2/opencv.hpp>
//#include <iostream>

//using namespace cv;
//using namespace std;

//// Globals for UI (Hu-moments matching)
//static Mat g_imgGray;
//static Mat g_templateImg;
//static bool g_hasTemplate = false;
//static Rect g_templateRect;
//static int g_templateSize = 50;
//static int g_matchThresholdPerc = 50; // unused for Hu but kept
//static int g_hu_threshold = 50; // trackbar value (scaled)
//static std::vector<double> g_templateHu(7, 0.0);

//void drawResult(Mat &disp, const Rect &r, bool match) {
//    Scalar col = match ? Scalar(0,255,0) : Scalar(0,0,255);
//    rectangle(disp, r, col, 2);
//}

//static std::vector<double> computeHuLog(const Mat &patch) {
//    Mat bin;
//    // binarize on >0 (edges), assume patch is single-channel
//    cv::threshold(patch, bin, 0, 255, THRESH_BINARY);
//    Moments m = moments(bin, true);
//    double hu[7]; HuMoments(m, hu);
//    std::vector<double> out(7);
//    for (int i = 0; i < 7; ++i) {
//        double v = hu[i];
//        double a = std::abs(v);
//        if (a < 1e-30) a = 1e-30;
//        // log transform with sign preserved
//        out[i] = -std::copysign(1.0, v) * std::log10(a);
//    }
//    return out;
//}

//static double huDistance(const std::vector<double>& a, const std::vector<double>& b) {
//    double s = 0.0;
//    for (int i = 0; i < 7; ++i) {
//        double d = a[i] - b[i]; s += d*d;
//    }
//    return std::sqrt(s);
//}

//void onGrayMouse(int event, int x, int y, int flags, void* userdata) {
//    if (event == EVENT_LBUTTONDOWN) {
//        // left click: set template from patch centered at click
//        int half = g_templateSize/2;
//        int sx = std::max(0, x - half);
//        int sy = std::max(0, y - half);
//        if (sx + g_templateSize > g_imgGray.cols) sx = g_imgGray.cols - g_templateSize;
//        if (sy + g_templateSize > g_imgGray.rows) sy = g_imgGray.rows - g_templateSize;
//        Rect roi(sx, sy, g_templateSize, g_templateSize);
//        g_templateImg = g_imgGray(roi).clone();
//        g_hasTemplate = true;
//        g_templateRect = roi;
//        g_templateHu = computeHuLog(g_templateImg);
//        cout << "Template (Hu) set at: " << roi.x << "," << roi.y << " size " << roi.width << "\n";
//        Mat disp; cv::cvtColor(g_imgGray, disp, COLOR_GRAY2BGR);
//        rectangle(disp, roi, Scalar(255,255,255), 2);
//        imshow("GraySource", disp);
//    } else if (event == EVENT_RBUTTONDOWN) {
//        // right click: test candidate patch around click
//        if (!g_hasTemplate) {
//            cout << "No template set. Left-click to set template." << endl;
//            return;
//        }
//        int half = g_templateSize/2;
//        int sx = std::max(0, x - half);
//        int sy = std::max(0, y - half);
//        if (sx + g_templateSize > g_imgGray.cols) sx = g_imgGray.cols - g_templateSize;
//        if (sy + g_templateSize > g_imgGray.rows) sy = g_imgGray.rows - g_templateSize;
//        Rect roi(sx, sy, g_templateSize, g_templateSize);
//        Mat patch = g_imgGray(roi).clone();
//        std::vector<double> hu = computeHuLog(patch);
//        double dist = huDistance(g_templateHu, hu);
//        // map trackbar to threshold: scale trackbar 0..100 into reasonable range (0..5)
//        double thresh = (g_hu_threshold / 100.0) * 5.0;
//        bool match = (dist <= thresh);
//        Mat disp; cv::cvtColor(g_imgGray, disp, COLOR_GRAY2BGR);
//        // draw template rect
//        rectangle(disp, g_templateRect, Scalar(255,255,255), 2);
//        // draw candidate rect colored by match
//        drawResult(disp, roi, match);
//        // annotate
//        char buf[256]; sprintf(buf, "HuDist=%.3f thresh=%.3f", dist, thresh);
//        putText(disp, buf, Point(10,20), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255,255,0), 1);
//        imshow("GraySource", disp);
//        cout << "Candidate at " << roi.x <<","<<roi.y<<" HuDist="<<dist<<" thresh="<<thresh<<" -> " << (match?"MATCH":"NO") << endl;
//    }
//}

//int main(int argc, char** argv) {
//    if (argc < 2) {
//        cerr << "Usage: " << argv[0] << " <image>" << endl;
//        return -1;
//    }
//    Mat img = imread(argv[1]);
//    if (img.empty()) { cerr<<"Failed load"<<endl; return -1; }
//    cv::cvtColor(img, g_imgGray, COLOR_BGR2GRAY);
//    // optional: compute edges to focus on shape
////    cv::Canny(g_imgGray, g_imgGray, 93, 93 * 3);

//    namedWindow("GraySource", WINDOW_NORMAL);
//    imshow("GraySource", g_imgGray);
//    setMouseCallback("GraySource", onGrayMouse, nullptr);

//    // threshold trackbar for Hu distance
//    namedWindow("Controls", WINDOW_NORMAL);
//    createTrackbar("HuThresh", "Controls", &g_hu_threshold, 100);

//    cout << "Left-click to set template (patch). Right-click to test candidate using Hu-moments." << endl;

//    while (true) {
//        int k = waitKey(0);
//        if (k == 27) break;
//    }
//    return 0;
//}
