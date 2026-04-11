#include "ChessImageProcessing.h"

ChessImageProcessing::ChessImageProcessing()
{
    m_sourceConnected = false;
    m_isBlackSide = true;
    m_chessBoardRow = 8;
    m_chessBoardBox = 60;
    m_chessBoardSize = m_chessBoardRow * m_chessBoardBox;
    m_chessBoardCorners = {cv::Point2f (0,0),
                         cv::Point2f (m_chessBoardSize,0),
                         cv::Point2f (m_chessBoardSize,m_chessBoardSize),
                         cv::Point2f (0,m_chessBoardSize)};
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
std::vector<cv::Point>& ChessImageProcessing::corners() {
    return m_corners;
}

void ChessImageProcessing::extractMove(const cv::Mat prevColor, const cv::Mat nextColor, std::vector<cv::Rect>& moves){
    struct timeval start, end;
    long secs_used,micros_used;
    gettimeofday(&start, NULL);
    cv::Mat prev, next;
    cv::Mat motion;
    moves.clear();
    cvtColor( prevColor, prev, cv::COLOR_BGR2GRAY ); // Convert the image to Gray
    cvtColor( nextColor, next, cv::COLOR_BGR2GRAY ); // Convert the image to Gray
    cv::absdiff(prev, next, motion);
    cv::imwrite("1.jpg",prevColor);
    cv::imwrite("2.jpg",nextColor);
    cv::imwrite("diff.jpg",motion);
#ifdef DEBUG_SHOW
    cv::imshow("Diff",motion);
#endif
    cv::threshold(motion, motion, m_threshold, 255, cv::THRESH_BINARY);
    cv::erode(motion, motion, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3)));
    cv::dilate(motion, motion, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(10,10)));
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    findContours(motion, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_NONE);
    std::vector<std::vector<cv::Point> > contours_poly( contours.size() );
    std::vector<cv::Rect> boundRect( contours.size() );

    for( size_t i = 0; i < contours.size(); i++ )
    {
        approxPolyDP( contours[i], contours_poly[i], 3, true );
        boundRect[i] = boundingRect( contours_poly[i] );
        if(boundRect[i].width > 20 && boundRect[i].height > 20) {
            moves.push_back(boundRect[i]);
            rectangle( motion, boundRect[i].tl(), boundRect[i].br(), cv::Scalar(255,255,255), 2 );
        }
    }
    gettimeofday(&end, NULL);
    secs_used=(end.tv_sec - start.tv_sec); //avoid overflow by subtracting first
    micros_used= ((secs_used*1000000) + end.tv_usec) - (start.tv_usec);

    printf("micros_used: %d\n",micros_used);
    cv::imwrite("motion.jpg",motion);
#ifdef DEBUG_SHOW
    cv::imshow("motion",motion);
#endif
}
void ChessImageProcessing::convertChessMove(const std::vector<cv::Rect> moves, std::vector<cv::Point>& chessMoves) {
    if(m_corners.size() != 4) {
        return;
    }
    cv::Point2f source_points[4];
    cv::Point2f dest_points[4];
    for(int i=0; i< m_corners.size(); i++) {
        source_points[i].x = (float)(m_corners[i].x);
        source_points[i].y = (float)(m_corners[i].y);
        dest_points[i].x = (float)(m_chessBoardCorners[i].x);
        dest_points[i].y = (float)(m_chessBoardCorners[i].y);
    }
    m_transformMatrix = cv::getPerspectiveTransform(source_points, dest_points);
#ifdef DEBUG_MOVE_DETECTOR
    std::cout << "transformMatrix :" << m_transformMatrix << std::endl;
#endif
    std::vector<cv::Rect> warpedMoves;
    for( size_t i = 0; i < moves.size(); i++ ){
        std::vector<cv::Point2f> obj_corners,scene_corners;
        obj_corners.push_back(cv::Point2f(moves[i].tl().x,moves[i].tl().y));
        obj_corners.push_back(cv::Point2f(moves[i].tl().x,moves[i].br().y));
        obj_corners.push_back(cv::Point2f(moves[i].br().x,moves[i].br().y));
        obj_corners.push_back(cv::Point2f(moves[i].br().x,moves[i].tl().y));
        cv::perspectiveTransform(obj_corners,scene_corners,m_transformMatrix);
        warpedMoves.push_back(cv::Rect(scene_corners[0],scene_corners[2]));
        int row = (warpedMoves[i].x + warpedMoves[i].width/2)/m_chessBoardBox;
        int col = (warpedMoves[i].y + warpedMoves[i].height/2)/m_chessBoardBox;
        if(row >=0 && row <m_chessBoardRow &&
                col >=0 && col <m_chessBoardRow ) {
            chessMoves.push_back(cv::Point(row,col));
        }
    }
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
