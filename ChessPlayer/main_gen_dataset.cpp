//#include <iostream>
//#include <vector>
//#include <string>
//#include <sstream>
//#include <fstream>
//#include <iostream>
//#include <string>
//// Platform conditional inclusions
//#ifdef _WIN32
//    #include <windows.h>
//    #include <direct.h> // For _mkdir and _access
//    #include <io.h>     // For _access flags
//#else
//    #include <dirent.h>
//    #include <sys/types.h>
//    #include <sys/stat.h>
//    #include <unistd.h>
//    #define _access access
//    #define _mkdir(path) mkdir(path, 0777)
//#endif

///* command
//➜  build git:(add_robot_init_sequence_from_app) ✗ ./ChessImageProcessing 4 4 16
//create folder: gen-Pawn
//➜  build git:(add_robot_init_sequence_from_app) ✗ ./ChessImageProcessing 0 4 4
//create folder: gen-Bishop
//➜  build git:(add_robot_init_sequence_from_app) ✗ ./ChessImageProcessing 2 4 4
//create folder: gen-King
//➜  build git:(add_robot_init_sequence_from_app) ✗ ./ChessImageProcessing 5 4 4
//create folder: gen-Queen
//➜  build git:(add_robot_init_sequence_from_app) ✗ ./ChessImageProcessing 3 4 2
//create folder: gen-Knight
//➜  build git:(add_robot_init_sequence_from_app) ✗ ./ChessImageProcessing 6 4 1
//create folder: gen-Rook
//*/
//#include <opencv2/opencv.hpp>
//const int WARP_SIZE = 1920;
//// Define piece types matching your folders
//enum PieceType { BISHOP, EMPTY, KING, KNIGHT, PAWN, QUEEN, ROOK };

//std::string getFolderName(PieceType piece, bool genFolder = false) {
//    switch (piece) {
//    case BISHOP: return genFolder?"gen-Bishop":"data-Bishop-extra";
//    case EMPTY:  return genFolder?"gen-Empty":"data-Empty-extra";
//    case KING:   return genFolder?"gen-King":"data-King-extra";
//    case KNIGHT: return genFolder?"gen-Knight":"data-Knight-extra";
//    case PAWN:   return genFolder?"gen-Pawn":"data-Pawn-extra";
//    case QUEEN:  return genFolder?"gen-Queen":"data-Queen-extra";
//    case ROOK:   return genFolder?"gen-Rook":"data-Rook-extra";
//    }
//    return "data-Empty";
//}

//// C++11 fallback to check file existence and determine sequential index
//int getNextFileIndex(const std::string& folderPath) {
//    int index = 0;
//    while (true) {
//        std::stringstream ss;
//        ss << folderPath << "/" << index << ".jpg";
//        std::ifstream file(ss.str());

//        // If the file doesn't open, the slot is available
//        if (!file.good()) {
//            return index;
//        }
//        index++;
//    }
//}

//// Safely creates single or nested directories
//bool createFolderTree(const std::string& path) {
//    std::string currentPath = "";

//    for (size_t i = 0; i < path.length(); ++i) {
//        currentPath += path[i];

//        // When we reach a slash or the end of the string, check/create that layer
//        if (path[i] == '/' || path[i] == '\\' || i == path.length() - 1) {
//            if (currentPath.empty() || currentPath == "/" || currentPath == "\\") {
//                continue;
//            }

//            // Check if this sub-path exists (0 means existence check)
//            if (_access(currentPath.c_str(), 0) != 0) {
//                if (_mkdir(currentPath.c_str()) != 0) {
//                    return false; // Creation failed
//                }
//            }
//        }
//    }
//    return true;
//}

//std::vector<std::string> getFileList(const std::string& folderPath) {
//    std::vector<std::string> fileList;

//#ifdef _WIN32
//    // --- Windows Implementation ---
//    std::string searchPath = folderPath + "\\*.*";
//    WIN32_FIND_DATAA fileData;
//    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &fileData);

//    if (hFind != INVALID_HANDLE_VALUE) {
//        do {
//            std::string filename = fileData.cFileName;
//            // Skip directory shortcuts and subdirectories
//            if (filename != "." && filename != ".." && !(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
//                fileList.push_back(folderPath + "\\" + filename);
//            }
//        } while (FindNextFileA(hFind, &fileData));
//        FindClose(hFind);
//    }
//#else
//    // --- Linux / macOS POSIX Implementation ---
//    DIR* dir = opendir(folderPath.c_str());
//    if (dir != nullptr) {
//        struct dirent* entry;
//        while ((entry = readdir(dir)) != nullptr) {
//            std::string filename = entry->d_name;
//            // Skip directory shortcuts
//            if (filename != "." && filename != "..") {
//                // Check if it's a regular file (not a folder)
//                if (entry->d_type == DT_REG) {
//                    fileList.push_back(folderPath + "/" + filename);
//                }
//            }
//        }
//        closedir(dir);
//    }
//#endif

//    return fileList;
//}

//int parseIndexFromFullPath(const std::string& fullPath) {
//    // 1. Find the last path separator (checking both Windows '\\' and Linux '/')
//    size_t lastSlash = fullPath.find_last_of("\\/");

//    // Isolate the filename from the path
//    std::string filename = (lastSlash == std::string::npos) ? fullPath : fullPath.substr(lastSlash + 1);

//    // 2. Find the positions of the hyphen and the dot within the filename
//    size_t hyphenPos = filename.find('-');
//    size_t dotPos = filename.find_last_of('.');

//    // 3. Extract and convert if structural criteria are met
//    if (hyphenPos != std::string::npos && dotPos != std::string::npos && dotPos > hyphenPos) {
//        std::string numStr = filename.substr(hyphenPos + 1, dotPos - hyphenPos - 1);

//        // std::stoi converts "0001" directly to the integer 1
//        return std::stoi(numStr);
//    }

//    return -1; // Return error code if parsing fails
//}

//void generateDataFromSingleImage(PieceType currentPiece, std::string imagePath, int numSample, int numPieces) {
//    // 1. Load your source frame
//    cv::Mat srcImage = cv::imread(imagePath);
//    if (srcImage.empty()) {
//        std::cerr << "Error: Could not load input image." << std::endl;
//        return ;
//    }

//    // 2. Wrap your 4 pre-calculated input coordinates
//    // Order from your program: [Top-Left, Top-Right, Bottom-Right, Bottom-Left]
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
//    cv::warpPerspective(srcImage, warpedBoard, homographyMatrix, cv::Size(WARP_SIZE, WARP_SIZE));
////    cv::imshow("imageWrap",warpedBoard);
////    cv::waitKey(0);
//    int cellSize = WARP_SIZE / 8;
//    int imageIndex = parseIndexFromFullPath(imagePath);
//    if(imageIndex < 0) return;
//    std::vector<cv::Point> listCell;
//    if(numPieces<=8) {
//        int row = (imageIndex - 1) / numSample / (8/numPieces);
//        int step = (((imageIndex - 1) / numSample) % (8/numPieces)) * numPieces;
//        bool isEvenRow = row%2==0;
//        int col = isEvenRow ? (7 - step) : step;
//        for(int i=0; i< numPieces; i++) {
//            int cellCol = isEvenRow?col-i:col+i;
//            int cellRow = row;
//            listCell.push_back(cv::Point(cellCol,cellRow));
//        }
//    } else {
//        int startRow = ((imageIndex - 1) / numSample) % (64/numPieces);
//        for(int i=0;i<numPieces; i++) {
//            int cellRow = startRow * (numPieces/8) + i / 8;
//            int cellCol = i % 8;
//            listCell.push_back(cv::Point(cellCol,cellRow));
//        }
//    }

//    for(cv::Point cell: listCell) {
//#ifdef DEBUG_ROI
//        printf("row[%d] col[%d]\r\n",cell.y,cell.x);
//#endif
//        // Stretch the bounding box upwards to swallow full tall piece outlines
//        int cropX = cell.x * cellSize;
//        int cropY = cell.y * cellSize;
//        int cropW = cellSize;
//        int cropH = cellSize;

//        // Safe image-canvas bound clamping checks
//        if (cropX < 0) cropX = 0;
//        if (cropY < 0) cropY = 0;
//        if (cropX + cropW > warpedBoard.cols) cropW = warpedBoard.cols - cropX;
//        if (cropY + cropH > warpedBoard.rows) cropH = warpedBoard.rows - cropY;

//        cv::Rect tallCellROI(cropX, cropY, cropW, cropH);
//#ifdef DEBUG_ROI
//        cv::rectangle(warpedBoard,tallCellROI,cv::Scalar(0,255,255),2);
//#endif
//        cv::Mat croppedCell = warpedBoard(tallCellROI);

//        // Shape standardizer step for incoming CNN processing structures
//        cv::Mat standardizedInput;
//        cv::resize(croppedCell, standardizedInput, cv::Size(WARP_SIZE/8, WARP_SIZE/8));

//        // Find target folder mapping parameters
//        std::string targetFolder = getFolderName(currentPiece,true);

//        int fileIndex = getNextFileIndex(targetFolder);
//        std::stringstream pathStream;
//        pathStream << targetFolder << "/" << fileIndex << ".jpg";
//        // Write output file
//        cv::imwrite(pathStream.str(), standardizedInput);
//    }
//#ifdef DEBUG_ROI
//    cv::Mat scaledWarped;
//    cv::resize(warpedBoard,scaledWarped,cv::Size(WARP_SIZE/3,WARP_SIZE/3),0,0, cv::INTER_NEAREST);
//    cv::imshow("scaledWarped",scaledWarped);
//    cv::waitKey();
//#endif
//}

//int main(int argc, char** argv) {
//#ifdef DEBUG_ROI
//    int numSample = atoi(argv[2]);
//    int numPieces = atoi(argv[3]);
//    int pieceType = atoi(argv[1]);
//    generateDataFromSingleImage((PieceType)pieceType,argv[4],numSample,numPieces);
//#else
//    int numSample = atoi(argv[2]);
//    int numPieces = atoi(argv[3]);
//    int pieceType = atoi(argv[1]);
//    std::string dataFolder = getFolderName((PieceType)pieceType);
//    std::string genFolder = getFolderName((PieceType)pieceType,true);
//    createFolderTree(genFolder);
//    std::cout << "create folder: " << genFolder << std::endl;
//    std::vector<std::string> listFile = getFileList(dataFolder);
//    for(std::string filePath : listFile) {
//        generateDataFromSingleImage((PieceType)pieceType,filePath,numSample,numPieces);
//    }
//#endif
//    return 0;
//}
