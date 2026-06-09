#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <iostream>
#include <fstream>

using namespace cv;
using namespace cv::dnn;
using namespace std;

static Mat g_img;
static vector<Point2f> g_corners;
static Mat g_warp;
static bool g_warpReady = false;
static Net g_net;
static vector<string> g_labels;
static Size g_inputSize(256,256);

void drawCornersAndShow() {
    Mat disp = g_img.clone();
    for (size_t i = 0; i < g_corners.size(); ++i) {
        circle(disp, g_corners[i], 6, Scalar(0, 255, 0), -1);
        putText(disp, to_string((int)i+1), g_corners[i] + Point2f(6.f, -6.f), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0,255,0), 2);
    }
    imshow("Input", disp);
}

void computeWarp() {
    if (g_corners.size() != 4) return;
    vector<Point2f> dst{
        Point2f(0,0), Point2f(640-1,0), Point2f(640-1,640-1), Point2f(0,640-1)
    };
    Mat M = getPerspectiveTransform(g_corners, dst);
    warpPerspective(g_img, g_warp, M, Size(640,640));
    g_warpReady = true;
    imshow("Warped", g_warp);
}

void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == EVENT_LBUTTONDOWN) {
        if (g_corners.size() < 4) {
            g_corners.emplace_back((float)x, (float)y);
            drawCornersAndShow();
            if (g_corners.size() == 4) computeWarp();
        }
    } else if (event == EVENT_RBUTTONDOWN) {
        // reset
        g_corners.clear();
        g_warpReady = false;
        destroyWindow("Warped");
        imshow("Input", g_img);
    }
}

bool loadLabels(const string &path, vector<string> &labels) {
    if (path.empty()) return false;
    ifstream ifs(path);
    if (!ifs.is_open()) return false;
    string line;
    while (getline(ifs, line)) {
        if (!line.empty()) {
            labels.push_back(line);
            std::cout << "label: " << line << std::endl;
        }
    }
    return !labels.empty();
}

static Mat sigmoidMat(const Mat &m) {
    Mat out;
    m.convertTo(out, CV_32F);
    // apply sigmoid elementwise
    out.forEach<float>([](float &v, const int * pos) {
        v = 1.0f / (1.0f + std::exp(-v));
    });
    return out;
}

void runRecognitionAndShow() {
    if (!g_warpReady) return;
    Mat vis; cvtColor(g_warp, vis, COLOR_BGR2RGB); // keep color
    int cellW = g_warp.cols / 1;
    int cellH = g_warp.rows / 1;

    // helper to access raw float pointer for 4D Mat [N,C,H,W]
    auto get4 = [&](const Mat &m, int n, int c, int h, int w)->float {
        // assume continuous
        const float* data = (const float*)m.ptr<float>(0);
        int C = m.size[1];
        int H = m.size[2];
        int W = m.size[3];
        size_t idx = ((size_t)n * C + c);
        idx = (idx * H + h);
        idx = (idx * W + w);
        return data[idx];
    };
    int r = 0;
    int c = 0;
//    for (int r = 0; r < 8; ++r)
    {
//        for (int c = 0; c < 8; ++c)
        {
            Rect cell(c*cellW, r*cellH, cellW, cellH);
            Mat roi = g_warp(cell);
            Mat input;
            resize(roi, input, g_inputSize);
            Mat blob = blobFromImage(input, 1.0/255.0, g_inputSize, Scalar(), true, false);
            g_net.setInput(blob);
            Mat out = g_net.forward();

            int classId = -1;
            float confidence = 0.0f;

            // DEBUG: print out dims
            // cout << "out.dims=" << out.dims << " total=" << out.total() << "\n";

            if ((int)out.total() == (int)g_labels.size()) {
                // flat vector of class scores
                Mat vec = out.reshape(1, 1);
                double minv, maxv; Point minLoc, maxLoc;
                minMaxLoc(vec, &minv, &maxv, &minLoc, &maxLoc);
                classId = maxLoc.x;
                confidence = (float)maxv;
            } else if (out.dims == 4) {
                int N = out.size[0];
                int Cn = out.size[1];
                int Hn = out.size[2];
                int Wn = out.size[3];
                if (Cn == (int)g_labels.size() && Hn == 1 && Wn == 1) {
                    // shape [1, C, 1, 1] -> class scores
                    double best = -1e9; int bestc = -1;
                    for (int ch = 0; ch < Cn; ++ch) {
                        float v = get4(out, 0, ch, 0, 0);
                        if (v > best) { best = v; bestc = ch; }
                    }
                    classId = bestc; confidence = (float)best;
                } else if (Cn == 1) {
                    // single-channel segmentation: take mean probability after sigmoid
                    Mat map(Hn, Wn, CV_32F);
                    for (int yy = 0; yy < Hn; ++yy) for (int xx = 0; xx < Wn; ++xx) map.at<float>(yy,xx) = get4(out,0,0,yy,xx);
                    Mat prob = sigmoidMat(map);
                    Scalar avg = mean(prob);
                    // if average probability high -> mark occupied; choose label 'unknown' (index 0 = empty)
                    // find best label: if occupied -> pick first non-empty label index 1
                    if (avg[0] > 0.2f) {
                        // approximate: pick label with highest prior (not available) => mark as unknown; use label 1 if exists
                        if ((int)g_labels.size() > 1) classId = 1; else classId = 0;
                    } else {
                        classId = 0; // empty
                    }
                    confidence = (float)avg[0];
                } else if (Cn > 1) {
                    // multiclass per-pixel segmentation [1,C,H,W] -> compute mode across pixels
                    vector<int> counts(Cn, 0);
                    for (int yy = 0; yy < Hn; ++yy) {
                        for (int xx = 0; xx < Wn; ++xx) {
                            int bestc = 0; float bestv = get4(out,0,0,yy,xx);
                            for (int ch = 1; ch < Cn; ++ch) {
                                float v = get4(out,0,ch,yy,xx);
                                if (v > bestv) { bestv = v; bestc = ch; }
                            }
                            counts[bestc]++;
                        }
                    }
                    int bestc = 0; int bestcnt = counts[0];
                    for (int ch = 1; ch < Cn; ++ch) if (counts[ch] > bestcnt) { bestcnt = counts[ch]; bestc = ch; }
                    classId = bestc;
                    confidence = (float)bestcnt / (float)(Hn * Wn);
                }
            } else if (out.total() == 1) {
                // single scalar probability
                float v = out.at<float>(0);
                classId = (v > 0.5f && g_labels.size()>1) ? 1 : 0;
                confidence = v;
            } else {
                // fallback: try argmax on flattened vector
                Mat vec = out.reshape(1, 1);
                double minv, maxv; Point minLoc, maxLoc;
                minMaxLoc(vec, &minv, &maxv, &minLoc, &maxLoc);
                classId = maxLoc.x % (int)g_labels.size();
                confidence = (float)maxv;
            }

            string label = "?";
            if (classId >= 0 && classId < (int)g_labels.size()) label = g_labels[classId];
            // draw
            rectangle(vis, cell, Scalar(0,255,0), 1);
            string text = label;
//            text += " " + to_string((int)(confidence*100)) + "%";
            int baseline=0;
            Size tsize = getTextSize(text, FONT_HERSHEY_SIMPLEX, 0.4, 1, &baseline);
            int tx = cell.x + (cell.width - tsize.width)/2;
            int ty = cell.y + (cell.height + tsize.height)/2;
            putText(vis, text, Point(tx, ty), FONT_HERSHEY_SIMPLEX, 1, Scalar(0,0,255), 1);
        }
    }
    cvtColor(vis, vis, COLOR_BGR2RGB);
    imshow("Warped DNN", vis);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " <image> <model.onnx> [labels.txt] [input_w] [input_h]" << endl;
        return 0;
    }
    string imgPath = argv[1];
    string modelPath = argv[2];
    string labelsPath = (argc >= 4) ? argv[3] : string();
    if (argc >= 5) g_inputSize.width = atoi(argv[4]);
    if (argc >= 6) g_inputSize.height = atoi(argv[5]);

    g_img = imread(imgPath);
    if (g_img.empty()) { cerr << "Failed to load image" << endl; return -1; }

    // load network
    try {
        g_net = readNet(modelPath);
    } catch (const std::exception &ex) {
        cerr << "Failed to load model: " << ex.what() << endl; return -1;
    }
    g_net.setPreferableBackend(DNN_BACKEND_DEFAULT);
    g_net.setPreferableTarget(DNN_TARGET_CPU);

    if (!labelsPath.empty()) loadLabels(labelsPath, g_labels);

    namedWindow("Input", WINDOW_NORMAL);
    imshow("Input", g_img);
    setMouseCallback("Input", onMouse, nullptr);

    cout << "Click 4 corners (clockwise) to warp board. Right-click to reset corners." << endl;

    while (true) {
        int k = waitKey(10);
        if (k == 27) break;
        if (g_warpReady) {
            runRecognitionAndShow();
            // small delay to avoid busy loop
            waitKey(0);
        }
    }
    return 0;
}
