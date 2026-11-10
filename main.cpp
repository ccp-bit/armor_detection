#include "armor_detection.h"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/geometry.hpp>
#include <algorithm>
#include <iostream>
#include <string>
#include <windows.h>

int main(int argc, char**argv){
    SetConsoleOutputCP(CP_UTF8);
    if (argc < 2){
        std::cerr << "\n【错误】你没有输入图片路径！" << "\n";
        std::cout << "正确用法示例: " << argv[0] << " images/outpost_frame.png" << "\n";
        return -1;
    }
    cv::Mat image = cv::imread(argv[1], cv::IMREAD_COLOR);                                     // 读取图片
    if (image.empty()){
        std::cerr << "\n【错误】 图片读取失败！请检查路径和文件名。" << "\n";
        return -1;
    }
    std::cout << "\n图片读取成功！" << "\n";
    std::cout << "图片大小为: " << image.size() << "\n";
    std::cout << "图片通道数为: " << image.channels() << "\n\n";

    cv::Mat debugMask;
    std::vector<LightBar> debugBars;
    std::vector<Armor> armors = detectArmor(image, debugMask, debugBars);                      // 输入单帧图，输出图中装甲板数据
    cv::imwrite("output/mask.png", debugMask);
    cv::Mat light_candidates = image.clone();
    for (const auto& bar: debugBars){                                                          // 画灯条候选图
        cv::Point2f corners[4];
        bar.rect.points(corners);
        for (int k = 0; k < 4; k++){
            cv::line(light_candidates, corners[k], corners[(k+1)%4], cv::Scalar(0,255,0), 1);
        }
        cv::putText(light_candidates, std::to_string(bar.id), bar.rect.center, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,255,255), 1);
    }
    cv::imwrite("output/light_candidates.png", light_candidates);                              

    cv::Mat targetImage = image.clone();
    for (const auto& armor: armors){                                                           // 根据装甲板数据画出装甲板外框
        for (int k = 0; k < 4; k++){
            cv::line(targetImage, armor.corners[k], armor.corners[(k+1)%4], cv::Scalar(0,0,255), 1);
            cv::putText(targetImage, std::to_string(k), armor.corners[k], cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255,150,0), 1);
        }
        cv::circle(targetImage, armor.center, 4, cv::Scalar(0,0,255), -1);
        std::cout << "--------------装甲板" << armors.size() << "----------------\n";          // 打印装甲板相关信息
        std::cout << "外接框信息如下:\n";
        std::cout << "中心点坐标为: " << armor.center << "\n";
        std::cout << "四个顶点坐标为:\n";
        std::cout << "  左上角: " << armor.corners[0] << "\n";
        std::cout << "  右上角: " << armor.corners[1] << "\n";
        std::cout << "  左下角: " << armor.corners[3] << "\n";
        std::cout << "  右下角: " << armor.corners[2] << "\n";
        std::cout << "-------------------------------------\n";
    } 
    cv::imwrite("output/target.png", targetImage);
    cv::namedWindow("Armor Detection", cv::WINDOW_NORMAL);
    cv::imshow("Armor Detection", targetImage);
    cv::waitKey(0);
    cv::destroyAllWindows;
    return 0;
}