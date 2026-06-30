//#include <opencv2/opencv.hpp>

//#include <opencv2/imgproc.hpp>

//#include <iostream>

//#include <vector>

////#define CALIBRATION_COLOR
//using namespace cv;
//using namespace std;
//typedef enum {
//    BLACK,
//    WHITE,
//    YELLOW
//} COLOR_FILTER;
//static Mat img, warped, hsvWarp;
//static vector < Point2f > srcCorners;
//static bool warpedReady = false;

//static Vec3b sampledHSV = Vec3b(0, 0, 0);
//static bool hasSample = false;
//static int hTolSample = 55;
//static int sTolSample = 87;
//static int vTolSample = 12;
//static int roiPercentSample = 60;

//static int minWhitePercentSample = 10;
//static int maxBlackPercentSample = 25;

//// Calibration slots
//static Vec3b blackHSV = Vec3b(96,87,40);
////static Vec3b whiteHSV = Vec3b(85,44,145);
//static Vec3b whiteHSV = Vec3b(93,60,145);
//static Vec3b yellowHSV = Vec3b(25,81,181);

//#ifndef CALIBRATION_COLOR
//static bool blackCalibrated = true;
//static bool whiteCalibrated = true;
//static bool yellowCalibrated = true;
//#else
//static bool blackCalibrated = false;
//static bool whiteCalibrated = false;
//static bool yellowCalibrated = false;

//#endif
//static int hTolCalibrated[5] = {38, 49, 11};
//static int sTolCalibrated[5] = {60, 26, 85};
//static int vTolCalibrated[5] = {60, 131, 113};
//static int roiPercentCalibrated[5] = {71, 54, 69};

//static int minWhitePercentCalibrated[5] = {70,86,20};
//static int maxBlackPercentCalibrated[5] = {25,25,25};

//const int WARP_SIZE = 640;


//// Helper to print matrix properties cleanly to the console log
//static void printMatrix(const string & name,
//                        const vector < vector < int >> & mat) {
//    cout << "\n--- 8x8 Matrix: " << name << " ---" << endl;
//    for (int r = 0; r < 8; ++r) {
//        for (int c = 0; c < 8; ++c) {
//            cout << mat[r][c] << " ";
//        }
//        cout << endl;
//    }
//}

//static void drawCornersAndShow() {
//    Mat disp = img.clone();
//    for (size_t i = 0; i < srcCorners.size(); ++i) {
//        circle(disp, srcCorners[i], 6, Scalar(0, 255, 0), -1);
//        putText(disp, to_string((int) i + 1), srcCorners[i] + Point2f(6.f, -6.f), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0, 255, 0), 2);
//    }
//    imshow("Original", disp);
//}

//static void computeWarp() {
//    if (srcCorners.size() != 4) return;
//    vector < Point2f > dst {
//        Point2f(0, 0), Point2f(WARP_SIZE - 1, 0), Point2f(WARP_SIZE - 1, WARP_SIZE - 1), Point2f(0, WARP_SIZE - 1)
//    };
//    Mat M = getPerspectiveTransform(srcCorners, dst);
//    warpPerspective(img, warped, M, Size(WARP_SIZE, WARP_SIZE));
//    cvtColor(warped, hsvWarp, COLOR_BGR2HSV);
//    warpedReady = true;
//}

//// PARAMETERIZED API: Accepts an HSV target input and returns an 8x8 status matrix
//static vector < vector < int >> updateAndShow(Vec3b targetHSV,
//                                              int hTol, int sTol, int vTol,
//                                              int roiPercent, int minWhitePercent, int maxBlackPercent,
//                                              bool updateUI = true) {
//    vector < vector < int >> matrix(8, vector < int > (8, 0));
//    if (!warpedReady) return matrix;

//    Mat display = warped.clone();
//    Mat maskAll = Mat::zeros(warped.size(), CV_8UC1);

//    int h = targetHSV[0];
//    int s = targetHSV[1];
//    int v = targetHSV[2];

//    int lowH = h - hTol;
//    int highH = h + hTol;
//    int lowS = max(0, s - sTol);
//    int highS = min(255, s + sTol);
//    int lowV = max(0, v - vTol);
//    int highV = min(255, v + vTol);

//    Mat mask;
//    if (lowH < 0) {
//        Mat m1, m2;
//        inRange(hsvWarp, Scalar(0, lowS, lowV), Scalar(highH, highS, highV), m1);
//        inRange(hsvWarp, Scalar(180 + lowH, lowS, lowV), Scalar(180, highS, highV), m2);
//        bitwise_or(m1, m2, mask);
//    } else if (highH > 180) {
//        Mat m1, m2;
//        inRange(hsvWarp, Scalar(lowH, lowS, lowV), Scalar(180, highS, highV), m1);
//        inRange(hsvWarp, Scalar(0, lowS, lowV), Scalar(highH - 180, highS, highV), m2);
//        bitwise_or(m1, m2, mask);
//    } else {
//        inRange(hsvWarp, Scalar(lowH, lowS, lowV), Scalar(highH, highS, highV), mask);
//    }

//    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
//    morphologyEx(mask, mask, MORPH_OPEN, kernel);
//    morphologyEx(mask, mask, MORPH_CLOSE, kernel);

//    const int cellW = WARP_SIZE / 8;
//    const int cellH = WARP_SIZE / 8;
//    const int roiW = max(2, (cellW * roiPercent) / 100);
//    const int roiH = max(2, (cellH * roiPercent) / 100);

//    for (int r = 0; r < 8; ++r) {
//        for (int c = 0; c < 8; ++c) {
//            int cx = c * cellW + cellW / 2;
//            int cy = r * cellH + cellH / 2;
//            int x0 = cx - roiW / 2;
//            int y0 = cy - roiH / 2;
//            Rect roiRect(x0, y0, roiW, roiH);
//            roiRect &= Rect(0, 0, warped.cols, warped.rows);

//            Mat roiMask = mask(roiRect);
//            double whitePixels = countNonZero(roiMask);
//            double area = roiRect.width * roiRect.height;
//            double blackPixels = area - whitePixels;

//            double whiteFrac = area > 0 ? (whitePixels / area) : 0.0;
//            double blackFrac = area > 0 ? (blackPixels / area) : 0.0;

//            double minWhiteThresh = minWhitePercent / 100.0;
//            double maxBlackThresh = maxBlackPercent / 100.0;

//            // Logic: Mostly the selected input color backdrop containing a small edge profile/shadow
//            //      bool isColorDetected = (whiteFrac >= minWhiteThresh) && (blackFrac > 0.01) && (blackFrac <= maxBlackThresh);
//            bool isColorDetected = whiteFrac >= minWhiteThresh;
//            if (isColorDetected) {
//                matrix[r][c] = 1;
//            }

//            if (updateUI) {
//                Scalar boxColor = isColorDetected ? Scalar(0, 255, 0) : Scalar(0, 0, 255);
//                rectangle(display, roiRect, boxColor, 2);
//                if (isColorDetected) {
//                    putText(display, "MATCH", Point(roiRect.x + 2, roiRect.y + 16), FONT_HERSHEY_SIMPLEX, 0.4, boxColor, 1);
//                }
//                Mat maskAllRoi = maskAll(roiRect);
//                bitwise_or(maskAllRoi, roiMask, maskAllRoi);
//            }
//        }
//    }

//    if (updateUI) {
//        Mat overlay = Mat::zeros(warped.size(), CV_8UC3);
//        overlay.setTo(Scalar(0, 255, 255), maskAll);
//        addWeighted(overlay, 0.4, display, 0.6, 0, display);

//        stringstream ss;
//        ss << "Testing HSV(" << h << "," << s << "," << v << ")";
//        putText(display, ss.str(), Point(10, 20), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255, 255, 255), 2);
//        imshow("Warped", display);
//        imshow("Mask", maskAll);
//    }
//    printMatrix("matrix",matrix);
//    return matrix;
//}

//// Trackbar callback redirection helper
//static void onTrackbarChange(int, void * ) {
//    if (hasSample) updateAndShow(sampledHSV,
//                                 hTolSample,sTolSample,vTolSample,
//                                 roiPercentSample,minWhitePercentSample,maxBlackPercentSample,
//                                 true);
//}

//static void onOriginalMouse(int event, int x, int y, int flags, void * userdata) {
//    if (event != EVENT_LBUTTONDOWN) return;
//    if (srcCorners.size() < 4) {
//        srcCorners.emplace_back((float) x, (float) y);
//        drawCornersAndShow();
//        if (srcCorners.size() == 4) {
//            computeWarp();
//            updateAndShow(sampledHSV,
//                          hTolSample,sTolSample,vTolSample,
//                          roiPercentSample,minWhitePercentSample,maxBlackPercentSample,
//                          true);
//        }
//    }
//}

//static void onWarpedMouse(int event, int x, int y, int flags, void * userdata) {
//    if (event != EVENT_LBUTTONDOWN) return;
//    if (!warpedReady) return;
//    if (x < 0 || x >= warped.cols || y < 0 || y >= warped.rows) return;

//    int r = 3;
//    int x0 = max(0, x - r), x1 = min(warped.cols - 1, x + r);
//    int y0 = max(0, y - r), y1 = min(warped.rows - 1, y + r);
//    Mat roi = hsvWarp(Range(y0, y1 + 1), Range(x0, x1 + 1));
//    Scalar avg = mean(roi);
//    sampledHSV[0] = static_cast < uchar > (avg[0]);
//    sampledHSV[1] = static_cast < uchar > (avg[1]);
//    sampledHSV[2] = static_cast < uchar > (avg[2]);
//    hasSample = true;
//    updateAndShow(sampledHSV,
//                  hTolSample,sTolSample,vTolSample,
//                  roiPercentSample,minWhitePercentSample,maxBlackPercentSample,
//                  true);
//}

//int main(int argc, char ** argv) {
//    if (argc < 2) {
//        cout << "Usage: " << argv[0] << " <image_path>" << endl;
//        return -1;
//    }

//    img = imread(argv[1], IMREAD_COLOR);
//    if (img.empty()) {
//        cerr << "Failed to open image" << endl;
//        return -1;
//    }

//    namedWindow("Original", WINDOW_NORMAL);
//    namedWindow("Warped", WINDOW_NORMAL);
//    namedWindow("Mask", WINDOW_NORMAL);

//    setMouseCallback("Original", onOriginalMouse);
//    setMouseCallback("Warped", onWarpedMouse);

//    createTrackbar("H Tol", "Warped", & hTolSample, 90, onTrackbarChange);
//    createTrackbar("S Tol", "Warped", & sTolSample, 255, onTrackbarChange);
//    createTrackbar("V Tol", "Warped", & vTolSample, 255, onTrackbarChange);
//    createTrackbar("ROI %", "Warped", & roiPercentSample, 100, onTrackbarChange);
//    createTrackbar("Min White %", "Warped", & minWhitePercentSample, 100, onTrackbarChange);
//    createTrackbar("Max Black %", "Warped", & maxBlackPercentSample, 100, onTrackbarChange);

//    cout << "Instructions:\n" <<
//            " - Select 4 corners on 'Original'.\n" <<
//            " - Click anywhere on 'Warped' to track live color attributes.\n" <<
//            " - Press '1' to save live selection as BLACK cell color.\n" <<
//            " - Press '2' to save live selection as WHITE cell color.\n" <<
//            " - Press '3' to save live selection as YELLOW chess piece color.\n" <<
//            " - Press '4' to compute Silver chess pieces matrix & display on standalone frame.\n" <<
//            " - Press 'r' to clear corners, 'q' to exit.\n" << endl;

//    drawCornersAndShow();

//    while (true) {
//        char key = (char) waitKey(30);
//        if (key == 'q' || key == 27) break;
//        if (key == 'r') {
//            srcCorners.clear();
//            warpedReady = false;
//            hasSample = false;
//            drawCornersAndShow();
//        }

//        if (key == '1') // Calibrate Black cell
//        {
//            if (hasSample) {
//                blackHSV = sampledHSV;
//                hTolCalibrated[BLACK] = hTolSample;
//                sTolCalibrated[BLACK] = sTolSample;
//                vTolCalibrated[BLACK] = vTolSample;
//                roiPercentCalibrated[BLACK] = roiPercentSample;
//                minWhitePercentCalibrated[BLACK] = minWhitePercentSample;
//                maxBlackPercentCalibrated[BLACK] = maxBlackPercentSample;
//                blackCalibrated = true;
//                cout << "[CALIB] Saved Black Cells HSV: (" << (int) blackHSV[0] << "," << (int) blackHSV[1] << "," << (int) blackHSV[2] << ")"
//                     <<"["<< hTolSample << ","
//                     << sTolSample << ","
//                     << vTolSample << ","
//                     << roiPercentSample << ","
//                     << minWhitePercentSample << ","
//                     << maxBlackPercentSample << "]" << endl;
//            } else {
//                cout << "[WARN] Please click to sample a color first!" << endl;
//            }
//        }
//        if (key == '2') // Calibrate White cell
//        {
//            if (hasSample) {
//                whiteHSV = sampledHSV;
//                hTolCalibrated[WHITE] = hTolSample;
//                sTolCalibrated[WHITE] = sTolSample;
//                vTolCalibrated[WHITE] = vTolSample;
//                roiPercentCalibrated[WHITE] = roiPercentSample;
//                minWhitePercentCalibrated[WHITE] = minWhitePercentSample;
//                maxBlackPercentCalibrated[WHITE] = maxBlackPercentSample;
//                whiteCalibrated = true;
//                cout << "[CALIB] Saved White Cells HSV: (" << (int) whiteHSV[0] << "," << (int) whiteHSV[1] << "," << (int) whiteHSV[2] << ")"
//                     <<"["<< hTolSample << ","
//                     << sTolSample << ","
//                     << vTolSample << ","
//                     << roiPercentSample << ","
//                     << minWhitePercentSample << ","
//                     << maxBlackPercentSample << "]" << endl;
//            } else {
//                cout << "[WARN] Please click to sample a color first!" << endl;
//            }
//        }
//        if (key == '3') // Calibrate Yellow chess pieces
//        {
//            if (hasSample) {
//                yellowHSV = sampledHSV;
//                hTolCalibrated[YELLOW] = hTolSample;
//                sTolCalibrated[YELLOW] = sTolSample;
//                vTolCalibrated[YELLOW] = vTolSample;
//                roiPercentCalibrated[YELLOW] = roiPercentSample;
//                minWhitePercentCalibrated[YELLOW] = minWhitePercentSample;
//                maxBlackPercentCalibrated[YELLOW] = maxBlackPercentSample;
//                yellowCalibrated = true;
//                cout << "[CALIB] Saved Yellow Pieces HSV: (" << (int) yellowHSV[0] << "," << (int) yellowHSV[1] << "," << (int) yellowHSV[2] << ")"
//                     <<"["<< hTolSample << ","
//                     << sTolSample << ","
//                     << vTolSample << ","
//                     << roiPercentSample << ","
//                     << minWhitePercentSample << ","
//                     << maxBlackPercentSample << "]" << endl;
//            } else {
//                cout << "[WARN] Click to sample a piece profile on the warped view first!" << endl;
//            }
//        }
//        if (key == '4') // Logical processing step
//        {
//            if (!warpedReady) {
//                cout << "[ERROR] You must finish defining the 4 board corners first." << endl;
//                continue;
//            }
//            if (!blackCalibrated || !whiteCalibrated || !yellowCalibrated) {
//                cout << "[ERROR] Calibration profiling incomplete! Run steps '1' and '2' first." << endl;
//                continue;
//            }
//            // Run analysis loops completely independent of current live preview drawing
//            auto blackResult = updateAndShow(blackHSV,
//                                             hTolCalibrated[BLACK],sTolCalibrated[BLACK],vTolCalibrated[BLACK],
//                                             roiPercentCalibrated[BLACK],minWhitePercentCalibrated[BLACK],maxBlackPercentCalibrated[BLACK],
//                                             false);
//            auto whiteResult = updateAndShow(whiteHSV,
//                                             hTolCalibrated[WHITE],sTolCalibrated[WHITE],vTolCalibrated[WHITE],
//                                             roiPercentCalibrated[WHITE],minWhitePercentCalibrated[WHITE],maxBlackPercentCalibrated[WHITE],
//                                             false);
//            auto yellowResult = updateAndShow(yellowHSV,
//                                              hTolCalibrated[YELLOW],sTolCalibrated[YELLOW],vTolCalibrated[YELLOW],
//                                              roiPercentCalibrated[YELLOW],minWhitePercentCalibrated[YELLOW],maxBlackPercentCalibrated[YELLOW],
//                                              false);
//            printMatrix("Black Targets", blackResult);
//            printMatrix("White Targets", whiteResult);
//            printMatrix("Yellow Pieces", yellowResult);
//            // Inversion matrix configuration: NOT black board, NOT white board, NOT yellow piece = Silver piece
//            vector < vector < int >> silverResult(8, vector<int>(8, 0));
//            Mat silverDisplay = warped.clone();
//            const int cellW = WARP_SIZE / 8;
//            const int cellH = WARP_SIZE / 8;
//            const int roiW = max(2, (cellW * roiPercentSample) / 100);
//            const int roiH = max(2, (cellH * roiPercentSample) / 100);
//            for (int r = 0; r < 8; ++r) {
//                for (int c = 0; c < 8; ++c) {
//                    int cx = c * cellW + cellW / 2;
//                    int cy = r * cellH + cellH / 2;
//                    int x0 = cx - roiW / 2;
//                    int y0 = cy - roiH / 2;
//                    Rect roiRect(x0, y0, roiW, roiH);
//                    roiRect &= Rect(0, 0, warped.cols, warped.rows);
//                    if (blackResult[r][c] == 1) {
//                        rectangle(silverDisplay, roiRect, Scalar(0, 0, 0), 2); // Highlight bounding box
//                        putText(silverDisplay, "BLACK", Point(roiRect.x + 2, roiRect.y + 16), FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 0, 0), 1);
//                    }
//                    if (whiteResult[r][c] == 1) {
//                        rectangle(silverDisplay, roiRect, Scalar(255, 255, 255), 2); // Highlight bounding box
//                        putText(silverDisplay, "WHITE", Point(roiRect.x + 2, roiRect.y + 16), FONT_HERSHEY_SIMPLEX, 0.4, Scalar(255, 255, 255), 1);
//                    }
//                    if (yellowResult[r][c] == 1) {
//                        rectangle(silverDisplay, roiRect, Scalar(0, 255, 255), 2); // Highlight bounding box
//                        putText(silverDisplay, "YELLOW", Point(roiRect.x + 2, roiRect.y + 16), FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 255, 255), 1);
//                    }
//                    if (blackResult[r][c] == 0 && whiteResult[r][c] == 0 && yellowResult[r][c] == 0) {
//                        silverResult[r][c] = 1;
//                        rectangle(silverDisplay, roiRect, Scalar(255, 255, 0), 2); // Highlight bounding box
//                        putText(silverDisplay, "SILVER", Point(roiRect.x + 2, roiRect.y + 16), FONT_HERSHEY_SIMPLEX, 0.4, Scalar(255, 255, 0), 1);
//                    }
//                }
//            }
//            printMatrix("Silver Pieces Output Array", silverResult);
//            namedWindow("Silver Pieces", WINDOW_NORMAL);
//            imshow("Silver Pieces", silverDisplay);
//            cout << "[LOG] Pass computations complete. Processing grids printed above." << endl;
//        }
//    }
//    return 0;
//}
