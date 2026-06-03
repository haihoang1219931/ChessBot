#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

// Workflow:
// 1) Click 4 corners on the original image window in clockwise order (or any consistent order).
// 2) The program warps the board to a top-down view (800x800) and shows the warped board.
// 3) Click on the warped image to sample a color to detect (HSV averaged in a small neighborhood).
// 4) Use trackbars to tune H/S/V tolerances and ROI percent (percent of cell used for matching).
// 5) The code splits the warped board into 8x8 cells and evaluates matches only inside each cell's center ROI.

static Mat img, warped, hsvWarp;
static vector<Point2f> srcCorners;
static bool warpedReady = false;

static Vec3b sampledHSV = Vec3b(0,0,0);
static bool hasSample = false;
static int hTol = 10;
static int sTol = 60;
static int vTol = 60;
static int roiPercent = 40; // percent of cell size used for ROI

const int WARP_SIZE = 800; // output warp will be WARP_SIZE x WARP_SIZE

static void drawCornersAndShow()
{
    Mat disp = img.clone();
    for (size_t i = 0; i < srcCorners.size(); ++i)
    {
        circle(disp, srcCorners[i], 6, Scalar(0,255,0), -1);
        putText(disp, to_string((int)i+1), srcCorners[i] + Point2f(6.f,-6.f), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0,255,0), 2);
    }
    imshow("Original", disp);
}

static void computeWarp()
{
    if (srcCorners.size() != 4) return;
    // Destination corners: top-left, top-right, bottom-right, bottom-left
    vector<Point2f> dst{Point2f(0,0), Point2f(WARP_SIZE-1,0), Point2f(WARP_SIZE-1,WARP_SIZE-1), Point2f(0,WARP_SIZE-1)};
    Mat M = getPerspectiveTransform(srcCorners, dst);
    warpPerspective(img, warped, M, Size(WARP_SIZE, WARP_SIZE));
    cvtColor(warped, hsvWarp, COLOR_BGR2HSV);
    warpedReady = true;
    imshow("Warped", warped);
}

static void updateAndShow(int /*val*/, void* /*userdata*/)
{
    if (!warpedReady)
    {
        // nothing to update, show original with corners
        drawCornersAndShow();
        return;
    }

    Mat display = warped.clone();
    Mat maskAll = Mat::zeros(warped.size(), CV_8UC1);

    if (hasSample)
    {
        int h = sampledHSV[0];
        int s = sampledHSV[1];
        int v = sampledHSV[2];

        int lowH = h - hTol; int highH = h + hTol;
        int lowS = max(0, s - sTol); int highS = min(255, s + sTol);
        int lowV = max(0, v - vTol); int highV = min(255, v + vTol);

        Mat mask;
        if (lowH < 0)
        {
            Mat m1, m2;
            inRange(hsvWarp, Scalar(0, lowS, lowV), Scalar(highH, highS, highV), m1);
            inRange(hsvWarp, Scalar(180 + lowH, lowS, lowV), Scalar(180, highS, highV), m2);
            bitwise_or(m1, m2, mask);
        }
        else if (highH > 180)
        {
            Mat m1, m2;
            inRange(hsvWarp, Scalar(lowH, lowS, lowV), Scalar(180, highS, highV), m1);
            inRange(hsvWarp, Scalar(0, lowS, lowV), Scalar(highH - 180, highS, highV), m2);
            bitwise_or(m1, m2, mask);
        }
        else
        {
            inRange(hsvWarp, Scalar(lowH, lowS, lowV), Scalar(highH, highS, highV), mask);
        }

        // clean mask
        Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3,3));
        morphologyEx(mask, mask, MORPH_OPEN, kernel);
        morphologyEx(mask, mask, MORPH_CLOSE, kernel);

        // Evaluate per-cell ROIs
        const int cellW = WARP_SIZE / 8;
        const int cellH = WARP_SIZE / 8;
        const int roiW = max(2, (cellW * roiPercent) / 100);
        const int roiH = max(2, (cellH * roiPercent) / 100);

        for (int r = 0; r < 8; ++r)
        {
            for (int c = 0; c < 8; ++c)
            {
                int cx = c * cellW + cellW/2;
                int cy = r * cellH + cellH/2;
                int x0 = cx - roiW/2; int y0 = cy - roiH/2;
                Rect roiRect(x0, y0, roiW, roiH);
                roiRect &= Rect(0,0,warped.cols, warped.rows);
                Mat roiMask = mask(roiRect);
                double nonZero = countNonZero(roiMask);
                double area = roiRect.width * roiRect.height;
                double frac = area > 0 ? nonZero / area : 0.0;
                // mark if fraction of matched pixels exceeds threshold
                const double FRAC_THRESH = 0.12; // tuneable
                Scalar boxColor = frac >= FRAC_THRESH ? Scalar(0,255,0) : Scalar(0,0,255);
                rectangle(display, roiRect, boxColor, 2);
                if (frac >= FRAC_THRESH)
                {
                    // label with simple mark
                    putText(display, "X", Point(roiRect.x + 4, roiRect.y + 14), FONT_HERSHEY_SIMPLEX, 0.6, boxColor, 2);
                }

                // accumulate mask for overlay visualization
                maskAll(roiRect) |= roiMask;
            }
        }

        // overlay matched ROIs semi-transparent
        Mat overlay;
        cvtColor(maskAll, overlay, COLOR_GRAY2BGR);
        overlay.setTo(Scalar(0,255,255), maskAll); // yellow overlay
        addWeighted(overlay, 0.4, display, 0.6, 0, display);

        // show sampled HSV
        stringstream ss; ss << "HSV(" << h << "," << s << "," << v << ")";
        putText(display, ss.str(), Point(10,20), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255,255,255), 2);
    }
    else
    {
        // draw grid to help selecting sample
        const int cellW = WARP_SIZE/8;
        const int cellH = WARP_SIZE/8;
        for (int i = 1; i < 8; ++i)
        {
            line(display, Point(i*cellW,0), Point(i*cellW,WARP_SIZE-1), Scalar(200,200,200), 1);
            line(display, Point(0,i*cellH), Point(WARP_SIZE-1,i*cellH), Scalar(200,200,200), 1);
        }
        putText(display, "Click on warped image to sample color (after corners are set)", Point(10,WARP_SIZE-10), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255,255,255), 1);
    }

    imshow("Warped", display);
    imshow("Mask", maskAll);
}

static void onOriginalMouse(int event, int x, int y, int flags, void* userdata)
{
    if (event != EVENT_LBUTTONDOWN) return;
    if (srcCorners.size() < 4)
    {
        srcCorners.emplace_back((float)x, (float)y);
        drawCornersAndShow();
        if (srcCorners.size() == 4)
        {
            computeWarp();
            updateAndShow(0, nullptr);
        }
    }
}

static void onWarpedMouse(int event, int x, int y, int flags, void* userdata)
{
    if (event != EVENT_LBUTTONDOWN) return;
    if (!warpedReady) return;
    if (x < 0 || x >= warped.cols || y < 0 || y >= warped.rows) return;
    // sample neighborhood in hsvWarp to reduce noise
    int r = 3;
    int x0 = max(0, x-r), x1 = min(warped.cols-1, x+r);
    int y0 = max(0, y-r), y1 = min(warped.rows-1, y+r);
    Mat roi = hsvWarp(Range(y0, y1+1), Range(x0, x1+1));
    Scalar avg = mean(roi);
    sampledHSV[0] = static_cast<uchar>(avg[0]);
    sampledHSV[1] = static_cast<uchar>(avg[1]);
    sampledHSV[2] = static_cast<uchar>(avg[2]);
    hasSample = true;
    updateAndShow(0, nullptr);
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        cout << "Usage: " << argv[0] << " <image_path>" << endl;
        return -1;
    }

    img = imread(argv[1], IMREAD_COLOR);
    if (img.empty())
    {
        cerr << "Failed to open image" << endl;
        return -1;
    }

    namedWindow("Original", WINDOW_NORMAL);
    namedWindow("Warped", WINDOW_NORMAL);
    namedWindow("Mask", WINDOW_NORMAL);

    setMouseCallback("Original", onOriginalMouse);
    setMouseCallback("Warped", onWarpedMouse);

    createTrackbar("H Tol", "Warped", &hTol, 90, updateAndShow);
    createTrackbar("S Tol", "Warped", &sTol, 255, updateAndShow);
    createTrackbar("V Tol", "Warped", &vTol, 255, updateAndShow);
    createTrackbar("ROI %", "Warped", &roiPercent, 100, updateAndShow);

    cout << "Instructions:\n - Click 4 corners on the Original image (clockwise or consistent order).\n"
         << " - After corners set, warped view appears. Click warped image to sample color.\n"
         << " - Adjust H/S/V tolerances and ROI% to refine detection.\n"
         << " - Press 'r' to reset corners, 'c' to clear sample, 'q' or ESC to quit." << endl;

    drawCornersAndShow();
    updateAndShow(0, nullptr);

    while (true)
    {
        int k = waitKey(10);
        if (k == 'q' || k == 27) break;
        if (k == 'r')
        {
            srcCorners.clear(); warpedReady = false; hasSample = false; sampledHSV = Vec3b(0,0,0);
            drawCornersAndShow(); imshow("Warped", Mat::zeros(WARP_SIZE,WARP_SIZE,CV_8UC3)); imshow("Mask", Mat::zeros(WARP_SIZE,WARP_SIZE,CV_8UC1));
        }
        if (k == 'c')
        {
            hasSample = false; sampledHSV = Vec3b(0,0,0); updateAndShow(0,nullptr);
        }
    }

    return 0;
}
