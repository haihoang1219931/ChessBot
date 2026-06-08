#include "chessDetector/ChessImageProcessing.h"
#include <iostream>
#include <vector>
#include <map>
#define DEBUG_SHOW_IMAGE
using namespace cv;
using namespace std;

void scaleUp(const cv::Mat& input, cv::Mat& output, int scale) {
    // Define your new dimensions
    cv::Size newSize(input.cols * scale, input.rows * scale);

    // Step 1: Scale using Nearest Neighbor to prevent blurring
    cv::resize(input, output, newSize, 0, 0, cv::INTER_NEAREST);
}
// --- GLOBAL STORAGE FOR SETUP ---
Mat img1, img2, warped1, warped2, display1, display2;
cv::Mat gray1, gray2, edges1, edges2;
vector<Point2f> corners;
ChessImageProcessing* m_chessDetector;
#define WARP_SIZE 640
void processAndDisplay() {
//    unsigned char imgData[64] = {
//        0,  0,  0,  0,  0,  0,  0,  0,
//        0,  0,  0,  0,  0,  0,  0,  0,
//        0,  0,255,  0,  0,  0,  0,  0,
//        0,255,  0,255,  0,  0,  0,  0,
//        0,  0,255,  0,  0,  0,  0,  0,
//        0,  0,  0,  0,  0,  0,  0,  0,
//        0,  0,  0,  0,  0,  0,  0,  0,
//        0,  0,  0,  0,  0,  0,  0,  0,
//    };
//    cv::Mat original = cv::Mat(8,8,CV_8UC1,imgData);
    vector < Point2f > dst {
        Point2f(0, 0), Point2f(WARP_SIZE - 1, 0), Point2f(WARP_SIZE - 1, WARP_SIZE - 1), Point2f(0, WARP_SIZE - 1)
    };
    MoveDetectParams params;
    Mat M = getPerspectiveTransform(corners, dst);
    warpPerspective(img1, warped1, M, Size(WARP_SIZE, WARP_SIZE));
    cv::cvtColor(warped1, gray1, cv::COLOR_BGR2GRAY);
    cv::Canny(gray1, edges1, 93, 93 * 3);
    int sq = edges1.cols / 8;
    std::vector<std::vector<int>> mat1 = m_chessDetector->getPieceMatrix(edges1, sq, params,"warp1");
//    int loop = 7;
//    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(loop*2+1, loop*2+1));
//    cv::morphologyEx(edges1, edges1, cv::MORPH_CLOSE, kernel);

//    cv::Point centerLargestArea;
//    cv::Mat matLargestArea = edges1.clone();
//    m_chessDetector->getCenterOfWhitePixels(matLargestArea,centerLargestArea);
//    cv::Mat scaled;
//    int scaleRatio = 1;
//    scaleUp(edges1,scaled,scaleRatio);
//    cv::Mat vis;
//    cv::cvtColor(scaled, vis, cv::COLOR_GRAY2BGR);

//    cv::circle(vis,
//               cv::Point(centerLargestArea.x*scaleRatio,
//                         centerLargestArea.y*scaleRatio),
//               5,cv::Scalar(0,0,255),2);
//    cv::imshow("scaled",vis);
}
void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == EVENT_LBUTTONDOWN && corners.size() < 4) {
        corners.push_back(Point2f(x, y));
        circle(display1, Point(x, y), 5, Scalar(0, 255, 0), -1);
        imshow("Setup", display1);
        if (corners.size() == 4) {
            processAndDisplay();
        }
    }
    if(event == EVENT_RBUTTONDOWN) {
        corners.clear();
        display1 = img1.clone();
        imshow("Setup", display1);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <image>" << endl;
        return -1;
    }

    img1 = imread(argv[1]);
    display1 = img1.clone();
    m_chessDetector = new ChessImageProcessing();
    imshow("Setup",img1);
    setMouseCallback("Setup", onMouse);
    cv::waitKey(0);
    return 0;
}
