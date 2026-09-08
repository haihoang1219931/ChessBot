//#include <opencv2/opencv.hpp>
//#include <opencv2/dnn.hpp>
//#include <iostream>
//#include <fstream>
//#include <vector>

//// Structure to store our 3D point cloud data
//struct Point3D {
//    float x, y, z;
//    uchar r, g, b;
//};

//// Function to save the 3D points to a standard PLY file format
//void saveToPLY(const std::string& filename, const std::vector<Point3D>& points) {
//    std::ofstream out(filename);
//    if (!out.is_open()) {
//        std::cerr << "Error: Could not open output file: " << filename << std::endl;
//        return;
//    }

//    // Write standard PLY ASCII header structure
//    out << "ply\n";
//    out << "format ascii 1.0\n";
//    out << "element vertex " << points.size() << "\n";
//    out << "property float x\n";
//    out << "property float y\n";
//    out << "property float z\n";
//    out << "property uchar red\n";
//    out << "property uchar green\n";
//    out << "property uchar blue\n";
//    out << "end_header\n";

//    // Write the structural 3D data grid loop
//    for (const auto& pt : points) {
//        out << pt.x << " " << pt.y << " " << pt.z << " "
//            << (int)pt.r << " " << (int)pt.g << " " << (int)pt.b << "\n";
//    }
//    out.close();
//    std::cout << "Success! Saved 3D model data to: " << filename << std::endl;
//}

//int main(int argc, char** argv) {
//    // 1. Load the target 2D photographic frame
//    std::string imagePath(argv[1]);
//    cv::Mat img = cv::imread(imagePath);

//    if (img.empty()) {
//        std::cerr << "Error: Could not find or load target image from: " << imagePath << std::endl;
//        return -1;
//    }
//    int h = img.rows;
//    int w = img.cols;
//    std::cout << "Loaded image layout dimension dimensions: " << w << "x" << h << std::endl;

//    // 2. Load the MiDaS Deep Learning Pipeline via the dnn binary module
//    std::string modelPath = "model-small.onnx";
//    std::cout << "Loading MiDaS network network layers..." << std::endl;
//    cv::dnn::Net net = cv::dnn::readNet(modelPath);

//    if (net.empty()) {
//        std::cerr << "Error: Failed to initialize Neural Network model configurations." << std::endl;
//        return -1;
//    }

//    // Explicitly target target CPU processing boundaries
//    net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
//    net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

//    // 3. Prepare the picture array for model entry execution patterns
//    cv::Mat blob = cv::dnn::blobFromImage(img, 1.0 / 255.0, cv::Size(256, 256), cv::Scalar(123.675, 116.28, 103.53), true, false);
//    net.setInput(blob);

//    std::cout << "Executing structural depth inference algorithms..." << std::endl;
//    cv::Mat output = net.forward();

//    // CORRECTED: Safe structural conversion from 4D Network Output Tensor -> 2D cv::Mat Image Architecture
//    // MiDaS outputs shape. We target the first 2D channel layer plane directly.
//    cv::Mat depthMap2D(256, 256, CV_32F, output.ptr<float>());

//    // Resize the 256x256 depth map back up matching your native asset proportions (240x240)
//    cv::Mat depthMap;
//    cv::resize(depthMap2D, depthMap, cv::Size(w, h));

//    // Normalize scale ranges mapping minimum/maximum structural target environments safely
//    cv::normalize(depthMap, depthMap, 0.1, 10.0, cv::NORM_MINMAX);

//    // 4. Transform individual 2D pixel mappings directly to a 3D target array
//    std::vector<Point3D> points;
//    points.reserve(h * w);

//    float focalLength = std::max(w, h);
//    float cx = w / 2.0f;
//    float cy = h / 2.0f;

//    for (int y = 0; y < h; ++y) {
//        for (int x = 0; x < w; ++x) {
//            float Z = depthMap.at<float>(y, x); // Depth mapping calculation point

//            // Traditional pinhole lens equations map 2D data indexes cleanly over 3D coordinates
//            float X = (x - cx) * Z / focalLength;
//            float Y = (y - cy) * Z / focalLength;

//            cv::Vec3b color = img.at<cv::Vec3b>(y, x); // OpenCV layout color reads in standard BGR order

//            Point3D pt;
//            pt.x = X;
//            pt.y = -Y; // Invert axis lines mapping orientation standard alignment layouts upright
//            pt.z = -Z;
//            pt.r = color[2]; // Map standard Red values
//            pt.g = color[1]; // Map standard Green values
//            pt.b = color[0]; // Map standard Blue values

//            points.push_back(pt);
//        }
//    }

//    // 5. Generate and export the resulting .ply file structure
//    saveToPLY("output_model.ply", points);

//    return 0;
//}
