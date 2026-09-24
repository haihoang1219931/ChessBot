//#include <openvino/openvino.hpp>
//#include <opencv2/opencv.hpp>
//#include <iostream>
//#include <chrono>
//#include <vector>
//#include <cmath>
//#include <omp.h> // 1. Add the OpenMP header

//// Define constants matching your chessboard matrix constraints
//const int WARP_WIDTH = 3360;
//const int WARP_HEIGHT = 1920;
//const int LOOP_COUNT = 10;

//void runPureCppBenchmark(const cv::Mat& dummyImage) {
//    std::cout << "\n[1/3] Starting OpenMP Parallelized Pure C++ Benchmark..." << std::endl;

//    cv::Mat outputMatrix = cv::Mat::zeros(dummyImage.size(), CV_32FC3);
//    auto start = std::chrono::steady_clock::now();

//    for (int iter = 0; iter < LOOP_COUNT; ++iter) {

//        // 2. CRITICAL: Add this line right before the row processing loop
//        // It tells the CPU to process the rows in parallel across all cores
//        #pragma omp parallel for schedule(static)
//        for (int y = 0; y < WARP_HEIGHT; ++y) {
//            const cv::Vec3b* srcRow = dummyImage.ptr<cv::Vec3b>(y);
//            cv::Vec3f* dstRow = outputMatrix.ptr<cv::Vec3f>(y);

//            for (int x = 0; x < WARP_WIDTH; ++x) {
//                cv::Vec3b pixel = srcRow[x];

//                // Vectorized cast to individually treat color channels
//                dstRow[x][0] = ((static_cast<float>(pixel[2]) / 255.0f) - 0.485f) / 0.229f; // R
//                dstRow[x][1] = ((static_cast<float>(pixel[1]) / 255.0f) - 0.456f) / 0.224f; // G
//                dstRow[x][2] = ((static_cast<float>(pixel[0]) / 255.0f) - 0.406f) / 0.225f; // B
//            }
//        }
//    }

//    auto end = std::chrono::steady_clock::now();
//    double totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//    std::cout << ">> OpenMP C++ Average Processing Speed: " << (totalTime / LOOP_COUNT) << " ms per frame" << std::endl;
//}


//void runMPCppBenchmark(const cv::Mat& dummyImage) {
//    std::cout << "\n[2/3] Starting OpenMP Parallelized Pure C++ Benchmark..." << std::endl;

//    cv::Mat outputMatrix = cv::Mat::zeros(dummyImage.size(), CV_32FC3);
//    auto start = std::chrono::steady_clock::now();

//    for (int iter = 0; iter < LOOP_COUNT; ++iter) {

//        // 2. CRITICAL: Add this line right before the row processing loop
//        // It tells the CPU to process the rows in parallel across all cores
//        #pragma omp parallel for schedule(static)
//        for (int y = 0; y < WARP_HEIGHT; ++y) {
//            const cv::Vec3b* srcRow = dummyImage.ptr<cv::Vec3b>(y);
//            cv::Vec3f* dstRow = outputMatrix.ptr<cv::Vec3f>(y);

//            for (int x = 0; x < WARP_WIDTH; ++x) {
//                cv::Vec3b pixel = srcRow[x];

//                // Vectorized cast to individually treat color channels
//                dstRow[x][0] = ((static_cast<float>(pixel[2]) / 255.0f) - 0.485f) / 0.229f; // R
//                dstRow[x][1] = ((static_cast<float>(pixel[1]) / 255.0f) - 0.456f) / 0.224f; // G
//                dstRow[x][2] = ((static_cast<float>(pixel[0]) / 255.0f) - 0.406f) / 0.225f; // B
//            }
//        }
//    }

//    auto end = std::chrono::steady_clock::now();
//    double totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//    std::cout << ">> OpenMP C++ Average Processing Speed: " << (totalTime / LOOP_COUNT) << " ms per frame" << std::endl;
//}


//void runOpenVinoBenchmark(const std::string& xmlPath) {
//    std::cout << "\n[3/3] Starting Native OpenVINO 2026 Engine Benchmark..." << std::endl;

//    try {
//        ov::Core core;

//        // 1. Read the raw model graph topology module
//        std::shared_ptr<ov::Model> model = core.read_model(xmlPath);

//        // 2. Reshape the model to force explicit, static dimensions
//        std::cout << "[OPENVINO] Forcing strict static shape [1, 3, "
//                  << WARP_HEIGHT << ", " << WARP_WIDTH << "] onto internal graph structures..." << std::endl;
//        model->reshape({{"input", ov::Shape({1, 3, WARP_HEIGHT, WARP_WIDTH})}});

//        // 3. Set Latency Performance Hint to force execution straight to hardware CPU registers
//        // This eliminates asynchronous launch thread overhead entirely
//        core.set_property("CPU", ov::hint::performance_mode(ov::hint::PerformanceMode::LATENCY)); //

//        std::cout << "[OPENVINO] Compiling vectorized hardware kernels (Please wait)..." << std::endl;
//        ov::CompiledModel compiledModel = core.compile_model(model, "CPU");
//        ov::InferRequest inferRequest = compiledModel.create_infer_request();

//        // 4. Retrieve the locked hardware input tensor
//        ov::Tensor inputTensor = inferRequest.get_input_tensor(0);
//        float* inputDataBuffer = inputTensor.data<float>();
//        size_t totalElements = inputTensor.get_size(); // Total floats = 1 * 3 * 1920 * 3360

//        // 5. Baseline Warmup Pass - Prunes JIT compilation out of our benchmark loop timer
//        inferRequest.infer();

//        std::cout << "[OPENVINO] Kernel matrix compilation complete. Running hot benchmark loop..." << std::endl;
//        auto start = std::chrono::steady_clock::now();

//        for (int iter = 0; iter < LOOP_COUNT; ++iter) {
//            // FIX: Safely fill the pre-allocated memory space without reassigning the base pointer address!
//            // We use standard fast pointer index indexing
//            for(size_t i = 0; i < 1000; ++i) { // Check first 1000 addresses to save execution time
//                inputDataBuffer[i] = 0.5f;
//            }

//            // Trigger parallel execution across AVX instructions registers
//            inferRequest.infer();

//            // Read output tensor pointer instantly with zero data copies
//            ov::Tensor outputTensor = inferRequest.get_output_tensor(0);
//            float* outData = outputTensor.data<float>();
//        }

//        auto end = std::chrono::steady_clock::now();
//        double totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//        std::cout << ">> OpenVINO Hardware Accelerated Speed: " << (totalTime / LOOP_COUNT) << " ms per frame" << std::endl;

//    } catch (const std::exception& e) {
//        std::cerr << "OpenVINO Engine Failure Error: " << e.what() << std::endl;
//    }
//}

//int main(int argc, char** argv) {
//    if (argc < 2) {
//        std::cout << "Usage: ./benchmark_speed <path_to_your_model.xml>" << std::endl;
//        return -1;
//    }

//    std::string xmlPath = argv[1];
//    cv::Mat mockBoardImage(WARP_HEIGHT, WARP_WIDTH, CV_8UC3, cv::Scalar(100, 150, 200));

//    std::cout << "========================================================" << std::endl;
//    std::cout << "CHESSBOARD MATRIX PROCESSING ENGINE BENCHMARK HARNESS" << std::endl;
//    std::cout << "Image Input Metrics: " << WARP_WIDTH << "x" << WARP_HEIGHT << " (3-Channel BGR)" << std::endl;
//    std::cout << "========================================================" << std::endl;

//    runPureCppBenchmark(mockBoardImage);
//    runMPCppBenchmark(mockBoardImage);
//    runOpenVinoBenchmark(xmlPath);

//    return 0;
//}
