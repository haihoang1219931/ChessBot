#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <random>

using namespace cv;
using namespace std;
namespace fs = std::filesystem;

// Global workspace structures
Mat srcImage, displayImage, resultImage, interactiveView;
vector<Point2f> originalCorners;
vector<Point> originalCenters;
double scaleFactor = 1.0;
int selectedCellIndex = -1; // Index of the cell clicked in the final view

// Maps shortcuts to folder names
const map<char, string> pieceFolders = {
    {'r', "white_rook"},   {'R', "black_rook"},
    {'n', "white_knight"}, {'N', "black_knight"},
    {'b', "white_bishop"}, {'B', "black_bishop"},
    {'q', "white_queen"},  {'Q', "black_queen"},
    {'k', "white_king"},   {'K', "black_king"},
    {'p', "white_pawn"},   {'P', "black_pawn"},
    {'e', "empty"}
};

// Callback for corner selection (Step 1)
void cornerMouseHandler(int event, int x, int y, int flags, void* param) {
    if (event == EVENT_LBUTTONDOWN && originalCorners.size() < 4) {
        float origX = (float)x / scaleFactor;
        float origY = (float)y / scaleFactor;
        originalCorners.push_back(Point2f(origX, origY));

        circle(displayImage, Point(x, y), 5, Scalar(0, 0, 255), -1);
        imshow("Select 4 Corners (Scaled)", displayImage);
    }
}

// Callback to select a drawn rectangle (Step 2)
void cropMouseHandler(int event, int x, int y, int flags, void* param) {
    if (event == EVENT_LBUTTONDOWN && !originalCenters.empty()) {
        // Map window-space coordinates back to high-res source space
        float origX = (float)x / scaleFactor;
        float origY = (float)y / scaleFactor;
        Point clickPoint(origX, origY);

        double minDst = 1e9;
        int bestIdx = -1;

        // Find the closest grid center within the 128x128 threshold bounds
        for (size_t i = 0; i < originalCenters.size(); ++i) {
            double dst = norm(clickPoint - originalCenters[i]);
            if (dst < minDst && dst <= 64.0) { // Max distance check within cell bounds
                minDst = dst;
                bestIdx = i;
            }
        }

        if (bestIdx != -1) {
            selectedCellIndex = bestIdx;

            // Draw UI highlight overlay
            interactiveView = resultImage.clone();
            Point center = originalCenters[selectedCellIndex];
            Rect highlightRect(center.x - 64, center.y - 64, 128, 128);

            // Draw thick yellow highlight box over selected cell
            rectangle(interactiveView, highlightRect, Scalar(0, 255, 255), 4);

            Mat viewResult;
            resize(interactiveView, viewResult, Size(), scaleFactor, scaleFactor, INTER_AREA);
            imshow("Dataset Editor: Click Center -> Press Key", viewResult);
            cout << "Selected cell center at: [" << center.x << ", " << center.y << "]. Press piece key..." << endl;
        }
    }
}

// Helper: Generates folders and resolves auto-incremented file names
string getNextFilePath(const string& folderName) {
    // 80% train / 20% validation random assignment logic
    static random_device rd;
    static mt19937 gen(rd());
    static uniform_real_distribution<> dis(0.0, 1.0);
    string split = (dis(gen) < 0.8) ? "train" : "val";

    fs::path targetDir = fs::path("chess_classifier") / "dataset" / split / folderName;
    fs::create_directories(targetDir);

    int maxNumber = -1;
    for (const auto& entry : fs::directory_iterator(targetDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".jpg") {
            string stem = entry.path().stem().string();
            try {
                int num = stoi(stem);
                if (num > maxNumber) maxNumber = num;
            } catch (...) {}
        }
    }

    int nextNumber = maxNumber + 1;
    stringstream ss;
    ss << setfill('0') << setw(4) << nextNumber << ".jpg";
    return (targetDir / ss.str()).string();
}

int main(int argc, char** argv) {
    CommandLineParser parser(argc, argv, "{keys||}{@input|chess.jpg|}");
    string inputPath = parser.get<string>("@input");

    srcImage = imread(inputPath);
    if (srcImage.empty()) {
        cout << "Error: Could not open source image: " << inputPath << endl;
        return -1;
    }

    // --- Monitor Fit Calculations ---
    int maxDisplayWidth = 1280, maxDisplayHeight = 720;
    double widthScale = (double)maxDisplayWidth / srcImage.cols;
    double heightScale = (double)maxDisplayHeight / srcImage.rows;
    scaleFactor = min({widthScale, heightScale, 1.0});

    resize(srcImage, displayImage, Size(), scaleFactor, scaleFactor, INTER_AREA);

    // Step 1: Manual corner mapping
    namedWindow("Select 4 Corners (Scaled)", WINDOW_AUTOSIZE);
    setMouseCallback("Select 4 Corners (Scaled)", cornerMouseHandler, NULL);
    cout << "Click the 4 corners: Top-Left -> Top-Right -> Bottom-Right -> Bottom-Left." << endl;
    imshow("Select 4 Corners (Scaled)", displayImage);
    waitKey(0);
    destroyWindow("Select 4 Corners (Scaled)");

    if (originalCorners.size() < 4) {
        cout << "Exiting: 4 valid points required." << endl;
        return -1;
    }

    // Compute transformations in raw image resolution spaces
    vector<Point2f> dstCorners = { Point2f(0, 0), Point2f(640, 0), Point2f(640, 640), Point2f(0, 640) };
    Mat transMatrix = getPerspectiveTransform(originalCorners, dstCorners);
    Mat invTransMatrix;
    invert(transMatrix, invTransMatrix);

    resultImage = srcImage.clone();
    int gridSize = 8;
    float cellSize = 640.0f / gridSize;

    // Track original pixel locations of centers
    for (int row = 0; row < gridSize; row++) {
        for (int col = 0; col < gridSize; col++) {
            float warpedCenterX = col * cellSize + (cellSize / 2.0f);
            float warpedCenterY = row * cellSize + (cellSize / 2.0f);

            vector<Point2f> warpedPt = { Point2f(warpedCenterX, warpedCenterY) };
            vector<Point2f> originalPt;
            perspectiveTransform(warpedPt, originalPt, invTransMatrix);

            Point orgCenter(cvRound(originalPt[0].x), cvRound(originalPt[0].y));
            originalCenters.push_back(orgCenter);

            // Canvas drawing (Full Resolution)
            Rect drawRect(orgCenter.x - 64, orgCenter.y - 64, 128, 128);
            rectangle(resultImage, drawRect, Scalar(0, 255, 0), 2);
            circle(resultImage, orgCenter, 3, Scalar(255, 0, 0), -1);
        }
    }

    interactiveView = resultImage.clone();
    namedWindow("Dataset Editor: Click Center -> Press Key", WINDOW_AUTOSIZE);
    setMouseCallback("Dataset Editor: Click Center -> Press Key", cropMouseHandler, NULL);

    cout << "\n--- INTERACTIVE CLASSIFIER MODE ---" << endl;
    cout << "1. Left-click inside any target box to select it." << endl;
    cout << "2. Press shortcut key to classify and export crop." << endl;
    cout << "   (Lowercase = White pieces | Uppercase = Black pieces)" << endl;
    cout << "3. Press [ESC] or [ENTER] when completely finished.\n" << endl;

    while (true) {
        Mat viewResult;
        resize(interactiveView, viewResult, Size(), scaleFactor, scaleFactor, INTER_AREA);
        imshow("Dataset Editor: Click Center -> Press Key", viewResult);

        int key = waitKey(0);
        if (key == 27 || key == 13) break; // ESC or Enter exits main runtime execution

        char pressedChar = (char)key;
        if (selectedCellIndex != -1 && pieceFolders.count(pressedChar)) {
            Point center = originalCenters[selectedCellIndex];

            // Extract a bounding-safe 128x128 crop out of raw source matrix space
            Rect cropRegion(center.x - 64, center.y - 64, 128, 128);

            // Prevent out-of-boundary memory access issues
            cropRegion &= Rect(0, 0, srcImage.cols, srcImage.rows);

            if (cropRegion.width == 128 && cropRegion.height == 128) {
                Mat cellCrop = srcImage(cropRegion);
                string outPath = getNextFilePath(pieceFolders.at(pressedChar));

                imwrite(outPath, cellCrop);
                cout << "Saved high-res crop to: " << outPath << endl;

                // Provide a green success flash loop on the current bounding box
                rectangle(resultImage, cropRegion, Scalar(0, 255, 0), -1); // fill old area
                interactiveView = resultImage.clone();
                selectedCellIndex = -1; // Clear selection context
            } else {
                cout << "Warning: Bounding box falls outside image border. Crop skipped." << endl;
            }
        }
    }

    destroyAllWindows();
    return 0;
}
