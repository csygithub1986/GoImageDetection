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


//结构体
struct CircleF
{
	Point2f Center;
	float Radius;
	CircleF() {}
	CircleF(Point2f center, float radius)
	{
		Center = center;
		Radius = radius;
	}
};

struct LineSegment2DF
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
enum CrossType
{
	None = 0, Left, Up, Right, Down, LeftUp, RightUp, RightDown, LeftDown, Center
};


//导出函数
EX void Detect(unsigned char*, int, int, int, int, int[]);

EX void SetConfig(double, double, double, double, double, double);

//内部函数
Mat InitImage(Mat);
vector<CircleF> DetectCircle(Mat uimage, int boardSize);
map<CrossType, list<Point>> DetectCross(uchar *imageBytes, int width, int height);
void LineFit(vector<Point2f> points, Point2f *direction, Point2f *pointOnLine);
Point2f FindLineCross(Point2f direction1, Point2f pointOnLine1, Point2f direction2, Point2f pointOnLine2);
Point *GetGridCoordinate(LineSegment2DF *horizontalLines, LineSegment2DF *verticalLines);
void GetEvenDevideLines(Point2f *conors, Point2f directionLeft, Point2f directionRight, Point2f directionUp, Point2f directionDown, LineSegment2DF *horizontalLines, LineSegment2DF *verticalLines);
Point2f  *FindConor(Point2f *directionLeft, Point2f *directionRight, Point2f *directionUp, Point2f *directionDown);
int FindStone(int index, uchar *cannyBytes, uchar *grayImageData);



