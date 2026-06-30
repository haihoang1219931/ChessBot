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
#include <iostream>
#include <vector>
#include <map>
#include <math.h>
#include <sys/time.h> // for clock_gettime()
#include <unistd.h> // for usleep()
// Parameters container for detection (expandable)
struct MoveDetectParams {
    int threshold = 500;         // general threshold (unused currently)
    int roi_percent = 80;       // ROI percent of cell used for diff counting
    int canny_low = 14;         // Canny low threshold
    int diff_thresh = 30;       // threshold for absdiff -> binary
    int pieceMinPoints = 200;   // minimum edge points to consider a piece present
    int pieceRoiPercent = 80;  // ROI percent for piece detection
    int colorThreshold = 93;  // Color threshold
    int numLoopCheckPiece = 5;  // Color threshold
    std::string playerSide = "white";
};
class ChessImageProcessing
{
public:
    ChessImageProcessing();
    void connectSource(char* source);
    cv::Mat getNewImageSide();
    bool detectSide(cv::Mat image);
    bool isBlackSide();
    void setCorners(float topLeftX, float topLeftY,
                    float topRightX, float topRightY,
                    float bottomRightX, float bottomRightY,
                    float bottomLeftX, float bottomLeftY);
    cv::Mat getTranformMatrix();
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
                                      std::vector<cv::Point>& top3cells);
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
    bool detectMovePhase3Classification();
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

private:
    bool m_sourceConnected;
    bool m_isBlackSide;
    int m_chessBoardRow;
    int m_chessBoardBox;
    int m_chessBoardSize;
    int m_threshold;
    cv::Mat m_prevImage;
    cv::Mat m_currImage;
    cv::Mat m_transformMatrix;
    bool m_transformMaxtrixValid;
    int m_detectState;
};

#endif // CHESSIMAGEPROCESSING_H
