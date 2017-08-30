#pragma once
#include <iostream>  
#include<map>
#include <math.h>
#include <opencv2/opencv.hpp> 
#include <opencv2/highgui/highgui.hpp>  
#include<cv.h>
#include<cxcore.h>
#include<highgui.h>

using namespace cv;
using namespace std;
//using cv::Mat;
//using cv::Vec3f;
//using cv::Point2f;
//using cv::Point;

#define EX extern "C" __declspec(dllexport)
#define GetArrayLen(x) (sizeof((x)) / sizeof(x)[0])

template <class T>
int getArrayLen(T& array)
{
	return (sizeof(array) / sizeof(array[0]));
}


double MinWidthRate = 0.7;
double CannyThreshold = 90;   // 参数5：边缘检测阈值（30~180）
double CannyThreshold2 = 90 * 0.6;
double CircleAccumulatorThreshold = 30;       // 参数6：累加器阈值（圆心重合点，越低的时候圆弧就越容易当成圆）
double CircleCannyThresh = 90;    // 圆的边缘检测阈值（30~180）
double CrossFillRate = 0.2; //十字检测


//导出函数
EX void Detect(unsigned char*, int, int, int, int, int[]);

EX void SetConfig(double, double, double, double, double, double);

//内部函数
Mat InitImage(Mat);
vector<Vec3f> DetectCircle(Mat uimage, int boardSize);
map<CrossType, list<Point>> DetectCross(char imageBytes[], int width);
void LineFit(vector<Point2f> points, Point2f *direction, Point2f *pointOnLine);
Point2f FindLineCross(Point2f direction1, Point2f pointOnLine1, Point2f direction2, Point2f pointOnLine2);

//结构体
typedef struct CircleF
{
	Point2f Center;
	float Radius;
};

typedef struct LineSegment2DF
{
	Point2f P1;
	Point2f P2;
	double Length;
	Point2f Direction;

	LineSegment2DF() {	}
	LineSegment2DF(Point2f p1, Point2f p2)
	{
		P1 = p1;
		P2 = p2;
	}
};

//枚举
typedef enum CrossType
{
	None = 0, Left, Up, Right, Down, LeftUp, RightUp, RightDown, LeftDown, Center
};

