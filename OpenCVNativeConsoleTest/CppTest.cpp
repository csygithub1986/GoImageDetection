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
/// ͼ��Ԥ����
/// </summary>
Mat InitImage(Mat mat)
{
	//תΪ�Ҷȼ�ͼ��
	Mat	grayMat;
	cvtColor(mat, grayMat, COLOR_RGB2GRAY);
	//use image pyr to remove noise ���룬Ϊ�˸�׼ȷ������Ե���
	Mat pyrDownMat;
	pyrDown(grayMat, pyrDownMat);
	pyrUp(pyrDownMat, grayMat);
	return grayMat;
}

void  SetConfig(double minWidthRate, double cannyThreshold, double circleAccumulatorThreshold, double circleCannyThresh, double crossFillRate)
{
	MinWidthRate = minWidthRate;
	CannyThreshold = cannyThreshold;   // ����5����Ե�����ֵ��30~180��
	CircleAccumulatorThreshold = circleAccumulatorThreshold;       // ����6���ۼ�����ֵ��Բ���غϵ㣬Խ�͵�ʱ��Բ����Խ���׵���Բ��
	CircleCannyThresh = circleCannyThresh;    // Բ�ı�Ե�����ֵ��30~180��
	CrossFillRate = crossFillRate; //ʮ�ּ��
}
