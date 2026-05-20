#ifndef CHESSIMAGEPROCESSING_H
#define CHESSIMAGEPROCESSING_H

#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>
#include "opencv2/features2d/features2d.hpp"
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
    std::string coordToNotation(cv::Point pt, const std::string& playerSide);
    cv::Point notationToCoord(const std::string& notation, const std::string& playerSide);
    std::vector<std::string> findPossibleMoves(const cv::Mat& img_start, const cv::Mat& img_end,
                                   int threshold_val, int roi_percent,
                                   int canny_low, int diff_thresh,
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

};

#endif // CHESSIMAGEPROCESSING_H
