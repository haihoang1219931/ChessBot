#ifndef CHESSIMAGEPROCESSING_H
#define CHESSIMAGEPROCESSING_H

#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include <opencv2/dnn.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <math.h>
#include <sys/time.h> // for clock_gettime()
#include <unistd.h> // for usleep()

const int NUM_COL = 14;
const int NUM_ROW = 8;
const int CELL_SIZE = 240;
const int IMAGE_WIDTH = 1920;
const int IMAGE_HEIGHT = 1080;
const int WARP_WIDTH = CELL_SIZE*NUM_COL;
const int WARP_HEIGHT = CELL_SIZE*NUM_ROW;
const int CELL_SMALL_SIZE = 80;
const int WARP_SMALL_WIDTH = CELL_SMALL_SIZE*NUM_COL;
const int WARP_SMALL_HEIGHT = CELL_SMALL_SIZE*NUM_ROW;
const int MIN_BINARY_POINT = 600;

typedef enum {
    DETECT_MOVE_PHASE1_BINARY,
    DETECT_MOVE_PHASE2_SUBSTRACTION,
    DETECT_MOVE_PHASE3_CLASSIFICATION,
    DETECT_MOVE_VERIFY_RESULT,
    DETECT_MOVE_DONE_SUCCESS,
    DETECT_MOVE_DONE_FAIL,
} CHESSBOARD_DETECT_STATE;

// Parameters container for detection (expandable)
struct MoveDetectParams {
    int threshold = 500;         // general threshold (unused currently)
    int roi_percent = 80;       // ROI percent of cell used for diff counting
    int canny_low = 14;         // Canny low threshold
    int diff_thresh = 70;       // threshold for absdiff -> binary
    int pieceMinPoints = 200;   // minimum edge points to consider a piece present
    int pieceRoiPercent = 80;  // ROI percent for piece detection
    int colorThreshold = 93;  // Color threshold
    int numLoopCheckPiece = 5;  // Color threshold
    std::string playerSide = "white";
};
struct TargetColor {
    cv::Scalar hsvValue;
    int hTolerance;
    int sTolerance;
    int vTolerance;
};
typedef struct {
    char className;
    float probability;
    char className2;
    float probability2;
    std::string color;
    int row;
    int col;
    int grayPixels;
    int goldPixels;
} ClassificationResult;

class ChessImageProcessing
{
public:
    ChessImageProcessing();
    void setDnnNetAllPieces(char* source, const std::vector<char>& dnnClassNames);
    void setDnnNetSpecial(char* source, const std::vector<char>& dnnClassNames);
    void connectSource(char* source);
    cv::Mat getNewImageSide();
    bool detectSide(cv::Mat image);
    bool isBlackSide();
    void setCorners(float topLeftX, float topLeftY,
                    float topRightX, float topRightY,
                    float bottomRightX, float bottomRightY,
                    float bottomLeftX, float bottomLeftY);
    cv::Mat getSubTranformMatrix();
    cv::Mat getFullTranformMatrix();
    int chessBoardBox();
    int chessBoardRow();
    int chessBoardSize();
    void setThreshold(int threshold);

    bool detectMovePhase1Binary(const cv::Mat& edges1, const cv::Mat& edges2,
                                std::vector<cv::Point>& starts, std::vector<cv::Point>& ends,
                                const MoveDetectParams& params);
    bool detectMovePhase1ColorFilter(const cv::Mat& color1, const cv::Mat& color2,
                                std::vector<cv::Point>& starts, std::vector<cv::Point>& ends,
                                const MoveDetectParams& params,
                                std::vector<std::vector<int>>& matColorMapBefore,
                                std::vector<std::vector<int>>& matColorMapAfter);
    // now accepts a params struct rather than many separate arguments
    bool detectMovePhase2Substraction(const cv::Mat& img_start, const cv::Mat& img_end,
                                      const MoveDetectParams& params,
                                      std::vector<cv::Point>& listChangedCells);
    std::vector<std::string> detectMovePhase3ColorMatching(const cv::Mat& warped1, const cv::Mat& warped2,
                                    const std::vector<cv::Point>& startCells, std::vector<cv::Point> listChangedCell,
                                    const MoveDetectParams& params);
    std::vector<std::string> detectMovePhase3ColorMatchingFromFilter(const std::vector<cv::Point>& startCells,
                                    std::vector<cv::Point> listChangedCell,
                                    const MoveDetectParams& params,
                                    const std::vector<std::vector<int>> matColorMapBefore,
                                    const std::vector<std::vector<int>> matColorMapAfter);
    std::vector < std::vector < int >> cellColorFilterToMatrix(const cv::Mat& colorWarped, cv::Vec3b targetHSV,
                                                  int hTol, int sTol, int vTol,
                                                  int roiPercent, int minWhitePercent, int maxBlackPercent,
                                                  std::string name);
    int countMatchPixelColor(const cv::Mat& imageHSV, const std::vector<TargetColor>& targetColors, int maxH, int maxSV, std::string showName);
    void checkPieceColor(const cv::Mat& imageRGB, ClassificationResult& pieceClass, int row, int col);
    bool detectMovePhase3Classification();
    ClassificationResult classifyImage(const cv::Mat& input_mat, int row, int col);
    void classsifyWholeBoardAtOnce(const cv::Mat& warpedBoard, int targetWidth = WARP_WIDTH, int targetHeight = WARP_HEIGHT, int channels = 3);
    void classsifyChessBoardImage(const cv::Mat& warpedBoard, int targetWidth = CELL_SIZE, int targetHeight = CELL_SIZE, int channels = 3);
    void classsifyChessBoardImage2(const cv::Mat& warpedBoard);
    void excludeCellList(std::vector<cv::Point> listCell);
    std::string coordToNotation(cv::Point pt, const std::string& playerSide);
    cv::Point notationToCoord(const std::string& notation, const std::string& playerSide);
    bool getCenterOfWhitePixels(const cv::Mat& binary_img, cv::Point& center);
    bool isChessPieceCell(const cv::Mat& edges, int c, int r, int sq, int min_points, int roi_percent, cv::Mat& display);
    std::vector<std::vector<int>> getPieceMatrix(const cv::Mat& gray, int sq, const MoveDetectParams& params, std::string show_name);
    std::vector<std::vector<int>> getPieceMatrixColor(const cv::Mat& color, const MoveDetectParams& params, std::string show_name);
    void comparePieceMatrices(const std::vector<std::vector<int>>& mat1, const std::vector<std::vector<int>>& mat2,
                              std::vector<cv::Point>& starts, std::vector<cv::Point>& ends);
    // findPossibleMoves now takes a MoveDetectParams struct
    std::vector<std::string> findPossibleMoves(const cv::Mat& img_start, const cv::Mat& img_end,
                                    const MoveDetectParams& params);

    std::vector<std::string> findPossibleMoves2(const cv::Mat& imgCurrent,const char* prevBoard,
                                    const MoveDetectParams& params);

    // GUI helpers: create a shared Controls window (main should call) and read current params
    void createControlsWindow(const MoveDetectParams& defaults);
    MoveDetectParams readControlsFromWindow();

    // Find two cells (from a list of cell coordinates) whose ROI colors in a warped color image are similar.
    // Returns a pair of points (first, second). If not found, returns pair of (-1,-1).
    std::pair<cv::Point, cv::Point> findSameColoredCellsInImage(const cv::Mat& warpedColor, const std::vector<cv::Point>& cells, const MoveDetectParams& params, double colorThreshold = 30.0);

    // Match a start cell color (from warpedStartColor) to candidate cells (from warpedEndColor).
    // Returns matched candidate cell or (-1,-1) if none found. Also shows visualization windows.
    std::vector<cv::Point> matchStartToCandidates(const cv::Mat& warpedStartColor, const cv::Mat& warpedEndColor,
                                     const cv::Point& startCell, const std::vector<cv::Point>& candidates,
                                     const MoveDetectParams& params, double colorThreshold = 30.0);
    bool filterCellColor(
      cv::Mat warpedCell, cv::Vec3b targetHSV,
      int hTol, int sTol, int vTol,
      int roiPercent, int minWhitePercent, int maxBlackPercent,
      std::string nameToShow);
    bool isCastleMove(const cv::Mat& warpedGray1, const cv::Mat& warpedGray2, const MoveDetectParams& params,
                      cv::Point& startCell, cv::Point& endCell);
    bool isCastleMove(char* prevBoard, char* currBoard,
                      cv::Point& startCell, cv::Point& endCell,
                      bool whiteMove);
    bool isPromoteMove(char* prevBoard, char* currBoard,
                       cv::Point& startCell, cv::Point& endCell,char& promotePice,
                       bool whiteMove);
    bool findDropCells(std::vector<cv::Point>& dropCells);
    bool findPromotePiece(cv::Point& promoteCell, char piece);
private:
    bool m_sourceConnected;
    bool m_isBlackSide;
    int m_chessBoardRow;
    int m_chessBoardBox;
    int m_chessBoardSize;
    int m_threshold;
    cv::Mat m_prevImage;
    cv::Mat m_currImage;
    cv::Mat m_transformHeadPiecesWholeBoard;
    cv::Mat m_transformMatrix;
    bool m_transformMaxtrixValid;
    int m_detectState;
    cv::dnn::Net m_dnnNetAllPieces;
    std::vector<char> m_dnnAllPiecesNames;
    cv::dnn::Net m_dnnNetBishopPawn;
    std::vector<char> m_dnnBishopPawnNames;
    uint8_t m_mapExcludedCell[NUM_ROW][NUM_COL];
    char m_mapClassifiedCell[NUM_ROW][NUM_COL];
    // Internal layer parameters mapped from training weights file
    cv::Mat m_fcWeightsMat; // Size: [7 x 512]
    cv::Mat m_fcBiasMat;    // Size: [7 x 1]
    bool m_isFcLayersInitialized = false;
    // Internal helper initialization method
    void initializeManualClassificationHead();
};

#endif // CHESSIMAGEPROCESSING_H
