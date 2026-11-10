#include "armor_detection.h"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/geometry.hpp>
#include <algorithm>
#include <iostream>
#include <string>

// 灯条提取函数
bool BarDetection(const std::vector<cv::Point>& BarContour){
    double area = cv::contourArea(BarContour);
    if (area < 25.0)  {return false;}                           // 筛掉细小光点
    cv::RotatedRect rectangle = cv::minAreaRect(BarContour);
    float width = rectangle.size.width;
    float height = rectangle.size.height;
    float maxLen = std::max(width, height);
    float minLen = std::min(width, height);
    if (minLen < 1.0f)  {return false;}
    float ratio = maxLen / minLen;
    if (ratio < 2.0f || ratio > 10.0f)  {return false;}         // 筛选掉长宽比异常的灯条
    else {return true;}
}

// 灯条配对函数: 筛选出装甲板两侧的灯条
bool ArmorDetection(const LightBar& bar1, const LightBar& bar2){
    cv::Point2f c1 = bar1.rect.center;                          // 定义两灯条的中心点
    cv::Point2f c2 = bar2.rect.center;
    float len1 = std::max(bar1.rect.size.width, bar1.rect.size.height);
    float len2 = std::max(bar2.rect.size.width, bar2.rect.size.height);
    float aveLen = (len1 + len2) / 2.0f;
    float LenRatio = len1 / len2;
    if (LenRatio < 0.8f || LenRatio > 1.25f) {return false;}    // 如果两灯条长度差距太大会被筛掉
    if (std::abs(c1.y - c2.y) > aveLen * 0.5f) {return false;}  // 如果两灯条高度差太大会被筛掉
    float disRatio = std::abs(c1.x - c2.x) / aveLen;
    if (disRatio < 1.2f || disRatio > 4.0f) {return false;}     // 两灯条间距与两灯条的平均长度的比值符合要求则会被识别为装甲板
    else {return true;}
}

// 四点排序函数: 
void SortPoints(cv::Point2f* corners){                          
    cv::Point2f sorted[4];                                      // 定义中间量数组，用于接收点的顺序
    float minSum = 5000.0f, maxSum = -5000.0f;
    float minDif = 5000.0f, maxDif = -5000.0f;
    int n1 = 0, n2 = 0, n3 = 0, n4 = 0;
    for (int i = 0; i < 4; i++){
        float sum = corners[i].x + corners[i].y;
        float dif = corners[i].x - corners[i].y;
        if (sum < minSum) {minSum = sum; n1 = i;}
        if (dif > maxDif) {maxDif = dif; n2 = i;}
        if (sum > maxSum) {maxSum = sum; n3 = i;}
        if (dif < minDif) {minDif = dif; n4 = i;}
    }
    sorted[0] = corners[n1];                                     // min(x+y)对应左上点corners[0]
    sorted[1] = corners[n2];                                     // max(x-y)对应右上点corners[1]
    sorted[2] = corners[n3];                                     // max(x+y)对应右下点corners[2]
    sorted[3] = corners[n4];                                     // min(x-y)对应左下点corners[3]
    for (int i = 0; i < 4; i++) {corners[i] = sorted[i];}
}

// 核心功能: 单帧图片处理函数，实现输入一帧图像即识别图片中的装甲板并输出装甲板数据
std::vector<Armor> detectArmor(const cv::Mat& image, cv::Mat& debugMask, std::vector<LightBar>& debugBars){
    std::vector<Armor> armors;                                   // 图中可能会有多个装甲板
    std::vector<LightBar> bars;                                  // 定义能储存多组灯条数据的数组
    
    cv::Mat hsvImage, mask;                                      
    cv::cvtColor(image, hsvImage, cv::COLOR_BGR2HSV);            // 转HSV图
    cv::Scalar lowerOrange(10, 20, 175);                         
    cv::Scalar upperOrange(35, 255, 255);
    cv::inRange(hsvImage, lowerOrange, upperOrange, mask);       // 图像颜色过滤
    cv::Mat remask = mask.clone();
    cv::Mat closeKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5,5));
    cv::morphologyEx(remask, remask, cv::MORPH_CLOSE, closeKernel);                  // 形态学操作
    std::vector<std::vector<cv::Point>> contours;                
    cv::findContours(remask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);  // 提取灯条轮廓
    debugMask = remask.clone();                                                      // 将灯条初步筛选图像输出
    
    int Id = 0;
    for (const auto& contour: contours){
        cv::RotatedRect rect = cv::minAreaRect(contour);
        if (BarDetection(contour) == false) {continue;}                              // 筛选灯条
        cv::Point2f LightCorners[4];
        rect.points(LightCorners);
        LightBar bar;
        bar.rect = rect;
        bar.id = Id++;                                                               // 给四点标号
        bars.push_back(bar);                                                         // 将灯条信息存入数组
    }
    debugBars = bars;                                                                // 将所有灯条的信息输出

    for (size_t i = 0; i < bars.size(); i++){                    
        for (size_t j = i+1; j < bars.size(); j++){                                  // 让灯条两两配对
            if (ArmorDetection(bars[i], bars[j]) == false) {continue;}               // 筛选出配对成功的灯条进行后续操作
            std::vector<cv::Point2f> allPoints;                                      // 将两个灯条的八个顶点存入数组
            cv::Point2f corners_1[4], corners_2[4];
            bars[i].rect.points(corners_1);
            bars[j].rect.points(corners_2);
            for (int k = 0; k < 4; k++){
                allPoints.push_back(corners_1[k]);
                allPoints.push_back(corners_2[k]);
            }
            cv::RotatedRect armorRect = cv::minAreaRect(allPoints);                  // 用最小矩形框框住八个顶点，即为框住装甲板
            cv::Point2f armorCorners[4];
            armorRect.points(armorCorners);
            SortPoints(armorCorners);                                                // 对装甲板外框的四个顶点进行排序
            Armor armor;                                                             // 对装甲板数组进行实例化对象
            armor.center = armorRect.center;
            armor.boundingBox = cv::boundingRect(allPoints);
            armor.isValid = true;
            for (int k = 0; k < 4; k++)  {armor.corners.push_back(armorCorners[k]);} // 将每个装甲板的数据存入实例化对象
            armors.push_back(armor);                                                 // 将每个装甲板数据存入数组
            break;                                                                   // 防止已经配对好的灯条与第三个灯条配对
        }
    }
    return armors;                                               // 输出所有装甲板的相关数据
}