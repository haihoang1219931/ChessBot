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

void processAndDisplay(const MoveDetectParams& params, const string& playerSide = "white") {
    if (img1.empty() || img2.empty()) return;
    std::vector<std::string> listMoves;
    listMoves = m_chessDetector->findPossibleMoves(img1, img2, params, playerSide);
    for(std::string move: listMoves) {
        std::cout << "Possible move: " << move << std::endl;
    }
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
