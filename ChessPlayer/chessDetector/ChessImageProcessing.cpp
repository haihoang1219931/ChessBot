#include "ChessImageProcessing.h"

typedef enum {
    DETECT_MOVE_PHASE1_BINARY,
    DETECT_MOVE_PHASE2_SUBSTRACTION,
    DETECT_MOVE_PHASE3_CLASSIFICATION,
    DETECT_MOVE_VERIFY_RESULT,
    DETECT_MOVE_DONE_SUCCESS,
    DETECT_MOVE_DONE_FAIL,
} CHESSBOARD_DETECT_STATE;
ChessImageProcessing::ChessImageProcessing()
{
    m_sourceConnected = false;
    m_isBlackSide = true;
    m_chessBoardRow = 8;
    m_chessBoardBox = 60;
    m_chessBoardSize = m_chessBoardRow * m_chessBoardBox;
    m_transformMaxtrixValid = false;
}

void ChessImageProcessing::connectSource(char* source) {

}
cv::Mat ChessImageProcessing::getNewImageSide() {
    return cv::Mat();
}
bool ChessImageProcessing::detectSide(cv::Mat image) {
    return true;
}
bool ChessImageProcessing::isBlackSide() {
    return m_isBlackSide;
}
void ChessImageProcessing::setCorners(float topLeftX, float topLeftY,
                                      float topRightX, float topRightY,
                                      float bottomRightX, float bottomRightY,
                                      float bottomLeftX, float bottomLeftY)
{
    std::vector<cv::Point2f> corners;
    corners.push_back(cv::Point2f(topLeftX, topLeftY));
    corners.push_back(cv::Point2f(topRightX, topRightY));
    corners.push_back(cv::Point2f(bottomRightX, bottomRightY));
    corners.push_back(cv::Point2f(bottomLeftX, bottomLeftY));
    m_transformMatrix = getPerspectiveTransform(corners, std::vector<cv::Point2f>{{0,0},{640,0},{640,640},{0,640}});
    m_transformMaxtrixValid = true;
}
cv::Mat ChessImageProcessing::getTranformMatrix() {
    return m_transformMatrix;
}
int ChessImageProcessing::chessBoardBox() {
    return m_chessBoardBox;
}
int ChessImageProcessing::chessBoardRow() {
    return m_chessBoardRow;
}
int ChessImageProcessing::chessBoardSize() {
    return m_chessBoardSize;
}
void ChessImageProcessing::setThreshold(int threshold) {
    m_threshold = threshold;
}

// Helper: Convert board coordinates to chess notation
std::string ChessImageProcessing::coordToNotation(cv::Point pt, const std::string& playerSide)
{
    if (pt.x < 0 || pt.x > 7 || pt.y < 0 || pt.y > 7) return "";
    char file, rank;
    if (playerSide == "black") {
        file = 'a' + pt.x;
        rank = '8' - pt.y;
    } else { // black at bottom
        file = 'h' - pt.x;
        rank = '1' + pt.y;
    }
    return std::string(1, file) + std::string(1, rank);
}

cv::Point ChessImageProcessing::notationToCoord(const std::string& notation, const std::string& playerSide)
{
    // Validate input length
    if (notation.length() < 2) return cv::Point(-1, -1);

    char file = notation[0];
    char rank = notation[1];

    // Validate chess boundaries
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') return cv::Point(-1, -1);

    int x, y;

    if (playerSide == "black") {
        x = file - 'a';
        y = '8' - rank;
    } else { // white at bottom / black at top
        x = 'h' - file;
        y = rank - '1';
    }

    return cv::Point(x, y);
}
/**
 * @brief Pure function to find the best move between two board states.
 * @param playerSide: "white" or "black" at the bottom
 * @param moveStr: output move string in chess notation (e.g., e2e4)
 */
std::vector<std::string> ChessImageProcessing::findPossibleMoves(const cv::Mat& img_start, const cv::Mat& img_end,
                                                                 int threshold, int roiPercent, int cannyLow, int diffThresh,
                                                                 int pieceMinPoints, int pieceRoiPercent,
                                                                 const std::string& playerSide)
{
    // No longer identify start/stop/occupied, just collect top 3 cells
    std::vector<std::string> listMoves;
    if (img_start.empty() || img_end.empty() || !m_transformMaxtrixValid) return listMoves;
    std::vector<cv::Point> top3cells;
    cv::Mat gray1, gray2, warped1, warped2, diff_bin, edges1, edges2;

    // warp image before calculation
    warpPerspective(img_start, warped1, m_transformMatrix, cv::Size(640, 640));
    warpPerspective(img_end, warped2, m_transformMatrix, cv::Size(640, 640));    
    cvtColor(warped1, gray1, cv::COLOR_BGR2GRAY);
    cvtColor(warped2, gray2, cv::COLOR_BGR2GRAY);

    Canny(gray1, edges1, cannyLow, cannyLow * 3);
    Canny(gray2, edges2, cannyLow, cannyLow * 3);

    cv::Point start;
    std::vector<cv::Point> ends;
    detectMovePhase1Binary(edges1,edges2,
                           start,ends,
                           pieceMinPoints,pieceRoiPercent, cannyLow,
                           playerSide);

    std::vector<cv::Point> listChangedCell;
    detectMovePhase2Substraction(edges1,edges2,
                                 threshold,roiPercent,cannyLow,diffThresh,
                                 &listChangedCell,
                                 playerSide);

//    detectMovePhase3Classification();
    return listMoves;
}

bool ChessImageProcessing::detectMovePhase1Binary(const cv::Mat& edges1, const cv::Mat& edges2,
                                                  cv::Point& start, std::vector<cv::Point>& ends,
                                                  int min_points, int roi_percent, int canny_low, const std::string& playerSide)
{
    if (edges1.empty() || edges2.empty()) return false;
    int sq = edges1.cols / 8;
    std::vector<std::vector<int>> mat1 = getPieceMatrix(edges1, sq, min_points, roi_percent, canny_low,"warp1");
    std::vector<std::vector<int>> mat2 = getPieceMatrix(edges2, sq, min_points, roi_percent, canny_low,"warp2");

    comparePieceMatrices(mat1, mat2, start, ends);
    std::string startStr = coordToNotation(start, playerSide);
    std::cout << "Move start: " << startStr << std::endl;
    std::cout << "Possible ends: ";
    for (const auto& pt : ends) {
        std::cout << coordToNotation(pt, playerSide) << " ";
    }
    std::cout << std::endl;
#ifdef DEBUG_SHOW_IMAGE
    // --- Draw on output image ---
    cv::Mat out = edges1.clone();
    // Draw start position (red rectangle)
    if (start.x >= 0 && start.y >= 0) {
        cv::Rect box(start.x * sq, start.y * sq, sq, sq);
        cv::rectangle(out, box, cv::Scalar::all(255),3);
        putText(out, "FROM", box.tl() + cv::Point(5, 25), 1, 1.0, cv::Scalar::all(255), 2);
    }
    // Draw end positions (green rectangles)
    for (const auto& pt : ends) {
        cv::Rect box(pt.x * sq, pt.y * sq, sq, sq);
        cv::rectangle(out, box, cv::Scalar::all(255),3);
        putText(out, "TO", box.tl() + cv::Point(5, 25), 1, 1.0, cv::Scalar::all(255), 2);
    }
    imshow("Move Detection", out);
#endif
    if(start.x = -1 || start.y == -1 || ends.size() == 0) {
        return false;
    } else {
        return false;
    }
}

bool ChessImageProcessing::detectMovePhase2Substraction(const cv::Mat& gray1, const cv::Mat& gray2,
                                                        int threshold_val, int roi_percent,
                                                        int canny_low, int diff_thresh,
                                                        std::vector<cv::Point>* top3cells,
                                                        const std::string& playerSide)
{
    cv::Mat diff_bin;
    absdiff(gray1, gray2, diff_bin);
    threshold(diff_bin, diff_bin, diff_thresh, 255, cv::THRESH_BINARY);

    int sq = gray1.cols / 8;
    int sub = MAX(1, (sq * roi_percent) / 100);
    int off = (sq - sub) / 2;

    // Prepare visualization image (color) from diff for drawing counts
    cv::Mat vis;
    cv::cvtColor(diff_bin, vis, cv::COLOR_GRAY2BGR);

    // Compute count of non-zero pixels in each cell's ROI (centered)
    std::vector<std::tuple<int, int, int>> counts; // (count, col, row)
    counts.reserve(64);
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            int x = c * sq + off;
            int y = r * sq + off;
            cv::Rect roiRect(x, y, sub, sub);
            // clamp
            roiRect &= cv::Rect(0, 0, diff_bin.cols, diff_bin.rows);
            int diff_px = 0;
            if (roiRect.width > 0 && roiRect.height > 0)
                diff_px = countNonZero(diff_bin(roiRect));
            counts.emplace_back(diff_px, c, r);

            // draw small rectangle and count
            cv::Scalar col = diff_px > 0 ? cv::Scalar(0, 0, 255) : cv::Scalar(120, 120, 120);
            cv::rectangle(vis, roiRect, col, 1);
            std::string txt = std::to_string(diff_px);
            int font = cv::FONT_HERSHEY_SIMPLEX;
            double fs = 0.5;
            int thickness = 3;
            cv::Point textOrg(roiRect.x + 2, roiRect.y + std::max(12, roiRect.height/5));
            cv::putText(vis, txt, textOrg, font, fs, col, thickness);
        }
    }

    // Highlight center cell(s)
    int centerR = 3; int centerC = 3; // choose (3,3) as center-ish
    int cx = centerC * sq + off;
    int cy = centerR * sq + off;
    cv::Rect centerRect(cx, cy, sub, sub);
    centerRect &= cv::Rect(0,0,diff_bin.cols,diff_bin.rows);
    cv::rectangle(vis, centerRect, cv::Scalar(0,255,0), 2);
    cv::putText(vis, "CENTER", cv::Point(centerRect.x+2, centerRect.y+centerRect.height-2), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,255,0), 2);

    // Sort counts descending to get top changed cells
    std::sort(counts.begin(), counts.end(), [](const std::tuple<int,int,int>& a, const std::tuple<int,int,int>& b){
        return std::get<0>(a) > std::get<0>(b);
    });

    if (top3cells) {
        top3cells->clear();
        for (int i = 0; i < 3 && i < (int)counts.size(); ++i) {
            int cnt = std::get<0>(counts[i]);
            int c = std::get<1>(counts[i]);
            int r = std::get<2>(counts[i]);
            top3cells->push_back(cv::Point(c, r));
            // mark top cells with thicker rectangle
            int x = c * sq + off; int y = r * sq + off;
            cv::Rect roiRect(x, y, sub, sub);
            roiRect &= cv::Rect(0,0,diff_bin.cols,diff_bin.rows);
            cv::rectangle(vis, roiRect, cv::Scalar(255,0,0), 2);
            cv::putText(vis, "TOP", cv::Point(roiRect.x+2, roiRect.y+12), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255,0,0), 2);
        }
    }

#ifdef DEBUG_SHOW_IMAGE
    cv::imshow("diff_bin", diff_bin);
    cv::imshow("diff_counts", vis);
#else
    // always show the counts visualization to user as requested
    cv::imshow("diff_counts", vis);
#endif

    return true;
}

bool ChessImageProcessing::detectMovePhase3Classification()
{
    return true;
}
//    DETECT_MOVE_PHASE1_BINARY,
//    DETECT_MOVE_PHASE2_SUBSTRACTION,
//    DETECT_MOVE_PHASE3_CLASSIFICATION,
std::vector<std::vector<int>> ChessImageProcessing::getPieceMatrix(const cv::Mat& edges, int sq, int min_points, int roi_percent, int canny_low, std::string show_name) {
    cv::Mat edgesClone;
    edgesClone = edges.clone();
    std::vector<std::vector<int>> mat(8, std::vector<int>(8, 0));
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (isChessPieceCell(edges, c, r, sq, min_points, roi_percent, edgesClone)) {
                mat[r][c] = 1;
            }
        }
    }
    cv::imshow(show_name, edgesClone);
    return mat;
}

// Overload isChessPieceCell to allow passing max_bbox_percent (for use in getPieceMatrix)
bool ChessImageProcessing::isChessPieceCell(const cv::Mat& edges, int c, int r, int sq, int min_points, int roi_percent, cv::Mat& display) {
    int sub = MAX(1, (sq * roi_percent) / 100);
    int off = (sq - sub) / 2;
    cv::Rect roi(c * sq + off, r * sq + off, sub, sub);
    cv::Mat roiMat = edges(roi);
    int points = countNonZero(roiMat);
    if (points > min_points) {
        std::vector<cv::Point> nz;
        findNonZero(roiMat, nz);
        if (!nz.empty()) {
            rectangle(display,roi,cv::Scalar(255,255,255),2);
            char buffer[10];
            int value = nz.size();
            sprintf(buffer,"%d",value);
            putText(display, std::string(buffer) ,cv::Point(c * sq + sq/2,r * sq+ sq/2), 1, 1.5, cv::Scalar(255, 255, 255), 3);
            return true;
        }
    }
    return false;
}

// Compare two 8x8 matrices, return start (1->0) and list of possible ends (0->1)
void ChessImageProcessing::comparePieceMatrices(const std::vector<std::vector<int>>& mat1, const std::vector<std::vector<int>>& mat2, cv::Point& start, std::vector<cv::Point>& ends) {
    // Print mat1
    std::cout << "mat1 (before):" << std::endl;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            std::cout << mat1[r][c] << " ";
        }
        std::cout << std::endl;
    }
    // Print mat2
    std::cout << "mat2 (after):" << std::endl;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            std::cout << mat2[r][c] << " ";
        }
        std::cout << std::endl;
    }
    start = cv::Point(-1, -1);
    ends.clear();
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (mat1[r][c] == 1 && mat2[r][c] == 0) {
                start = cv::Point(c, r);
            } else if (mat1[r][c] == 0 && mat2[r][c] == 1) {
                ends.push_back(cv::Point(c, r));
            }
        }
    }
}

