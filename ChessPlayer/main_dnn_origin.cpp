//#include <opencv2/opencv.hpp>
//#include <opencv2/dnn.hpp>
//#include <iostream>
//#include <string>
//#include <vector>
//#include <cmath>

//using namespace cv;
//using namespace cv::dnn;
//using namespace std;

//static Mat sigmoidMat(const Mat &m) {
//    Mat out;
//    // assume m is CV_32F
//    out.create(m.size(), m.type());
//    const float* pm = (const float*)m.ptr<float>(0);
//    float* po = (float*)out.ptr<float>(0);
//    size_t tot = m.total();
//    for (size_t i = 0; i < tot; ++i) po[i] = 1.0f / (1.0f + std::exp(-pm[i]));
//    return out;
//}

//int main(int argc, char** argv) {
//    if (argc < 3) {
//        cout << "Usage: " << argv[0] << " <model.onnx> <image> [threshold=0.5]" << endl;
//        return 0;
//    }

//    string modelPath = argv[1];
//    string imagePath = argv[2];
//    float threshold = 0.5f;
//    if (argc >= 4) threshold = static_cast<float>(atof(argv[3]));

//    // load image
//    Mat img = imread(imagePath);
//    if (img.empty()) {
//        cerr << "Failed to load image: " << imagePath << endl;
//        return -1;
//    }

//    // load network
//    Net net;
//    try {
//        net = readNetFromONNX(modelPath);
//    } catch (const std::exception &e) {
//        cerr << "Failed to load ONNX model: " << e.what() << endl;
//        return -1;
//    }

//    // preprocessing: resize to 256x256, convert BGR->RGB if needed, normalize to [0,1]
//    const Size modelSize(256, 256);
//    Mat img_rgb;
//    cvtColor(img, img_rgb, COLOR_BGR2RGB);
//    Mat img_resized;
//    resize(img_rgb, img_resized, modelSize);
//    Mat img_float;
//    img_resized.convertTo(img_float, CV_32F, 1.0/255.0);

//    // create blob (NCHW)
//    Mat blob = blobFromImage(img_float); // defaults: scalefactor=1, size=modelSize, mean=Scalar(), swapRB=false (we already swapped)

//    net.setInput(blob);
//    Mat out = net.forward();

//    // out can be [1, C, H, W] or [1, H, W, C] or flat. We'll try to get single-channel map.
//    // Convert out to CV_32F single-channel Mat of size modelSize
//    Mat outMap;
//    if (out.dims == 4) {
//        // assume shape [1,1,H,W] or [1,C,H,W]
//        int dims[4];
//        for (int i = 0; i < 4; ++i) dims[i] = out.size[i];
//        int N = dims[0], C = dims[1], H = dims[2], W = dims[3];
//        Mat reshaped(H, W, CV_32F);
//        // if C>1 take first channel
//        vector<Mat> planes(C);
//        // split is not directly supported for 4D; use Mat::reshape
//        Mat outReshaped = out.reshape(1, {N*C, H, W});
//        // copy first channel
//        for (int r = 0; r < H; ++r) {
//            float* dst = reshaped.ptr<float>(r);
//            const float* src = out.ptr<float>(0, 0, r);
//            memcpy(dst, src, W * sizeof(float));
//        }
//        outMap = reshaped.clone();
//    } else if (out.total() == modelSize.width * modelSize.height) {
//        outMap = out.reshape(1, modelSize.height).clone();
//        outMap.convertTo(outMap, CV_32F);
//    } else {
//        // fallback: flatten and resize
//        Mat flat = out.reshape(1, 1);
//        Mat flatF; flat.convertTo(flatF, CV_32F);
//        outMap = flatF.reshape(1, modelSize.height).clone();
//        if (outMap.size() != modelSize) resize(outMap, outMap, modelSize);
//    }

//    // apply sigmoid
//    Mat mask = sigmoidMat(outMap);

//    // compute stats
//    double minVal, maxVal;
//    minMaxLoc(mask, &minVal, &maxVal);

//    // binary mask
//    Mat bin;
//    cv::threshold(mask, bin, (double)threshold, 1.0, THRESH_BINARY);
//    bin.convertTo(bin, CV_8U, 255.0);

//    // heatmap for visualization
//    Mat heat;
//    Mat mask8;
//    mask.convertTo(mask8, CV_8U, 255.0);
//    applyColorMap(mask8, heat, COLORMAP_JET);

//    // overlay on original (resize heat to original image size)
//    Mat heat_resized;
//    resize(heat, heat_resized, img.size());
//    Mat overlay = img.clone();
//    // blend where bin true
//    Mat bin_resized;
//    resize(bin, bin_resized, img.size());
//    for (int y = 0; y < img.rows; ++y) {
//        for (int x = 0; x < img.cols; ++x) {
//            if (bin_resized.at<uchar>(y,x) > 0) {
//                overlay.at<Vec3b>(y,x) = Vec3b(0,0,255); // red
//            }
//        }
//    }

//    // show windows
//    imshow("Original", img);
//    imshow("Heatmap", heat_resized);
//    imshow("Binary", bin_resized);
//    imshow("Overlay", overlay);

//    cout << "✅ Chess board segmentation completed!" << endl;
//    cout << "Mask shape: " << mask.rows << " x " << mask.cols << endl;
//    cout << "Mask range: " << minVal << " - " << maxVal << endl;

//    waitKey(0);
//    return 0;
//}
