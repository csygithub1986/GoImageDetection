#include "CppTest.h"
#include <iostream>  
#include <opencv2/opencv.hpp> 
#include <opencv2/highgui/highgui.hpp>  
#include<cv.h>
#include<cxcore.h>
#include<highgui.h>

using namespace cv;


int* Detect(unsigned char* src, int w, int h, int channel)
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
	//int lineByte = (w + 3) / 4 * 4;
	Mat img(h, w, format, src);
	int thickness = -1;
	int lineType = 8;

	//��һ��Բ
	/*circle(img,
		Point(200, 200),
		w / 16,
		Scalar(0, 0, 255),
		thickness,
		lineType);*/

	img.data = src;
	namedWindow("test1", CV_WINDOW_FREERATIO);
	resizeWindow("test1", 800, 600);
	imshow("test1", img);
	int a[] = { 123 };
	return a;
}



//int *EX Detect(IplImage a)
//{
//	Mat mat = Mat()
//
//		this.boardSize = boardSize;
//	UMat uimage = InitImage(bitmap);
//	//return null;
//	imageWidth = uimage.Cols;
//	imageHeight = uimage.Rows;
//	maxGridWidth = uimage.Size.Width / (boardSize - 1);
//	minGridWidth = (int)(uimage.Size.Width / (boardSize - 1) * minWidthRate);
//	crossDetectLen = minGridWidth / 4;
//	//crossDetectWidth = crossDetectWidth = crossDetectLen / 4;
//
//	Circles = DetectCircle(uimage, boardSize);
//
//	//List<LineSegment2D> horizontalLines;
//	//List<LineSegment2D> verticalLines;
//	//DetectLine(uimage, boardSize, out horizontalLines, out verticalLines);
//	//CrossPoints = CalculateCross(horizontalLines, verticalLines);
//
//	cannyEdges = new UMat();
//	//�������ĸ������ֱ�Ϊ��Ե�����ֵ��������ֵ�����ڵ�һ����Ϊ�߽磬С�ڵڶ�������������֮��ʱ���õ��Ƿ������������߽�㣩
//	CvInvoke.Canny(uimage, cannyEdges, cannyThreshold, cannyThreshold * 0.6);
//	DateTime t1 = DateTime.Now;
//	CrossPoints = DetectCross(cannyEdges.Bytes, cannyEdges.Cols);
//	DateTime t2 = DateTime.Now;
//	Console.WriteLine("DetectCross " + (t2 - t1).TotalMilliseconds + " ms");
//
//	//foreach (var item in CrossPoints)
//	//{
//	//    Console.WriteLine(item.Key + "  " + item.Value.Count());
//	//}
//
//
//	//�ҽ���
//	PointF directionLeft = PointF.Empty;
//	PointF directionRight = PointF.Empty;
//	PointF directionUp = PointF.Empty;
//	PointF directionDown = PointF.Empty;
//	conors = FindConor(out directionLeft, out directionRight, out directionUp, out directionDown);
//	DateTime t3 = DateTime.Now;
//	Console.WriteLine("FindConor " + (t3 - t2).TotalMilliseconds + " ms");
//	if (conors == null)
//	{
//		return null;
//	}
//
//	LineSegment2DF[] horizontalLines = null;
//	LineSegment2DF[] verticalLines = null;
//	GetEvenDevideLines(conors, directionLeft, directionRight, directionUp, directionDown, out horizontalLines, out verticalLines);
//	allCoordinate = GetGridCoordinate(horizontalLines, verticalLines);
//	DateTime t4 = DateTime.Now;
//	Console.WriteLine("GetGridCoordinate " + (t4 - t3).TotalMilliseconds + " ms");
//
//	int[] stones = new int[boardSize * boardSize];
//	byte[] imageByte = cannyEdges.Bytes;//���ֱ���������cannyEdges.Bytes�е�Ԫ�أ���ǳ���ʱ
//	byte[] grayImageData = grayImage.Bytes;
//	for (int i = 0; i < stones.Length; i++)
//	{
//		stones[i] = FindStone(i, imageByte, grayImageData);
//	}
//	DateTime t5 = DateTime.Now;
//	Console.WriteLine("FindStone " + (t5 - t4).TotalMilliseconds + " ms");
//	return stones;
//}


