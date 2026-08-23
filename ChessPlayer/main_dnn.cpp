//#include <opencv2/opencv.hpp>
//#include <opencv2/dnn.hpp>
//#include <iostream>
//#include <vector>
//#include <string>
//#include <cmath>
//#include <numeric>
//#define DEBUG_ROI
//const int WARP_SIZE = 1920;
//// Function that accepts a cv::Mat and runs ONNX inference
//typedef struct {
//    std::string className;
//    float probability;
//} ClassificationResult;
//ClassificationResult classifyImage(const cv::Mat& input_mat, cv::dnn::Net& net, const std::vector<std::string>& class_names) {
//    ClassificationResult result{"",0};
//    if (input_mat.empty()) {
//        std::cerr << "Error: Provided input cv::Mat is empty.\n";
//        return result;
//    }

//    // 1. Setup preprocessing parameters
//    cv::Size target_size(240, 240);
//    double scale_factor = 1.0 / 255.0; // Scale pixels to [0.0, 1.0]

//    // PyTorch ImageNet mean values multiplied by 255.0 because blobFromImage
//    // subtracts the raw mean *before* multiplying by the scalefactor.
//    cv::Scalar mean(0.485 * 255.0, 0.456 * 255.0, 0.406 * 255.0);

//    // PyTorch ImageNet standard deviation values
//    cv::Scalar std_dev(0.229, 0.224, 0.225);

//    // 2. Generate the 4D input blob
//    cv::Mat blob;
//    cv::dnn::blobFromImage(
//        input_mat,
//        blob,
//        scale_factor,
//        target_size,
//        mean,
//        true,  // swapRB = true (Converts BGR to RGB)
//        false  // crop = false
//    );

//    // 3. Manually divide by standard deviation (OpenCV DNN doesn't do this automatically)
//    cv::divide(blob, std_dev, blob);

//    // 4. Run inference pass
//    net.setInput(blob);
//    cv::Mat outputs = net.forward(); // Output shape: [1, num_classes]

//    // 5. Post-processing: Apply manual Softmax to the row vector
//    float* data_ptr = outputs.ptr<float>(0);
//    int num_classes = outputs.cols;

//    std::vector<float> raw_scores(data_ptr, data_ptr + num_classes);
//    std::vector<float> exp_scores(num_classes);

//    float max_score = *std::max_element(raw_scores.begin(), raw_scores.end());
//    float sum_exp = 0.0f;

//    for (int i = 0; i < num_classes; ++i) {
//        exp_scores[i] = std::exp(raw_scores[i] - max_score); // Stable Softmax implementation
//        sum_exp += exp_scores[i];
//    }

//    // 6. Find the highest probability index
//    int predicted_idx = 0;
//    float max_prob = 0.0f;
//    for (int i = 0; i < num_classes; ++i) {
//        float prob = exp_scores[i] / sum_exp;
//        if (prob > max_prob) {
//            max_prob = prob;
//            predicted_idx = i;
//        }
//    }
//#ifdef DEBUG_CLASSIFICATION
//    // 7. Print Results
//    std::cout << "Prediction Result: " << class_names[predicted_idx] << "\n";
//    std::cout << "Confidence Level: " << std::fixed << std::setprecision(2) << (max_prob * 100.0f) << "%\n";
//#endif
//    result.className = class_names[predicted_idx];
//    result.probability = max_prob * 100.0f;
//    return result;
//}

//void classsifyChessBoardImage(cv::Mat& warpedBoard, cv::dnn::Net& net, const std::vector<std::string>& class_names) {
//        int cellSize = WARP_SIZE / 8;
//    for(int row = 0; row < 8; row ++) {
//        for(int col = 0; col < 8; col ++) {
//            // Stretch the bounding box upwards to swallow full tall piece outlines
//            int cropX = col * cellSize;
//            int cropY = row * cellSize;
//            int cropW = cellSize;
//            int cropH = cellSize;

//            // Safe image-canvas bound clamping checks
//            if (cropX < 0) cropX = 0;
//            if (cropY < 0) cropY = 0;
//            if (cropX + cropW > warpedBoard.cols) cropW = warpedBoard.cols - cropX;
//            if (cropY + cropH > warpedBoard.rows) cropH = warpedBoard.rows - cropY;

//            cv::Rect tallCellROI(cropX, cropY, cropW, cropH);
//            cv::Mat croppedCell = warpedBoard(tallCellROI);
//            ClassificationResult piece = classifyImage(croppedCell,net,class_names);
//#ifdef DEBUG_ROI
//            cv::rectangle(warpedBoard,tallCellROI,cv::Scalar(0,255,255),2);
//            cv::putText(warpedBoard,piece.className + ":" +std::to_string((int)piece.probability),
//                        cv::Point(cropX + 20,cropY+ 60),
//                         cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);

//#endif
//            printf("%s ",piece.className.c_str());
//        }
//        printf("\r\n");
//    }
//#ifdef DEBUG_ROI
//    cv::Mat scaledWarped;
//    cv::resize(warpedBoard,scaledWarped,cv::Size(WARP_SIZE/3,WARP_SIZE/3),0,0, cv::INTER_NEAREST);
//    cv::imshow("scaledWarped",scaledWarped);
//    cv::waitKey();
//#endif
//}

//int main(int argc, char** argv) {
//    std::string onnx_path(argv[1]);
//    std::string image_path(argv[2]);

//    // Strict list order matching your folder layout
//    std::vector<std::string> class_names = {
//        "gen-Bishop", "gen-Empty", "gen-King", "gen-Knight", "gen-Pawn", "gen-Queen", "gen-Rook"
//    };
//    std::vector<std::string> print_names = {
//        "b", ".", "k", "n", "p", "q", "r"
//    };

//    // Load the network configuration
//    cv::dnn::Net net = cv::dnn::readNetFromONNX(onnx_path);

//    // Optional CPU configuration
//    net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
//    net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

//    // Simulate reading an image into a cv::Mat (e.g. from camera stream or file)
//    cv::Mat frame = cv::imread(image_path);
//    if (frame.empty()) {
//        std::cerr << "Could not open target image file.\n";
//        return -1;
//    }

//    std::vector<cv::Point2f> srcCorners;
//    float scaleFactor = 3.0f;
//    srcCorners.push_back(cv::Point2f(scaleFactor*176.121f, scaleFactor*26.5017f));
//    srcCorners.push_back(cv::Point2f(scaleFactor*474.672f, scaleFactor*29.3803f));
//    srcCorners.push_back(cv::Point2f(scaleFactor*516.384f, scaleFactor*335.672f));
//    srcCorners.push_back(cv::Point2f(scaleFactor*138.976f, scaleFactor*335.899f));
//    // 3. Set destination anchors inside the expanded layout grid canvas
//    std::vector<cv::Point2f> dstCorners {
//        cv::Point2f(0, 0),
//        cv::Point2f(WARP_SIZE - 1, 0),
//        cv::Point2f(WARP_SIZE - 1, WARP_SIZE - 1),
//        cv::Point2f(0, WARP_SIZE - 1)
//    };
//    // 4. Generate transformation matrix and execute warp
//    cv::Mat homographyMatrix = cv::getPerspectiveTransform(srcCorners, dstCorners);
//    cv::Mat warpedBoard;
//    cv::warpPerspective(frame, warpedBoard, homographyMatrix, cv::Size(WARP_SIZE, WARP_SIZE));
//    // Pass the cv::Mat matrix directly into your classifier function
//    classsifyChessBoardImage(warpedBoard, net, print_names);

//    return 0;
//}
