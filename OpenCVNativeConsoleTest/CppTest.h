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
double CannyThreshold = 90;   // ����5����Ե�����ֵ��30~180��
double CannyThreshold2 = 90 * 0.6;
double CircleAccumulatorThreshold = 30;       // ����6���ۼ�����ֵ��Բ���غϵ㣬Խ�͵�ʱ��Բ����Խ���׵���Բ��
double CircleCannyThresh = 90;    // Բ�ı�Ե�����ֵ��30~180��
double CrossFillRate = 0.2; //ʮ�ּ��


//��������
EX void Detect(unsigned char*, int, int, int, int, int[]);

EX void SetConfig(double, double, double, double, double, double);

//�ڲ�����
Mat InitImage(Mat);
vector<Vec3f> DetectCircle(Mat uimage, int boardSize);
map<CrossType, list<Point>> DetectCross(char imageBytes[], int width);
void LineFit(vector<Point2f> points, Point2f *direction, Point2f *pointOnLine);
Point2f FindLineCross(Point2f direction1, Point2f pointOnLine1, Point2f direction2, Point2f pointOnLine2);

//�ṹ��
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

//ö��
typedef enum CrossType
{
	None = 0, Left, Up, Right, Down, LeftUp, RightUp, RightDown, LeftDown, Center
};

