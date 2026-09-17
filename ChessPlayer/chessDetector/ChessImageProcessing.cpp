#include "ChessImageProcessing.h"
#include <set>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cfloat>
#include <algorithm>

ChessImageProcessing::ChessImageProcessing()
{
    m_sourceConnected = false;
    m_isBlackSide = true;
    m_chessBoardRow = 8;
    m_chessBoardBox = 60;
    m_chessBoardSize = m_chessBoardRow * m_chessBoardBox;
    m_transformMaxtrixValid = false;
//    unsigned char testBoardPrev[64] = {
//        '.','k','.','.','.','.','.','K',
//        '.','.','.','.','.','.','.','.',
//        '.','.','.','.','.','.','.','.',
//        '.','.','.','.','.','.','.','.',
//        '.','.','.','.','.','.','.','.',
//        '.','.','.','.','.','.','.','.',
//        'N','p','.','.','.','.','.','.',
//        'R','N','.','.','.','.','.','.'};
//    unsigned char testBoardAfter[NUM_ROW][NUM_COL] = {
//    {'.','.','.', '.','k','.','.','.','.','.','K', '.','.','.'},
//    {'.','.','.', '.','.','.','.','.','.','.','.', '.','.','.'},
//    {'.','.','.', '.','.','.','.','.','.','.','.', '.','.','.'},
//    {'.','.','.', '.','.','.','.','.','.','.','.', '.','.','.'},
//    {'.','.','.', '.','.','.','.','.','.','.','.', '.','.','.'},
//    {'.','.','.', '.','.','.','.','.','.','.','.', '.','.','.'},
//    {'.','.','.', 'N','.','.','.','.','.','.','.', '.','.','.'},
//    {'.','.','.', 'q','N','.','.','.','.','.','.', '.','.','.'}};
//    for(int row = 0; row < NUM_ROW; row++) {
//        for(int col=0; col< NUM_COL; col++) {
//            m_mapClassifiedCell[row][col] = testBoardAfter[row][col];
//        }
//    }
//    MoveDetectParams params;
//    params.playerSide = "black";
//    findPossibleMoves2(cv::Mat(),(const char*)testBoardPrev,params);
}

void ChessImageProcessing::setDnnDetector(char *source, const std::vector<char> &dnnClassNames, int size, int channels)
{
    m_dnnDetector = cv::dnn::readNetFromONNX(source);
    m_dnnDetectorClassList = dnnClassNames;
    m_dnnDetector.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    m_dnnDetector.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    m_dnnDetectorSize = size;
    m_dnnDetectorChannels = channels;
    // 2. Set parallel processing worker threads to match your CPU capacity
    cv::setNumThreads(cv::getNumberOfCPUs());
    // Add this temporarily inside setDnnNetAllPieces to list all available layers
#if defined (DEBUG_CNN_LAYER)
    std::vector<std::string> layer_names = m_dnnDetector.getLayerNames();
    for (const auto& name : layer_names) {
        std::cout << "Layer available in ONNX graph: " << name << std::endl;
    }
    printf("%s [%s] ",__FUNCTION__,source);
    for(char className: dnnClassNames) {
        printf("%c ",className);
    }
    printf("\r\n");
#endif
}

void ChessImageProcessing::setDnnVerify(char *source, const std::vector<char> &dnnClassNames, int size, int channels)
{
    m_dnnVerify = cv::dnn::readNetFromONNX(source);
    m_dnnVerifyClassList = dnnClassNames;
    m_dnnVerify.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    m_dnnVerify.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    m_dnnVerifySize = size;
    m_dnnVerifyChannels = channels;
    // 2. Set parallel processing worker threads to match your CPU capacity
    cv::setNumThreads(cv::getNumberOfCPUs());
    // Add this temporarily inside setDnnNetAllPieces to list all available layers
#if defined (DEBUG_CNN_LAYER)
    std::vector<std::string> layer_names = m_dnnDetector.getLayerNames();
    for (const auto& name : layer_names) {
        std::cout << "Layer available in ONNX graph: " << name << std::endl;
    }
    printf("%s [%s] ",__FUNCTION__,source);
    for(char className: dnnClassNames) {
        printf("%c ",className);
    }
    printf("\r\n");
#endif
}

#if defined (USE_OPENVINO)
void ChessImageProcessing::setDnnNetAllPieces2(char* source, const std::vector<char>& dnnClassNames)
{
    m_dnnDetectorClassList = dnnClassNames;

    std::string sourcePath(source);
    size_t lastDot = sourcePath.find_last_of(".");
    std::string basePath = (lastDot != std::string::npos) ? sourcePath.substr(0, lastDot) : sourcePath;

    std::string xmlPath = basePath + ".xml";

    std::cout << "[OPENVINO NATIVE] Reading Model Graph: " << xmlPath << "\n";
    try {
        // 1. Read the network graph topology from the XML file
        std::shared_ptr<ov::Model> model = m_ovCore.read_model(xmlPath);

        // =========================================================================
        // FORCE MODEL INTERNAL LAYERS TO BE 100% STATIC (ELIMINATES 1460ms JIT DELAY)
        // =========================================================================
        std::cout << "[OPENVINO NATIVE] Forcing static dimension shapes [1, 3, "
                  << WARP_HEIGHT << ", " << WARP_WIDTH << "] onto internal network paths...\n";

        // Lock the primary input node's partial shape properties to unyielding static bounds
        model->reshape({{"input", ov::Shape({1, 3, WARP_HEIGHT, WARP_WIDTH})}});
        // =========================================================================

        // 2. Compile the locked, static model graph natively for your Intel CPU
        // Because the shape is now fully static, OpenVINO compiles optimized
        // AVX/SIMD instructions once right here at boot time!
        m_ovCompiledModel = m_ovCore.compile_model(model, "CPU");

        // 3. Create the optimized inference request pipeline
        m_ovInferRequest = m_ovCompiledModel.create_infer_request();

        // 4. Trigger one initial warm-up execution to prime the hardware cache registers
        std::cout << "[OPENVINO NATIVE] Triggering hardware cache compilation warmup pass...\n";
        m_ovInferRequest.infer();

        std::cout << "[OPENVINO NATIVE] System fully accelerated and ready for sub-25ms runs!\n";
    } catch (const std::exception& e) {
        std::cerr << "[OPENVINO ERROR] Native compiler failed: " << e.what() << std::endl;
    }
}
#endif

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
            goldPixels > 2000) {
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
                                        cv::Point& startCell, cv::Point& endCell,
                                        bool whiteMove) {
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

bool ChessImageProcessing::isPromoteMove(char* prevBoard, char* currBoard,
        cv::Point& startCell, cv::Point& endCell, char& promotePiece,
        bool whiteMove) {
    int rowPawn = 6;
    int rowPromote = 7;
    bool foundPromoteMove = false;
    startCell.x = -1;
    startCell.y = -1;
    endCell.x = -1;
    endCell.y = -1;
    promotePiece = '.';
    int sortedRow[2] = {6,7};
    printf("prev row:\r\n");
    for (int i = 0; i < 2; i++) {
        for (int c = 0; c < 8; ++c) {
            printf("%c ",*(prevBoard + sortedRow[i]*8 + c));
        }
        printf("\r\n");
    }
    printf("curr row:\r\n");
    for (int i = 0; i < 2; i++) {
        for (int c = 0; c < 8; ++c) {
            printf("%c ",*(currBoard + sortedRow[i]*8 + c));
        }
        printf("\r\n");
    }
    printf("\r\n");
    for(int pawnCol = 0; pawnCol < 8; pawnCol++) {
        if(*(prevBoard + rowPawn*8 + pawnCol) != (whiteMove?'P':'p')) continue;
        if(*(currBoard + rowPawn*8 + pawnCol) != '.') continue;
        // List of possible end move
        std::vector<int> possiblePromoteCol;
        if(pawnCol>0)possiblePromoteCol.push_back(pawnCol-1);
        possiblePromoteCol.push_back(pawnCol);
        if(pawnCol<7)possiblePromoteCol.push_back(pawnCol+1);
        printf("possiblePromoteCol: ");
        for(int promoteCol: possiblePromoteCol) {
            printf("%d ",promoteCol);
        }
        printf("\r\n");
        for(int promoteCol: possiblePromoteCol) {
            char piecePrev = *(prevBoard + rowPromote*8 + promoteCol);
            char pieceCurr = *(currBoard + rowPromote*8 + promoteCol);

            bool isValidEndMove =
                    (piecePrev == '.' && promoteCol == pawnCol) ||
                    (piecePrev == (!whiteMove?'Q':'q') && promoteCol != pawnCol) ||
                    (piecePrev == (!whiteMove?'R':'r') && promoteCol != pawnCol) ||
                    (piecePrev == (!whiteMove?'B':'b') && promoteCol != pawnCol) ||
                    (piecePrev == (!whiteMove?'N':'n') && promoteCol != pawnCol);
            bool isValidPromote =
                    pieceCurr == (whiteMove?'Q':'q') ||
                    pieceCurr == (whiteMove?'R':'r') ||
                    pieceCurr == (whiteMove?'B':'b') ||
                    pieceCurr == (whiteMove?'N':'n');
            printf("promoteCol: %d piecePrev[%c] pieceCurr[%c]"
                   "isValidEndMove[%s] "
                   "isValidPromote[%s]\r\n",
                   promoteCol,piecePrev,pieceCurr,
                   isValidEndMove?"true":"false",
                   isValidPromote?"true":"false");
            if(isValidEndMove && isValidPromote)
            {
                startCell.x = pawnCol;
                startCell.y = rowPawn;
                endCell.x = promoteCol;
                endCell.y = rowPromote;
                promotePiece = pieceCurr;
                foundPromoteMove = true;
                printf("Found promote startCell(x,y)(%d,%d) endCell(x,y)(%d,%d) promotePiece(%c)\r\n",
                       startCell.x,startCell.y,endCell.x,endCell.y,promotePiece);
                break;
            }
        }
        if(foundPromoteMove) break;
    }
    return foundPromoteMove;
}

ClassificationResult ChessImageProcessing::classifyImage(cv::dnn::Net& dnn, std::vector<char>& classList,
                                                         const cv::Mat& input_mat, int row, int col) {
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
    dnn.setInput(blob);
    cv::Mat outputs = dnn.forward(); // Output shape: [1, num_classes]

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
        printf("class[%s] prob[%f]\r\n",m_dnnDetectorClassList[i].c_str(),prob);
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
    std::cout << "Prediction Result: " << m_dnnDetectorClassList[predicted_idx] << "\n";
    std::cout << "Confidence Level: " << std::fixed << (max_prob * 100.0f) << "%\n";
#endif
    result.className = classList[predicted_idx];
    result.probability = max_prob * 100.0f;
    result.className2 = classList[predicted2_idx];
    result.probability2 = max2_prob * 100.0f;
    return result;
}
void ChessImageProcessing::initializeManualClassificationHead() {
    if (m_isFcLayersInitialized) return;

    // 1. Allocate your 7x512 matrix buffers
    m_fcWeightsMat = cv::Mat::zeros(7, 512, CV_32F);
    m_fcBiasMat = cv::Mat::zeros(7, 1, CV_32F);

    // 2. PASTE THE ARRAYS GENERATED FROM THE PYTHON SCRIPT HERE
    float fc_w[7][512] = {
        {0.03465308f, -0.00896876f, -0.01585735f, -0.05203325f, 0.05040428f, 0.05284245f, 0.01501312f, 0.00562584f, 0.06110382f, -0.00024928f, -0.04353357f, -0.04863997f, -0.05828564f, -0.03360134f, -0.04143484f, 0.05844466f, -0.00204907f, 0.02562679f, -0.01715310f, 0.04347937f, -0.02648856f, -0.05125313f, -0.02609954f, 0.04588690f, 0.03349015f, -0.00192665f, -0.06206458f, 0.00932763f, -0.01864798f, -0.02690504f, 0.00848358f, 0.00687431f, -0.07126023f, 0.04008577f, 0.01997527f, -0.01872418f, -0.00874981f, 0.03525451f, 0.00588451f, 0.01760094f, -0.05846168f, -0.03382939f, 0.06485935f, 0.02228607f, 0.02901779f, -0.04902654f, -0.01041371f, 0.07458510f, 0.05114544f, 0.03172970f, 0.03150978f, 0.03393223f, -0.03443530f, 0.05196164f, 0.01155255f, -0.02198213f, 0.03440581f, 0.06366690f, 0.05106530f, 0.03188507f, 0.06823247f, -0.00285641f, -0.06548460f, 0.01387615f, -0.00027317f, -0.01993633f, -0.03728484f, 0.05381423f, 0.04749684f, 0.00621690f, 0.00406103f, -0.02798568f, 0.05411337f, 0.00675139f, -0.02220331f, 0.02327770f, -0.03469380f, -0.04328842f, 0.01758356f, -0.02041922f, -0.02710622f, -0.02254842f, 0.03759110f, -0.02723694f, 0.04927546f, -0.00057543f, -0.01959346f, -0.04345888f, -0.00926250f, -0.04093121f, -0.04731857f, -0.04274258f, -0.00800551f, -0.01167154f, 0.04808694f, -0.02168800f, 0.02028087f, 0.05734071f, 0.01993165f, -0.07254183f, 0.07892513f, 0.03455229f, 0.03921795f, -0.04923163f, 0.06024966f, -0.01075581f, -0.04644943f, 0.03056764f, 0.05239905f, 0.05597624f, 0.05350228f, -0.05378594f, 0.05174822f, 0.08926436f, 0.03422894f, 0.03899243f, 0.04887958f, -0.03397090f, -0.02527484f, 0.00412822f, -0.06993126f, -0.04202507f, -0.05586881f, 0.02002877f, -0.00342113f, 0.09858669f, -0.05477770f, -0.00356757f, 0.06003740f, -0.00286546f, -0.02375596f, 0.03849950f, -0.02471220f, -0.01965440f, -0.04843432f, -0.00177240f, -0.01473496f, 0.05453856f, -0.00504270f, 0.03933509f, 0.06761574f, 0.07567424f, 0.03629376f, -0.06624132f, 0.06942644f, 0.00056302f, 0.00304427f, -0.02432464f, -0.03013463f, -0.02338893f, 0.00531291f, -0.04209842f, -0.04286333f, 0.03529078f, -0.04443715f, 0.04966746f, 0.03581302f, -0.03263199f, 0.03669046f, 0.05611087f, 0.01975502f, 0.05405172f, 0.05596804f, 0.02167096f, -0.03015055f, -0.03651037f, 0.00765186f, 0.05359368f, -0.05043211f, -0.02612280f, -0.04243248f, -0.03622039f, -0.04268769f, -0.00395371f, -0.04088500f, -0.05374696f, 0.04456161f, 0.00895825f, -0.03191033f, 0.05242852f, 0.01214285f, -0.00352603f, -0.03869351f, -0.04477385f, -0.00607049f, 0.03200362f, -0.02616255f, -0.04939480f, -0.04539124f, 0.05768710f, -0.00700813f, -0.04389567f, 0.03309056f, -0.00169696f, 0.04000085f, 0.06362671f, 0.02858697f, -0.04060173f, -0.04278986f, -0.06533708f, -0.03482121f, 0.01709025f, -0.00943960f, -0.00893954f, -0.03063368f, -0.02089179f, 0.02762529f, -0.05139472f, 0.02146965f, -0.00912949f, 0.05112173f, 0.04130138f, 0.03739845f, 0.00252207f, 0.00818352f, 0.03458923f, 0.00642038f, 0.07997566f, -0.05978407f, -0.02134029f, 0.04885205f, 0.03682189f, 0.01099883f, 0.05885723f, 0.00955117f, 0.04321918f, -0.05467206f, -0.03781700f, 0.08258801f, 0.00768394f, -0.01747972f, 0.08955648f, -0.00799195f, 0.02709481f, -0.03119140f, -0.00312057f, -0.00559979f, -0.01837453f, 0.06800567f, 0.01918600f, 0.01098191f, -0.05637978f, 0.07515097f, 0.08192275f, -0.06177364f, -0.01049174f, -0.07177532f, 0.00602803f, -0.04062058f, 0.03219299f, 0.00382840f, -0.02914967f, 0.04507769f, -0.00583872f, -0.00960130f, -0.00062174f, 0.01149505f, 0.05795000f, -0.03649175f, -0.04539348f, 0.02827279f, -0.05349929f, -0.03457073f, -0.02612018f, -0.02250251f, -0.01219630f, -0.02482220f, 0.05635973f, -0.01526205f, -0.05452698f, 0.00919857f, -0.00889257f, 0.04048854f, 0.01576469f, 0.05366600f, -0.05981998f, -0.01423823f, 0.03794084f, -0.05735544f, 0.05934644f, -0.02292715f, -0.03755629f, -0.02722082f, 0.05722098f, 0.01283860f, 0.02657587f, 0.00389935f, 0.08615240f, 0.02447247f, 0.00380521f, 0.06058525f, 0.06601369f, -0.00661275f, 0.02503730f, 0.01333529f, -0.05219819f, -0.02167366f, -0.04156533f, -0.00625683f, 0.03221786f, 0.02920558f, -0.01501732f, -0.02257136f, 0.00927807f, -0.03682661f, 0.01251062f, 0.07067025f, 0.00701406f, -0.00997210f, 0.03704545f, 0.05832268f, 0.00367079f, -0.03823803f, 0.01499027f, 0.01995506f, 0.05197097f, 0.05262046f, 0.03809857f, -0.01049081f, 0.04637755f, 0.03523193f, -0.04647727f, -0.01719027f, 0.00852776f, -0.08416031f, 0.05404815f, -0.03390206f, -0.02286392f, 0.09456174f, -0.05043821f, -0.04446527f, 0.04098423f, 0.04411869f, 0.03839148f, -0.05278564f, -0.01349532f, 0.00283972f, 0.00810977f, -0.01770587f, 0.01359640f, 0.02556519f, 0.02938146f, 0.02513818f, -0.05186823f, 0.00554182f, -0.01596342f, -0.03398206f, 0.02741190f, 0.04211693f, -0.03119645f, -0.04191203f, -0.03685257f, 0.01389950f, -0.03380192f, -0.04531613f, 0.01595931f, -0.02723350f, -0.03334440f, 0.00454518f, 0.04106329f, -0.01240102f, -0.03507381f, 0.04346739f, 0.06373470f, -0.02308878f, 0.02677721f, 0.03464672f, -0.02333203f, -0.04669965f, -0.03043136f, -0.01635654f, -0.06388052f, -0.05711156f, 0.06830876f, 0.06121381f, 0.01037221f, 0.05198750f, -0.01741981f, -0.05026095f, -0.05831395f, -0.06577206f, -0.01033518f, 0.02932109f, 0.03188308f, 0.01685281f, -0.02122199f, 0.02633622f, 0.02319549f, -0.04731727f, -0.00772309f, 0.00289035f, 0.02525461f, 0.06887855f, 0.04533489f, 0.03071110f, -0.04799753f, -0.00749031f, -0.04895772f, 0.04925225f, 0.01220992f, -0.02450499f, 0.01401829f, 0.03301731f, -0.04462633f, 0.03246665f, 0.00297272f, -0.04094344f, 0.05638285f, -0.04312482f, 0.00813299f, 0.03111420f, -0.03387820f, 0.00800203f, 0.06368740f, -0.04785196f, -0.00375679f, -0.06023999f, -0.06466740f, -0.04985345f, -0.02875031f, 0.01641572f, 0.02396188f, 0.00350855f, 0.00443910f, 0.05139585f, 0.04883130f, -0.04258929f, -0.02383976f, 0.01277640f, 0.02833719f, 0.04142329f, 0.04399725f, -0.04749170f, -0.02164690f, -0.02766214f, -0.01957994f, 0.00509684f, -0.00968736f, 0.03826549f, 0.04671367f, -0.03825413f, -0.04315092f, 0.03458977f, -0.02503409f, 0.00201069f, 0.00412042f, 0.01534483f, -0.05235522f, -0.07943051f, 0.01642039f, 0.05129949f, 0.03747179f, 0.01280549f, 0.06071441f, -0.04230144f, 0.05219197f, -0.02960861f, -0.00944239f, -0.05972163f, -0.02184140f, 0.02399890f, 0.02705805f, -0.04397094f, 0.03479045f, 0.01042909f, -0.00235355f, -0.05487303f, 0.04863322f, -0.04126810f, -0.05640616f, 0.01535545f, 0.01267218f, 0.02711587f, 0.00430089f, 0.08101133f, 0.05790692f, -0.00999433f, -0.01220765f, -0.03588613f, -0.03874316f, 0.03243814f, 0.02855364f, 0.02302181f, 0.05880964f, 0.03691151f, -0.00268618f, -0.01904056f, 0.04314248f, -0.01243414f, -0.04649598f, -0.02284064f, -0.02343771f, 0.06238808f, 0.03632359f, -0.06265260f, 0.01307306f, 0.01626836f, 0.02744634f, -0.03937897f, 0.01636689f, -0.00835700f, -0.04430025f, -0.00782690f, -0.04200051f, 0.01157601f, -0.02883529f, 0.05125659f, -0.01721219f, -0.07695171f, -0.05534026f, 0.05916390f, -0.00637873f},
        {-0.04733917f, -0.05316356f, 0.01894157f, -0.06706548f, -0.05309444f, -0.03240475f, -0.02441956f, -0.06515930f, -0.04238519f, 0.08434640f, -0.06052085f, -0.05373555f, -0.05270773f, 0.06740645f, -0.00123916f, -0.00136019f, 0.05741275f, 0.05963458f, -0.05000088f, -0.05020169f, -0.03707727f, -0.03419616f, 0.06471223f, -0.02120575f, 0.01814815f, -0.01116308f, -0.02073880f, -0.02372299f, -0.05393567f, 0.01172914f, 0.03865273f, 0.00549599f, -0.05192985f, 0.00992109f, -0.00633659f, -0.05868036f, 0.03913137f, -0.06059896f, -0.05265247f, -0.00140844f, -0.04097271f, -0.01929184f, -0.02484221f, 0.03864833f, -0.04095682f, 0.00848730f, -0.02355314f, -0.00210057f, -0.05179899f, 0.01586629f, -0.03553842f, -0.07031367f, -0.03848718f, -0.02224540f, 0.03235045f, 0.01468975f, 0.02709945f, 0.00527632f, 0.03288400f, -0.01807069f, -0.06319174f, 0.00055264f, -0.02110206f, -0.06332865f, -0.03330892f, -0.04979808f, 0.06526925f, -0.06405303f, -0.03971585f, -0.03697180f, -0.05109159f, 0.02149050f, -0.04319105f, -0.02170004f, -0.04455823f, -0.06096286f, -0.03773114f, 0.00748910f, -0.06752612f, -0.03763007f, -0.07130963f, 0.02000263f, 0.00743940f, -0.07319023f, -0.00153250f, 0.02944981f, -0.03275000f, 0.02919530f, -0.00155204f, -0.07979731f, -0.04567790f, 0.01279945f, 0.04721219f, -0.03217593f, -0.05102102f, -0.02541322f, -0.05423823f, 0.02134262f, -0.06203095f, 0.03083885f, -0.06742536f, -0.03242888f, -0.01707313f, -0.02013443f, -0.05653135f, 0.04578662f, -0.00546466f, -0.04879204f, -0.03097491f, -0.01409381f, 0.02362320f, -0.04318061f, -0.03326957f, -0.00644974f, -0.01858598f, -0.04318908f, 0.03623673f, 0.02757475f, -0.00548457f, 0.00241606f, -0.03319348f, 0.01755266f, 0.01178659f, 0.02659588f, -0.07243041f, -0.00384609f, -0.00752830f, -0.05136655f, -0.03788904f, -0.04938713f, -0.02169519f, -0.00816646f, -0.02273367f, -0.00678303f, -0.01980894f, 0.01485986f, 0.09972184f, -0.00019594f, -0.02791438f, -0.02653870f, -0.00794284f, 0.00528091f, -0.05719919f, 0.00167003f, -0.04089019f, -0.06924751f, -0.05726752f, -0.03924777f, -0.00518772f, -0.01527103f, 0.01088735f, -0.05297699f, 0.04051741f, -0.04886675f, -0.08616897f, -0.04999854f, 0.01170100f, -0.03798552f, -0.02222572f, -0.03106318f, 0.01480320f, -0.04122007f, -0.03567424f, -0.06813864f, -0.06061833f, 0.03854837f, 0.02771101f, -0.03828571f, -0.09448645f, 0.00678895f, -0.05809656f, -0.03153391f, 0.05380401f, -0.06198958f, 0.01781805f, -0.05733287f, 0.02632881f, 0.07370230f, 0.00922136f, -0.05951144f, 0.00496674f, -0.07021637f, 0.00755735f, -0.03155059f, -0.03203217f, 0.04233927f, -0.04744099f, -0.03221007f, -0.05113637f, -0.04127572f, 0.00052052f, 0.06432553f, -0.04680778f, -0.08148154f, -0.06230739f, 0.01275839f, 0.03352227f, -0.05058082f, -0.02552677f, -0.02559575f, 0.01651100f, 0.03133682f, -0.07169442f, 0.03887563f, -0.05019308f, 0.00284485f, 0.00446037f, 0.02544740f, 0.04470836f, 0.05663112f, -0.00810196f, -0.00452975f, -0.04392642f, 0.03051908f, 0.03512174f, -0.07577111f, 0.05637998f, -0.02800278f, -0.07581005f, -0.00100068f, 0.00799843f, -0.05055473f, -0.06166363f, 0.01123735f, -0.04942590f, 0.03621918f, -0.00553164f, -0.05401562f, 0.00404499f, -0.00683771f, -0.03853717f, -0.03636653f, -0.01742213f, -0.08244110f, -0.02124296f, 0.00865146f, -0.01570053f, -0.00416618f, 0.00048498f, -0.00421363f, 0.01263960f, 0.07099381f, -0.06206971f, -0.01827761f, 0.04134508f, 0.00259922f, 0.00192312f, 0.00969476f, -0.00882563f, -0.02085125f, -0.00978032f, -0.01011506f, -0.00738924f, 0.01205272f, -0.02956030f, -0.02710709f, -0.01823390f, -0.07153243f, 0.02585853f, -0.06822228f, -0.03389443f, -0.00210615f, -0.06645290f, 0.02955151f, -0.02754541f, -0.04399935f, -0.04941477f, -0.03476366f, -0.04558549f, -0.04064161f, -0.03137570f, 0.03615373f, 0.05022842f, -0.06672235f, -0.02094034f, 0.00796910f, 0.04341592f, 0.01977259f, 0.01688733f, -0.04920736f, 0.01052368f, 0.01518504f, 0.00742592f, -0.00710756f, 0.02250492f, 0.02243311f, -0.06877375f, -0.02727839f, -0.06514129f, -0.06382237f, 0.00193901f, 0.05195904f, 0.06259939f, -0.03064778f, -0.04890966f, -0.07985733f, 0.00281639f, 0.04753332f, -0.02102498f, -0.02228613f, -0.03883693f, -0.01964596f, 0.04271654f, -0.05351358f, -0.01542664f, 0.03066171f, -0.09232191f, 0.04637909f, -0.04620620f, 0.00043645f, -0.03927720f, -0.01652835f, 0.00666614f, 0.01446929f, -0.07064612f, -0.05480530f, 0.00116522f, -0.06390959f, -0.04857970f, 0.01777005f, -0.01153149f, -0.00336829f, -0.02243001f, -0.03608191f, -0.07513497f, 0.02536533f, 0.01858117f, -0.02908349f, 0.00983329f, -0.01536137f, -0.01083323f, -0.04564705f, 0.01565385f, -0.05854094f, -0.04933265f, -0.04780814f, -0.03729125f, -0.06181220f, -0.04734180f, 0.00595157f, -0.06741464f, -0.03184210f, -0.01376115f, 0.05125951f, 0.00076666f, -0.01135165f, 0.02709402f, -0.01085944f, -0.03906059f, -0.01171984f, 0.02710336f, -0.03677668f, 0.03780341f, 0.04710082f, -0.01416944f, -0.06771874f, 0.00312063f, 0.00726234f, 0.00495532f, -0.02676461f, -0.06656179f, -0.04335756f, -0.05159557f, -0.05578632f, 0.01160918f, -0.06422132f, -0.02707501f, 0.06093197f, 0.05306024f, 0.00658406f, -0.05951537f, -0.06578297f, -0.02864109f, -0.03900233f, -0.00509695f, -0.02164033f, -0.07370100f, -0.08047694f, -0.04810256f, -0.05842255f, 0.05422572f, 0.06683566f, -0.01355020f, -0.05529034f, 0.00284056f, -0.02768442f, 0.01533796f, -0.01361852f, 0.03557872f, -0.08067646f, 0.00476188f, -0.02102417f, 0.00355504f, -0.05549240f, -0.06783672f, 0.02266228f, 0.02357950f, 0.01032663f, -0.05543108f, 0.01366712f, 0.00872667f, -0.01930740f, -0.03217228f, -0.05159849f, -0.00137889f, -0.01778376f, 0.02668053f, -0.05623071f, -0.02674956f, -0.04570105f, -0.01451306f, -0.06335887f, -0.05387137f, 0.00200720f, -0.03088302f, -0.08567298f, 0.00873297f, 0.03562177f, -0.01263613f, 0.01624924f, 0.00191367f, -0.07507875f, -0.01169013f, -0.04734493f, -0.03012142f, -0.03992171f, -0.06179284f, 0.00182148f, -0.04838709f, -0.04327454f, -0.06117288f, -0.08278650f, -0.06124556f, -0.02085741f, -0.06037434f, -0.05960115f, -0.04348126f, -0.05533526f, 0.00371467f, -0.03272924f, -0.01633725f, -0.06132221f, -0.00859081f, 0.03717636f, -0.08943384f, 0.05290835f, -0.00879065f, -0.04114281f, 0.01502470f, 0.00278273f, -0.05684531f, -0.05218000f, -0.06618314f, -0.00632383f, -0.06334428f, -0.02238560f, -0.01295470f, 0.04075757f, -0.04079011f, -0.00241754f, -0.01125882f, -0.07379144f, -0.01597891f, -0.05081331f, -0.06997806f, -0.00823423f, 0.04981070f, -0.06412090f, 0.03249896f, 0.03423361f, -0.03044678f, -0.04414293f, -0.02992054f, -0.06535441f, 0.01191071f, -0.00534900f, 0.07422802f, -0.02983946f, 0.04833280f, 0.06204943f, -0.03280762f, 0.01526196f, -0.03756433f, -0.03145920f, -0.04814930f, -0.03283257f, -0.03510004f, 0.03038906f, -0.04708307f, 0.04406098f, 0.05162601f, -0.01265366f, -0.03573022f, -0.03920324f, 0.03570011f, -0.04176585f, -0.03574486f, -0.03016260f, -0.03467467f, -0.06116162f, -0.02449734f, -0.05906532f, -0.05097784f, 0.00858895f, 0.00054811f, -0.03822352f, -0.02175216f, -0.03232625f, -0.08059542f, -0.05559955f, -0.05947295f, -0.01172634f},
        {0.04447519f, 0.03607584f, -0.05350597f, 0.01113410f, 0.02386109f, 0.05449356f, 0.06697781f, -0.03864652f, 0.04759425f, 0.02442968f, 0.05155684f, 0.05169915f, -0.01443361f, -0.01170878f, 0.05561959f, 0.07445627f, 0.04949787f, -0.04209035f, -0.01786011f, 0.01562204f, 0.05338176f, -0.01138829f, -0.03006180f, -0.05771576f, -0.00147648f, -0.01866199f, 0.01661239f, -0.07327587f, 0.01080501f, -0.04491809f, 0.03487952f, -0.03311193f, -0.01914011f, -0.05271864f, -0.03033242f, 0.06714342f, -0.05871066f, 0.05857057f, 0.02886599f, -0.05690384f, 0.05446149f, 0.04888546f, -0.00231546f, 0.04773952f, -0.05413172f, 0.05116265f, -0.00045308f, 0.01730985f, 0.04223089f, -0.04216965f, -0.00819616f, -0.04173211f, -0.01878675f, -0.01419643f, 0.08215526f, 0.02220800f, 0.02960141f, 0.03677989f, -0.03476283f, 0.02832064f, 0.01799676f, -0.02821665f, 0.06586359f, 0.03681643f, -0.05744462f, 0.04818609f, -0.00526468f, -0.07723258f, -0.03182625f, 0.07863791f, -0.05078711f, 0.01025332f, -0.03781126f, -0.00538396f, 0.00244223f, -0.04402290f, -0.06316314f, 0.05417593f, -0.08212425f, -0.05505776f, -0.04868048f, 0.04150163f, -0.07954121f, 0.05981867f, 0.04507720f, 0.01051301f, 0.02496021f, 0.02559811f, 0.05395500f, -0.00581342f, 0.04608529f, -0.04167089f, -0.03009328f, 0.03644814f, -0.02039638f, 0.02221382f, -0.05380194f, -0.03843763f, -0.01669961f, 0.03345104f, -0.03065064f, 0.01973920f, 0.04054597f, 0.05586268f, 0.04401614f, -0.02573245f, 0.04680323f, -0.03916769f, -0.03307497f, -0.05107948f, -0.03662121f, -0.00536836f, 0.04947384f, -0.05868478f, 0.02257529f, -0.00405570f, 0.02979751f, 0.02914261f, -0.04735279f, 0.04442324f, -0.02420741f, 0.01852113f, 0.06599390f, -0.02725794f, -0.00823191f, 0.01044322f, 0.05194852f, -0.04697780f, -0.02766182f, 0.05427928f, 0.05044523f, -0.04051351f, -0.02730479f, 0.00672483f, 0.00960649f, -0.05350637f, -0.01084920f, 0.00623504f, 0.04675861f, -0.09450611f, 0.03371691f, -0.04638312f, -0.00920282f, 0.06592844f, 0.04078027f, 0.00452712f, 0.02318638f, -0.02509077f, -0.03007570f, -0.04132555f, -0.02129184f, 0.06246440f, -0.04328895f, -0.00785671f, 0.02470918f, -0.04035646f, -0.03954582f, 0.00000177f, 0.06178018f, 0.05749052f, 0.04885897f, 0.00395787f, 0.06829409f, -0.02726076f, 0.03976394f, 0.01323761f, -0.04082908f, 0.01787883f, -0.05019990f, -0.03585151f, 0.00581599f, -0.01706272f, -0.04121773f, -0.03966551f, -0.04863362f, -0.05321677f, -0.01013365f, 0.01624417f, 0.06833520f, -0.04356685f, -0.06804648f, -0.02789310f, -0.01329413f, -0.02408256f, 0.00154714f, -0.04651198f, -0.02642716f, 0.05161849f, 0.05165977f, -0.00963456f, 0.06478558f, -0.06656643f, -0.03176631f, -0.04295602f, -0.05693639f, 0.03672165f, -0.05681312f, -0.03990430f, 0.01066811f, 0.03635202f, -0.01231989f, -0.04251831f, -0.03868419f, -0.04839443f, 0.04214463f, -0.06318620f, -0.05413171f, -0.04633590f, -0.02028925f, -0.03638516f, -0.03338730f, -0.04108180f, -0.02399917f, -0.06063506f, -0.00633297f, 0.01761268f, -0.05291776f, 0.03424075f, -0.07141008f, 0.07103477f, 0.05140839f, -0.01177539f, 0.03271363f, 0.01120072f, 0.04899077f, 0.02976916f, -0.00026052f, 0.01583828f, -0.01391338f, -0.03030663f, -0.00214291f, -0.02440997f, -0.02656576f, -0.03914800f, -0.00684576f, 0.10404343f, -0.05378604f, -0.04158657f, 0.00324631f, -0.04508661f, 0.02273264f, -0.03521089f, 0.05224975f, 0.04021815f, -0.02684979f, 0.06836047f, -0.04396288f, -0.04992731f, -0.05569157f, 0.03893118f, 0.06390873f, -0.01172914f, -0.01882032f, -0.05034522f, -0.00103222f, -0.01571357f, 0.04721553f, -0.02013473f, 0.04655556f, 0.04655559f, 0.00006290f, 0.06873958f, -0.04235356f, 0.06298267f, 0.03468144f, 0.05854081f, 0.05008720f, -0.05642515f, -0.03086762f, 0.02406815f, 0.03853607f, 0.02139496f, 0.01575567f, -0.06319675f, -0.00895479f, 0.06425226f, -0.04782507f, 0.01563572f, -0.05962450f, 0.04139305f, 0.05348342f, -0.05210154f, -0.05038827f, 0.07177542f, 0.03738353f, 0.03724677f, -0.06063169f, 0.01162179f, -0.03151644f, -0.02692012f, -0.01973615f, 0.02121037f, -0.01356718f, 0.06940616f, -0.05080630f, -0.07442675f, 0.03683281f, -0.03210000f, 0.05603446f, -0.00974647f, -0.05659252f, -0.03960565f, 0.01007347f, -0.05475575f, 0.03865124f, -0.05677408f, -0.03810897f, -0.04780420f, 0.00906451f, 0.00132464f, -0.02257892f, 0.03358389f, -0.00640054f, 0.00204277f, -0.06050914f, -0.01535844f, -0.05732826f, 0.06039580f, -0.04069949f, 0.04727665f, -0.00161716f, 0.04091282f, 0.05627613f, 0.06839304f, 0.07815217f, 0.04517950f, -0.03310018f, 0.03240707f, 0.04623213f, 0.02551558f, 0.02919239f, 0.01278807f, -0.00638442f, -0.02835663f, -0.00176192f, 0.02943755f, 0.01590848f, -0.03811628f, -0.07011300f, 0.04445546f, 0.01664546f, -0.03796814f, -0.02431094f, 0.03221510f, -0.06961153f, -0.03012026f, 0.04629346f, -0.04235163f, 0.05184999f, 0.01401628f, -0.03549483f, -0.02981031f, -0.03060403f, -0.05015403f, -0.02530379f, 0.05013092f, -0.05825514f, -0.02127146f, -0.01887667f, -0.06287850f, 0.06062886f, 0.03529597f, -0.05736573f, -0.01861093f, 0.06015424f, -0.00626973f, 0.07568853f, 0.04662564f, -0.02329934f, 0.04565259f, 0.00503073f, 0.05919842f, 0.03951450f, 0.01336257f, 0.00124383f, 0.02564100f, -0.01547783f, -0.03068797f, 0.06245422f, 0.00036392f, 0.01340487f, 0.07591867f, 0.01203167f, 0.04990653f, -0.04709626f, -0.03692451f, -0.07036193f, 0.07027445f, -0.00027907f, 0.01252530f, 0.02865567f, -0.07260162f, -0.06885111f, 0.00115652f, 0.01893291f, -0.01723303f, 0.03216429f, 0.01403032f, 0.03450808f, 0.04052313f, 0.01164720f, -0.03531067f, 0.01238912f, 0.04907756f, -0.05303818f, 0.06575254f, -0.05123050f, -0.01270593f, 0.01430799f, -0.04961546f, 0.07324591f, -0.00739230f, 0.02924229f, -0.08477928f, 0.00652997f, -0.05092169f, -0.02974606f, 0.02212755f, -0.00823587f, 0.04416234f, -0.01672431f, -0.10119853f, 0.01631698f, 0.03739770f, -0.01630106f, -0.04951251f, -0.00266589f, 0.01915044f, 0.02994269f, 0.00657993f, -0.01588230f, -0.05530962f, -0.00921140f, 0.05601202f, 0.05270645f, -0.06263611f, -0.02830403f, -0.03290090f, -0.02626877f, 0.03393591f, 0.08770347f, 0.03694296f, -0.06051373f, -0.01953212f, -0.04187005f, -0.02533328f, -0.05296301f, 0.06618808f, 0.04072276f, -0.02860787f, -0.02974794f, -0.06091503f, -0.05951743f, 0.03636800f, 0.07939497f, -0.04603808f, 0.01150931f, 0.02340295f, 0.01195145f, 0.03259045f, -0.02625170f, -0.06492005f, 0.07011226f, -0.04254609f, -0.00886207f, 0.06147980f, -0.03730104f, -0.03725046f, -0.06306275f, 0.02699708f, 0.02841869f, 0.04320703f, -0.03688340f, -0.03783942f, -0.04854925f, 0.01828815f, -0.03476173f, 0.06688040f, 0.04301311f, -0.01178467f, 0.04096475f, -0.03172029f, -0.05280674f, 0.05960816f, 0.03753829f, -0.02103595f, -0.03802511f, -0.00175594f, 0.01455509f, -0.03348076f, 0.00088408f, 0.01354888f, -0.02842992f, -0.02354869f, -0.03116631f, 0.05224083f, 0.01335953f, 0.00935860f, 0.04400425f, 0.01414624f, -0.05589388f, 0.07108930f, 0.06890714f, 0.07616056f, -0.00159361f, -0.00327533f, 0.03612458f, 0.00790353f, 0.05291232f, -0.03861118f, -0.06288494f, -0.01185684f},
        {-0.00472464f, 0.03874717f, -0.03524029f, 0.00363702f, -0.02752345f, -0.05320153f, -0.06995831f, 0.00862326f, 0.00088212f, 0.02231403f, 0.01963061f, -0.05769971f, 0.01080388f, -0.03093751f, 0.00085250f, -0.04661655f, 0.02503420f, 0.00903574f, 0.01585628f, -0.03711845f, 0.00119205f, -0.07651471f, -0.00496669f, -0.00404073f, -0.04941116f, 0.06053453f, -0.04588198f, 0.08941896f, -0.02901630f, 0.05467336f, -0.00876676f, -0.09048878f, 0.05707707f, 0.03595474f, -0.06250501f, -0.08508963f, -0.02595068f, -0.01801794f, 0.04285235f, -0.02390330f, -0.02894929f, 0.05550996f, -0.01363887f, 0.05085627f, -0.05604254f, 0.00555105f, 0.01027905f, -0.00869085f, -0.06501420f, 0.07060485f, 0.07221965f, 0.05763528f, -0.06108224f, 0.00398742f, 0.01303063f, -0.06396256f, -0.05998883f, -0.03856722f, -0.05040253f, -0.06052835f, -0.00937990f, 0.02249359f, 0.02055817f, -0.05490953f, -0.02880951f, -0.07375835f, -0.00425520f, 0.04596554f, -0.00939236f, -0.05065263f, -0.04030173f, 0.02920917f, -0.01419183f, -0.00683709f, -0.08211190f, -0.00624919f, -0.01214320f, 0.01098527f, -0.05707482f, -0.03952326f, -0.02697637f, -0.05491588f, -0.01961000f, 0.00475903f, 0.04180031f, -0.06176380f, -0.08475098f, 0.03800130f, -0.00442919f, 0.00890854f, -0.05836758f, -0.02208845f, 0.03877108f, 0.05781828f, 0.07308849f, -0.01829968f, 0.03030267f, -0.05850460f, 0.03092229f, 0.04554855f, -0.06518079f, -0.08495897f, 0.05587030f, 0.02912119f, 0.03676446f, -0.00401970f, -0.03210267f, -0.01045801f, -0.02283780f, -0.02206231f, -0.02067804f, -0.06818486f, 0.01316798f, -0.02459788f, 0.05478176f, 0.05557156f, -0.04789884f, -0.04715205f, -0.06496995f, -0.05440672f, 0.03642530f, 0.04585860f, -0.00032499f, 0.05019160f, 0.01395412f, -0.03675394f, -0.01105677f, 0.04637910f, 0.03242030f, -0.06550158f, -0.05579729f, 0.05495007f, 0.00903230f, 0.04508745f, -0.04104557f, -0.00091097f, -0.01437388f, -0.05341862f, 0.00257765f, -0.04416513f, -0.02690893f, -0.01189902f, 0.03495359f, -0.06790225f, -0.07493640f, 0.00603787f, -0.07004084f, 0.02818279f, 0.03196055f, 0.04287111f, 0.02975306f, -0.06696079f, 0.03207621f, 0.05710889f, 0.04981659f, 0.03959263f, -0.00094587f, 0.06136626f, -0.05852002f, -0.06212430f, -0.02300900f, -0.03053209f, -0.05748298f, -0.00009151f, -0.03562091f, 0.00123210f, 0.02319521f, -0.07175053f, 0.03632853f, 0.04547400f, 0.04630136f, 0.03258894f, -0.03180946f, 0.02019392f, -0.03961035f, -0.05795570f, 0.04528225f, 0.03386721f, 0.03049590f, -0.06046695f, 0.02300664f, 0.02281859f, 0.04917444f, -0.04553884f, -0.06173746f, -0.02244676f, -0.04493670f, -0.07548548f, 0.04109705f, -0.02030338f, -0.06466123f, 0.05037167f, -0.06988639f, 0.05788980f, -0.07030773f, 0.05504623f, -0.02829585f, 0.01776801f, 0.04780138f, 0.02578052f, -0.05353574f, 0.07625319f, 0.06609361f, 0.05212402f, -0.01055719f, -0.06811145f, 0.06739846f, -0.04647479f, -0.00357898f, -0.05594939f, 0.00089594f, 0.03358212f, -0.04473902f, -0.02065900f, -0.03957106f, 0.02313533f, 0.01855026f, 0.00409992f, 0.02994758f, -0.10830012f, -0.01007874f, 0.07557783f, -0.06497878f, -0.00601724f, 0.05151021f, 0.00031452f, 0.03008050f, 0.04160608f, -0.04457644f, -0.07079209f, -0.05356184f, 0.00027938f, -0.05805511f, -0.00291124f, 0.07655987f, -0.01421777f, 0.03063180f, -0.02334701f, 0.01336714f, 0.00304241f, -0.04574003f, 0.00851939f, -0.07003555f, -0.02324509f, -0.00227265f, 0.02227383f, 0.01255780f, -0.03994068f, 0.04008387f, -0.06757132f, -0.06204529f, -0.02692032f, 0.06922005f, -0.00149657f, -0.08161078f, 0.02420810f, -0.05847699f, 0.03787765f, -0.03390608f, 0.03713833f, -0.04670319f, -0.07294802f, -0.07567616f, -0.01814443f, -0.06838222f, -0.03383880f, -0.04143062f, -0.01002441f, 0.04924563f, 0.04996390f, -0.04039129f, -0.05999533f, 0.04349389f, 0.06266991f, -0.02860856f, -0.06654226f, -0.04543629f, -0.06646651f, -0.03543651f, -0.07981159f, -0.04121465f, 0.00753025f, 0.04693145f, -0.04366869f, -0.01634427f, -0.04867614f, 0.04967296f, -0.00296483f, 0.00583333f, 0.04826158f, 0.01670792f, -0.03755188f, -0.01103482f, -0.04022745f, 0.02998201f, 0.01306006f, -0.06812890f, -0.04038572f, 0.02046353f, 0.00644310f, -0.05437293f, 0.02527872f, 0.03044086f, 0.02232651f, -0.02855661f, -0.02917995f, 0.02615153f, -0.02837105f, 0.04223785f, 0.05059498f, 0.01920096f, -0.04632040f, -0.03571595f, 0.02597238f, 0.05701948f, 0.00624379f, 0.04533202f, 0.03212295f, 0.01152445f, -0.04106777f, -0.04279390f, -0.04790728f, -0.01888449f, -0.01124487f, 0.01570424f, -0.03914960f, 0.04773382f, -0.07007030f, 0.02716418f, 0.05975462f, -0.06818115f, 0.04937325f, -0.07987677f, -0.03050555f, 0.03540156f, 0.03066235f, 0.04017189f, 0.05562834f, 0.01542648f, -0.05831579f, 0.01127467f, 0.07122331f, -0.04877078f, 0.02999290f, -0.01997356f, -0.03772761f, 0.00153098f, 0.02228813f, -0.07498712f, -0.01803078f, -0.04208896f, 0.02764336f, 0.05216154f, 0.00808389f, -0.06821243f, -0.03561446f, 0.00506311f, 0.01944535f, -0.04001585f, -0.07130089f, 0.04508034f, -0.07305207f, 0.05056779f, 0.05837322f, -0.06974230f, 0.02624219f, -0.06607568f, 0.00758804f, 0.00925799f, -0.02874546f, 0.05995743f, 0.05780586f, -0.01117593f, -0.01252501f, 0.05224459f, -0.04245630f, 0.04504206f, 0.07320210f, -0.03029003f, 0.04585786f, 0.02538344f, -0.05157482f, 0.03876381f, -0.01008055f, -0.07591423f, -0.05954969f, -0.04241640f, 0.05188671f, 0.00467165f, -0.06991567f, 0.02441459f, 0.06238719f, -0.00946317f, -0.04480845f, 0.06542822f, 0.03993684f, -0.02376895f, -0.07404325f, 0.05872501f, -0.01465830f, -0.04051416f, -0.06616151f, -0.03781907f, 0.06452928f, -0.05401740f, 0.02091226f, -0.01925690f, 0.03316407f, 0.01885661f, -0.03726122f, -0.00991726f, -0.02472784f, 0.02278505f, -0.06946008f, 0.03346694f, 0.03115661f, -0.07737424f, -0.01476679f, 0.03295220f, -0.02180613f, -0.06396877f, -0.02229040f, 0.06381323f, 0.02607048f, -0.00819990f, -0.02257260f, -0.07803192f, 0.04749435f, 0.05846210f, -0.06048235f, -0.07756145f, 0.06020569f, 0.04244207f, -0.06608558f, 0.02530811f, -0.01711090f, -0.07433993f, 0.05520108f, -0.00107020f, 0.05568214f, -0.06294494f, 0.03994400f, -0.02403203f, 0.01787806f, 0.04286912f, 0.01375507f, 0.02211009f, -0.03989545f, -0.01286642f, -0.03327270f, -0.01318162f, -0.03372031f, 0.06972177f, -0.06997763f, -0.02705810f, 0.04927338f, 0.00993283f, 0.03085716f, -0.07091627f, -0.07247487f, -0.03714920f, 0.00736125f, 0.05055704f, 0.04666922f, 0.04022078f, 0.06982330f, 0.01956504f, -0.07602052f, 0.00645365f, -0.03870964f, -0.07500747f, 0.05156020f, -0.00134044f, 0.04911237f, -0.00915184f, -0.06130173f, 0.01674810f, -0.01857701f, -0.03369569f, -0.01780626f, 0.03062688f, 0.04683790f, -0.02358755f, -0.00829882f, -0.02653967f, -0.07054086f, 0.04958572f, -0.05535035f, -0.06077092f, -0.03473568f, 0.01716176f, -0.04701860f, -0.07308542f, -0.03768212f, -0.01790878f, -0.05369291f, -0.06041560f, 0.00720118f, 0.07372260f, 0.00831596f, -0.01088852f, 0.06788673f, -0.05804755f, 0.03558248f, -0.05750961f, -0.06696803f, -0.02868757f, -0.03760836f, 0.05149203f, -0.00430522f, -0.04252543f, 0.08138842f},
        {-0.02164607f, -0.02084966f, 0.05985306f, -0.04062677f, -0.03019151f, 0.05004688f, -0.06185527f, 0.05535211f, -0.06619184f, 0.00665108f, -0.02793822f, -0.01206503f, -0.01640490f, 0.05020108f, 0.00509203f, 0.01583125f, 0.02357664f, 0.04573248f, -0.02646433f, -0.05313772f, -0.02058894f, 0.04415004f, 0.02070280f, 0.04512643f, -0.01988872f, -0.06625419f, 0.01804233f, 0.00799705f, -0.04486433f, -0.02825828f, -0.04214134f, 0.02277313f, -0.05920084f, -0.01275808f, 0.07862607f, -0.09085214f, -0.03158877f, -0.04925116f, -0.06541470f, -0.01460224f, -0.05089165f, 0.05073016f, -0.02082083f, 0.02165421f, 0.03111194f, -0.02323931f, 0.03338607f, -0.05906604f, -0.07655776f, -0.03249243f, -0.05380726f, 0.05033689f, 0.01055625f, -0.03386755f, -0.02337956f, 0.02191300f, -0.04178232f, 0.04570755f, -0.05907337f, 0.04828992f, -0.06426123f, -0.04056050f, 0.03601404f, -0.03857763f, 0.04961568f, -0.06381170f, -0.04772215f, -0.03813719f, 0.06602128f, 0.02866508f, 0.03402531f, 0.02171565f, 0.06427492f, 0.07273938f, 0.00891044f, -0.02629680f, 0.08866553f, -0.01317375f, 0.03267121f, 0.02902944f, 0.05297747f, -0.05177667f, 0.01135918f, 0.05624009f, 0.05620198f, 0.06006301f, -0.07122891f, -0.05436626f, -0.02424681f, 0.03926287f, -0.05575132f, 0.00569399f, -0.04152926f, -0.04464594f, 0.04384346f, -0.04971063f, 0.06495446f, -0.02031753f, 0.06459994f, -0.03514634f, 0.04358044f, 0.03038105f, -0.03135600f, -0.03757188f, -0.03386905f, -0.00830162f, 0.01326002f, -0.02238888f, -0.07173342f, 0.02725498f, -0.05369126f, 0.04715258f, 0.01008477f, -0.01763935f, -0.04810343f, -0.04902442f, -0.00564476f, 0.02819539f, 0.05725447f, -0.02943186f, -0.05129898f, -0.05703619f, -0.02286236f, 0.00464307f, 0.07509408f, -0.08439621f, -0.06325626f, 0.03556132f, -0.02234460f, 0.04801367f, -0.02037362f, -0.06144465f, 0.01828604f, -0.04509181f, 0.01209952f, -0.05126757f, 0.02789421f, -0.00718301f, 0.03473543f, 0.05634313f, 0.03997576f, -0.03544060f, -0.01259839f, -0.07209760f, -0.01303593f, 0.03391859f, -0.02312795f, 0.06017063f, 0.02442144f, -0.01493123f, -0.04157787f, -0.02381426f, -0.00912715f, -0.00560060f, 0.05623063f, -0.01346984f, 0.02650912f, -0.05417340f, 0.02605218f, 0.03980371f, 0.00482714f, 0.04783909f, -0.08132575f, 0.02787875f, -0.07371479f, 0.04445905f, 0.04748020f, -0.06530723f, 0.01223442f, -0.00147100f, -0.00124656f, 0.05611990f, 0.02475070f, -0.06329560f, 0.03691080f, 0.01942315f, 0.01206004f, -0.04277823f, -0.00605823f, 0.05259496f, 0.03999065f, 0.09077154f, -0.01313762f, -0.03692695f, -0.06138662f, -0.04937439f, -0.00652923f, -0.01074256f, -0.05589863f, 0.05248323f, -0.02306465f, -0.03388968f, 0.01891800f, 0.03816474f, -0.01583752f, -0.09373774f, 0.01240642f, -0.03685451f, -0.04402744f, -0.01197034f, -0.04996672f, 0.03912330f, 0.03338220f, 0.04355893f, 0.02843131f, 0.06848294f, -0.05079409f, 0.05807856f, 0.03213163f, -0.04126799f, 0.00698814f, 0.05877350f, -0.06003518f, 0.05668200f, -0.00516776f, -0.02378748f, -0.02591862f, -0.06415586f, 0.05542438f, 0.01557218f, -0.04808006f, -0.02462896f, 0.02726813f, -0.07745390f, -0.00114328f, -0.00305940f, -0.05536593f, -0.07639806f, 0.03125105f, -0.07033085f, 0.02487841f, -0.02881436f, -0.04866026f, 0.03527297f, -0.05244875f, -0.05505609f, 0.05327859f, 0.02917177f, -0.05604723f, -0.00269973f, -0.05461290f, -0.02977881f, -0.04016575f, -0.05921427f, -0.02719232f, -0.00615855f, -0.02220744f, 0.05351299f, -0.01207311f, -0.03997662f, -0.02034534f, 0.04681550f, -0.02063770f, -0.04969643f, -0.02362045f, 0.06126742f, -0.05476371f, 0.04439810f, -0.03411989f, -0.00343830f, -0.04314936f, -0.02396630f, 0.00455921f, -0.05095906f, 0.04005373f, -0.03452933f, 0.03996283f, -0.02599045f, -0.03621407f, 0.00619917f, -0.03888222f, -0.04284037f, -0.04744771f, -0.03210919f, -0.06664063f, -0.04441360f, 0.05070192f, -0.02869619f, -0.05845515f, 0.03603109f, 0.01542540f, 0.05449587f, -0.03510048f, -0.02958780f, -0.03271531f, -0.06013733f, 0.04315060f, -0.06389225f, -0.05748073f, -0.02005364f, 0.05757445f, -0.06060307f, 0.03500699f, -0.04401757f, -0.04867177f, 0.04505881f, 0.00515682f, 0.05061239f, -0.06861898f, -0.02289349f, 0.05957721f, 0.04279495f, -0.01121289f, -0.05113887f, -0.01381628f, -0.05648547f, 0.05722322f, 0.00170427f, 0.06797699f, -0.03170351f, 0.05302639f, 0.04305055f, 0.02590178f, -0.03174070f, 0.08743397f, 0.02205157f, 0.03202836f, -0.06870637f, -0.05736039f, -0.07037222f, 0.02455606f, -0.05059956f, 0.05397628f, -0.05234399f, 0.07532467f, -0.02878854f, 0.01347825f, -0.09036871f, 0.00555907f, 0.00830195f, -0.03210272f, -0.04583592f, -0.04953298f, 0.01005608f, 0.01280511f, -0.02964373f, -0.03170020f, -0.02531819f, 0.03881808f, -0.02489569f, 0.04897732f, -0.02605749f, -0.04523473f, -0.02312448f, 0.01755851f, 0.05805252f, 0.07542086f, 0.05094911f, -0.01474211f, 0.01667346f, -0.05047246f, 0.04055620f, 0.00851745f, -0.01552071f, 0.08122017f, -0.00915439f, -0.00958502f, 0.03258290f, -0.08917372f, -0.04636297f, -0.01266350f, 0.00293628f, 0.02142511f, -0.10942292f, -0.06674404f, -0.03750509f, -0.07736323f, 0.04344251f, 0.00578779f, -0.05350741f, -0.01217473f, 0.00659582f, -0.01685583f, -0.06273440f, 0.02622406f, -0.05245130f, -0.03810221f, 0.05427512f, 0.03487093f, -0.05998670f, 0.01014202f, -0.05921879f, 0.00380487f, -0.03395506f, -0.04000004f, 0.07882680f, -0.03522350f, -0.04686747f, 0.01886048f, 0.04346969f, 0.04874541f, 0.03641298f, -0.01040744f, -0.05409292f, -0.02828035f, -0.04566393f, -0.01749630f, -0.07518003f, -0.00723187f, 0.04861074f, -0.01237167f, -0.03318420f, -0.03556512f, -0.03496066f, 0.04033472f, -0.07044154f, 0.06215025f, -0.05527252f, -0.02083536f, -0.00730856f, 0.05461039f, 0.03966669f, 0.03645619f, 0.06027797f, -0.03516841f, 0.05559394f, -0.03396622f, -0.05045788f, -0.02797421f, 0.03344730f, -0.04406858f, 0.06488154f, -0.03363087f, 0.06988214f, 0.03314074f, -0.00028024f, 0.00145699f, -0.01321345f, 0.01285393f, -0.04106951f, 0.02018428f, 0.00387987f, -0.04730709f, -0.06575975f, -0.03634368f, 0.06437833f, 0.02333708f, 0.04820019f, 0.06147007f, -0.07253967f, 0.01173405f, -0.03956914f, 0.04245289f, 0.00327311f, -0.02348648f, -0.04133868f, -0.06648690f, -0.02448763f, -0.05102162f, 0.01385108f, 0.05870654f, -0.04904325f, 0.04479324f, -0.01467367f, -0.01276016f, 0.01747142f, 0.02203155f, -0.02049275f, 0.05201259f, -0.05351374f, 0.04705932f, 0.07268973f, -0.03879942f, 0.03635904f, 0.05301140f, -0.00819827f, 0.00128356f, 0.02423741f, 0.06513672f, -0.00638763f, 0.04489027f, -0.01286873f, -0.07140210f, 0.03113613f, 0.02989509f, -0.05568746f, 0.05197647f, 0.03256120f, -0.05727572f, 0.00205249f, -0.04269011f, -0.04479800f, -0.05867106f, 0.01278939f, 0.04590797f, 0.08006255f, -0.04162478f, 0.01214380f, 0.04431742f, 0.05276014f, -0.03056986f, -0.00032999f, 0.03575159f, -0.06755257f, -0.03035987f, -0.03171630f, -0.05582249f, 0.06378227f, 0.01711434f, -0.04684481f, -0.02696211f, -0.02323607f, -0.01273994f, 0.02212944f, -0.01546842f, 0.05949929f, -0.00754965f, 0.01486411f, 0.03289050f, 0.06314984f, 0.04634690f, -0.04014945f},
        {-0.00277259f, -0.02857850f, -0.02940705f, -0.04942834f, 0.04097982f, 0.00627006f, 0.05027283f, -0.04094563f, -0.03732251f, -0.03790116f, 0.04682126f, 0.08808421f, 0.06172571f, 0.01123251f, 0.05269046f, 0.02611243f, -0.04789681f, -0.06375741f, -0.04956093f, -0.01625063f, -0.05750679f, 0.00906808f, 0.02451601f, -0.04183977f, 0.02873369f, -0.03975197f, 0.07715037f, 0.02839541f, 0.05068032f, -0.05817373f, -0.03363137f, 0.00383305f, 0.05261416f, 0.08077568f, -0.03835508f, 0.00128302f, 0.05951857f, -0.02218057f, -0.03571199f, 0.02423598f, -0.04699078f, -0.07853213f, -0.02184460f, -0.03550562f, 0.00511940f, -0.03197328f, 0.07011317f, -0.03435044f, -0.00236882f, -0.02923286f, -0.01968937f, -0.00432071f, 0.07467533f, -0.03458553f, 0.01049483f, 0.06333878f, -0.03294073f, -0.04519831f, -0.03734286f, -0.02163650f, 0.04906536f, -0.01750999f, -0.02829486f, 0.03336457f, 0.04498915f, -0.02483843f, 0.02350909f, 0.01117540f, -0.01154240f, -0.00016148f, 0.04712823f, -0.02050417f, -0.03526787f, -0.03041731f, 0.04343112f, 0.04788036f, -0.01831492f, -0.02626005f, -0.05450295f, 0.09959553f, -0.00081033f, -0.00907398f, 0.06437326f, -0.02393408f, -0.04390791f, -0.02726830f, 0.06480135f, 0.06683992f, 0.07124617f, 0.03810634f, -0.02439139f, -0.04050101f, -0.02364111f, -0.01422245f, -0.04158156f, 0.07301935f, -0.01217075f, -0.04916741f, -0.02507176f, -0.04729572f, 0.05183203f, -0.02834567f, -0.01445922f, -0.00325315f, 0.00485110f, -0.05626207f, 0.04110785f, 0.07522620f, -0.01827608f, -0.04241249f, -0.03199944f, -0.05307712f, -0.04536800f, 0.03076684f, 0.04642687f, -0.03311044f, -0.05822992f, -0.04567483f, 0.06252535f, 0.09717397f, 0.02786345f, -0.05861351f, 0.03556423f, -0.01591104f, 0.02560231f, 0.05097577f, 0.06023711f, -0.03078290f, -0.04832277f, 0.02937006f, 0.01785525f, 0.05900425f, 0.03444191f, -0.02688689f, 0.07436257f, 0.05411386f, -0.00510673f, 0.03123101f, -0.04079476f, 0.03080828f, -0.01204321f, 0.06702311f, 0.00840096f, 0.01530220f, -0.01793827f, -0.03632613f, 0.02642316f, -0.04678683f, 0.07991295f, -0.06241582f, -0.03797034f, -0.04846054f, -0.04231552f, -0.06326255f, -0.05303723f, 0.07955193f, 0.09351464f, 0.01577689f, 0.03347900f, 0.01532939f, -0.05780279f, -0.02440652f, -0.03219067f, -0.02473936f, 0.07806743f, -0.04196872f, -0.03008083f, -0.00301586f, -0.07115151f, -0.00987474f, 0.03895086f, -0.06235675f, -0.02878177f, 0.05630438f, 0.04111205f, -0.01001594f, 0.04469813f, -0.00733383f, -0.00724688f, -0.00526522f, 0.06371735f, 0.00651600f, 0.04088932f, 0.07904767f, 0.03546384f, 0.03256775f, 0.03977110f, -0.04995583f, 0.05349185f, -0.05209420f, 0.08594713f, 0.01160559f, 0.05263573f, -0.01553750f, 0.00847797f, -0.00932172f, -0.01828049f, 0.03861312f, -0.08165161f, -0.01601572f, 0.07753871f, -0.01260532f, -0.03742090f, -0.04576357f, -0.04675730f, 0.04026122f, 0.02538143f, 0.03079061f, -0.04871993f, 0.01204595f, -0.01307814f, -0.07337226f, 0.07074854f, 0.03299708f, 0.04318134f, 0.01845267f, 0.04205792f, -0.02159820f, 0.04927498f, -0.01241326f, 0.03262074f, -0.03430992f, -0.05978930f, 0.04060132f, -0.02201538f, -0.00451216f, -0.01199820f, 0.08828108f, 0.06068759f, 0.04897972f, 0.04558346f, 0.00367164f, 0.04405453f, -0.05450095f, -0.02195579f, 0.03333543f, -0.02252223f, -0.03083657f, 0.01438099f, 0.05277215f, -0.02775083f, -0.00298056f, 0.02175102f, -0.02844421f, 0.05704372f, -0.00623379f, -0.06149543f, -0.06565116f, 0.02269986f, 0.01330771f, 0.04328442f, 0.01660164f, -0.06023149f, 0.04348126f, 0.09117929f, -0.02109747f, 0.05547120f, 0.03040624f, 0.02890452f, -0.03751749f, -0.04903383f, 0.04767541f, 0.00002938f, -0.05075445f, -0.04179163f, 0.06417751f, -0.04378222f, -0.05103884f, 0.02385564f, -0.03867146f, 0.07093766f, -0.01209268f, -0.00204037f, -0.00546699f, 0.03015295f, 0.05244714f, 0.06175585f, 0.07127608f, 0.09542861f, -0.02872492f, 0.04874429f, -0.02118990f, 0.06794361f, 0.00881000f, -0.02532789f, -0.00272954f, -0.01757674f, -0.01296255f, 0.00729338f, -0.02183482f, -0.05303034f, -0.03263431f, -0.05570250f, 0.00092608f, -0.02236670f, -0.05913041f, -0.01294515f, -0.01287420f, 0.06970409f, 0.06487695f, -0.04939226f, -0.02512098f, -0.00952393f, -0.01428536f, 0.07388997f, 0.06133467f, -0.06027724f, 0.00870658f, -0.03546717f, -0.03071288f, -0.03590370f, -0.02836742f, 0.02717486f, -0.06022406f, -0.04339515f, 0.01746110f, 0.05811091f, 0.02601962f, 0.02464102f, 0.05371482f, -0.03385709f, 0.06968135f, -0.06470024f, 0.02348423f, -0.03535775f, -0.02811923f, -0.05902442f, 0.04386516f, -0.02177221f, 0.07551151f, 0.04029533f, -0.04652659f, 0.03961289f, -0.01941201f, -0.06893251f, -0.03492114f, -0.01540833f, 0.05932184f, -0.05173407f, 0.02780169f, 0.04241265f, -0.03598562f, -0.00856290f, -0.03324301f, -0.00586131f, -0.00350752f, -0.01822986f, -0.02450099f, -0.02649904f, -0.04345280f, 0.03852379f, 0.05098857f, -0.02015256f, -0.03644592f, 0.06061186f, -0.02963114f, 0.04364678f, -0.04087169f, 0.06337635f, -0.02060968f, -0.01243310f, -0.00771304f, -0.04234181f, 0.06310923f, -0.00473834f, 0.04388095f, -0.02585390f, 0.02426502f, -0.03922118f, 0.06631903f, -0.03636269f, -0.04989114f, 0.05292827f, 0.07344849f, -0.05099261f, -0.02457156f, 0.00170900f, -0.02089059f, -0.04086358f, -0.00268988f, -0.03346613f, 0.03174752f, 0.02937549f, -0.05184517f, 0.03178422f, -0.02186522f, 0.09649676f, -0.03472178f, 0.06451500f, -0.02741457f, -0.03993934f, -0.00349971f, 0.05137618f, -0.05730883f, -0.05464343f, 0.06960210f, -0.00053701f, 0.03858102f, -0.00419257f, -0.00418683f, -0.03024640f, 0.06164863f, -0.01412944f, -0.02692593f, -0.03472187f, 0.00923984f, 0.05380715f, -0.04284089f, 0.06922364f, -0.03210843f, 0.01826889f, -0.00503982f, -0.06041366f, 0.05578091f, 0.04166046f, -0.02662833f, 0.09459126f, 0.00883842f, 0.08817580f, -0.01441693f, 0.05511717f, 0.02549132f, -0.00902294f, -0.01414157f, -0.05500968f, 0.00988578f, 0.04114143f, 0.03911560f, 0.00668448f, -0.00760165f, -0.02107756f, 0.00660748f, 0.04475302f, -0.01130006f, -0.00759875f, 0.02566998f, -0.03061688f, 0.02398208f, 0.01678750f, -0.02557021f, 0.03364708f, 0.05360701f, -0.06667696f, -0.03033295f, -0.04534505f, -0.04785387f, 0.07777196f, 0.08511867f, 0.04244529f, -0.01210443f, -0.03003673f, -0.03819837f, -0.04951900f, 0.02643729f, -0.01750279f, 0.00372302f, 0.00725578f, -0.05179241f, -0.00383617f, 0.03687970f, 0.01072633f, 0.01296399f, -0.03870887f, -0.04105747f, 0.04852580f, 0.04169109f, 0.02988547f, 0.07050021f, -0.06696527f, -0.00230187f, -0.03050330f, 0.02371876f, 0.03375596f, -0.05819957f, 0.05189041f, 0.01134898f, -0.07478832f, -0.02261058f, 0.05115095f, -0.02288505f, 0.04865709f, 0.03033001f, 0.05031583f, -0.05404026f, 0.00395302f, 0.06417418f, -0.00451955f, -0.02680414f, 0.04495105f, 0.03575782f, -0.05742700f, 0.07358892f, -0.00472134f, -0.02257448f, -0.04228928f, 0.01324382f, 0.02960845f, -0.04552796f, -0.00617515f, 0.06338462f, 0.04885146f, -0.08123527f, -0.00132062f, 0.02670885f, 0.04703690f, 0.05728387f, 0.04093015f, 0.05551603f, 0.04299251f, 0.05484073f, -0.02765517f, 0.04743697f},
        {-0.07707491f, -0.00775136f, 0.07127690f, 0.01559202f, -0.04043825f, -0.09816314f, 0.06261618f, -0.03806551f, -0.03245645f, -0.03269726f, -0.03224767f, 0.01031231f, 0.05112172f, -0.04626458f, -0.05195398f, 0.01727843f, -0.02598243f, 0.01940260f, 0.07792389f, 0.01880356f, 0.04506603f, 0.05927007f, -0.05494940f, -0.06098666f, 0.05897184f, 0.09504820f, -0.01689504f, 0.03255550f, 0.08744508f, -0.02603748f, -0.08584443f, 0.07370534f, 0.06554156f, -0.00154438f, -0.04712175f, 0.04550084f, 0.05884243f, -0.04167725f, -0.03907718f, 0.04224318f, 0.07901832f, -0.00146145f, 0.05404925f, 0.04320870f, 0.04803370f, 0.07603519f, -0.02796528f, -0.03419343f, 0.01369612f, -0.06300297f, -0.01625945f, 0.04962723f, 0.02368466f, 0.03436154f, -0.00630847f, 0.05692110f, 0.03936731f, -0.03282638f, 0.07275614f, -0.03178256f, 0.01551691f, -0.04896060f, -0.05331358f, -0.03574072f, -0.02828031f, 0.06023897f, -0.02101593f, 0.02480124f, -0.07205385f, 0.05837317f, -0.05351439f, -0.05660662f, -0.01818096f, -0.06606714f, 0.03913403f, 0.04949183f, 0.04627268f, 0.04648031f, 0.01899870f, 0.02443543f, 0.00385969f, -0.06159916f, 0.05548396f, 0.02577198f, -0.03476219f, 0.04992259f, -0.03309212f, -0.00628344f, -0.04094769f, -0.05367242f, 0.08993069f, 0.04542020f, -0.05724712f, -0.03112128f, 0.03380477f, -0.04311560f, 0.04686765f, 0.06434889f, 0.05997947f, 0.05414543f, -0.01463105f, 0.01994114f, -0.05548614f, 0.04044980f, -0.06074567f, -0.01228369f, 0.07412433f, -0.07156976f, 0.06156004f, 0.07484143f, 0.02439416f, 0.05352594f, -0.05845640f, 0.02163720f, 0.01227726f, 0.00117964f, 0.02824082f, 0.06381343f, -0.04954736f, -0.03041301f, 0.05456248f, 0.04729708f, -0.01612244f, -0.03639279f, -0.04448390f, -0.05646985f, -0.06169074f, -0.01498210f, -0.02517201f, -0.04725830f, -0.03110898f, -0.02223646f, 0.02644847f, 0.10322607f, -0.01842836f, -0.05705546f, 0.01646224f, 0.06742436f, -0.05322732f, -0.04527949f, -0.04189795f, -0.00242177f, 0.06752964f, 0.07192828f, -0.01652268f, -0.03566980f, 0.01898207f, 0.04605593f, -0.04790325f, 0.02396782f, 0.02726206f, 0.07118295f, 0.06366943f, 0.04793331f, -0.06272703f, -0.01935942f, -0.08304897f, 0.07126691f, 0.05544195f, -0.09052715f, -0.02175240f, -0.04294987f, 0.00578334f, 0.02349417f, 0.05350703f, 0.05168499f, -0.05797067f, 0.04629457f, 0.01772168f, -0.03281099f, -0.04975674f, 0.06714137f, 0.02871600f, 0.04237169f, 0.03171042f, 0.07800772f, -0.00240608f, 0.01255086f, -0.00237073f, -0.01177662f, -0.03140360f, -0.05323988f, 0.05509568f, 0.04908296f, 0.10236686f, -0.03479524f, 0.07774807f, 0.08256890f, -0.05532585f, -0.04836936f, -0.01045974f, 0.02356744f, 0.06962144f, 0.01909849f, 0.05289038f, -0.07866897f, -0.06146878f, 0.02849720f, 0.08824215f, 0.08489111f, 0.02812161f, -0.03085084f, -0.00685541f, -0.03887364f, 0.04347623f, 0.04875847f, 0.01538194f, -0.04105978f, -0.04990631f, 0.05055835f, 0.01611714f, -0.00491809f, 0.06952915f, 0.00278861f, -0.03499595f, -0.02185721f, -0.04356285f, -0.02729255f, -0.01658515f, 0.01381386f, 0.04093812f, -0.03677427f, -0.03440595f, -0.02858291f, -0.02001646f, 0.06227249f, 0.02262768f, -0.00618105f, -0.02415403f, 0.07219066f, -0.02926631f, 0.06858686f, 0.04361171f, 0.04242967f, -0.01885513f, -0.08403717f, -0.02387851f, -0.05501652f, 0.02035404f, 0.00736670f, 0.08050451f, -0.00287176f, 0.03190402f, -0.01862326f, -0.00620037f, -0.03840677f, 0.00199907f, 0.05317565f, -0.00615583f, -0.02824706f, -0.04485578f, 0.05618056f, -0.06729264f, 0.06614253f, -0.02814161f, -0.03557682f, -0.03542616f, -0.01881956f, -0.03521072f, 0.06564008f, -0.06078699f, 0.00940692f, 0.02640351f, 0.07262874f, -0.04538053f, 0.01861297f, -0.02398206f, 0.02199281f, 0.01621216f, -0.00880065f, 0.00792760f, 0.05913928f, -0.04505093f, 0.04914415f, 0.00493443f, 0.03507073f, -0.01997413f, -0.04858250f, 0.06274240f, 0.00310813f, 0.02283711f, -0.01014209f, -0.03746561f, 0.02993738f, 0.01252286f, -0.05838160f, -0.04790388f, 0.01987046f, -0.07683745f, -0.05397963f, -0.00172853f, 0.01932849f, 0.04912788f, -0.00340028f, -0.04347723f, 0.03130299f, 0.07925891f, -0.01517670f, 0.00585244f, 0.04423294f, -0.03357648f, 0.00770469f, -0.03986515f, 0.01827772f, -0.02703708f, 0.03374880f, -0.05657083f, -0.02165439f, 0.00000985f, -0.05787902f, 0.01834561f, 0.02103284f, 0.06026055f, -0.03434132f, -0.04498888f, -0.03551626f, -0.05203279f, -0.03252276f, -0.03283382f, -0.05427362f, -0.04409878f, -0.05701372f, -0.03144412f, -0.03354933f, 0.01681078f, 0.04342400f, 0.04204898f, -0.06602342f, -0.06816152f, -0.03785721f, 0.03388906f, -0.01049128f, -0.01735158f, 0.02895944f, 0.00145710f, -0.04752470f, -0.03956277f, -0.00497100f, -0.02828680f, 0.06017097f, -0.06526371f, 0.00096602f, 0.07618265f, -0.04483644f, -0.06630288f, 0.02280774f, 0.01197036f, -0.02180048f, -0.02075365f, -0.05539541f, 0.02321193f, -0.02901006f, -0.00015019f, -0.04865865f, -0.05561441f, -0.04667774f, 0.07421759f, -0.05007912f, -0.06598325f, 0.07064418f, 0.02149170f, 0.05628955f, 0.00672797f, 0.01243531f, 0.02734519f, -0.00107229f, -0.06198520f, -0.05421762f, -0.05519497f, -0.00616805f, 0.02136290f, 0.03941551f, 0.07744885f, -0.05756710f, -0.02786274f, -0.00387570f, -0.03032752f, -0.03642239f, 0.03729103f, 0.05179957f, -0.00914573f, -0.01413027f, -0.04413886f, 0.05574612f, 0.04443893f, 0.06356884f, 0.00155635f, -0.07266286f, -0.04787509f, -0.07385488f, 0.01685429f, -0.02915022f, -0.04337206f, -0.07457307f, 0.04236402f, -0.07006352f, 0.00552717f, 0.08733294f, 0.01016898f, -0.02477432f, -0.06217397f, -0.03624839f, -0.03977618f, 0.02749091f, -0.03237511f, -0.05219539f, -0.05047301f, 0.00696733f, 0.03154017f, 0.08235829f, -0.00617426f, -0.01932096f, 0.08855449f, -0.03403884f, -0.05673419f, -0.01785362f, 0.04294068f, -0.05526580f, 0.06040146f, 0.05502189f, 0.03899026f, 0.06084921f, 0.08078440f, -0.00349868f, 0.05528840f, 0.02250088f, -0.09015740f, -0.03632022f, -0.01973197f, -0.06662300f, 0.05346926f, 0.02562334f, 0.06685109f, 0.06643602f, -0.03483677f, 0.01400299f, 0.02815862f, -0.03570746f, 0.00849263f, 0.03664770f, -0.05088599f, 0.06467230f, -0.04335897f, -0.01005698f, 0.04022842f, -0.03623259f, 0.07394561f, -0.00809827f, -0.00613692f, 0.05059751f, 0.00911341f, 0.06062963f, -0.02383477f, 0.03194094f, 0.02996130f, -0.05749090f, -0.06974477f, 0.06848798f, 0.07826719f, -0.02811800f, -0.00393178f, -0.02190543f, 0.02276018f, -0.00344931f, -0.00304159f, -0.02448804f, 0.09166852f, 0.03862458f, -0.01843773f, 0.04380836f, 0.04753826f, -0.05818171f, 0.00796745f, -0.04327432f, -0.03278241f, -0.01788299f, -0.04131943f, -0.01633031f, -0.03611862f, -0.01482250f, -0.02685237f, -0.01057819f, -0.01027320f, -0.03964105f, -0.07311724f, -0.05739982f, 0.01121372f, -0.02893154f, -0.05570284f, 0.00532766f, 0.02747462f, -0.01775219f, 0.01640636f, 0.07718134f, -0.04544142f, 0.04221409f, 0.01060946f, -0.02834106f, -0.01719299f, -0.06671398f, 0.06697737f, 0.03498004f, -0.04838786f, -0.04693728f, -0.06238935f, -0.03129157f, 0.00347447f, -0.05930179f, 0.01744043f, -0.01647323f, -0.03662678f, 0.01259799f},
    };

    float fc_b[7] = {0.03012012f, 0.03195569f, -0.02340960f, 0.04477080f, 0.02127935f, -0.03061499f, -0.03678811f};

    // 3. Efficiently copy the memory arrays into the cv::Mat headers
    for (int r = 0; r < 7; ++r) {
        m_fcBiasMat.at<float>(r) = fc_b[r];
        for (int c = 0; c < 512; ++c) {
            m_fcWeightsMat.at<float>(r, c) = fc_w[r][c];
        }
    }

    m_isFcLayersInitialized = true;
    printf("Manual Classification Linear Head successfully initialized with trained parameters!\n");
}


void ChessImageProcessing::classsifyWholeBoardAtOnce(const cv::Mat& warpedBoard) {
    int cellSize = CELL_SIZE; // Matches the legacy layout constant tracker properties (240)
    printf("classsifyWholeBoardAtOnce (OPENVINO VECTORIZED MULTI-THREAD EXTRACTION ENGINE ACTIVE):\r\n");
    auto start = std::chrono::steady_clock::now();

    // Ensure our manual linear multiplier matrix is safe to run queries against
    initializeManualClassificationHead(); //

    // Phase 1: Handle input color routing configuration dynamically
    cv::Mat preparedBoard;
    if (m_dnnDetectorChannels == 1) {
        cv::cvtColor(warpedBoard, preparedBoard, cv::COLOR_BGR2GRAY);
    } else if (m_dnnDetectorChannels == 3) {
        preparedBoard = warpedBoard.clone(); // Preserves raw frame parameters clean
    } else {
        std::cerr << "Error: Supported channels specification are 1 or 3. Received: " << m_dnnDetectorChannels << "\n";
        return;
    }

    // Phase 2: Create a single 4D Tensor Blob capturing the entire chessboard image context at once
    cv::Size target_size(m_dnnDetectorSize, m_dnnDetectorSize);
    double scale_factor = 1.0 / 255.0;
    bool swapChannels = (m_dnnDetectorChannels == 3); // Swap R and B lanes if processing your native 3-channel RGB model setup
    cv::Mat wholeBoardBlob;

    if (m_dnnDetectorChannels == 1) {
        cv::Scalar mean_grayscale(127.5);

        cv::dnn::blobFromImage(
            preparedBoard, wholeBoardBlob, scale_factor, target_size,
            mean_grayscale, false, false
        );
    } else {
        // Dynamic alignment matching standard 240x240 RGB ImageNet training metrics rules
        // Note: OpenCV DNN blobFromImage handles structural normalization scalings uniformly across channels.
        // To maintain perfect mathematical parity with individual cell std-dev normalization variations:
        // [pixel/255.0 - mean] / std -> pixel * [1.0 / (255.0 * std)] - [mean / std]
        // Since std ranges from 0.229 to 0.225, we use the intermediate mean scale factor and run fine adjustments.
        double scale_factor = 1.0 / 255.0;
        cv::Scalar mean_rgb(0.485 * 255.0, 0.456 * 255.0, 0.406 * 255.0); // PyTorch ImageNet mean values

        cv::dnn::blobFromImage(
            preparedBoard, wholeBoardBlob, scale_factor, target_size,
            mean_rgb, true, false // swapRB = true (Converts BGR to RGB natively)
        );

        // Apply channel-wise standard deviation normalization values using highly vectorized SIMD extensions
        cv::Scalar std_dev_rgb(0.229, 0.224, 0.225); // PyTorch ImageNet standard deviation values
        cv::divide(wholeBoardBlob, std_dev_rgb, wholeBoardBlob);
    }

    // Phase 3: Execute ONE single parallel forward pass optimizing CPU cache residency
    m_dnnDetector.setInput(wholeBoardBlob, "input"); //

    // Bypasses global pooling logic entirely by extracting the raw output map layer directly from layer4
    cv::Mat featMap = m_dnnDetector.forward("onnx_node!/layer4/layer4.1/relu_1/Relu"); //

    // Extract tensor geometry parameters dynamically from the output matrix
    int featChannels = featMap.size[1]; // 512 feature mappings
    int featHeight   = featMap.size[2]; // Target dimensional height layout mapping row grids
    int featWidth    = featMap.size[3]; // Target dimensional width layout mapping column grids

    // Mathematical calculations tracking cell spatial mapping intervals
    float stepRow = static_cast<float>(featHeight) / static_cast<float>(NUM_ROW); //
    float stepCol = static_cast<float>(featWidth) / static_cast<float>(NUM_COL); //

    int num_classes = static_cast<int>(m_dnnDetectorClassList.size()); //
    int totalCells = NUM_ROW * NUM_COL;

    // Phase 4: Packed Feature Extraction Matrix Generation
    // Create an unified matrix to pack ALL cell feature vectors together at once: [512 rows x 112 columns]
    cv::Mat packedFeatures(featChannels, totalCells, CV_32F);
    std::vector<std::pair<int, int>> spatialMap(totalCells);

    int cellIdx = 0;
    for(int row = 0; row < NUM_ROW; row++) {
        for(int col = 0; col < NUM_COL; col++) {
            spatialMap[cellIdx] = {row, col};

            // Enforce uniform spatial rounding centers across the feature grid mapping bounds
            int featY = cvRound((row + 0.5f) * stepRow - 0.5f);
            int featX = cvRound((col + 0.5f) * stepCol - 0.5f);
            featY = std::max(0, std::min(featY, featHeight - 1));
            featX = std::max(0, std::min(featX, featWidth - 1));

            // Copy feature slice column data swiftly across memory addresses
            float* featMapPtr = featMap.ptr<float>(0);
            for (int c = 0; c < featChannels; ++c) {
                int tensorOffset = (c * featHeight * featWidth) + (featY * featWidth) + featX;
                packedFeatures.at<float>(c, cellIdx) = featMapPtr[tensorOffset];
            }
            cellIdx++;
        }
    }

    // Phase 5: High-speed Batch Linear Layer Matrix Multiplication (GEMM)
    // Replicates standard PyTorch FC operation in parallel: batchLogits = Weights * PackedFeatures
    cv::Mat batchLogits;
    cv::gemm(m_fcWeightsMat, packedFeatures, 1.0, cv::Mat(), 0.0, batchLogits); //

    // Apply the bias offsets row-by-row across cell coordinate columns using vector registers
    for (int r = 0; r < batchLogits.rows; ++r) {
        batchLogits.row(r) += m_fcBiasMat.at<float>(r); //
    }

    // Phase 6: Parse classifications out of the optimized batchLogits matrix map natively
    for (int i = 0; i < totalCells; ++i) {
        int row = spatialMap[i].first;
        int col = spatialMap[i].second;

        if(m_mapExcludedCell[row][col] == 0) { //
            m_mapClassifiedCell[row][col] = '.'; //
            continue;
        }

        // Compute standard Softmax routing configurations to yield precise probability vectors
        std::vector<float> exp_scores(num_classes); //
        float max_score = -FLT_MAX; //

        for(int c = 0; c < num_classes; ++c) {
            float score = batchLogits.at<float>(c, i);
            if(score > max_score) max_score = score; //
        }

        float sum_exp = 0.0f; //
        for (int c = 0; c < num_classes; ++c) {
            exp_scores[c] = std::exp(batchLogits.at<float>(c, i) - max_score); // Stable Softmax implementation
            sum_exp += exp_scores[c]; //
        }

        int predicted_idx = 0; //
        float max_prob = 0.0f; //
        int predicted2_idx = 0; //
        float max2_prob = 0.0f; //

        for (int c = 0; c < num_classes; ++c) {
            float prob = exp_scores[c] / sum_exp; //
            if (prob > max_prob) { //
                max2_prob = max_prob; //
                predicted2_idx = predicted_idx; //
                max_prob = prob; //
                predicted_idx = c; //
            } else if (prob > max2_prob && prob != max_prob) { //
                max2_prob = prob; //
                predicted2_idx = c; //
            }
        }

        ClassificationResult piece; //
        piece.row = row; //
        piece.col = col; //
        piece.className = m_dnnDetectorClassList[predicted_idx]; //
        piece.probability = max_prob * 100.0f; //
        piece.className2 = m_dnnDetectorClassList[predicted2_idx]; //
        piece.probability2 = max2_prob * 100.0f; //

        // Isolate individual cell boundaries to pass down to legacy color checking routines
        int cropX = col * cellSize; //
        int cropY = row * cellSize; //
        cv::Rect tallCellROI(cropX, cropY, cellSize, cellSize); //
        tallCellROI &= cv::Rect(0, 0, warpedBoard.cols, warpedBoard.rows); //
        cv::Mat croppedCellBGR = warpedBoard(tallCellROI); //

        // Execute background pixel color metrics calculations mapping legacy rules
        checkPieceColor(croppedCellBGR, piece, row, col); //
        m_mapClassifiedCell[row][col] = piece.className; //

#ifdef DEBUG_ROI //
        // Replicate the exact visual layout mapping tracking criteria set inside classsifyChessBoardImage
        cv::rectangle(warpedBoard, tallCellROI, cv::Scalar(0, 255, 255), 2); //
        cv::putText(warpedBoard, std::string{piece.className} + " :" + std::to_string((int)piece.probability),
                    cv::Point(cropX + 20, cropY + 60),
                    cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_AA); //
        cv::putText(warpedBoard, std::string{piece.className2} + " :" + std::to_string((int)piece.probability2),
                    cv::Point(cropX + 20, cropY + 90),
                    cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_AA); //
        cv::putText(warpedBoard, "Gray: " + std::to_string(piece.grayPixels),
                    cv::Point(cropX + 20, cropY + 120),
                    cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_AA); //
        cv::putText(warpedBoard, "Gold: " + std::to_string(piece.goldPixels),
                    cv::Point(cropX + 20, cropY + 150),
                    cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_AA); //
#endif
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Fully Convolutional Whole-board processing loop complete in: " << elapsed << " ms" << std::endl;

#ifdef DEBUG_ROI
    cv::Mat scaledWarped;
    cv::resize(warpedBoard, scaledWarped, cv::Size(WARP_SMALL_WIDTH, WARP_SMALL_HEIGHT), 0, 0, cv::INTER_NEAREST);
    cv::imshow("classification", scaledWarped);
#endif

    printf("Mapped Board State Layout:\r\n");
    for(int row = 0; row < NUM_ROW; row++) {
        for(int col = 0; col < NUM_COL; col++) {
            printf("%c ", m_mapClassifiedCell[row][col]);
        }
        printf("\r\n");
    }
}

#if defined (USE_OPENVINO)
void ChessImageProcessing::classsifyWholeBoardAtOnce2(const cv::Mat& warpedBoard) {
    int cellSize = CELL_SIZE;
    printf("classsifyWholeBoardAtOnce (OPENVINO ZERO-COPY INTEGRAL ENGINE ACTIVE):\r\n");
    auto start = std::chrono::steady_clock::now();

    initializeManualClassificationHead();

    // =========================================================================
    // 1. HIGH-SPEED DIRECT CACHE STRIDE WRITING (FIXES 1734ms BOOT & WRONG PIECES)
    // =========================================================================
    // Retrieve a direct reference to the pre-compiled static input memory block
    ov::Tensor inputTensor = m_ovInferRequest.get_input_tensor(0);
    float* inputBufferPtr = inputTensor.data<float>();

    // Constants for ImageNet normalization matching your PyTorch script model rules
    float mean_vals[3] = {0.485f, 0.456f, 0.406f};
    float std_vals[3]  = {0.229f, 0.224f, 0.225f};

    int totalPixelsPerChannel = WARP_HEIGHT * WARP_WIDTH;
    int r_offset = 0;
    int g_offset = totalPixelsPerChannel;
    int b_offset = totalPixelsPerChannel * 2;

    // Direct single-pass cache-friendly loop to format Interleaved BGR straight into Planar RGB
    for (int y = 0; y < WARP_HEIGHT; ++y) {
        const cv::Vec3b* rowPtr = warpedBoard.ptr<cv::Vec3b>(y);
        int pixelRowIdx = y * WARP_WIDTH;

        for (int x = 0; x < WARP_WIDTH; ++x) {
            int writeIndex = pixelRowIdx + x;
            cv::Vec3b bgrPixel = rowPtr[x];

            // Normalize, convert channel lanes, and copy directly into OpenVINO memory space
            inputBufferPtr[r_offset + writeIndex] = ((static_cast<float>(bgrPixel[2]) / 255.0f) - mean_vals[0]) / std_vals[0]; // Red
            inputBufferPtr[g_offset + writeIndex] = ((static_cast<float>(bgrPixel[1]) / 255.0f) - mean_vals[1]) / std_vals[1]; // Green
            inputBufferPtr[b_offset + writeIndex] = ((static_cast<float>(bgrPixel[0]) / 255.0f) - mean_vals[2]) / std_vals[2]; // Blue
        }
    }

    // 2. Execute Zero-Copy Hardware Inference
    m_ovInferRequest.infer();

    // 3. Extract Accelerated Outputs from locked hardware memory registers
    ov::Tensor featTensor  = m_ovInferRequest.get_output_tensor(0);
    ov::Tensor logitTensor = m_ovInferRequest.get_output_tensor(1);

    auto featShape = featTensor.get_shape();
    int featChannels = static_cast<int>(featShape[1]); // 512 channels
    int featHeight   = static_cast<int>(featShape[2]); // 60
    int featWidth    = static_cast<int>(featShape[3]); // 105

    int sizes[] = {1, featChannels, featHeight, featWidth};
    float* rawFeatDataPtr = static_cast<float*>(featTensor.data());
    cv::Mat featMap(4, sizes, CV_32F, rawFeatDataPtr);

    std::cout << "[OPENVINO NATIVE] Feature Map Geometry resolved: Channels="
              << featChannels << ", H=" << featHeight << ", W=" << featWidth << "\n";

    float stepRow = static_cast<float>(featHeight) / static_cast<float>(NUM_ROW);
    float stepCol = static_cast<float>(featWidth) / static_cast<float>(NUM_COL);
    int num_classes = static_cast<int>(m_dnnDetectorClassList.size());
    int totalCells = NUM_ROW * NUM_COL;

    // 4. Packed Feature Extraction Matrix Generation
    cv::Mat packedFeatures(featChannels, totalCells, CV_32F);
    std::vector<std::pair<int, int>> spatialMap(totalCells);

    float* featMapPtr = featMap.ptr<float>(0);
    int planeStride = featHeight * featWidth;

    int cellIdx = 0;
    for(int row = 0; row < NUM_ROW; row++) {
        for(int col = 0; col < NUM_COL; col++) {
            spatialMap[cellIdx] = {row, col};

            int featY = cvRound((row + 0.5f) * stepRow - 0.5f);
            int featX = cvRound((col + 0.5f) * stepCol - 0.5f);
            featY = std::max(0, std::min(featY, featHeight - 1));
            featX = std::max(0, std::min(featX, featWidth - 1));

            int pixelOffset = featY * featWidth + featX;
            for (int c = 0; c < featChannels; ++c) {
                int tensorOffset = (c * planeStride) + pixelOffset;
                packedFeatures.ptr<float>(c)[cellIdx] = featMapPtr[tensorOffset];
            }
            cellIdx++;
        }
    }

    // 5. High-speed Batch Linear Layer Matrix Multiplication (GEMM)
    cv::Mat batchLogits;
    cv::gemm(m_fcWeightsMat, packedFeatures, 1.0, cv::Mat(), 0.0, batchLogits);

    for (int r = 0; r < batchLogits.rows; ++r) {
        float* rowPtr = batchLogits.ptr<float>(r);
        float biasVal = m_fcBiasMat.at<float>(r);
        for (int c = 0; c < batchLogits.cols; ++c) {
            rowPtr[c] += biasVal;
        }
    }

    // 6. Global Integral Image Color Pass for Backdrops
    cv::Mat globalHSV;
    cv::cvtColor(warpedBoard, globalHSV, cv::COLOR_BGR2HSV);

    cv::Mat globalGrayMask = cv::Mat::zeros(globalHSV.size(), CV_8UC1);
    cv::Mat globalGoldMask = cv::Mat::zeros(globalHSV.size(), CV_8UC1);

    int gray_hsv[3][3] = {{20,8,91}, {0,0,156}, {95,35,167}};
    int gray_tols[3][3] = {{50,40,40}, {50,40,40}, {10,40,40}};
    int gold_hsv[3][3] = {{18,190,185}, {21,98,243}, {15,204,80}};
    int gold_tols[3][3] = {{50,40,40}, {50,40,40}, {10,40,40}};

    cv::Mat tempMask;
    for (int k = 0; k < 3; ++k) {
        cv::Scalar lowGray(std::max(0, gray_hsv[k][0]-gray_tols[k][0]), std::max(0, gray_hsv[k][1]-gray_tols[k][1]), std::max(0, gray_hsv[k][2]-gray_tols[k][2]));
        cv::Scalar highGray(std::min(180, gray_hsv[k][0]+gray_tols[k][0]), std::min(255, gray_hsv[k][1]+gray_tols[k][1]), std::min(255, gray_hsv[k][2]+gray_tols[k][2]));
        cv::inRange(globalHSV, lowGray, highGray, tempMask);
        cv::bitwise_or(globalGrayMask, tempMask, globalGrayMask);

        cv::Scalar lowGold(std::max(0, gold_hsv[k][0]-gold_tols[k][0]), std::max(0, gold_hsv[k][1]-gold_tols[k][1]), std::max(0, gold_hsv[k][2]-gold_tols[k][2]));
        cv::Scalar highGold(std::min(180, gold_hsv[k][0]+gold_tols[k][0]), std::min(255, gold_hsv[k][1]+gold_tols[k][1]), std::min(255, gold_hsv[k][2]+gold_tols[k][2]));
        cv::inRange(globalHSV, lowGold, highGold, tempMask);
        cv::bitwise_or(globalGoldMask, tempMask, globalGoldMask);
    }

    cv::Mat integralGray, integralGold;
    cv::integral(globalGrayMask, integralGray, CV_32S);
    cv::integral(globalGoldMask, integralGold, CV_32S);

    // Phase 7: Evaluate classifications mapping spatial cells directly
    for (int i = 0; i < totalCells; ++i) {
        int row = spatialMap[i].first;
        int col = spatialMap[i].second;

        if(m_mapExcludedCell[row][col] == 0) {
            m_mapClassifiedCell[row][col] = '.';
            continue;
        }

        std::vector<float> exp_scores(num_classes);
        float max_score = -FLT_MAX;

        for(int c = 0; c < num_classes; ++c) {
            float score = batchLogits.at<float>(c, i);
            if(score > max_score) max_score = score;
        }

        float sum_exp = 0.0f;
        for (int c = 0; c < num_classes; ++c) {
            exp_scores[c] = std::exp(batchLogits.at<float>(c, i) - max_score);
            sum_exp += exp_scores[c];
        }

        int predicted_idx = 0;
        float max_prob = 0.0f;

        for (int c = 0; c < num_classes; ++c) {
            float prob = exp_scores[c] / sum_exp;
            if (prob > max_prob) {
                max_prob = prob;
                predicted_idx = c;
            }
        }

        char finalClassName = m_dnnDetectorClassList[predicted_idx];
        int cropX = col * cellSize;
        int cropY = row * cellSize;

        cv::Rect cropRect;
        if(col <= 3) {
            cropRect.width = cellSize * 2 / 3;
            cropRect.height = cellSize * 2 / 3;
            cropRect.x = cropX + cellSize - cropRect.width;
            cropRect.y = cropY + cellSize - cropRect.height;
        } else if(col >= 10) {
            cropRect.width = cellSize * 2 / 3;
            cropRect.height = cellSize * 2 / 3;
            cropRect.x = cropX;
            cropRect.y = cropY + cellSize - cropRect.height;
        } else {
            cropRect.width = cellSize;
            cropRect.height = cellSize / 2;
            cropRect.x = cropX;
            cropRect.y = cropY + cellSize - cropRect.height;
        }
        cropRect &= cv::Rect(0, 0, warpedBoard.cols, warpedBoard.rows);

        int x1 = cropRect.x; int y1 = cropRect.y;
        int x2 = cropRect.x + cropRect.width; int y2 = cropRect.y + cropRect.height;

        int grayPixels = (integralGray.at<int>(y2, x2) - integralGray.at<int>(y1, x2) - integralGray.at<int>(y2, x1) + integralGray.at<int>(y1, x1)) / 255;
        int goldPixels = (integralGold.at<int>(y2, x2) - integralGold.at<int>(y1, x2) - integralGold.at<int>(y2, x1) + integralGold.at<int>(y1, x1)) / 255;

        if(grayPixels > 3 * goldPixels / 2 && grayPixels > 1500) {
            finalClassName = std::tolower(finalClassName); // Black Side Piece mapping
        } else if((goldPixels > 3 * grayPixels / 2 && goldPixels > 1500) || goldPixels > 2000) {
            finalClassName = std::toupper(finalClassName); // White Side Piece mapping
        } else if(goldPixels + grayPixels < 2000){
            finalClassName = '.';
        }

        m_mapClassifiedCell[row][col] = finalClassName;
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Fully Vectorized Whole-board processing loop complete in: " << elapsed << " ms" << std::endl;
#ifdef DEBUG_ROI
    cv::Mat scaledWarped;
    cv::resize(warpedBoard, scaledWarped, cv::Size(WARP_SMALL_WIDTH, WARP_SMALL_HEIGHT), 0, 0, cv::INTER_NEAREST);
    cv::imshow("classification", scaledWarped);
#endif

    printf("Mapped Board State Layout:\r\n");
    for(int row = 0; row < NUM_ROW; row++) {
        for(int col = 0; col < NUM_COL; col++) {
            printf("%c ", m_mapClassifiedCell[row][col]);
        }
        printf("\r\n");
    }
}
#endif
void ChessImageProcessing::classsifyChessBoardImage(const cv::Mat& warpedBoard) {
    int cellSize = CELL_SIZE;
    printf("classsifyChessBoardImage (BATCH INF MODE: %dx%d, %d Channel(s)):\r\n",
           m_dnnDetectorSize, m_dnnDetectorSize, m_dnnDetectorChannels);
    auto start = std::chrono::steady_clock::now();

    // Pre-allocate containers to eliminate memory thrashing inside the core loop
    std::vector<cv::Mat> batchImages;
    std::vector<std::pair<int, int>> validCellPositions; // Stores tracking mappings: {row, col}
    batchImages.reserve(NUM_ROW * NUM_COL);
    validCellPositions.reserve(NUM_ROW * NUM_COL);

    // Phase 1: Rapidly parse coordinates and preprocess structural cells
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
            cv::Mat croppedCellBGR = warpedBoard(tallCellROI);
            cv::Mat preparedCell;

            // Handle channel mapping dynamically based on input parameter
            if (m_dnnDetectorChannels == 1) {
                // Convert 3-channel BGR to 1-channel Grayscale
                cv::cvtColor(croppedCellBGR, preparedCell, cv::COLOR_BGR2GRAY);
            } else if (m_dnnDetectorChannels == 3) {
                // Keep original BGR channels (blobFromImages will handle the RGB swap later)
                preparedCell = croppedCellBGR;
            } else {
                std::cerr << "Error: Supported channels configuration are 1 or 3. Received: " << m_dnnDetectorChannels << "\n";
                return;
            }

            // Collect processed references safely
            batchImages.push_back(preparedCell);
            validCellPositions.push_back({row, col});
        }
    }

    size_t totalValidPieces = batchImages.size();
    if (totalValidPieces == 0) {
        printf("No active piece cells identified for matching.\n");
        return;
    }

    // Phase 2: Create a 4D Tensor Batch Blob using 'blobFromImages'
    cv::Size target_size(m_dnnDetectorSize, m_dnnDetectorSize);
    double scale_factor = 1.0 / 255.0; // Scale pixels to [0.0, 1.0]

    cv::Mat batchBlob;
    bool swapChannels = (m_dnnDetectorChannels == 3); // Swap R and B channels only if we are feeding a 3-channel model

    if (m_dnnDetectorChannels == 1) {
        // Grayscale 1-channel custom weights normalization parity: (pixel - 127.5) * (1/255) / 0.5
        cv::Scalar mean_grayscale(127.5);
        double std_dev_grayscale = 0.5;

        cv::dnn::blobFromImages(
            batchImages, batchBlob, scale_factor, target_size,
            mean_grayscale, swapChannels, false
        );
        cv::divide(batchBlob, std_dev_grayscale, batchBlob);

    } else {
        // Legacy Multi-channel standard ImageNet weights normalization parity:
        // PyTorch applies: (pixel / 255.0 - mean) / std.
        // OpenCV subtracts the raw mean BEFORE applying scale_factor, so raw_mean = target_mean * 255.0
        cv::Scalar mean_rgb(0.485 * 255.0, 0.456 * 255.0, 0.406 * 255.0);
        cv::Scalar std_dev_rgb(0.229, 0.224, 0.225);

        cv::dnn::blobFromImages(
            batchImages, batchBlob, scale_factor, target_size,
            mean_rgb, swapChannels, false
        );
        cv::divide(batchBlob, std_dev_rgb, batchBlob);
    }

    // Phase 3: Execute full batch processing in a single forward pass
    m_dnnDetector.setInput(batchBlob, "input"); // "input" explicitly maps to the layer name set in torch.onnx.export
    cv::Mat outputs = m_dnnDetector.forward("output"); // "output" matches your exported ONNX configuration graph node

    int num_classes = outputs.cols;

    // Phase 4: Parse back the tensor outputs maps
    for (size_t i = 0; i < totalValidPieces; ++i) {
        int row = validCellPositions[i].first;
        int col = validCellPositions[i].second;

        // Re-extract the local BGR crop version purely for your legacy checkPieceColor overlay function
        int cropX = col * cellSize;
        int cropY = row * cellSize;
        cv::Rect tallCellROI(cropX, cropY, cellSize, cellSize);
        tallCellROI &= cv::Rect(0, 0, warpedBoard.cols, warpedBoard.rows);
        cv::Mat croppedCellBGR = warpedBoard(tallCellROI);

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
        piece.className = m_dnnDetectorClassList[predicted_idx];
        piece.probability = max_prob * 100.0f;
        piece.className2 = m_dnnDetectorClassList[predicted2_idx];
        piece.probability2 = max2_prob * 100.0f;

        // Execute background color checks locally (Uses original color matrix mapping rules)
        checkPieceColor(croppedCellBGR, piece, row, col);
        m_mapClassifiedCell[row][col] = piece.className;

#ifdef DEBUG_ROI
        cv::rectangle(warpedBoard, tallCellROI,
                      piece.probability > 95 ? cv::Scalar(0,255,255): cv::Scalar(0,255,0),
                      piece.probability > 95 ? 2 : 6);
        cv::putText(warpedBoard, std::string{piece.className} + ":" + std::to_string((int)piece.probability),
                    cv::Point(cropX + 0, cropY + 30),
                    cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
        cv::putText(warpedBoard, std::string{piece.className2} + ":" + std::to_string((int)piece.probability2),
                    cv::Point(cropX + 0, cropY + 60),
                    cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
        cv::putText(warpedBoard,
                    "GR:" + std::to_string(piece.grayPixels),
                    cv::Point(cropX + 0, cropY + 90),
                    cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
        cv::putText(warpedBoard,
                    "GO:" + std::to_string(piece.goldPixels),
                    cv::Point(cropX + 0, cropY + 110),
                    cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
#endif
    }
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Elapsed time: " << elapsed << " ms" << std::endl;

#ifdef DEBUG_ROI
    cv::Mat scaledWarped;
    cv::resize(warpedBoard, scaledWarped, cv::Size(WARP_SMALL_WIDTH, WARP_SMALL_HEIGHT), 0, 0, cv::INTER_NEAREST);
    cv::imwrite("scaledWarped.jpg",scaledWarped);
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

void ChessImageProcessing::warpChessBoardImage(const cv::Mat& imgCurrent,
                                               cv::Mat& imgWarped)
{
    // 1. Check board status
    cv::Mat homographyMatrix = getFullTranformMatrix();
    cv::warpPerspective(imgCurrent, imgWarped, homographyMatrix, cv::Size(WARP_WIDTH, WARP_HEIGHT));
    printf("warpedBoard[%dx%d]\r\n",imgWarped.cols,imgWarped.rows);
}
void ChessImageProcessing::getAnalyzeResult(std::vector<std::string>& analyzeResult)
{
    analyzeResult.clear();
    for(int row = 0; row < NUM_ROW; row++) {
        for(int col = 0; col < NUM_COL; col++) {
            std::string cellInfo;
            std::string pieceType(1, m_mapClassifiedCell[row][col]);
            cellInfo += pieceType;
            analyzeResult.push_back(cellInfo);
        }
    }
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
                    startCastle,endCastle,
                    params.playerSide == "white")) {
        std::string detectMove = coordToNotation(startCastle, params.playerSide)+
                coordToNotation(endCastle, params.playerSide);
        listMoves.push_back(detectMove);
        printf("Castle found\r\n");
        return listMoves;
    }

    // 3. Check for promotion
    cv::Point startPromote,endPromote;
    char promotePiece;
    if(isPromoteMove((char*)convertedPrevBoard,(char*)currentBoard,
                     startPromote,endPromote,promotePiece,
                     params.playerSide == "white")) {
        std::string promoteMove = coordToNotation(startPromote, params.playerSide)+
                coordToNotation(endPromote, params.playerSide)+std::string(1, promotePiece);
        listMoves.push_back(promoteMove);
        printf("Promote found [%s]\r\n",promoteMove.c_str());
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
