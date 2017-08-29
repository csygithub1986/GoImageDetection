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


int* Detect(unsigned char* src, int w, int h, int channel,int boardSize)
{
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

	BoardSize = boardSize;

	//Mat uimage = InitImage(bitmap);
	//return null;
	ImageWidth = w;
	ImageHeight = h;
	maxGridWidth = uimage.Size.Width / (boardSize - 1);
	minGridWidth = (int)(uimage.Size.Width / (boardSize - 1) * minWidthRate);
	crossDetectLen = minGridWidth / 4;
	//crossDetectWidth = crossDetectWidth = crossDetectLen / 4;

	Circles = DetectCircle(uimage, boardSize);








	int a[] = { 123 };
	return a;
}

void  SetConfig(double minWidthRate, double cannyThreshold, double circleAccumulatorThreshold, double circleCannyThresh, double crossFillRate)
{
	MinWidthRate = minWidthRate;
	CannyThreshold = cannyThreshold;   // ����5����Ե�����ֵ��30~180��
	CircleAccumulatorThreshold = circleAccumulatorThreshold;       // ����6���ۼ�����ֵ��Բ���غϵ㣬Խ�͵�ʱ��Բ����Խ���׵���Բ��
	CircleCannyThresh = circleCannyThresh;    // Բ�ı�Ե�����ֵ��30~180��
	CrossFillRate = crossFillRate; //ʮ�ּ��
}
