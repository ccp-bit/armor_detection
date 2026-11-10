#ifndef ARMOR_DETECTION_H 
#define ARMOR_DETECTION_H

#include <opencv2/opencv.hpp>
#include <vector>

struct LightBar {
    cv::RotatedRect rect;
    int id;
};

struct Armor{
    cv::Rect boundingBox;
    cv::Point2f center;
    std::vector<cv::Point2f> corners;
    bool isValid;
};

bool BarDetection(const std::vector<cv::Point>& BarContour);
bool ArmorDetection(const LightBar& bar1, const LightBar& bar2);
void SortPoints(cv::Point2f* corners);
std::vector<Armor> detectArmor(const cv::Mat& image, cv::Mat& debugMask, std::vector<LightBar>& debugBars);

#endif