#include "CppTest.h"
#include <iostream>  
#include <opencv2/opencv.hpp> 
#include <opencv2/highgui/highgui.hpp>  
#include<cv.h>
#include<cxcore.h>
#include<highgui.h>

using namespace cv;

int BoardSize;
int ImageWidth;
int ImageHeight;
int MaxGridWidth;
int MinGridWidth;
int CrossDetectLen;

 circleF Circles[];
 Dictionary<CrossType, List<Point>> CrossPoints;

 UMat cannyEdges;
UMat grayImage;


int* Detect(unsigned char* src, int w, int h, int channel, int boardSize)
{
	BoardSize = boardSize;
	ImageWidth = w;
	ImageHeight = h;
	MaxGridWidth = (ImageWidth + ImageHeight) / 2 / (boardSize - 1);
	MinGridWidth = (int)(MaxGridWidth * MinWidthRate);
	CrossDetectLen = MinGridWidth / 4;

	int format;
	switch (channel)
	{
	case 1:
		format = CV_8UC1;
		break;
	case 2:
		format = CV_8UC2;
		break;
	case 3:
		format = CV_8UC3;
		break;
	default:
		format = CV_8UC4;
		break;
	}
	Mat img(h, w, format, src);


	Mat uimage = InitImage(img);
	Circles = DetectCircle(uimage, boardSize);








	int a[] = { 123 };
	return a;
}

/// <summary>
/// 图形预处理
/// </summary>
Mat InitImage(Mat mat)
{
	//转为灰度级图像
	Mat	grayMat;
	cvtColor(mat, grayMat, COLOR_RGB2GRAY);
	//use image pyr to remove noise 降噪，为了更准确的做边缘检测
	Mat pyrDownMat;
	pyrDown(grayMat, pyrDownMat);
	pyrUp(pyrDownMat, grayMat);
	return grayMat;
}

void  SetConfig(double minWidthRate, double cannyThreshold, double circleAccumulatorThreshold, double circleCannyThresh, double crossFillRate)
{
	MinWidthRate = minWidthRate;
	CannyThreshold = cannyThreshold;   // 参数5：边缘检测阈值（30~180）
	CircleAccumulatorThreshold = circleAccumulatorThreshold;       // 参数6：累加器阈值（圆心重合点，越低的时候圆弧就越容易当成圆）
	CircleCannyThresh = circleCannyThresh;    // 圆的边缘检测阈值（30~180）
	CrossFillRate = crossFillRate; //十字检测
}
