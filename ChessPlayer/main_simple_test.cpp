#include "chessDetector/ChessImageProcessing.h"
#include <iostream>
#include <vector>
#include <map>

using namespace cv;
using namespace std;

// --- GLOBAL STORAGE FOR SETUP ---
Mat img1, img2, warped1, warped2;
vector<Point2f> corners;
ChessImageProcessing* m_chessDetector;

void processAndDisplay(const MoveDetectParams& params) {
    if (img1.empty() || img2.empty()) return;
    std::vector<std::string> listMoves;
    listMoves = m_chessDetector->findPossibleMoves(img1, img2, params);
    for(std::string move: listMoves) {
        std::cout << "Possible move: " << move << std::endl;
    }

    // Visualize first detected move on the original (unwarped) image
    Mat vis = img2.clone();
    if (!listMoves.empty()) {
        // get transform matrix (maps source -> warped). We need inverse to map warped->source
        Mat T = m_chessDetector->getTranformMatrix();
        Mat Tinv;
        if (!T.empty()) cv::invert(T, Tinv);

        float sq = 640.0f / 8.0f; // warped square size used by detector

        for (const auto &mv : listMoves) {
            if (mv.size() >= 4) {
                string from = mv.substr(0,2);
                string to = mv.substr(2,2);
                Point fromCoord = m_chessDetector->notationToCoord(from, params.playerSide);
                Point toCoord = m_chessDetector->notationToCoord(to, params.playerSide);
                if (fromCoord.x >= 0 && toCoord.x >= 0) {
                    // centers in warped image
                    Point2f wp_from((fromCoord.x * sq) + sq/2.0f, (fromCoord.y * sq) + sq/2.0f);
                    Point2f wp_to((toCoord.x * sq) + sq/2.0f, (toCoord.y * sq) + sq/2.0f);
                    std::vector<Point2f> wpts{wp_from, wp_to};
                    std::vector<Point2f> srcpts(2);
                    if (!Tinv.empty()) cv::perspectiveTransform(wpts, srcpts, Tinv);
                    // draw start/end and arrow
                    circle(vis, srcpts[0], 8, Scalar(0,255,0), -1);
                    circle(vis, srcpts[1], 8, Scalar(0,0,255), -1);
                    arrowedLine(vis, srcpts[0], srcpts[1], Scalar(255,0,0), 3, LINE_AA, 0, 0.3);
                    // annotate notation
                    putText(vis, from + "->" + to, Point((int)srcpts[0].x+5, (int)srcpts[0].y-5), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255,255,255), 2);
                }
            } else if (mv.size() == 2) {
                // single cell notation: highlight only
                Point pt = m_chessDetector->notationToCoord(mv, params.playerSide);
                if (pt.x >= 0) {
                    Point2f wp((pt.x * sq) + sq/2.0f, (pt.y * sq) + sq/2.0f);
                    std::vector<Point2f> wpts{wp}; std::vector<Point2f> srcpts(1);
                    if (!Tinv.empty()) cv::perspectiveTransform(wpts, srcpts, Tinv);
                    circle(vis, srcpts[0], 10, Scalar(0,255,255), 3);
                }
            }
            // only display the first move visually for clarity
            break;
        }
    }
    imshow("Result", vis);
}

void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == EVENT_LBUTTONDOWN && corners.size() < 4) {
        corners.push_back(Point2f(x, y));
        circle(img1, Point(x, y), 5, Scalar(0, 255, 0), -1);
        imshow("Setup", img1);
        if (corners.size() == 4) {
            m_chessDetector->setCorners(corners[0].x,corners[0].y,
                    corners[1].x,corners[1].y,
                    corners[2].x,corners[2].y,
                    corners[3].x,corners[3].y);
            // run once with current controls
            MoveDetectParams p = m_chessDetector->readControlsFromWindow();
            processAndDisplay(p);
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <image1> <image2>" << endl;
        return -1;
    }

    img1 = imread(argv[1]);
    img2 = imread(argv[2]);
    if (img1.empty() || img2.empty()) {
        cerr << "Failed to load images" << endl;
        return -1;
    }
    cv::resize(img1,img1, cv::Size(1280,720));
    cv::resize(img2,img2, cv::Size(1280,720));
    m_chessDetector = new ChessImageProcessing();

    namedWindow("Setup");

    // create a Controls window via the detector helper, with defaults mapped from existing globals
    MoveDetectParams defaults;

    m_chessDetector->createControlsWindow(defaults);

    setMouseCallback("Setup", onMouse);
    imshow("Setup", img1);

    // Main loop: read controls each frame and re-run detection
    while (true) {
        int key = waitKey();
        if (key == 27) break; // ESC
        MoveDetectParams params = m_chessDetector->readControlsFromWindow();
        // Only process after corners set
        if (corners.size() == 4) {
            processAndDisplay(params);
        }
    }

    return 0;
}
