#include "ChessImageProcessing.h"

ChessImageProcessing::ChessImageProcessing()
{
    m_sourceConnected = false;
    m_isBlackSide = true;
    m_chessBoardRow = 8;
    m_chessBoardBox = 60;
    m_chessBoardSize = m_chessBoardRow * m_chessBoardBox;
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
                               int threshold_val, int roi_percent,
                               int canny_low, int diff_thresh,

                               const std::string& playerSide)
{
    // No longer identify start/stop/occupied, just collect top 3 cells
    std::vector<std::string> listMoves;
    if (img_start.empty() || img_end.empty()) return listMoves;
    std::vector<cv::Point> top3cells;
    cv::Mat gray1, gray2, warped1, warped2, diff_bin, edges1, edges2;

    // warp image before calculation
    warpPerspective(img_start, warped1, m_transformMatrix, cv::Size(640, 640));
    warpPerspective(img_end, warped2, m_transformMatrix, cv::Size(640, 640));

    cvtColor(warped1, gray1, cv::COLOR_BGR2GRAY);
    cvtColor(warped2, gray2, cv::COLOR_BGR2GRAY);

    Canny(gray1, edges1, canny_low, canny_low * 3);
    Canny(gray2, edges2, canny_low, canny_low * 3);

    absdiff(gray1, gray2, diff_bin);
    threshold(diff_bin, diff_bin, diff_thresh, 255, cv::THRESH_BINARY);
    int sq = img_start.cols / 8;
    int sub = MAX(1, (sq * roi_percent) / 100);
    int off = (sq - sub) / 2;

    std::vector<std::pair<int, cv::Point>> topCells;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            cv::Rect roi(c * sq + off, r * sq + off, sub, sub);
            int diff_px = countNonZero(diff_bin(roi));
            topCells.push_back({diff_px, cv::Point(c, r)});
        }
    }
    sort(topCells.begin(), topCells.end(), [](const std::pair<int, cv::Point>& a, const std::pair<int, cv::Point>& b){ return a.first > b.first; });
    for (int i = 0; i < 3 && i < (int)topCells.size(); ++i) {
        top3cells.push_back(topCells[i].second);
    }
    // Output move string in chess notation for all 2-cell combinations from top 3 cells
    int n = (int)top3cells.size();
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i == j) continue;
            std::string fromStr = coordToNotation((top3cells)[i], playerSide);
            std::string toStr = coordToNotation((top3cells)[j], playerSide);
            if (!fromStr.empty() && !toStr.empty()) {
                listMoves.push_back(fromStr + toStr);
            }
        }
    }
    return listMoves;
}
