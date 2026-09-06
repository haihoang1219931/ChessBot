#include "ChessImageProcessing.h"
#include <set>
#include <algorithm>
#include <chrono>

ChessImageProcessing::ChessImageProcessing()
{
    m_sourceConnected = false;
    m_isBlackSide = true;
    m_chessBoardRow = 8;
    m_chessBoardBox = 60;
    m_chessBoardSize = m_chessBoardRow * m_chessBoardBox;
    m_transformMaxtrixValid = false;
//    unsigned char testBoardPrev[64] = {
//        'q','q','Q','Q','R','N','P','P',
//        'b','b','B','b','p','p','p','p',
//        'k','k','k','k','K','.','.','.',
//        'p','p','p','p','r','r','R','r',
//        'p','P','N','B','.','.','.','.',
//        '.','.','.','.','.','.','.','.',
//        'P','.','.','.','.','.','.','.',
//        'P','P','P','P','N','n','R','n'};
//    unsigned char testBoardAfter[NUM_ROW][NUM_COL] = {
//    {'.','.','.','.','R','n','N','P','P','P','P','.','.','.'},
//    {'.','.','.','n','.','.','.','.','.','.','P','.','.','.'},
//    {'.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
//    {'.','.','.','.','.','.','.','B','N','P','P','.','.','.'},
//    {'.','.','.','r','R','r','r','p','p','p','p','.','.','.'},
//    {'.','.','.','.','.','.','.','K','k','k','k','.','.','.'},
//    {'.','.','.','p','p','p','p','b','B','b','b','.','.','.'},
//    {'.','.','.','P','P','N','R','Q','Q','q','q','.','.','.'}};
//    for(int row = 0; row < NUM_ROW; row++) {
//        for(int col=0; col< NUM_COL; col++) {
//            m_mapClassifiedCell[row][col] = testBoardAfter[row][col];
//        }
//    }
//    MoveDetectParams params;
//    findPossibleMoves2(cv::Mat(),testBoardPrev,params);
}

void ChessImageProcessing::setDnnNetAllPieces(char* source, const std::vector<char>& dnnClassNames)
{
    m_dnnNetAllPieces = cv::dnn::readNetFromONNX(source);
    m_dnnAllPiecesNames = dnnClassNames;
    printf("setDnnNetAllPieces [%s] ",source);
    for(char className: dnnClassNames) {
        printf("%c ",className);
    }
    printf("\r\n");
}

void ChessImageProcessing::setDnnNetSpecial(char* source, const std::vector<char>& dnnClassNames)
{
    m_dnnNetBishopPawn = cv::dnn::readNetFromONNX(source);
    m_dnnBishopPawnNames = dnnClassNames;
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
    std::vector<cv::Point2f> clicked_points;
    clicked_points.push_back(cv::Point2f(topLeftX, topLeftY));
    clicked_points.push_back(cv::Point2f(topRightX, topRightY));
    clicked_points.push_back(cv::Point2f(bottomRightX, bottomRightY));
    clicked_points.push_back(cv::Point2f(bottomLeftX, bottomLeftY));
    std::vector<cv::Point2f> dstCorners;
    std::vector<cv::Point2f> srcCorners;
    std::vector<cv::Point2f> dstGroundCorners;
    std::vector<cv::Point2f> srcGroundCorners;
    // Base parameters calculated from your 4 corner clicks
    cv::Mat base_rvec, base_tvec;
    double base_tilt_deg = 0.0;
    bool is_pnp_initialized = false;

    // Trackbar variables (scaled to integers for OpenCV)
    // These now serve purely as structural adjustments (+/- offsets) relative to the PnP base values
    int track_dx = 500;    // Range 0-1000, mapped to -5.0 to 5.0 offset
    int track_dy = 500;    // Range 0-1000, mapped to -5.0 to 5.0 offset
    int track_dz = 500;    // Range 0-1000, mapped to -5.0 to 5.0 offset
    int track_fov = 60;    // Range 1-179 degrees (Absolute field of view)
    int track_tilt_offset = 180; // Range 0-360, maps to -90 to +90 degrees tilt change
    int track_height = 20;  // Range 0-200, mapped to 0.0 to 2.0 units height above the board

    // 1. Map trackbars back to actual float values
    float dx_offset = (track_dx - 500) / 100.0f;
    float dy_offset = (track_dy - 500) / 100.0f;
    float dz_offset = (track_dz - 500) / 100.0f;
    float fov = (float)track_fov;

    float tilt_offset_deg = (float)(track_tilt_offset - 180) * 0.5f;
    float tilt_offset_rad = tilt_offset_deg * CV_PI / 180.0f;
    float piece_height = -(float)track_height / 100.0f;

    // 2. Define standard 3D coordinates for Ground Chessboard & Elevated Head Plane
    std::vector<cv::Point3f> ground_object_points;
    std::vector<cv::Point3f> elevated_object_points;

    int total_rows = NUM_ROW + 1;    // 8 board squares = 9 grid lines
    int total_columns = NUM_COL + 1; // 14 board squares = 15 grid lines
    float square_size = 0.2f;

    for (int i = 0; i < total_rows; ++i) {
        for (int j = 0; j < total_columns; ++j) {
            // Center the grid's X and Y coordinates around the local origin
            float x_coord = j * square_size - ((total_columns - 1) * square_size / 2.0f);
            float y_coord = i * square_size - ((total_rows - 1) * square_size / 2.0f);

            ground_object_points.push_back(cv::Point3f(x_coord, y_coord, 0.0f));
            elevated_object_points.push_back(cv::Point3f(x_coord, y_coord, piece_height));
        }
    }

    // 3. Define the 4 corner ground 3D points matching your 4 user mouse clicks
    // Winding Order: Top-Left -> Top-Right -> Bottom-Right -> Bottom-Left
    float half_width = (total_columns - 1) * square_size / 2.0f;  // 14 * 0.2 / 2 = 1.4
    float half_height = (total_rows - 1) * square_size / 2.0f;    //  8 * 0.2 / 2 = 0.8

    std::vector<cv::Point3f> board_corners_3d = {
        cv::Point3f(-half_width, -half_height, 0.0f), // Top-Left
        cv::Point3f( half_width, -half_height, 0.0f), // Top-Right
        cv::Point3f( half_width,  half_height, 0.0f), // Bottom-Right
        cv::Point3f(-half_width,  half_height, 0.0f)  // Bottom-Left
    };

    // 4. Construct camera intrinsics matrix based on FOV
    float width = IMAGE_WIDTH;
    float height = IMAGE_HEIGHT;
    float f_val = (width / 2.0f) / tan((fov * CV_PI / 180.0f) / 2.0f);

    cv::Mat camera_matrix = (cv::Mat_<double>(3, 3) <<
        f_val, 0, width / 2.0f,
        0, f_val, height / 2.0f,
        0, 0, 1);

    cv::Mat dist_coeffs = cv::Mat::zeros(4, 1, CV_64F);

    // 5. ALWAYS calculate the base camera pose relative to the current FOV
    // to anchor the 3D base board corners perfectly to the static 2D clicked points.
    cv::solvePnP(board_corners_3d, clicked_points, camera_matrix, dist_coeffs, base_rvec, base_tvec);

    // Calculate base tilt angle
    cv::Mat R_initial;
    cv::Rodrigues(base_rvec, R_initial);
    cv::Mat R_T = R_initial.t();
    cv::Mat cam_pos_W = -R_T * base_tvec;

    double ax = cam_pos_W.at<double>(0);
    double ay = cam_pos_W.at<double>(1);
    double az = cam_pos_W.at<double>(2);

    double ao_norm = std::sqrt(ax*ax + ay*ay + az*az);
    double am_x = R_T.at<double>(0, 2);
    double am_y = R_T.at<double>(1, 2);
    double am_z = R_T.at<double>(2, 2);
    double am_norm = std::sqrt(am_x*am_x + am_y*am_y + am_z*am_z);

    double dot_product = (am_x * (-ax)) + (am_y * (-ay)) + (am_z * (-az));
    double cos_theta = std::max(-1.0, std::min(1.0, dot_product / (ao_norm * am_norm)));
    base_tilt_deg = std::acos(cos_theta) * 180.0f / CV_PI;

    // 6. Keep base values completely clean for the ground grid.
    // Apply offsets ONLY to a separate working set for the elevated/transformed grid.
    cv::Mat working_tvec = base_tvec.clone();
    working_tvec.at<double>(0) += dx_offset;
    working_tvec.at<double>(1) += dy_offset;
    working_tvec.at<double>(2) += dz_offset;

    cv::Mat R_working;
    cv::Rodrigues(base_rvec, R_working);

    // Apply local camera tilt rotation matrix
    cv::Mat R_tilt = (cv::Mat_<double>(3, 3) <<
        1, 0,                   0,
        0, cos(tilt_offset_rad), -sin(tilt_offset_rad),
        0, sin(tilt_offset_rad),  cos(tilt_offset_rad));

    R_working = R_working * R_tilt;

    cv::Mat working_rvec;
    cv::Rodrigues(R_working, working_rvec);

    // 7. Project both grids onto the screen space map
    std::vector<cv::Point2f> projected_ground_points;
    std::vector<cv::Point2f> projected_elevated_points;

    // FIX: Ground points use base_rvec/base_tvec so they remain locked to your clicks
    cv::projectPoints(ground_object_points, base_rvec, base_tvec, camera_matrix, dist_coeffs, projected_ground_points);

    // Elevated points use working_rvec/working_tvec to respond to sliders
    cv::projectPoints(elevated_object_points, working_rvec, working_tvec, camera_matrix, dist_coeffs, projected_elevated_points);

    // 8. Generate warp matrix for full 8x14 chess board
    // Top-Left: 0, Top-Right: 14, Bottom-Right: (9*15)-1 = 134, Bottom-Left: 9*15 - 15 = 120
    int corner_indices[] = {0, total_columns - 1, (total_rows * total_columns) - 1, (total_rows * total_columns) - total_columns};

    dstCorners = {
        cv::Point2f(0, 0),
        cv::Point2f(WARP_WIDTH - 1, 0),
        cv::Point2f(WARP_WIDTH - 1, WARP_HEIGHT - 1),
        cv::Point2f(0, WARP_HEIGHT - 1)
    };
    srcCorners.clear();
    for(int idx : corner_indices) {
        srcCorners.push_back(cv::Point2f(projected_elevated_points[idx].x,
                                         projected_elevated_points[idx].y));
    }
    for(cv::Point2f corner: srcCorners) {
        std::cout << "elevated corner (" << corner.x << ", " << corner.y << ")\n";
    }
    m_transformHeadPiecesWholeBoard = cv::getPerspectiveTransform(srcCorners, dstCorners);

    // 9. Generate warp matrix for only 8x8 chess board
    // Top-Left: 3, Top-Right: 11, Bottom-Right: (9*15)-4 = 131, Bottom-Left: 9*15 - 15 + 3 = 123
    int groundCorner_indices[] = {3, total_columns - 4,
                                  (total_rows * total_columns) - 4, (total_rows * total_columns) - total_columns + 3};
    for(int idx : groundCorner_indices) {
        srcGroundCorners.push_back(cv::Point2f(projected_ground_points[idx].x,
                                         projected_ground_points[idx].y));
    }

    dstGroundCorners = {
        cv::Point2f(0, 0),
        cv::Point2f(WARP_SMALL_HEIGHT - 1, 0),
        cv::Point2f(WARP_SMALL_HEIGHT - 1, WARP_SMALL_HEIGHT - 1),
        cv::Point2f(0, WARP_SMALL_HEIGHT - 1)
    };

    m_transformMatrix = cv::getPerspectiveTransform(srcGroundCorners, dstGroundCorners);
    m_transformMaxtrixValid = true;
}

cv::Mat ChessImageProcessing::getSubTranformMatrix() {
    return m_transformMatrix;
}

cv::Mat ChessImageProcessing::getFullTranformMatrix() {
    return m_transformHeadPiecesWholeBoard;
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
    printf("findPossibleMoves\r\n");
    // No longer identify start/stop/occupied, just collect top 3 cells
    std::vector<std::string> listMoves;
    if (img_start.empty() || img_end.empty() || !m_transformMaxtrixValid) {
        printf("Return due to img_start.empty() = %s | img_end.empty() = %s | m_transformMaxtrixValid = %s\r\n",
               img_start.empty() ? "true" : "false",
               img_end.empty() ? "true" : "false",
               m_transformMaxtrixValid ? "true" : "false");
        return listMoves;
    }
    printf("findPossibleMoves preprocessing\r\n");
    cv::Mat gray1, gray2, warped1, warped2, edges1, edges2;
    std::vector<cv::Point> startCellsBinary, startCellsColor;
    std::vector<cv::Point> endsCellsBinary, endCellsColor;
    cv::Point startCellCastle, endCellCastle;
    std::vector<std::vector<int>> matColorMap1(8, std::vector<int>(8, 0));
    std::vector<std::vector<int>> matColorMap2(8, std::vector<int>(8, 0));
    // warp image before calculation (also keep color warped images for color-matching)
    cv::warpPerspective(img_start, warped1, m_transformMatrix, cv::Size(WARP_SMALL_WIDTH, WARP_SMALL_HEIGHT));
    cv::warpPerspective(img_end, warped2, m_transformMatrix, cv::Size(WARP_SMALL_WIDTH, WARP_SMALL_HEIGHT));

    cv::cvtColor(warped1, gray1, cv::COLOR_BGR2GRAY);
    cv::cvtColor(warped2, gray2, cv::COLOR_BGR2GRAY);

    // 1) check castle move
    if(isCastleMove(gray1,gray2,params,startCellCastle,endCellCastle)) {
        std::string from = coordToNotation(startCellCastle, params.playerSide);
        std::string to = coordToNotation(endCellCastle, params.playerSide);
        listMoves.push_back(from + to);
        return listMoves;
    }

    // 2) detect start cell and candidate end cells
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
    printf("Color checking start binary[%d] color[%d] sub[%d]\r\n",
           startCellsBinary.size(),
           startCellsColor.size(),
           listChangedCell.size());
//    listMoves = detectMovePhase3ColorMatching(warped1, warped2, startCells, listChangedCell, params);
    if(startCellsColor.size() > 0) {
        listMoves = detectMovePhase3ColorMatchingFromFilter(startCellsColor, listChangedCell, params,
                                                        matColorMap1, matColorMap2);
    } else {
        listMoves = detectMovePhase3ColorMatchingFromFilter(startCellsBinary, listChangedCell, params,
                                                        matColorMap1, matColorMap2);
    }
    printf("Color checking done\r\n");
    return listMoves;
}

bool ChessImageProcessing::detectMovePhase1Binary(const cv::Mat& edges1, const cv::Mat& edges2,
                                                  std::vector<cv::Point>& starts, std::vector<cv::Point>& ends,
                                                  const MoveDetectParams& params)
{
    std::cout << "detectMovePhase1Binary" << std::endl;
    if (edges1.empty() || edges2.empty()) return false;
    int sq = edges1.cols / 8;
    std::vector<std::vector<int>> mat1 = getPieceMatrix(edges1, sq, params,"warp1");
    std::vector<std::vector<int>> mat2 = getPieceMatrix(edges2, sq, params,"warp2");

    comparePieceMatrices(mat1, mat2, starts, ends);
    for(cv::Point start: starts) {
        std::cout << "Possible start: " << start << std::endl;
#if defined(DEBUG_SHOW_IMAGE) || defined(DEBUG_WRITE_IMAGE)
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
#endif
#if defined(DEBUG_SHOW_IMAGE)
    imshow("Move Detection", out);
#elif defined(DEBUG_WRITE_IMAGE)
    cv::imwrite("move_detect.jpg",out);
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
    std::cout << "detectMovePhase1ColorFilter" << std::endl;
    if (color1.empty() || color2.empty()) return false;
    int sq = color1.cols / 8;
    matColorMapBefore = getPieceMatrixColor(color1,params,"warp1");
    matColorMapAfter = getPieceMatrixColor(color2,params,"warp2");

    comparePieceMatrices(matColorMapBefore, matColorMapAfter, starts, ends);
    std::cout << "comparePieceMatrices done with num start " << starts.size() << std::endl;
    if(starts.size() > 0) return false;
    for(cv::Point start: starts) {
        std::cout << "Possible start: " << start << std::endl;
#if defined(DEBUG_SHOW_IMAGE) || defined(DEBUG_WRITE_IMAGE)
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
#endif
#if defined(DEBUG_SHOW_IMAGE)
    imshow("Move Detection", out);
#elif defined(DEBUG_WRITE_IMAGE)
    cv::imwrite("move_detect.jpg",out);
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
                                                        std::vector<cv::Point>& listChangedCells)
{
    // use passed images (expected to be warped/grayscale or edge images)
    const cv::Mat& gray1 = img_start;
    const cv::Mat& gray2 = img_end;
    std::cout << "detectMovePhase2Substraction" << std::endl;
    if (gray1.empty() || gray2.empty()) return false;

    cv::Mat diff_gray, diff_bin;
    cv::absdiff(gray1, gray2, diff_gray);
#if defined(DEBUG_SHOW_IMAGE)
    cv::imshow("diff_bin_gray",diff_gray);
#elif defined(DEBUG_WRITE_IMAGE)
    cv::imwrite("diff_bin_gray.jpg",diff_gray);
#endif
    int listThresh[2] = {30,70};
    listChangedCells.clear();
    for(int i = 0; i< sizeof(listThresh)/sizeof(int); i++){
        cv::threshold(diff_gray, diff_bin, listThresh[i], 255, cv::THRESH_BINARY);
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
#if defined(DEBUG_SHOW_IMAGE) || defined(DEBUG_WRITE_IMAGE)
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
#if defined(DEBUG_SHOW_IMAGE) || defined(DEBUG_WRITE_IMAGE)
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
        std::vector<cv::Point> listChangedCellsInThresh;
        for (int i = 0; i < (int)counts.size(); ++i) {
            if(std::get<0>(counts[i]) < MIN_BINARY_POINT) continue;
            int cnt = std::get<0>(counts[i]);
            int c = std::get<1>(counts[i]);
            int r = std::get<2>(counts[i]);
            listChangedCellsInThresh.push_back(cv::Point(c, r));
#if defined(DEBUG_SHOW_IMAGE) || defined(DEBUG_WRITE_IMAGE)
            // mark top cells with thicker rectangle
            int x = c * sq + off; int y = r * sq + off;
            cv::Rect roiRect(x, y, sub, sub);
            roiRect &= cv::Rect(0,0,diff_bin.cols,diff_bin.rows);
            cv::rectangle(vis, roiRect, cv::Scalar(255,0,0), 2);
            std::string txt = "TOP:" + std::to_string(cnt);
            cv::putText(vis, txt, cv::Point(roiRect.x+2, roiRect.y+12), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255,255,0), 2);
#endif
        }

#if defined(DEBUG_SHOW_IMAGE)
        cv::imshow("diff_bin_thresh_"+std::to_string(listThresh[i]), diff_bin);
        cv::imshow("diff_counts_thresh_"+std::to_string(listThresh[i]), vis);
#elif defined (DEBUG_WRITE_IMAGE)
        cv::imwrite("diff_bin_thresh_"+std::to_string(listThresh[i])+".jpg",diff_bin);
        cv::imwrite("diff_counts_thresh_"+std::to_string(listThresh[i])+".jpg",vis);
#endif
        if(listChangedCellsInThresh.size() <= 5) {
            for(cv::Point tmpChangedCellInThresh: listChangedCellsInThresh) {
                bool dupCell = false;
                for(cv::Point tmpChangedCell: listChangedCells) {
                    if(tmpChangedCell.x == tmpChangedCellInThresh.x &&
                       tmpChangedCell.y == tmpChangedCellInThresh.y) {
                        dupCell = true;
                        break;
                    }
                }
                if(!dupCell) listChangedCells.push_back(tmpChangedCellInThresh);
            }
        }
    }
    std::cout << "detectMovePhase2Substraction done with " << listChangedCells.size()
              << " cells" << std::endl;
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
    fflush(stdout);
    return listMoves;
}

std::vector<std::string> ChessImageProcessing::detectMovePhase3ColorMatchingFromFilter(const std::vector<cv::Point>& startCells,
                                std::vector<cv::Point> listChangedCell,
                                const MoveDetectParams& params,
                                const std::vector<std::vector<int>> matColorMapBefore,
                                const std::vector<std::vector<int>> matColorMapAfter)
{
    std::vector<std::string> listMoves;
    std::vector<cv::Point> filterChangedCell;
    std::cout << "detectMovePhase3ColorMatchingFromFilter" << std::endl;
    for (int i=0; i< listChangedCell.size(); i++) {
        filterChangedCell.push_back(cv::Point(listChangedCell[i].x,listChangedCell[i].y));
    }
    // 3) If we have a start from phase1 and remaining candidates, try color matching
    for(cv::Point startCell: startCells) {
        bool startCellInListChangeCell = false;
        for(cv::Point changeCell: listChangedCell) {
            if(startCell.x == changeCell.x && startCell.y == changeCell.y) {
                startCellInListChangeCell = true;
                break;
            }
        }
        if(!startCellInListChangeCell) continue;
        std::string from = coordToNotation(startCell, params.playerSide);
        for(cv::Point filterCell: filterChangedCell) {
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
    }
    // No binary start found; if only changed cells remain, return their notations as possible moves
    for (int fromIndex = 0; fromIndex < listChangedCell.size(); fromIndex ++) {
        for (int toIndex = 0; toIndex < listChangedCell.size(); toIndex ++) {
            if(toIndex != fromIndex) {
                std::string detectMove = coordToNotation(listChangedCell[fromIndex], params.playerSide)+
                        coordToNotation(listChangedCell[toIndex], params.playerSide);
                bool existMove = false;
                for(std::string move: listMoves) {
                    if(move == detectMove) {
                        existMove = true;
                        break;
                    }
                }
                if(!existMove) listMoves.push_back(detectMove);
            }
        }
    }
    std::cout << "detectMovePhase3ColorMatchingFromFilter done" << std::endl;
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

    const int cellW = CELL_SMALL_SIZE;
    const int cellH = CELL_SMALL_SIZE;
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
            bool isColorDetected = whitePixels >= minWhitePercent;
            if (isColorDetected) {
                matrix[r][c] = 1;
            }
#if defined(DEBUG_SHOW_IMAGE) || defined(DEBUG_WRITE_IMAGE)
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
#if defined(DEBUG_SHOW_IMAGE) || defined(DEBUG_WRITE_IMAGE)
    cv::Mat overlay = cv::Mat::zeros(colorWarped.size(), CV_8UC3);
    overlay.setTo(cv::Scalar(0, 255, 255), maskAll);
    addWeighted(overlay, 0.4, display, 0.6, 0, display);

    std::stringstream ss;
    ss << "Testing HSV(" << h << "," << s << "," << v << ")";
    putText(display, ss.str(), cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
#endif
#if defined(DEBUG_SHOW_IMAGE)
    imshow("Warped"+name, display);
#elif defined (DEBUG_WRITE_IMAGE)
    cv::imwrite("Warped"+name+".jpg",display);
#endif
    return matrix;
}

int ChessImageProcessing::countMatchPixelColor(const cv::Mat& imageHSV,
                                               const std::vector<TargetColor>& targetColors,
                                               int maxH, int maxSV, std::string showName) {
    cv::Mat finalMask = cv::Mat::zeros(imageHSV.size(), CV_8UC1);
    // Loop through every standalone paired color configuration context block
    for (const auto& target : targetColors) {
        int lowerH = std::max(0, (int)target.hsvValue[0] - target.hTolerance);
        int upperH = std::min(maxH, (int)target.hsvValue[0] + target.hTolerance);

        int lowerS = std::max(0, (int)target.hsvValue[1] - target.sTolerance);
        int upperS = std::min(maxSV, (int)target.hsvValue[1] + target.sTolerance);

        int lowerV = std::max(0, (int)target.hsvValue[2] - target.vTolerance);
        int upperV = std::min(maxSV, (int)target.hsvValue[2] + target.vTolerance);

        cv::Scalar lowerBound(lowerH, lowerS, lowerV);
        cv::Scalar upperBound(upperH, upperS, upperV);

        cv::Mat singleMask;
        cv::inRange(imageHSV, lowerBound, upperBound, singleMask);

        // Merge mask arrays using logical bitwise operations
        cv::bitwise_or(finalMask, singleMask, finalMask);
    }
#if defined(DEBUG_SHOW_IMAGE) && defined(DEBUG_SINGLE_IMAGE)
    cv::imshow("final"+showName,finalMask);
#endif
    return cv::countNonZero(finalMask);
}

void ChessImageProcessing::checkPieceColor(const cv::Mat& imageRGB,
                                           ClassificationResult& pieceClass,
                                           int row, int col)
{
    cv::Mat imgHSV;
    cv::cvtColor(imageRGB, imgHSV, cv::COLOR_BGR2HSV);
    // Gray
    std::vector<TargetColor> configGray;
    configGray.push_back({cv::Scalar(20, 8, 91),50,40,40});
    configGray.push_back({cv::Scalar(0, 0, 156),50,40,40});
    configGray.push_back({cv::Scalar(95, 35, 167),10,40,40});

    // Gold
    std::vector<TargetColor> configGold;
    configGold.push_back({cv::Scalar(18, 190, 185),50,40,40});
    configGold.push_back({cv::Scalar(21, 98, 243),50,40,40});
    configGold.push_back({cv::Scalar(15, 204, 80),10,40,40});
    cv::Size originImageSize = imgHSV.size();
    cv::Rect cropRect;
    if(col <= 3) {
        cropRect.width = originImageSize.width * 2 / 3;
        cropRect.height = originImageSize.height * 2 / 3;
        cropRect.x = originImageSize.width - cropRect.width;
        cropRect.y = originImageSize.height - cropRect.height;
    } else if(col >= 10) {
        cropRect.width = originImageSize.width * 2 / 3;
        cropRect.height = originImageSize.height * 2 / 3;
        cropRect.x = 0;
        cropRect.y = originImageSize.height - cropRect.height;
    } else {
        cropRect.width = originImageSize.width;
        cropRect.height = originImageSize.height/2;
        cropRect.x = 0;
        cropRect.y = originImageSize.height - cropRect.height;
    }
    cv::Mat bottomHSV = imgHSV(cropRect);
    int grayPixels = countMatchPixelColor(bottomHSV,configGray,180,255,"gray");
    int goldPixels = countMatchPixelColor(bottomHSV,configGold,180,255,"gold");
    std::string pieceColor = "unknown";
    if(grayPixels > 3 * goldPixels / 2 && grayPixels > 1500) pieceColor = "black";
    else if((goldPixels > 3 * grayPixels / 2 && goldPixels > 1500) ||
            goldPixels > 5000) {
        pieceColor = "white";
        pieceClass.className = std::toupper(pieceClass.className);
    } else if(goldPixels + grayPixels < 2000){
        pieceClass.className = '.';
    }
    pieceClass.color = pieceColor;
    pieceClass.goldPixels = goldPixels;
    pieceClass.grayPixels = grayPixels;
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
#if defined(DEBUG_SHOW_IMAGE) || defined(DEBUG_WRITE_IMAGE)
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
#if defined(DEBUG_SHOW_IMAGE)
    cv::imshow(show_name, edgesClone);
#elif defined (DEBUG_WRITE_IMAGE)
    cv::imwrite(show_name+".jpg",edgesClone);
#endif
    return mat;
}

std::vector<std::vector<int>> ChessImageProcessing::getPieceMatrixColor(const cv::Mat& color, const MoveDetectParams& params,
                                                                        std::string show_name) {
    std::vector<std::vector<int>> matColorPieces(8, std::vector<int>(8, 0));
    cv::Mat hsvWarp;
    cv::cvtColor(color, hsvWarp, cv::COLOR_BGR2HSV);
    if(params.playerSide == "white")
        matColorPieces = cellColorFilterToMatrix(hsvWarp,cv::Vec3b(75,8,90),90,87,12,78,2,28,show_name+"White");
    else
        matColorPieces = cellColorFilterToMatrix(hsvWarp,cv::Vec3b(22,160,138),55,87,12,78,10,28,show_name+"Black");
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
//                    std::cout << "fail case: c" << c << ",r" << r << " Line:" << __LINE__ <<std::endl;
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
#if defined(DEBUG_SHOW_IMAGE) || defined(DEBUG_WRITE_IMAGE)
    std::cout << nameToShow << roiRect
              <<" cellW:" << cellW
              <<" cellH:" << cellH
              <<" roiW:" << roiW
              <<" roiH:" << roiH
              <<" whiteFrac:" << whiteFrac << " minWhiteThresh:" <<minWhiteThresh << std::endl;
#endif
#if defined(DEBUG_SHOW_IMAGE)
    cv::imshow(nameToShow,mask);
#elif defined(DEBUG_WRITE_IMAGE)
    cv::imwrite(nameToShow+".jpg",mask);
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
#if defined(DEBUG_SHOW_IMAGE) || defined(DEBUG_WRITE_IMAGE)
    // visualize start cell on its image
    cv::Mat visStart = warpedStartColor.clone();
    // draw full square (as used in detectMovePhase1Binary) and the ROI inside it
    cv::Rect fullStartRect(startCell.x * sq, startCell.y * sq, sq, sq);
    fullStartRect &= cv::Rect(0,0,warpedStartColor.cols, warpedStartColor.rows);
    cv::rectangle(visStart, fullStartRect, cv::Scalar(255,255,255), 3); // same style as Move Detection
    cv::rectangle(visStart, sroi, cv::Scalar(0,255,0), 2);
    cv::putText(visStart, "START", cv::Point(fullStartRect.x+5, fullStartRect.y+20), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,255,0), 2);
#endif
#if defined(DEBUG_SHOW_IMAGE)
    cv::imshow("start_cell", visStart);
#elif defined(DEBUG_WRITE_IMAGE)
    cv::imwrite("start_cell.jpg", visStart);
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
#if defined(DEBUG_SHOW_IMAGE) || defined(DEBUG_WRITE_IMAGE)
            // matched - draw full cell box plus ROI
            cv::Rect fullCandRect(cand.x * sq, cand.y * sq, sq, sq);
            fullCandRect &= cv::Rect(0,0,warpedEndColor.cols, warpedEndColor.rows);
            cv::rectangle(visEnd, fullCandRect, cv::Scalar(255,255,255), 3);
            cv::rectangle(visEnd, croi, cv::Scalar(0,255,0), 2);
            cv::putText(visEnd, "MATCH", cv::Point(fullCandRect.x+5, fullCandRect.y+20), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,255,0), 2);
#endif
            listEndPos.push_back(cand);
         } else {
#if defined(DEBUG_SHOW_IMAGE) || defined(DEBUG_WRITE_IMAGE)
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
#if defined(DEBUG_SHOW_IMAGE)
    cv::imshow("matched_candidate", visEnd);
#elif defined(DEBUG_WRITE_IMAGE)
    cv::imwrite("matched_candidate.jpg", visEnd);
#endif
    return listEndPos;
}

bool ChessImageProcessing::isCastleMove(const cv::Mat& warpedGray1, const cv::Mat& warpedGray2, const MoveDetectParams& params,
                                        cv::Point& startCell, cv::Point& endCell)
{
    bool foundCastle = false;
    printf("isCastleMove check\r\n");
    // use passed images (expected to be warped/grayscale or edge images)
    const cv::Mat& gray1 = warpedGray1(cv::Rect(0,0,warpedGray1.cols,warpedGray1.rows/8));
    const cv::Mat& gray2 = warpedGray2(cv::Rect(0,0,warpedGray1.cols,warpedGray1.rows/8));
    if (gray1.empty() || gray2.empty()) return false;

    cv::Mat diff_bin;
    cv::absdiff(gray1, gray2, diff_bin);
#if defined(DEBUG_SHOW_IMAGE)
    cv::imshow("catsle_diff_bin_gray",diff_bin);
#elif defined(DEBUG_WRITE_IMAGE)
    cv::imwrite("catsle_diff_bin_gray.jpg",diff_bin);
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
    for (int c = 0; c < 8; ++c) {
        int x = c * sq + off;
        int y = 0 * sq + off;
        cv::Rect roiRect(x, y, sub, sub);
        // clamp
        roiRect &= cv::Rect(0, 0, diff_bin.cols, diff_bin.rows);
        int diff_px = 0;
        if (roiRect.width > 0 && roiRect.height > 0)
            diff_px = cv::countNonZero(diff_bin(roiRect));
        if(diff_px < MIN_BINARY_POINT) continue;
        counts.emplace_back(diff_px, c, 0);
#if defined(DEBUG_SHOW_IMAGE) || defined(DEBUG_WRITE_IMAGE)
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
    if(counts.size() < 4) return false;
#if defined(DEBUG_SHOW_IMAGE) || defined(DEBUG_WRITE_IMAGE)
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

    // 1. Put the four numbers into a vector
    std::vector<int> nums = {std::get<1>(counts[0]), std::get<1>(counts[1]), std::get<1>(counts[2]),std::get<1>(counts[3])};

    // 2. Sort the vector in ascending order
    std::sort(nums.begin(), nums.end());
    printf("Sorted: ");
    for(int i = 0; i < nums.size(); i++) {
        printf("%d ",nums[i]);
    }
    printf("\r\n");
    // 3. Check the first possible window of 3 consecutive elements
    if (nums[1] == nums[0] + 1 && nums[2] == nums[1] + 1 && nums[3] == 7) {
        startCell.x = nums[0];
        startCell.y = 0;
        endCell.x = nums[2];
        endCell.y = 0;
        foundCastle = true;
    }

    // 4. Check the second possible window of 3 consecutive elements
    if (nums[3] == nums[2] + 1 && nums[2] == nums[1] + 1 && nums[0] == 0) {
        startCell.x = nums[3];
        startCell.y = 0;
        endCell.x = nums[1];
        endCell.y = 0;
        foundCastle = true;
    }
    printf("Start(%d,%d) end(%d,%d)\r\n",startCell.x,startCell.y,endCell.x,endCell.y);
#if defined(DEBUG_SHOW_IMAGE) || defined(DEBUG_WRITE_IMAGE)
    for (int i = 0; i < (int)counts.size(); ++i) {
        int cnt = std::get<0>(counts[i]);
        int c = std::get<1>(counts[i]);
        int r = std::get<2>(counts[i]);
        // mark top cells with thicker rectangle
        int x = c * sq + off; int y = r * sq + off;
        cv::Rect roiRect(x, y, sub, sub);
        roiRect &= cv::Rect(0,0,diff_bin.cols,diff_bin.rows);
        cv::rectangle(vis, roiRect, cv::Scalar(255,0,0), 2);
        std::string txt = "TOP:" + std::to_string(cnt);
        cv::putText(vis, txt, cv::Point(roiRect.x+2, roiRect.y+12), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255,255,0), 2);
    }
#endif

#if defined(DEBUG_SHOW_IMAGE)
    cv::imshow("castle_diff_bin", diff_bin);
    cv::imshow("castle_diff_counts", vis);
#elif defined (DEBUG_WRITE_IMAGE)
    cv::imwrite("castle_diff_bin.jpg",diff_bin);
    cv::imwrite("castle_diff_counts.jpg",vis);
#endif
    return foundCastle;
}

bool ChessImageProcessing::isCastleMove(char* prevBoard, char* currBoard,
                                        cv::Point& startCell, cv::Point& endCell) {
    int r = 0;
    startCell.x = -1;
    startCell.y = -1;
    endCell.x = -1;
    endCell.y = -1;
    printf("prev row:\r\n");
    for (int c = 0; c < 8; ++c) {
        printf("%c ",*(prevBoard + r*8 + c));
    }
    printf("\r\n");
    printf("curr row:\r\n");
    for (int c = 0; c < 8; ++c) {
        printf("%c ",*(currBoard + r*8 + c));
    }
    printf("\r\n");
    for (int c = 3; c <= 4; ++c) {
        if((*(prevBoard + r*8 + c) == 'k' || *(prevBoard + r*8 + c) == 'K') &&
                startCell.x < 0) {
            startCell.x = c;
            startCell.y = 0;
            break;
        }
    }
    for (int c = 1; c <= 6; ++c) {
        if((*(currBoard + r*8 + c) == 'k' || *(currBoard + r*8 + c) == 'K') &&
                endCell.x < 0) {
            endCell.x = c;
            endCell.y = 0;
            break;
        }
    }

    printf("startCell(x,y)=(%d,%d)\r\n",startCell.x,startCell.y);
    printf("endCell(x,y)=(%d,%d)\r\n",endCell.x,endCell.y);

    if(endCell.x>0 && startCell.x>0 && abs(endCell.x-startCell.x) == 2) {
        int prevRowRook = startCell.x < endCell.x?7:0;
        int currRowRook = (startCell.x + endCell.x)/2;
        printf("prevRowRook[%d] prevBoard[%c] newBoard[%c]\r\n",
               prevRowRook,
               *(prevBoard + r*8 + prevRowRook),
               *(currBoard + r*8 + prevRowRook));
        printf("currRowRook[%d] prevBoard[%c] newBoard[%c]\r\n",
               currRowRook,
               *(prevBoard + r*8 + currRowRook),
               *(currBoard + r*8 + currRowRook));
        if((*(prevBoard + r*8 + prevRowRook) == 'r' || *(prevBoard + r*8 + prevRowRook) == 'R') &&
           *(currBoard + r*8 + prevRowRook) == '.' &&
           (*(currBoard + r*8 + currRowRook) == 'r' || *(currBoard + r*8 + currRowRook) == 'R') &&
           *(prevBoard + r*8 + currRowRook) == '.')
        return true;
    }
    return false;
}

bool ChessImageProcessing::isPromoteMove(char* prevBoard, char* currBoard, cv::Point& startCell, cv::Point& endCell, char& promotePice) {
    return false;
}

ClassificationResult ChessImageProcessing::classifyImage(const cv::Mat& input_mat, int row, int col) {
    ClassificationResult result;
    result.className = '.';
    result.probability = 0;
    result.row = row;
    result.col = col;
    if (input_mat.empty()) {
        std::cerr << "Error: Provided input cv::Mat is empty.\n";
        return result;
    }

    // 1. Setup preprocessing parameters
    cv::Size target_size(240, 240);
    double scale_factor = 1.0 / 255.0; // Scale pixels to [0.0, 1.0]

    // PyTorch ImageNet mean values multiplied by 255.0 because blobFromImage
    // subtracts the raw mean *before* multiplying by the scalefactor.
    cv::Scalar mean(0.485 * 255.0, 0.456 * 255.0, 0.406 * 255.0);

    // PyTorch ImageNet standard deviation values
    cv::Scalar std_dev(0.229, 0.224, 0.225);

    // 2. Generate the 4D input blob
    cv::Mat blob;
    cv::dnn::blobFromImage(
        input_mat,
        blob,
        scale_factor,
        target_size,
        mean,
        true,  // swapRB = true (Converts BGR to RGB)
        false  // crop = false
    );

    // 3. Manually divide by standard deviation (OpenCV DNN doesn't do this automatically)
    cv::divide(blob, std_dev, blob);

    // 4. Run inference pass
    m_dnnNetAllPieces.setInput(blob);
    cv::Mat outputs = m_dnnNetAllPieces.forward(); // Output shape: [1, num_classes]

    // 5. Post-processing: Apply manual Softmax to the row vector
    float* data_ptr = outputs.ptr<float>(0);
    int num_classes = outputs.cols;

    std::vector<float> raw_scores(data_ptr, data_ptr + num_classes);
    std::vector<float> exp_scores(num_classes);

    float max_score = *std::max_element(raw_scores.begin(), raw_scores.end());
    float sum_exp = 0.0f;

    for (int i = 0; i < num_classes; ++i) {
        exp_scores[i] = std::exp(raw_scores[i] - max_score); // Stable Softmax implementation
        sum_exp += exp_scores[i];
    }

    // 6. Find the highest probability index
    int predicted_idx = 0;
    float max_prob = 0.0f;
    int predicted2_idx = 0;
    float max2_prob = 0.0f;
    for (int i = 0; i < num_classes; ++i) {
        float prob = exp_scores[i] / sum_exp;
#ifdef DEBUG_SINGLE_IMAGE
        printf("class[%s] prob[%f]\r\n",m_dnnAllPiecesNames[i].c_str(),prob);
#endif
        if (prob > max_prob) {
            max2_prob = max_prob;
            predicted2_idx = predicted_idx;
            max_prob = prob;
            predicted_idx = i;
        } else if (prob > max2_prob && prob != max_prob) {
            max2_prob = prob;
            predicted2_idx = i;
        }
    }
#if defined(DEBUG_CLASSIFICATION) && defined (DEBUG_SINGLE_IMAGE)
    // 7. Print Results
    std::cout << "Prediction Result: " << m_dnnAllPiecesNames[predicted_idx] << "\n";
    std::cout << "Confidence Level: " << std::fixed << (max_prob * 100.0f) << "%\n";
#endif
    result.className = m_dnnAllPiecesNames[predicted_idx];
    result.probability = max_prob * 100.0f;
    result.className2 = m_dnnAllPiecesNames[predicted2_idx];
    result.probability2 = max2_prob * 100.0f;
    return result;
}

void ChessImageProcessing::classsifyChessBoardImage(const cv::Mat& warpedBoard) {
    int cellSize = CELL_SIZE;
    printf("classsifyChessBoardImage (BATCH INF MODE ACTIVE):\r\n");
    auto start = std::chrono::steady_clock::now();
    // Pre-allocate containers to eliminate memory thrashing inside the core loop
    std::vector<cv::Mat> batchImages;
    std::vector<std::pair<int, int>> validCellPositions; // Stores tracking mappings: {row, col}
    batchImages.reserve(NUM_ROW * NUM_COL);
    validCellPositions.reserve(NUM_ROW * NUM_COL);

    // Phase 1: Rapidly parse coordinates and batch process structural cells
    for(int row = 0; row < NUM_ROW; row++) {
        for(int col = 0; col < NUM_COL; col++) {
            if(m_mapExcludedCell[row][col] == 0) {
                m_mapClassifiedCell[row][col] = '.';
                continue;
            }

            int cropX = col * cellSize;
            int cropY = row * cellSize;
            int cropW = cellSize;
            int cropH = cellSize;

            // Safe bound clamping logic
            if (cropX < 0) cropX = 0;
            if (cropY < 0) cropY = 0;
            if (cropX + cropW > warpedBoard.cols) cropW = warpedBoard.cols - cropX;
            if (cropY + cropH > warpedBoard.rows) cropH = warpedBoard.rows - cropY;

            cv::Rect tallCellROI(cropX, cropY, cropW, cropH);

            // Collect references safely without forcing local copies
            batchImages.push_back(warpedBoard(tallCellROI));
            validCellPositions.push_back({row, col});
        }
    }

    size_t totalValidPieces = batchImages.size();
    if (totalValidPieces == 0) {
        printf("No active piece cells identified for matching.\n");
        return;
    }

    // Phase 2: Create a 4D Tensor Batch Blob using 'blobFromImages'
    cv::Size target_size(240, 240);
    double scale_factor = 1.0 / 255.0;
    cv::Scalar mean(0.485 * 255.0, 0.456 * 255.0, 0.406 * 255.0);
    cv::Scalar std_dev(0.229, 0.224, 0.225);

    cv::Mat batchBlob;
    cv::dnn::blobFromImages(
        batchImages,
        batchBlob,
        scale_factor,
        target_size,
        mean,
        true,  // swapRB = true (Converts BGR to RGB)
        false  // crop = false
    );

    // Apply standard deviation correction across the 4D blob matrix elements
    cv::divide(batchBlob, std_dev, batchBlob);

    // Phase 3: Execute full batch processing in a single forward pass
    m_dnnNetAllPieces.setInput(batchBlob);
    cv::Mat outputs = m_dnnNetAllPieces.forward(); // Output matrix size: [totalValidPieces x num_classes]

    int num_classes = outputs.cols;

    // Phase 4: Parse back the tensor outputs maps
    for (size_t i = 0; i < totalValidPieces; ++i) {
        int row = validCellPositions[i].first;
        int col = validCellPositions[i].second;
        cv::Mat croppedCell = batchImages[i];

        // Fetch scores vector array pointer for item 'i'
        float* data_ptr = outputs.ptr<float>(static_cast<int>(i));

        std::vector<float> raw_scores(data_ptr, data_ptr + num_classes);
        std::vector<float> exp_scores(num_classes);

        float max_score = *std::max_element(raw_scores.begin(), raw_scores.end());
        float sum_exp = 0.0f;

        for (int c = 0; c < num_classes; ++c) {
            exp_scores[c] = std::exp(raw_scores[c] - max_score);
            sum_exp += exp_scores[c];
        }

        int predicted_idx = 0;
        float max_prob = 0.0f;
        int predicted2_idx = 0;
        float max2_prob = 0.0f;

        for (int c = 0; c < num_classes; ++c) {
            float prob = exp_scores[c] / sum_exp;
            if (prob > max_prob) {
                max2_prob = max_prob;
                predicted2_idx = predicted_idx;
                max_prob = prob;
                predicted_idx = c;
            } else if (prob > max2_prob && prob != max_prob) {
                max2_prob = prob;
                predicted2_idx = c;
            }
        }

        ClassificationResult piece;
        piece.row = row;
        piece.col = col;
        piece.className = m_dnnAllPiecesNames[predicted_idx];
        piece.probability = max_prob * 100.0f;
        piece.className2 = m_dnnAllPiecesNames[predicted2_idx];
        piece.probability2 = max2_prob * 100.0f;

        // Execute background color checks locally
        checkPieceColor(croppedCell, piece, row, col);
        m_mapClassifiedCell[row][col] = piece.className;

        // CRITICAL NOTE: Debug I/O commands ('cv::imwrite') removed from main loop logic
        // to prevent hard-disk read/write latency throttling. Only run when forced.
#ifdef DEBUG_ROI
        int cropX = col * cellSize;
        int cropY = row * cellSize;
        cv::Rect tallCellROI(cropX, cropY, croppedCell.cols, croppedCell.rows);
        cv::rectangle(warpedBoard,tallCellROI,cv::Scalar(0,255,255),2);
        cv::putText(warpedBoard,std::string{piece.className} + " :" +std::to_string((int)piece.probability),
                    cv::Point(cropX + 20,cropY+ 60),
                     cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
        cv::putText(warpedBoard,std::string{piece.className2} + " :" +std::to_string((int)piece.probability2),
                    cv::Point(cropX + 20,cropY+ 90),
                     cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
        cv::putText(warpedBoard,
                    "Gray: "+std::to_string(piece.grayPixels),
                    cv::Point(cropX + 20,cropY+ 120),
                     cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
        cv::putText(warpedBoard,
                    "Gold: "+std::to_string(piece.goldPixels),
                    cv::Point(cropX + 20,cropY+ 150),
                     cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);

#endif
    }
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Elapsed time: " << elapsed << " ms" << std::endl;

#ifdef DEBUG_ROI
    cv::Mat scaledWarped;
    cv::resize(warpedBoard, scaledWarped, cv::Size(WARP_SMALL_WIDTH, WARP_SMALL_HEIGHT), 0, 0, cv::INTER_NEAREST);
    cv::imshow("classification", scaledWarped);
#endif

    printf("Mapped Board:\r\n");
    for(int row = 0; row < NUM_ROW; row++) {
        for(int col = 0; col < NUM_COL; col++) {
            printf("%c ", m_mapClassifiedCell[row][col]);
        }
        printf("\r\n");
    }
}
void ChessImageProcessing::classsifyChessBoardImage2(const cv::Mat& warpedBoard) {
    int cellSize = CELL_SIZE;
    printf("classsifyChessBoardImage:\r\n");
    auto start = std::chrono::steady_clock::now();
    for(int row = 0; row < NUM_ROW; row ++) {
        for(int col = 0; col < NUM_COL; col ++) {
            if(m_mapExcludedCell[row][col] == 0) {
                m_mapClassifiedCell[row][col] = '.';
                continue;
            }
            // Stretch the bounding box upwards to swallow full tall piece outlines
            int cropX = col * cellSize;
            int cropY = row * cellSize;
            int cropW = cellSize;
            int cropH = cellSize;

            // Safe image-canvas bound clamping checks
            if (cropX < 0) cropX = 0;
            if (cropY < 0) cropY = 0;
            if (cropX + cropW > warpedBoard.cols) cropW = warpedBoard.cols - cropX;
            if (cropY + cropH > warpedBoard.rows) cropH = warpedBoard.rows - cropY;

            cv::Rect tallCellROI(cropX, cropY, cropW, cropH);
            cv::Mat croppedCell = warpedBoard(tallCellROI);
            ClassificationResult piece = classifyImage(croppedCell,row,col);
//            std::string cropCellName = "debug/"
//                                       "r"+std::to_string(row)+
//                                       "c"+std::to_string(col)+".jpg";
//            cv::imwrite(cropCellName,croppedCell);
            checkPieceColor(croppedCell, piece, row, col);
            m_mapClassifiedCell[row][col] = piece.className;
#ifdef DEBUG_ROI
            cv::rectangle(warpedBoard,tallCellROI,cv::Scalar(0,255,255),2);
            cv::putText(warpedBoard,std::string{piece.className} + " :" +std::to_string((int)piece.probability),
                        cv::Point(cropX + 20,cropY+ 60),
                         cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
            cv::putText(warpedBoard,std::string{piece.className2} + " :" +std::to_string((int)piece.probability2),
                        cv::Point(cropX + 20,cropY+ 90),
                         cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
            cv::putText(warpedBoard,
                        "Gray: "+std::to_string(piece.grayPixels),
                        cv::Point(cropX + 20,cropY+ 120),
                         cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
            cv::putText(warpedBoard,
                        "Gold: "+std::to_string(piece.goldPixels),
                        cv::Point(cropX + 20,cropY+ 150),
                         cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);

#endif
        }
    }
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Elapsed time: " << elapsed << " ms" << std::endl;

#ifdef DEBUG_ROI
    cv::Mat scaledWarped;
    cv::resize(warpedBoard,scaledWarped,cv::Size(WARP_SMALL_WIDTH,WARP_SMALL_HEIGHT),0,0, cv::INTER_NEAREST);
    cv::imshow("classification",scaledWarped);
#endif
    printf("Mapped Board:\r\n");
    for(int row = 0; row < NUM_ROW; row ++) {
        for(int col = 0; col < NUM_COL; col ++) {
            printf("%c ",m_mapClassifiedCell[row][col]);
        }
        printf("\r\n");
    }
}

void ChessImageProcessing::excludeCellList(std::vector<cv::Point> listCell) {
    memset(m_mapExcludedCell,1,NUM_ROW*NUM_COL);
    for(cv::Point cell: listCell) {
        if(cell.y >=0 && cell.y < NUM_ROW &&
            cell.x >=0 && cell.x < NUM_COL) {
            m_mapExcludedCell[cell.y][cell.x] = 0;
        }
    }
}

bool ChessImageProcessing::findDropCells(std::vector<cv::Point>& dropCells) {
    dropCells.clear();
    // Check drop zone on robot's right
    for(int row=NUM_ROW-1; row>=0; row--) {
        for(int col=NUM_COL-1; col>=NUM_COL-2; col--) {
            if(m_mapClassifiedCell[row][col] == '.') {
                // @Todo: Ignore first drop zone row
                if(NUM_ROW-1-row == 0 && NUM_COL-1-col<=1) continue;
                dropCells.push_back(cv::Point(NUM_COL-1-col,NUM_ROW-1-row));
                printf("dropCell r[%d] c[%d] from r[%d] c[%d]\r\n",
                       NUM_ROW-1-row,NUM_COL-1-col,row,col);
            }
        }
    }
    // Check drop zone on robot's left
    for(int row=NUM_ROW-1; row>=3; row--) {
        for(int col=1; col>=0; col--) {
            if(m_mapClassifiedCell[row][col] == '.') {
                dropCells.push_back(cv::Point(NUM_COL-1-col,NUM_ROW-1-row));
                printf("dropCell r[%d] c[%d] from r[%d] c[%d]\r\n",
                       NUM_ROW-1-row,NUM_COL-1-col,row,col);
            }
        }
    }
    return dropCells.size()>0;
}

bool ChessImageProcessing::findPromotePiece(cv::Point& promoteCell, char piece) {
    bool foundPromotePiece = false;
    // Check drop zone on robot's right
    for(int row=0; row<NUM_ROW; row++) {
        for(int col=NUM_COL-2; col<NUM_COL; col++) {
            if(m_mapClassifiedCell[row][col] == piece) {
                promoteCell = cv::Point(NUM_COL-1-col,NUM_ROW-1-row);
                foundPromotePiece = true;
                break;
            }
        }
        if(foundPromotePiece) break;
    }
    if(foundPromotePiece) return true;
    // Check drop zone on robot's left
    for(int row=3; row<NUM_ROW; row++) {
        for(int col=0; col<2; col++) {
            if(m_mapClassifiedCell[row][col] == piece) {
                promoteCell = cv::Point(NUM_COL-1-col,NUM_ROW-1-row);
                foundPromotePiece = true;
                break;
            }
        }
        if(foundPromotePiece) break;
    }
    return foundPromotePiece;
}

std::vector<std::string> ChessImageProcessing::findPossibleMoves2(
        const cv::Mat& imgCurrent,
        const char* prevBoard,
        const MoveDetectParams& params) {
    std::vector<std::string> listMoves;
    std::vector<cv::Point> listStartCell;
    std::vector<cv::Point> listChangedCell;
    // 1. Check board status
    cv::Mat warpImage;
    cv::Mat homographyMatrix = getFullTranformMatrix();
    cv::Mat warpedBoard;
    cv::warpPerspective(imgCurrent, warpedBoard, homographyMatrix, cv::Size(WARP_WIDTH, WARP_HEIGHT));
    printf("warpedBoard[%dx%d]\r\n",warpedBoard.cols,warpedBoard.rows);
    classsifyChessBoardImage(warpedBoard);
    char currentBoard[NUM_ROW][NUM_ROW];
    char convertedPrevBoard[NUM_ROW][NUM_ROW];
    for(int row = 0; row < NUM_ROW; row++) {
        for(int col = 0; col < NUM_ROW; col++) {
            currentBoard[row][col] =
                    m_mapClassifiedCell[row][col+3];
            if(params.playerSide == "white")
                convertedPrevBoard[NUM_ROW-1-row][NUM_ROW-1-col] =
                    prevBoard[row*NUM_ROW+col];
            else
                convertedPrevBoard[row][col] =
                    prevBoard[row*NUM_ROW+col];
        }
    }
    printf("Prev:\r\n");
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            printf("%c ",convertedPrevBoard[r][c]);
        }
        printf("\r\n");
    }
    printf("Curr:\r\n");
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            printf("%c ",currentBoard[r][c]);
        }
        printf("\r\n");
    }
    // 2. Check for castle
    cv::Point startCastle,endCastle;
    if(isCastleMove((char*)convertedPrevBoard,(char*)currentBoard,
                    startCastle,endCastle)) {
        std::string detectMove = coordToNotation(startCastle, params.playerSide)+
                coordToNotation(endCastle, params.playerSide);
        listMoves.push_back(detectMove);
        printf("Castle found\r\n");
        return listMoves;
    }

    // 3. Check for promotion
    cv::Point startPromote,endPromote;
    char promotePiece;
    if(isPromoteMove((char*)convertedPrevBoard,(char*)currentBoard,startPromote,endPromote,promotePiece)) {
        std::string promoteMove = coordToNotation(startCastle, params.playerSide)+
                coordToNotation(endCastle, params.playerSide)+std::string(1, promotePiece);
        listMoves.push_back(promoteMove);
        printf("Promote found\r\n");
        return listMoves;
    }
    // 4. Compare different with previous board
    for(int row = 0; row < NUM_ROW; row++) {
        for(int col = 0; col < NUM_ROW; col++) {
            // exception for pawn and Bishop
            if((convertedPrevBoard[row][col] == 'p' && currentBoard[row][col] == 'b') ||
               (convertedPrevBoard[row][col] == 'P' && currentBoard[row][col] == 'B')) {
                continue;
            }
            if(currentBoard[row][col] != convertedPrevBoard[row][col]) {
                printf("row[%d] col[%d] [%c] != [%c]\r\n",
                       row,col,
                       currentBoard[row][col],
                       convertedPrevBoard[row][col]);
                if(currentBoard[row][col] == '.') {
                    listStartCell.push_back(cv::Point(col,row));
                } else {
                    listChangedCell.push_back(cv::Point(col,row));
                }
            }
        }
    }


    // 5. Sort possible moves
    for (cv::Point fromCell: listStartCell) {
        for (cv::Point toCell: listChangedCell) {
            std::string detectMove = coordToNotation(fromCell, params.playerSide)+
                    coordToNotation(toCell, params.playerSide);
            bool existMove = false;
            for(std::string move: listMoves) {
                if(move == detectMove) {
                    existMove = true;
                    break;
                }
            }
            if(!existMove) listMoves.push_back(detectMove);
        }
    }
    std::cout << "findPossibleMoves2 done" << std::endl;
//    for(int i = 0; i< listMoves.size(); i++) {
//        printf("Possible Move %s\r\n",listMoves[i].c_str());
//    }
    return listMoves;
}
