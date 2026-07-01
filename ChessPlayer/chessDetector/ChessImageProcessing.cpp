#include "ChessImageProcessing.h"

const int WARP_SIZE = 640;

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

static void printMatrix(const std::string & name,
                        const std::vector < std::vector < int >> & mat) {
    std::cout << "\n--- 8x8 Matrix: " << name << " ---" << std::endl;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            std::cout << mat[r][c] << " ";
        }
        std::cout << std::endl;
    }
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
    m_transformMatrix = getPerspectiveTransform(corners, std::vector<cv::Point2f>{{0,0},{WARP_SIZE,0},{WARP_SIZE,WARP_SIZE},{0,WARP_SIZE}});
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
                                                                  const MoveDetectParams& params)
{
    // No longer identify start/stop/occupied, just collect top 3 cells
    std::vector<std::string> listMoves;
    if (img_start.empty() || img_end.empty() || !m_transformMaxtrixValid) return listMoves;
    std::vector<cv::Point> top3cells;
    cv::Mat gray1, gray2, warped1, warped2, edges1, edges2;
    std::vector<cv::Point> startCellsBinary, startCellsColor;
    std::vector<cv::Point> endsCellsBinary, endCellsColor;
    std::vector<std::vector<int>> matColorMap1(8, std::vector<int>(8, 0));
    std::vector<std::vector<int>> matColorMap2(8, std::vector<int>(8, 0));
    // warp image before calculation (also keep color warped images for color-matching)
    cv::warpPerspective(img_start, warped1, m_transformMatrix, cv::Size(WARP_SIZE, WARP_SIZE));
    cv::warpPerspective(img_end, warped2, m_transformMatrix, cv::Size(WARP_SIZE, WARP_SIZE));
    cv::cvtColor(warped1, gray1, cv::COLOR_BGR2GRAY);
    cv::cvtColor(warped2, gray2, cv::COLOR_BGR2GRAY);
//    cv::imshow("color1",img_start);
//    cv::imshow("color2",img_end);
//    cv::imshow("warp1",warped1);
//    cv::imshow("warp2",warped2);

    // 1) detect start cell and candidate end cells
    cv::Canny(gray1, edges1, params.canny_low, params.canny_low * 3);
    cv::Canny(gray2, edges2, params.canny_low, params.canny_low * 3);
    detectMovePhase1Binary(edges1, edges2,
                           startCellsBinary, endsCellsBinary,
                           params);

    detectMovePhase1ColorFilter(warped1, warped2,
                           startCellsColor, endCellsColor,
                           params,
                           matColorMap1,
                           matColorMap2);

    std::vector<cv::Point> listChangedCell;
    detectMovePhase2Substraction(gray1, gray2, params, listChangedCell);
//    listMoves = detectMovePhase3ColorMatching(warped1, warped2, startCells, listChangedCell, params);
    if(startCellsBinary.size() == 0) {
        listMoves = detectMovePhase3ColorMatchingFromFilter(startCellsColor, listChangedCell, params,
                                                        matColorMap1, matColorMap2);
    } else if(startCellsBinary.size() == 1) {
        listMoves = detectMovePhase3ColorMatchingFromFilter(startCellsBinary, listChangedCell, params,
                                                        matColorMap1, matColorMap2);
    } else {
        if(startCellsColor.size() == 1)
        listMoves = detectMovePhase3ColorMatchingFromFilter(startCellsColor, listChangedCell, params,
                                                        matColorMap1, matColorMap2);
    }

    return listMoves;
}

bool ChessImageProcessing::detectMovePhase1Binary(const cv::Mat& edges1, const cv::Mat& edges2,
                                                  std::vector<cv::Point>& starts, std::vector<cv::Point>& ends,
                                                  const MoveDetectParams& params)
{
    if (edges1.empty() || edges2.empty()) return false;
    int sq = edges1.cols / 8;
    std::vector<std::vector<int>> mat1 = getPieceMatrix(edges1, sq, params,"warp1");
    std::vector<std::vector<int>> mat2 = getPieceMatrix(edges2, sq, params,"warp2");

    comparePieceMatrices(mat1, mat2, starts, ends);
    for(cv::Point start: starts) {
        std::cout << "Possible start: " << start << std::endl;
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
    }
    // Return true only when we have a valid start and at least one end.
    // NOTE: fixed '=' -> '==' bug so 'start' value is not overwritten.
    if (starts.empty()) {
        return false;
    } else {
        return true;
    }
}

bool ChessImageProcessing::detectMovePhase1ColorFilter(const cv::Mat& color1, const cv::Mat& color2,
                                                  std::vector<cv::Point>& starts, std::vector<cv::Point>& ends,
                                                  const MoveDetectParams& params,
                                                  std::vector<std::vector<int>>& matColorMapBefore,
                                                  std::vector<std::vector<int>>& matColorMapAfter)
{
    if (color1.empty() || color2.empty()) return false;
    int sq = color1.cols / 8;
    matColorMapBefore = getPieceMatrixColor(color1,params,"warp1");
    matColorMapAfter = getPieceMatrixColor(color2,params,"warp2");

    comparePieceMatrices(matColorMapBefore, matColorMapAfter, starts, ends);
    for(cv::Point start: starts) {
        std::cout << "Possible start: " << start << std::endl;
#ifdef DEBUG_SHOW_IMAGE
        // --- Draw on output image ---
        cv::Mat out = color1.clone();
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
    }
    // Return true only when we have a valid start and at least one end.
    // NOTE: fixed '=' -> '==' bug so 'start' value is not overwritten.
    if (starts.empty()) {
        return false;
    } else {
        return true;
    }
}

bool ChessImageProcessing::detectMovePhase2Substraction(const cv::Mat& img_start, const cv::Mat& img_end,
                                                        const MoveDetectParams& params,
                                                        std::vector<cv::Point>& top3cells)
{
    // use passed images (expected to be warped/grayscale or edge images)
    const cv::Mat& gray1 = img_start;
    const cv::Mat& gray2 = img_end;
    if (gray1.empty() || gray2.empty()) return false;

    cv::Mat diff_bin;
    cv::absdiff(gray1, gray2, diff_bin);
#ifdef DEBUG_SHOW_IMAGE
    cv::imshow("diff_bin_gray",diff_bin);
#endif
    cv::threshold(diff_bin, diff_bin, params.diff_thresh, 255, cv::THRESH_BINARY);
    int sq = gray1.cols / 8;
    int sub = std::max(1, (sq * params.roi_percent) / 100);
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
                diff_px = cv::countNonZero(diff_bin(roiRect));
            counts.emplace_back(diff_px, c, r);
#ifdef DEBUG_SHOW_IMAGE
            // draw small rectangle and count
            cv::Scalar col = diff_px > 0 ? cv::Scalar(0, 0, 255) : cv::Scalar(120, 120, 120);
            cv::rectangle(vis, roiRect, col, 1);
            std::string txt = std::to_string(diff_px);
            int font = cv::FONT_HERSHEY_SIMPLEX;
            double fs = 0.5;
            int thickness = 1;
            cv::Point textOrg(roiRect.x + 2, roiRect.y + std::max(12, roiRect.height/5));
            cv::putText(vis, txt, textOrg, font, fs, col, thickness);
#endif
        }
    }
#ifdef DEBUG_SHOW_IMAGE
    // Highlight center cell(s)
    int centerR = 3; int centerC = 3; // choose (3,3) as center-ish
    int cx = centerC * sq + off;
    int cy = centerR * sq + off;
    cv::Rect centerRect(cx, cy, sub, sub);
    centerRect &= cv::Rect(0,0,diff_bin.cols,diff_bin.rows);
    cv::rectangle(vis, centerRect, cv::Scalar(0,255,0), 2);
    cv::putText(vis, "CENTER", cv::Point(centerRect.x+2, centerRect.y+centerRect.height-2), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,255,0), 2);
#endif
    // Sort counts descending to get top changed cells
    std::sort(counts.begin(), counts.end(), [](const std::tuple<int,int,int>& a, const std::tuple<int,int,int>& b){
        return std::get<0>(a) > std::get<0>(b);
    });

    top3cells.clear();
    for (int i = 0; i < 3 && i < (int)counts.size(); ++i) {
        int cnt = std::get<0>(counts[i]);
        int c = std::get<1>(counts[i]);
        int r = std::get<2>(counts[i]);
        top3cells.push_back(cv::Point(c, r));
#ifdef DEBUG_SHOW_IMAGE
        // mark top cells with thicker rectangle
        int x = c * sq + off; int y = r * sq + off;
        cv::Rect roiRect(x, y, sub, sub);
        roiRect &= cv::Rect(0,0,diff_bin.cols,diff_bin.rows);
        cv::rectangle(vis, roiRect, cv::Scalar(255,0,0), 2);
        std::string txt = "TOP:" + std::to_string(cnt);
        cv::putText(vis, txt, cv::Point(roiRect.x+2, roiRect.y+12), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255,255,0), 2);
#endif
    }

#ifdef DEBUG_SHOW_IMAGE
    cv::imshow("diff_bin", diff_bin);
    cv::imshow("diff_counts", vis);
#endif

    return true;
}

std::vector<std::string> ChessImageProcessing::detectMovePhase3ColorMatching(const cv::Mat& warped1, const cv::Mat& warped2,
                                                                             const std::vector<cv::Point>& startCells, std::vector<cv::Point> listChangedCell,
                                                       const MoveDetectParams& params) {
    std::vector<std::string> listMoves;
    for (int i=0; i< listChangedCell.size(); i++) {
        std::cout << "detectMovePhase2Substraction: end " << listChangedCell[i] << std::endl;
    }
    cv::Point startCell(-1,-1);
    bool foundValidStartCell = false;
    for(cv::Point tmpStartCell: startCells) {
        for(cv::Point tmpMoveCell: listChangedCell) {
            if(tmpStartCell.x == tmpMoveCell.x && tmpStartCell.y == tmpMoveCell.y) {
                startCell.x = tmpMoveCell.x;
                startCell.y = tmpMoveCell.y;
                foundValidStartCell = true;
                break;
            }
        }
        if(foundValidStartCell) break;
    }
    // 2) Exclude the start cell from changed-cell candidates (if present)
    if (startCell.x >= 0 && startCell.y >= 0 && !listChangedCell.empty()) {
        for (int i=0; i< listChangedCell.size(); i++) {
            if (listChangedCell[i].x == startCell.x && listChangedCell[i].y == startCell.y) {
                listChangedCell.erase(listChangedCell.begin() + i);
                break;
            }
        }
    }

    for(cv::Point filterCell: listChangedCell) {
        std::cout << "end cell [" << filterCell << "]" << std::endl;
    }
    // 3) If we have a start from phase1 and remaining candidates, try color matching
    if (startCell.x >= 0 && startCell.y >= 0 && !listChangedCell.empty()) {
        // read interactive colorThreshold from Controls window (default created with 30)
        int colorThresh = cv::getTrackbarPos("colorThreshold", "Controls");
        double colorThreshold = static_cast<double>(colorThresh);
        std::vector<cv::Point> matches = matchStartToCandidates(warped1, warped2,
                                                 startCell, listChangedCell,
                                                 params, colorThreshold);
        if (matches.size() >= 0) {
            std::string from = coordToNotation(startCell, params.playerSide);
            for(cv::Point match: matches) {
                std::string to = coordToNotation(match, params.playerSide);
                std::cout << "Player:" << params.playerSide
                          << " Color match: " << from << " -> " << to << std::endl;
                // build a move string and return as candidate
                listMoves.push_back(from + to);
            }
        } else {
            std::cout << "No color match found for start cell " << coordToNotation(startCell, params.playerSide) << std::endl;
        }
    } else if (!listChangedCell.empty()) {
        // No binary start found; if only changed cells remain, return their notations as possible moves
        for (const auto& pt : listChangedCell) {
            listMoves.push_back(coordToNotation(pt, params.playerSide));
        }
    }
    return listMoves;
}

std::vector<std::string> ChessImageProcessing::detectMovePhase3ColorMatchingFromFilter(const std::vector<cv::Point>& startCells,
                                std::vector<cv::Point> listChangedCell,
                                const MoveDetectParams& params,
                                const std::vector<std::vector<int>> matColorMapBefore,
                                const std::vector<std::vector<int>> matColorMapAfter)
{
    std::vector<std::string> listMoves;
    for (int i=0; i< listChangedCell.size(); i++) {
        std::cout << "detectMovePhase2Substraction: end " << listChangedCell[i] << std::endl;
    }
    cv::Point startCell(-1,-1);
    bool foundValidStartCell = false;
    for(cv::Point tmpStartCell: startCells) {
        for(cv::Point tmpMoveCell: listChangedCell) {
            if(tmpStartCell.x == tmpMoveCell.x && tmpStartCell.y == tmpMoveCell.y) {
                startCell.x = tmpMoveCell.x;
                startCell.y = tmpMoveCell.y;
                foundValidStartCell = true;
                break;
            }
        }
        if(foundValidStartCell) break;
    }
    // 2) Exclude the start cell from changed-cell candidates (if present)
    if (startCell.x >= 0 && startCell.y >= 0 && !listChangedCell.empty()) {
        for (int i=0; i< listChangedCell.size(); i++) {
            if (listChangedCell[i].x == startCell.x && listChangedCell[i].y == startCell.y) {
                listChangedCell.erase(listChangedCell.begin() + i);
                break;
            }
        }
    }

    for(cv::Point filterCell: listChangedCell) {
        std::cout << "end cell [" << filterCell << "]" << std::endl;
    }
    // 3) If we have a start from phase1 and remaining candidates, try color matching
    if (startCell.x >= 0 && startCell.y >= 0 && !listChangedCell.empty()) {
        std::string from = coordToNotation(startCell, params.playerSide);
        for(cv::Point filterCell: listChangedCell) {
            std::cout << "filterCell y:" << filterCell.y << " x:" << filterCell.x << " v:" << matColorMapAfter[filterCell.y][filterCell.x] << std::endl;
            if(matColorMapAfter[filterCell.y][filterCell.x] != 0 &&
                    matColorMapBefore[filterCell.y][filterCell.x] == 0) {
                std::string to = coordToNotation(filterCell, params.playerSide);
                std::cout << "Player:" << params.playerSide
                          << " Color match: " << from << " -> " << to << std::endl;
                // build a move string and return as candidate
                listMoves.push_back(from + to);
            }
        }
    } else if (!listChangedCell.empty()) {
        // No binary start found; if only changed cells remain, return their notations as possible moves
        for (const auto& pt : listChangedCell) {
            listMoves.push_back(coordToNotation(pt, params.playerSide));
        }
    }
    return listMoves;
}

std::vector < std::vector < int >> ChessImageProcessing::cellColorFilterToMatrix(const cv::Mat& colorWarped, cv::Vec3b targetHSV,
                                              int hTol, int sTol, int vTol,
                                              int roiPercent, int minWhitePercent, int maxBlackPercent,
                                              std::string name)
{
    std::vector < std::vector < int >> matrix(8, std::vector < int > (8, 0));

    cv::Mat display = colorWarped.clone();
    cv::Mat maskAll = cv::Mat::zeros(colorWarped.size(), CV_8UC1);

    int h = targetHSV[0];
    int s = targetHSV[1];
    int v = targetHSV[2];

    int lowH = h - hTol;
    int highH = h + hTol;
    int lowS = std::max(0, s - sTol);
    int highS = std::min(255, s + sTol);
    int lowV = std::max(0, v - vTol);
    int highV = std::min(255, v + vTol);

    cv::Mat mask;
    if (lowH < 0) {
        cv::Mat m1, m2;
        inRange(colorWarped, cv::Scalar(0, lowS, lowV), cv::Scalar(highH, highS, highV), m1);
        inRange(colorWarped, cv::Scalar(180 + lowH, lowS, lowV), cv::Scalar(180, highS, highV), m2);
        bitwise_or(m1, m2, mask);
    } else if (highH > 180) {
        cv::Mat m1, m2;
        inRange(colorWarped, cv::Scalar(lowH, lowS, lowV), cv::Scalar(180, highS, highV), m1);
        inRange(colorWarped, cv::Scalar(0, lowS, lowV), cv::Scalar(highH - 180, highS, highV), m2);
        bitwise_or(m1, m2, mask);
    } else {
        inRange(colorWarped, cv::Scalar(lowH, lowS, lowV), cv::Scalar(highH, highS, highV), mask);
    }

    cv::Mat kernel = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);

    const int cellW = WARP_SIZE / 8;
    const int cellH = WARP_SIZE / 8;
    const int roiW = cv::max(2, (cellW * roiPercent) / 100);
    const int roiH = cv::max(2, (cellH * roiPercent) / 100);

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            int cx = c * cellW + cellW / 2;
            int cy = r * cellH + cellH / 2;
            int x0 = cx - roiW / 2;
            int y0 = cy - roiH / 2;
            cv::Rect roiRect(x0, y0, roiW, roiH);
            roiRect &= cv::Rect(0, 0, colorWarped.cols, colorWarped.rows);

            cv::Mat roiMask = mask(roiRect);
            double whitePixels = countNonZero(roiMask);
            double area = roiRect.width * roiRect.height;
            double blackPixels = area - whitePixels;

            double whiteFrac = area > 0 ? (whitePixels / area) : 0.0;
            double blackFrac = area > 0 ? (blackPixels / area) : 0.0;

            double minWhiteThresh = minWhitePercent / 100.0;
            double maxBlackThresh = maxBlackPercent / 100.0;

            // Logic: Mostly the selected input color backdrop containing a small edge profile/shadow
            //      bool isColorDetected = (whiteFrac >= minWhiteThresh) && (blackFrac > 0.01) && (blackFrac <= maxBlackThresh);
            bool isColorDetected = whiteFrac >= minWhiteThresh;
            if (isColorDetected) {
                matrix[r][c] = 1;
            }
#ifdef DEBUG_SHOW_IMAGE
            cv::Scalar boxColor = isColorDetected ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
            rectangle(display, roiRect, boxColor, 2);
            if (isColorDetected) {
                putText(display, "MATCH", cv::Point(roiRect.x + 2, roiRect.y + 16), cv::FONT_HERSHEY_SIMPLEX, 0.4, boxColor, 1);
            }
            cv::Mat maskAllRoi = maskAll(roiRect);
            bitwise_or(maskAllRoi, roiMask, maskAllRoi);
#endif
        }
    }
#ifdef DEBUG_SHOW_IMAGE
    cv::Mat overlay = cv::Mat::zeros(colorWarped.size(), CV_8UC3);
    overlay.setTo(cv::Scalar(0, 255, 255), maskAll);
    addWeighted(overlay, 0.4, display, 0.6, 0, display);

    std::stringstream ss;
    ss << "Testing HSV(" << h << "," << s << "," << v << ")";
    putText(display, ss.str(), cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    imshow("Warped"+name, display);
#endif
    return matrix;
}

bool ChessImageProcessing::detectMovePhase3Classification()
{
    return true;
}
//    DETECT_MOVE_PHASE1_BINARY,
//    DETECT_MOVE_PHASE2_SUBSTRACTION,
//    DETECT_MOVE_PHASE3_CLASSIFICATION,

std::vector<std::vector<int>> ChessImageProcessing::getPieceMatrix(const cv::Mat& edges, int sq,
                                                                   const MoveDetectParams& params, std::string show_name) {
    cv::Mat edgesClone;

    std::vector<std::vector<int>> mat(8, std::vector<int>(8, 0));
    // 1. Create a structural element (kernel size 3x3 or 5x5)
//    for(int loop = 0; loop < params.numLoopCheckPiece; loop++)
    {

        int loop = 1;
        cv::Mat closed;
        if(loop == 0) {
            closed = edges.clone();
        } else {
            cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(loop*2+1, loop*2+1));
            // 2. Apply Closing (Dilation then Erosion) to bridge gaps
            cv::morphologyEx(edges, closed, cv::MORPH_CLOSE, kernel);
#ifdef DEBUG_SHOW_IMAGE
            cv::cvtColor(closed, edgesClone, cv::COLOR_GRAY2BGR);
#endif
        }

        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
//                if(r!=6 || c!= 3) continue;
                if (isChessPieceCell(closed, c, r, sq, params.pieceMinPoints, params.roi_percent, edgesClone)) {
                    mat[r][c]++;
                }
            }
        }
    }
//    cv::Mat edgesClone2;
//    edgesClone2 = edges.clone();
//    // 1. Create a structural element (kernel size 3x3 or 5x5)
//    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));

//    // 2. Apply Closing (Dilation then Erosion) to bridge gaps
//    cv::Mat closed;
//    cv::morphologyEx(edges, edgesClone2, cv::MORPH_CLOSE, kernel);
//    std::vector<std::vector<cv::Point>> contours;
//    cv::findContours(edgesClone2, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

//    for (const auto& contour : contours) {
//        // Filter out tiny noise contours
//        if (contour.size() < 20) {
//            continue;
//        }

//        int N = contour.size();

//        // 3. Formulate the Algebraic Least-Squares system: A * X = B
//        // Matrix A size: (N x 3), Matrix B size: (N x 1)
//        cv::Mat A(N, 3, CV_64F);
//        cv::Mat B(N, 1, CV_64F);

//        for (int i = 0; i < N; ++i) {
//            double x = contour[i].x;
//            double y = contour[i].y;

//            A.at<double>(i, 0) = x;
//            A.at<double>(i, 1) = y;
//            A.at<double>(i, 2) = 1.0;

//            B.at<double>(i, 0) = x * x + y * y;
//        }

//        // Solve for parameters vector X = [c1, c2, c3]^T using pseudo-inverse method
//        cv::Mat X;
//        cv::solve(A, B, X, cv::DECOMP_SVD);

//        double c1 = X.at<double>(0, 0);
//        double c2 = X.at<double>(1, 0);
//        double c3 = X.at<double>(2, 0);

//        // Extract center coordinates and radius from parameters
//        int xc = cvRound(c1 / 2.0);
//        int yc = cvRound(c2 / 2.0);
//        int radius = cvRound(std::sqrt(c3 + (c1 * c1 / 4.0) + (c2 * c2 / 4.0)));

//        // Validating geometry logic to prevent NaN values
//        if (radius <= 0) continue;

//        // 4. Filter the arc by assessing fit quality (Residuals)
//        double total_error = 0.0;
//        for (int i = 0; i < N; ++i) {
//            double dx = contour[i].x - xc;
//            double dy = contour[i].y - yc;
//            double distance = std::sqrt(dx * dx + dy * dy);
//            total_error += std::abs(distance - radius);
//        }
//        double mean_error = total_error / N;

//        // If the average distance error is low, it is a clean mathematical arc
//        if (mean_error < 3.0 && radius > 10 && radius < 40) {
//            // Draw the reconstructed full circle from the arc
//            cv::circle(edgesClone2, cv::Point(xc, yc), radius, cv::Scalar(255, 255, 255), 5);
//            // Draw center point
//            cv::circle(edgesClone2, cv::Point(xc, yc), 3, cv::Scalar(255, 255, 255), 3);
//        }
//    }
#ifdef DEBUG_SHOW_IMAGE
//    cv::imshow("circle:"+show_name, edgesClone2);
    cv::imshow(show_name, edgesClone);
#endif
    return mat;
}

std::vector<std::vector<int>> ChessImageProcessing::getPieceMatrixColor(const cv::Mat& color, const MoveDetectParams& params,
                                                                        std::string show_name) {
    std::vector<std::vector<int>> matColorPieces(8, std::vector<int>(8, 0));
    cv::Mat hsvWarp;
    cv::cvtColor(color, hsvWarp, cv::COLOR_BGR2HSV);
    if(params.playerSide == "white")
        matColorPieces = cellColorFilterToMatrix(hsvWarp,cv::Vec3b(75,8,102),90,87,12,63,5,28,show_name+"White");
    else
        matColorPieces = cellColorFilterToMatrix(hsvWarp,cv::Vec3b(22,160,138),55,87,12,61,10,28,show_name+"Black");
    return matColorPieces;
}

bool getCenterOfPoints(const cv::Mat& binary_img, cv::Point& center) {
    if (binary_img.empty()) {
        return false;
    }

    double sum_x = 0.0;
    double sum_y = 0.0;
    int numWhitePixels = 0;
    for (int r= 0; r < binary_img.rows; r++) {
        for (int c= 0; c < binary_img.cols; c++) {
            if(binary_img.at<unsigned char>(r,c) != 255) {
                sum_x += r;
                sum_y += c;
                numWhitePixels ++;
            }
        }
    }

    center.x = static_cast<int>(sum_x / numWhitePixels);
    center.y = static_cast<int>(sum_y / numWhitePixels);
    return true;
}
bool ChessImageProcessing::getCenterOfWhitePixels(const cv::Mat& binary_img, cv::Point& center) {
    // Treat the image as binary (non-zero pixels have weight 1.0)
    cv::Moments m = cv::moments(binary_img, true);

    // Prevent division by zero if the image contains no white pixels
    if (m.m00 > 0.0) {
        center.x = static_cast<int>(m.m10 / m.m00);
        center.y = static_cast<int>(m.m01 / m.m00);
        return true;
    }

    return false;
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
            // Find connected components / contours in the ROI to identify individual objects
            cv::Mat roiClone = roiMat.clone();
            // apply a small dilation first to connect nearby white pixels
//            cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3,3));
//            cv::dilate(roiClone, roiClone, element, cv::Point(-1,-1), 1);
            std::vector<std::vector<cv::Point>> contours;
            std::vector<cv::Vec4i> hierarchy;
            cv::findContours(roiClone, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            // If no contours found, fallback to previous behavior
            if (contours.empty()) {
                rectangle(display,roi,cv::Scalar(255,255,255),2);
                char buffer[10];
                int value = nz.size();
                sprintf(buffer,"%d",value);
                putText(display, std::string(buffer) ,cv::Point(c * sq + sq/2,r * sq+ sq/2), 1, 1.5, cv::Scalar(255, 255, 255), 2);
                return true;
            }

            // For each contour compute bounding rect and area, draw them (small) and track the largest
            double largestArea = 0.0;
            cv::Rect largestRect;
            for (size_t idx = 0; idx < contours.size(); ++idx) {
                double area = cv::contourArea(contours[idx]);
                cv::Rect br = cv::boundingRect(contours[idx]);
                // translate bounding rect into display coordinates
                cv::Rect brDisplay(br.x + roi.x, br.y + roi.y, br.width, br.height);
                // draw all object bounds in yellow
                cv::rectangle(display, brDisplay, cv::Scalar(0,255,255), 1);
                if (area > largestArea) {
                    largestArea = area;
                    largestRect = brDisplay;
                }
            }

            // Mark largest object clearly (white thicker rectangle) and annotate its area
            if (largestArea > 0) {
                cv::rectangle(display, largestRect, cv::Scalar(255,255,255), 3);
                // compute occupation area as percentage of ROI area (use bounding rect area relative to ROI)
                double roiArea = static_cast<double>(roiMat.cols * roiMat.rows);
                double occPercent = (static_cast<double>(largestRect.width * largestRect.height) / roiArea) * 100.0;
                char info[128];
                sprintf(info, "%d%d A:%.0f P:%.1f%%", r, c, largestArea, occPercent);
//                putText(display, std::string(info), cv::Point(largestRect.x + 2, largestRect.y + 12), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0,0,255), 1);
//                std::cout << "largestRect:" << largestRect.y + largestRect.height/2 <<std::endl;
//                std::cout << "roi:" << roi.y + roi.height/2 <<std::endl;
//                if((float)largestRect.height/(float)largestRect.width<0.6f) {
//                    std::cout << "fail case: c" << c << ",r" << r << " Line:" << __LINE__ <<std::endl;
//                    return false;
//                }
                cv::Point centerLargestArea;
                cv::Mat matLargestArea = edges(largestRect);
                getCenterOfWhitePixels(matLargestArea,centerLargestArea);
                cv::circle(display,
                           cv::Point(centerLargestArea.x + c* sq,centerLargestArea.y + r* sq),
                           5,cv::Scalar(0,0,255));
                if(centerLargestArea.y + 20 <
                        roi.y + roi.height/2 ) {
                    std::cout << "fail case: c" << c << ",r" << r << " Line:" << __LINE__ <<std::endl;
//                    return false;
                }
//                if(occPercent < 30) {
//                    std::cout << "fail case: c" << c << ",r" << r << " Line:" << __LINE__ <<std::endl;
//                    return false;
//                }
            }

            // Also draw a white rectangle around the ROI to preserve previous visualization
//            rectangle(display,roi,cv::Scalar(255,255,255),1);
            char buffer[10];
            int value = nz.size();
            sprintf(buffer,"%d",value);
//            putText(display, std::string(buffer) ,cv::Point(c * sq + sq/2,r * sq+ sq/2), 1, 1.5, cv::Scalar(0,0,255), 1);
            return true;
        }
    }
    return false;
}

// Compare two 8x8 matrices, return start (1->0) and list of possible ends (0->1)
void ChessImageProcessing::comparePieceMatrices(const std::vector<std::vector<int>>& mat1,
                                                const std::vector<std::vector<int>>& mat2,
                                                std::vector<cv::Point>& starts, std::vector<cv::Point>& ends) {
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
    starts.clear();
    ends.clear();
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (mat1[r][c] == 1 && mat2[r][c] == 0) {
                starts.push_back(cv::Point(c, r));
            } else if (mat1[r][c] == 0 && mat2[r][c] == 1) {
                ends.push_back(cv::Point(c, r));
            }
        }
    }
}

// GUI helpers implementation
void ChessImageProcessing::createControlsWindow(const MoveDetectParams& defaults) {
    cv::namedWindow("Controls", cv::WINDOW_NORMAL);
    cv::createTrackbar("White(0)/Black(1)", "Controls", nullptr, 1);
    // create trackbars and initialize with defaults
    cv::createTrackbar("roi_percent", "Controls", nullptr, 100);
    cv::createTrackbar("diff_thresh", "Controls", nullptr, 255);
    cv::createTrackbar("canny_low", "Controls", nullptr, 500);
    cv::createTrackbar("pieceMinPoints", "Controls", nullptr, 500);
    cv::createTrackbar("pieceRoiPercent", "Controls", nullptr, 100);
    // add trackbar to control color/shape matching threshold
    cv::createTrackbar("colorThreshold", "Controls", nullptr, 500);
    // add trackbar to control bottom-vs-top white-pixel difference threshold
    cv::createTrackbar("bottomTopThresh", "Controls", nullptr, 200);

    // set initial positions
    cv::setTrackbarPos("White(0)/Black(1)", "Controls", defaults.playerSide == "white"?0:1);
    cv::setTrackbarPos("roi_percent", "Controls", defaults.roi_percent);
    cv::setTrackbarPos("diff_thresh", "Controls", defaults.diff_thresh);
    cv::setTrackbarPos("canny_low", "Controls", defaults.canny_low);
    cv::setTrackbarPos("pieceMinPoints", "Controls", defaults.pieceMinPoints);
    cv::setTrackbarPos("pieceRoiPercent", "Controls", defaults.pieceRoiPercent);
    // default color threshold for matching (approx dist in BGR space)
    cv::setTrackbarPos("colorThreshold", "Controls", defaults.colorThreshold);
    // default bottom/top diff threshold
    cv::setTrackbarPos("bottomTopThresh", "Controls", 150);

}

MoveDetectParams ChessImageProcessing::readControlsFromWindow() {
    MoveDetectParams p;
    p.roi_percent = cv::getTrackbarPos("roi_percent", "Controls");
    p.diff_thresh = cv::getTrackbarPos("diff_thresh", "Controls");
    p.canny_low = cv::getTrackbarPos("canny_low", "Controls");
    p.pieceMinPoints = cv::getTrackbarPos("pieceMinPoints", "Controls");
    p.pieceRoiPercent = cv::getTrackbarPos("pieceRoiPercent", "Controls");
    p.playerSide = cv::getTrackbarPos("White(0)/Black(1)", "Controls") == 0?
                "white":"black";
    // Note: colorThreshold is read directly where needed (not stored in params)
    return p;
}

std::pair<cv::Point, cv::Point> ChessImageProcessing::findSameColoredCellsInImage(const cv::Mat& warpedColor, const std::vector<cv::Point>& cells, const MoveDetectParams& params, double colorThreshold) {
    if (warpedColor.empty() || cells.empty()) return {cv::Point(-1,-1), cv::Point(-1,-1)};
    int sq = warpedColor.cols / 8;
    int sub = std::max(1, (sq * params.pieceRoiPercent) / 100);
    int off = (sq - sub) / 2;

    // compute average color for each cell
    std::vector<cv::Vec3d> avgColors;
    avgColors.reserve(cells.size());
    for (const auto& pt : cells) {
        int c = pt.x; int r = pt.y;
        int x = c * sq + off; int y = r * sq + off;
        cv::Rect roiRect(x, y, sub, sub);
        roiRect &= cv::Rect(0,0,warpedColor.cols, warpedColor.rows);
        cv::Scalar avg = cv::mean(warpedColor(roiRect));
        avgColors.emplace_back(avg[0], avg[1], avg[2]);
    }

    // compare pairs
    for (size_t i = 0; i < cells.size(); ++i) {
        for (size_t j = i+1; j < cells.size(); ++j) {
            cv::Vec3d a = avgColors[i];
            cv::Vec3d b = avgColors[j];
            double dist = cv::norm(a - b);
            if (dist <= colorThreshold) {
                return {cells[i], cells[j]};
            }
        }
    }
    return {cv::Point(-1,-1), cv::Point(-1,-1)};
}

bool ChessImageProcessing::filterCellColor(
      cv::Mat warpedCell, cv::Vec3b targetHSV,
      int hTol, int sTol, int vTol,
      int roiPercent, int minWhitePercent, int maxBlackPercent,
      std::string nameToShow) {
    std::vector < std::vector < int >> matrix(8, std::vector < int > (8, 0));
    cv::Mat display = warpedCell.clone();
    cv::Mat maskAll = cv::Mat::zeros(warpedCell.size(), CV_8UC1);
    cv::Mat hsvWarp;
    cvtColor(warpedCell, hsvWarp, cv::COLOR_BGR2HSV);

    int h = targetHSV[0];
    int s = targetHSV[1];
    int v = targetHSV[2];

    int lowH = h - hTol;
    int highH = h + hTol;
    int lowS = std::max(0, s - sTol);
    int highS = std::min(255, s + sTol);
    int lowV = std::max(0, v - vTol);
    int highV = std::min(255, v + vTol);

    cv::Mat mask;
    if (lowH < 0) {
        cv::Mat m1, m2;
        cv::inRange(hsvWarp, cv::Scalar(0, lowS, lowV), cv::Scalar(highH, highS, highV), m1);
        inRange(hsvWarp, cv::Scalar(180 + lowH, lowS, lowV), cv::Scalar(180, highS, highV), m2);
        bitwise_or(m1, m2, mask);
    } else if (highH > 180) {
        cv::Mat m1, m2;
        inRange(hsvWarp, cv::Scalar(lowH, lowS, lowV), cv::Scalar(180, highS, highV), m1);
        inRange(hsvWarp, cv::Scalar(0, lowS, lowV), cv::Scalar(highH - 180, highS, highV), m2);
        bitwise_or(m1, m2, mask);
    } else {
        inRange(hsvWarp, cv::Scalar(lowH, lowS, lowV), cv::Scalar(highH, highS, highV), mask);
    }

    cv::Mat kernel = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
    morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);

    const int cellW = warpedCell.cols;
    const int cellH = warpedCell.rows;
    const int roiW = std::max(2, (cellW * roiPercent) / 100);
    const int roiH = std::max(2, (cellH * roiPercent) / 100);

    int cx = cellW / 2;
    int cy = cellH / 2;
    int x0 = cx - roiW / 2;
    int y0 = cy - roiH / 2;
    cv::Rect roiRect(x0, y0, roiW, roiH);
    roiRect &= cv::Rect(0, 0, warpedCell.cols, warpedCell.rows);
    cv::Mat roiMask = mask(roiRect);
    double whitePixels = countNonZero(roiMask);
    double area = roiRect.width * roiRect.height;
    double blackPixels = area - whitePixels;

    double whiteFrac = area > 0 ? (whitePixels / area) : 0.0;
    double blackFrac = area > 0 ? (blackPixels / area) : 0.0;

    double minWhiteThresh = minWhitePercent / 100.0;
    double maxBlackThresh = maxBlackPercent / 100.0;

    // Logic: Mostly the selected input color backdrop containing a small edge profile/shadow
    //      bool isColorDetected = (whiteFrac >= minWhiteThresh) && (blackFrac > 0.01) && (blackFrac <= maxBlackThresh);
    bool isColorDetected = whiteFrac >= minWhiteThresh;
#ifdef DEBUG_SHOW_IMAGE
    cv::imshow(nameToShow,mask);
    std::cout << nameToShow << roiRect
              <<" cellW:" << cellW
              <<" cellH:" << cellH
              <<" roiW:" << roiW
              <<" roiH:" << roiH
              <<" whiteFrac:" << whiteFrac << " minWhiteThresh:" <<minWhiteThresh << std::endl;
#endif
    return isColorDetected;
}
std::vector<cv::Point> ChessImageProcessing::matchStartToCandidates(const cv::Mat& warpedStartColor, const cv::Mat& warpedEndColor,
                                                     const cv::Point& startCell, const std::vector<cv::Point>& candidates,
                                                     const MoveDetectParams& params, double colorThreshold) {
    std::vector<cv::Point> listEndPos;
    if (warpedStartColor.empty() || warpedEndColor.empty()) return listEndPos;
    if (startCell.x < 0 || startCell.y < 0) return listEndPos;
    if (candidates.empty()) return listEndPos;

    int sq = warpedStartColor.cols / 8;
    int sub = std::max(1, (sq * params.pieceRoiPercent) / 100);
    int off = (sq - sub) / 2;

    // compute average color for start cell
    int sx = startCell.x * sq + off;
    int sy = startCell.y * sq + off;
    cv::Rect sroi(sx, sy, sub, sub);
    sroi &= cv::Rect(0,0,warpedStartColor.cols, warpedStartColor.rows);
    cv::Scalar savg = cv::mean(warpedStartColor(sroi));
    cv::Vec3d sColor(savg[0], savg[1], savg[2]);
#ifdef DEBUG_SHOW_IMAGE
    // visualize start cell on its image
    cv::Mat visStart = warpedStartColor.clone();
    // draw full square (as used in detectMovePhase1Binary) and the ROI inside it
    cv::Rect fullStartRect(startCell.x * sq, startCell.y * sq, sq, sq);
    fullStartRect &= cv::Rect(0,0,warpedStartColor.cols, warpedStartColor.rows);
    cv::rectangle(visStart, fullStartRect, cv::Scalar(255,255,255), 3); // same style as Move Detection
    cv::rectangle(visStart, sroi, cv::Scalar(0,255,0), 2);
    cv::putText(visStart, "START", cv::Point(fullStartRect.x+5, fullStartRect.y+20), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,255,0), 2);
    cv::imshow("start_cell", visStart);
#endif
    // search candidates on warpedEndColor
    cv::Mat visEnd = warpedEndColor.clone();
    for (const auto& cand : candidates) {
        int cx = cand.x * sq + off;
        int cy = cand.y * sq + off;
        cv::Rect croi(cx, cy, sub, sub);
        croi &= cv::Rect(0,0,warpedEndColor.cols, warpedEndColor.rows);
//        cv::Scalar cavg = cv::mean(warpedEndColor(croi));
//        cv::Vec3d cColor(cavg[0], cavg[1], cavg[2]);
//        double dist = cv::norm(sColor - cColor);
//        if (dist <= colorThreshold) {
        bool cellIsSameColor = false;
        if(params.playerSide == "white") {
            cellIsSameColor = filterCellColor(warpedEndColor(croi),cv::Vec3b(64,4,107),55,87,12,61,10,28,
                                                  std::to_string(cand.x)+","+std::to_string(cand.y));
        } else {
            cellIsSameColor = !filterCellColor(warpedEndColor(croi),cv::Vec3b(64,4,107),55,87,12,61,10,28,
                                                  std::to_string(cand.x)+","+std::to_string(cand.y));
        }
        if(cellIsSameColor) {
#ifdef DEBUG_SHOW_IMAGE
            // matched - draw full cell box plus ROI
            cv::Rect fullCandRect(cand.x * sq, cand.y * sq, sq, sq);
            fullCandRect &= cv::Rect(0,0,warpedEndColor.cols, warpedEndColor.rows);
            cv::rectangle(visEnd, fullCandRect, cv::Scalar(255,255,255), 3);
            cv::rectangle(visEnd, croi, cv::Scalar(0,255,0), 2);
            cv::putText(visEnd, "MATCH", cv::Point(fullCandRect.x+5, fullCandRect.y+20), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,255,0), 2);
            cv::imshow("matched_candidate", visEnd);
#endif
            listEndPos.push_back(cand);
         } else {
#ifdef DEBUG_SHOW_IMAGE
             // draw non-matching in red
            cv::Rect fullCandRect(cand.x * sq, cand.y * sq, sq, sq);
            fullCandRect &= cv::Rect(0,0,warpedEndColor.cols, warpedEndColor.rows);
            cv::rectangle(visEnd, fullCandRect, cv::Scalar(0,0,255), 1);
            cv::rectangle(visEnd, croi, cv::Scalar(0,0,255), 1);
//            cv::putText(visEnd, std::to_string((int)dist), cv::Point(croi.x+2, croi.y+12), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0,0,255), 1);
#endif
         }
     }

    // no match found
    std::cout << "No color match found for start cell " << coordToNotation(startCell, "white") << std::endl;
#ifdef DEBUG_SHOW_IMAGE
    cv::imshow("matched_candidate", visEnd);
#endif
    return listEndPos;
}
