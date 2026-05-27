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
    bool detectMovePhase1Binary(const cv::Mat& img1, const cv::Mat& img2,
                                cv::Point& start, std::vector<cv::Point>& ends,
                                int min_points, int roi_percent, int canny_low,
                                const std::string& playerSide = "white");
    bool detectMovePhase2Substraction(const cv::Mat& img_start, const cv::Mat& img_end,
                                      int threshold_val, int roi_percent,
                                      int canny_low, int diff_thresh,
                                      std::vector<cv::Point>* top3cells,
                                      const std::string& playerSide = "white");
    bool detectMovePhase3Classification();
    std::string coordToNotation(cv::Point pt, const std::string& playerSide);
    cv::Point notationToCoord(const std::string& notation, const std::string& playerSide);
    bool isChessPieceCell(const cv::Mat& edges, int c, int r, int sq, int min_points, int roi_percent, cv::Mat& display);
    std::vector<std::vector<int>> getPieceMatrix(const cv::Mat& img, int sq, int min_points, int roi_percent, int canny_low, std::string show_name);
    void comparePieceMatrices(const std::vector<std::vector<int>>& mat1, const std::vector<std::vector<int>>& mat2, cv::Point& start, std::vector<cv::Point>& ends);
    std::vector<std::string> findPossibleMoves(const cv::Mat& img_start, const cv::Mat& img_end,
                                   const std::string& playerSide = "white");
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
    int m_detectState;
};

#endif // CHESSIMAGEPROCESSING_H
