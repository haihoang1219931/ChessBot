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
    void extractMove(const cv::Mat prevColor, const cv::Mat nextColor, std::vector<cv::Rect>& moves);
    void convertChessMove(const std::vector<cv::Rect> moves, std::vector<cv::Point>& chessMoves);
    std::vector<cv::Point>& corners();
    cv::Mat getTranformMatrix();
    int chessBoardBox();
    int chessBoardRow();
    int chessBoardSize();
    void setThreshold(int threshold);
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
    std::vector<cv::Point2f> m_chessBoardCorners;
    std::vector<cv::Point> m_corners;

};

#endif // CHESSIMAGEPROCESSING_H
