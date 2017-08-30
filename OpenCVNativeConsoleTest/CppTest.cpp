#include "CppTest.h"

typedef map<CrossType, list<Point>> TypePointMap;
//using namespace cv;

int BoardSize;
int ImageWidth;
int ImageHeight;
int MaxGridWidth;
int MinGridWidth;
int CrossDetectLen;

//中间变量
vector<Vec3f> Circles;
map<CrossType, list<Point>> CrossPoints;
Point2f *Conors;

Mat cannyEdges;
Mat grayImage;


void Detect(unsigned char* src, int w, int h, int channel, int BoardSize, int result[])
{
	BoardSize = BoardSize;
	ImageWidth = w;
	ImageHeight = h;
	MaxGridWidth = (ImageWidth + ImageHeight) / 2 / (BoardSize - 1);
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

	//初始化，灰度、降噪
	Mat grayBlurImage = InitImage(img);

	//找圆
	Circles = DetectCircle(grayBlurImage, BoardSize);
	int size = Circles.size();

	//1、找交点
	Mat	cannyEdges;
	//第三、四个参数分别为边缘检测阈值和连接阈值（大于第一个作为边界，小于第二个舍弃，介于之间时看该点是否连接着其他边界点）
	cv::Canny(grayBlurImage, cannyEdges, CannyThreshold, CannyThreshold2);
	CrossPoints = DetectCross(cannyEdges.data, w, h);

	//2、找角
	Point2f *directionLeft;
	Point2f *directionRight;
	Point2f *directionUp;
	Point2f *directionDown;
	Conors = FindConor(directionLeft, directionRight, directionUp, directionDown);

	//3、透视修正，计算网格
	LineSegment2DF *horizontalLines;
	LineSegment2DF * verticalLines;
	GetEvenDevideLines(conors, directionLeft, directionRight, directionUp, directionDown, out horizontalLines, out verticalLines);
	allCoordinate = GetGridCoordinate(horizontalLines, verticalLines);

	//for (size_t i = 0; i < Circles.size(); i++)
	//{
	//	Point center(cvRound(Circles[i][0]), cvRound(Circles[i][1]));
	//	int radius = cvRound(Circles[i][2]);
	//	//绘制圆心  
	//	circle(img, center, 3, Scalar(0, 255, 0), -1, 8, 0);
	//	//绘制圆轮廓  
	//	circle(img, center, radius, Scalar(155, 50, 255), 3, 8, 0);
	//	//Scalar(55,100,195)参数中G、B、R颜色值的数值，得到想要的颜色  
	//}
	result[0] = size;
}


/// <summary>
/// 图形预处理，灰度和降噪
/// </summary>
Mat InitImage(Mat mat)
{
	//转为灰度级图像
	Mat	grayMat;
	cvtColor(mat, grayMat, COLOR_RGB2GRAY);
	//use image pyr to remove noise 降噪，为了更准确的做边缘检测
	Mat pyrDownMat;
	pyrDown(grayMat, pyrDownMat);//不知道和GaussianBlur有什么区别
	pyrUp(pyrDownMat, grayMat);
	return grayMat;
}

void  SetConfig(double minWidthRate, double cannyThreshold, double cannyThreshold2, double circleAccumulatorThreshold, double circleCannyThresh, double crossFillRate)
{
	MinWidthRate = minWidthRate;
	CannyThreshold = cannyThreshold;   // 参数5：边缘检测阈值（30~180）
	CannyThreshold2 = cannyThreshold2;   // 
	CircleAccumulatorThreshold = circleAccumulatorThreshold;       // 参数6：累加器阈值（圆心重合点，越低的时候圆弧就越容易当成圆）
	CircleCannyThresh = circleCannyThresh;    // 圆的边缘检测阈值（30~180）
	CrossFillRate = crossFillRate; //十字检测
}

/// <summary>
/// 检测圆
/// </summary>
vector<Vec3f> DetectCircle(Mat uimage, int BoardSize)
{
	//棋子最大半径
	int maxRadius = MaxGridWidth / 2;//棋子最大宽度为格的二分之一									 //棋子最小半径为最大半径*0.7
	int minRadius = (int)(maxRadius * MinWidthRate);
	//最小间距为最小直径
	int minDistance = minRadius * 2;
	vector<Vec3f> circles;//保存矢量
	int dp = 1;//不太懂这个参数
	HoughCircles(uimage, circles, CV_HOUGH_GRADIENT, dp, minDistance, CircleCannyThresh, CircleAccumulatorThreshold, minRadius, maxRadius);
	return circles;
}

#pragma region CrossDetection
/// <summary>
/// 对于某点，判断某个方向上是否有直线
/// </summary>
/// <param name="width">图形的宽度</param>
/// <param name="imageBytes">图形数据</param>
/// <param name="x">要计算的点坐标x</param>
/// <param name="y">要计算的点坐标y</param>
/// <param name="directionX">x方向，取-1,0,1</param>
/// <param name="directionY">y方向，取-1,0,1</param>
/// <returns></returns>
bool IsLineOnDirection(int width, uchar imageBytes[], int x, int y, int directionX, int directionY)
{
	//try
	//{
	int iMin = 0, iMax = 0, jMin = 0, jMax = 0;
	//根据要判断的方向，计算坐标
	if (directionX == 0)
	{
		iMin = -2;
		iMax = 2;
		if (directionY == 1)
		{
			jMin = 1;
			jMax = CrossDetectLen;
		}
		else
		{
			jMin = -CrossDetectLen;
			jMax = -1;
		}
	}
	else if (directionY == 0)
	{
		jMin = -2;
		jMax = 2;
		if (directionX == 1)
		{
			iMin = 1;
			iMax = CrossDetectLen;
		}
		else
		{
			iMin = -CrossDetectLen;
			iMax = -1;
		}
	}

	uchar *whiteBytes = new  uchar[5 * CrossDetectLen];
	int index = 0;
	for (int i = iMin; i <= iMax; i++)
	{
		for (int j = jMin; j <= jMax; j++)
		{
			if (x + i + (y + j) * width >= 0 && x + i + (y + j) * width < getArrayLen(imageBytes))
			{
				whiteBytes[index++] = imageBytes[x + i + (y + j) * width];
			}
		}
	}
	int whiteCount = 0;
	for (size_t i = 0; i < 5 * CrossDetectLen; i++)
	{
		if (whiteBytes[i] == 255)
		{
			whiteCount++;
		}
	}

	double	rate = (double)whiteCount / getArrayLen(whiteBytes);
	return whiteCount > getArrayLen(whiteBytes) * CrossFillRate;
	//}
	//catch (Exception ex)
	//{
	//    throw ex;
	//}
}

void AddPoint(map<CrossType, list<Point>> crossDic, CrossType type, int x, int y)
{
	map<CrossType, list<Point>>::iterator iter = crossDic.find(type);
	if (iter != crossDic.end())
	{
		list<Point> pList = iter->second;
		pList.push_back(Point(x, y));
	}
	else
	{
		list<Point> pList;
		pList.push_front(Point(x, y));
		crossDic.insert(map<CrossType, list<Point>>::value_type(type, pList));
	}
}


/// <summary>
/// 从上检查交叉点
/// </summary>
/// <param name="width"></param>
/// <param name="imageBytes"></param>
/// <param name="x"></param>
/// <param name="y"></param>
/// <returns>0表示非交叉点，1,2,3,4分别表示左上右下T字，5,6,7,8分别表示从左上角开始顺时针方向的角。所以在这里可能会有0,2,5,6四种情况</returns>
CrossType GetCrossFromUp(int width, uchar imageBytes[], int x, int y)
{
	bool toDown = IsLineOnDirection(width, imageBytes, x, y, 0, 1);
	if (toDown == false)
	{
		return None;
	}
	bool toUp = IsLineOnDirection(width, imageBytes, x, y, 0, -1);
	if (toUp)
	{
		return None;
	}
	bool toLeft = IsLineOnDirection(width, imageBytes, x, y, -1, 0);
	bool toRight = IsLineOnDirection(width, imageBytes, x, y, 1, 0);
	if (toLeft && toRight)
	{
		return Up;
	}
	if (toLeft && toRight == false)
	{
		return RightUp;
	}
	if (toLeft == false && toRight)
	{
		return LeftUp;
	}
	return None;
}

/// <summary>
/// 从下检查交叉点
/// </summary>
/// <param name="width"></param>
/// <param name="imageBytes"></param>
/// <param name="x"></param>
/// <param name="y"></param>
/// <returns>0表示非交叉点，1,2,3,4分别表示左上右下T字，5,6,7,8分别表示从左上角开始顺时针方向的角。所以在这里可能会有0,4,7,8四种情况</returns>
CrossType GetCrossFromDown(int width, uchar imageBytes[], int x, int y)
{
	bool toUp = IsLineOnDirection(width, imageBytes, x, y, 0, -1);
	if (toUp == false)
	{
		return None;
	}
	bool toDown = IsLineOnDirection(width, imageBytes, x, y, 0, 1);
	if (toDown)
	{
		return None;
	}
	bool toLeft = IsLineOnDirection(width, imageBytes, x, y, -1, 0);
	bool toRight = IsLineOnDirection(width, imageBytes, x, y, 1, 0);
	if (toLeft && toRight)
	{
		return Down;
	}
	if (toLeft && toRight == false)
	{
		return RightDown;
	}
	if (toLeft == false && toRight)
	{
		return LeftDown;
	}
	return None;
}

/// <summary>
/// 从左检查交叉点
/// </summary>
/// <param name="width"></param>
/// <param name="imageBytes"></param>
/// <param name="x"></param>
/// <param name="y"></param>
/// <returns>0表示非交叉点，1,2,3,4分别表示左上右下T字，5,6,7,8分别表示从左上角开始顺时针方向的角。所以在这里可能会有0,1两种情况</returns>
CrossType GetCrossFromLeft(int width, uchar imageBytes[], int x, int y)
{
	bool toRight = IsLineOnDirection(width, imageBytes, x, y, 1, 0);
	if (toRight == false)
	{
		return None;
	}
	bool toLeft = IsLineOnDirection(width, imageBytes, x, y, -1, 0);
	if (toLeft)
	{
		return None;
	}
	bool toUp = IsLineOnDirection(width, imageBytes, x, y, 0, -1);
	if (toUp == false)
	{
		return None;
	}
	bool toDown = IsLineOnDirection(width, imageBytes, x, y, 0, 1);
	if (toDown == false)
	{
		return None;
	}
	return Left;
}

/// <summary>
/// 从右检查交叉点
/// </summary>
/// <param name="width"></param>
/// <param name="imageBytes"></param>
/// <param name="x"></param>
/// <param name="y"></param>
/// <returns>0表示非交叉点，1,2,3,4分别表示左上右下T字，5,6,7,8分别表示从左上角开始顺时针方向的角。所以在这里可能会有0,3两种情况</returns>
CrossType GetCrossFromRight(int width, uchar imageBytes[], int x, int y)
{
	bool toLeft = IsLineOnDirection(width, imageBytes, x, y, -1, 0);
	if (toLeft == false)
	{
		return None;
	}
	bool toRight = IsLineOnDirection(width, imageBytes, x, y, 1, 0);
	if (toRight)
	{
		return None;
	}
	bool toUp = IsLineOnDirection(width, imageBytes, x, y, 0, -1);
	if (toUp == false)
	{
		return None;
	}
	bool toDown = IsLineOnDirection(width, imageBytes, x, y, 0, 1);
	if (toDown == false)
	{
		return None;
	}
	return Right;
}

CrossType GetCrossFromCenter(int width, uchar imageBytes[], int x, int y)
{
	bool toLeft = IsLineOnDirection(width, imageBytes, x, y, -1, 0);
	if (toLeft == false)
	{
		return None;
	}
	bool toRight = IsLineOnDirection(width, imageBytes, x, y, 1, 0);
	if (toRight == false)
	{
		return None;
	}
	bool toUp = IsLineOnDirection(width, imageBytes, x, y, 0, -1);
	if (toUp == false)
	{
		return None;
	}
	bool toDown = IsLineOnDirection(width, imageBytes, x, y, 0, 1);
	if (toDown == false)
	{
		return None;
	}
	return Center;
}



/// <summary>
/// 只检测四边的T型交叉点和四角的L型交叉点
/// 从第一行开始扫描┌ ┐┬形状，从扫到第一个┬开始，再向下扫两个最小格宽。最多扫1/10个图形高度。
/// 从底排扫描└ ┘┴形状，后面同上
/// 从左边扫描├，从右边扫描┤，后面同理
/// </summary>
/// <param name="width"></param>
/// <param name="imageBytes"></param>
/// <param name="x"></param>
/// <param name="y"></param>
/// <returns></returns>
map<CrossType, list<Point>> DetectCross(uchar imageBytes[], int width, int height)
{
	map<CrossType, list<Point>> crossDic;
	int scanRowCount = height / 7;//最多扫描的行数//TODO 消灭立即数
	int scanColCount = width / 7;
	//try
	//{
	//上 //TODO：优化，扫描到了以后就缩小扫描行数，或者扫描到19个了以后就结束，或者加上圆形一共有19个了就结束
	int edge = scanRowCount + CrossDetectLen;
	bool first = true;

	for (int j = CrossDetectLen; j < edge; j++)
	{
		for (int i = CrossDetectLen; i < width - CrossDetectLen; i++)
		{
			if (imageBytes[i + j * width] == 255)
			{
				CrossType type = GetCrossFromUp(width, imageBytes, i, j);
				if (type != None)
				{
					AddPoint(crossDic, type, i, j);
					if (first)
					{
						first = false;
						edge = j + MinGridWidth;
					}
					//并将之后的一片区域设为0，避免重复检查
					for (int ii = 0; ii < CrossDetectLen; ii++)
					{
						for (int jj = 0; jj < CrossDetectLen; jj++)
						{
							imageBytes[i + ii + (j + jj) * width] = 0;
						}
					}
				}
			}
		}
	}
	//下
	edge = height - scanRowCount - CrossDetectLen;
	first = true;
	for (int j = height - 1 - CrossDetectLen; j > edge; j--)
	{
		for (int i = CrossDetectLen; i < width - CrossDetectLen; i++)//用CrossDetectLen作为两边缓冲区
		{
			if (imageBytes[i + j * width] == 255)
			{
				CrossType type = GetCrossFromDown(width, imageBytes, i, j);
				if (type != None)
				{
					AddPoint(crossDic, type, i, j);
					if (first)
					{
						first = false;
						edge = j - MinGridWidth;
					}
					//并将之后的一片区域设为0，避免重复检查
					for (int ii = 0; ii < CrossDetectLen; ii++)
					{
						for (int jj = 0; jj < CrossDetectLen; jj++)
						{
							imageBytes[i + ii + (j + jj) * width] = 0;
						}
					}
				}
			}
		}
	}
	//左
	edge = scanColCount + CrossDetectLen;
	first = true;
	for (int i = CrossDetectLen; i < edge; i++)
	{
		for (int j = CrossDetectLen; j < height - CrossDetectLen; j++)
		{
			if (imageBytes[i + j * width] == 255)
			{
				CrossType type = GetCrossFromLeft(width, imageBytes, i, j);
				if (type != None)
				{
					AddPoint(crossDic, type, i, j);
					if (first)
					{
						first = false;
						edge = i + MinGridWidth;
					}
					//并将之后的一片区域设为0，避免重复检查
					for (int ii = 0; ii < CrossDetectLen; ii++)
					{
						for (int jj = 0; jj < CrossDetectLen; jj++)
						{
							imageBytes[i + ii + (j + jj) * width] = 0;
						}
					}
				}
			}
		}
	}
	//右
	edge = width - scanColCount - CrossDetectLen;
	first = true;
	for (int i = width - 1 - CrossDetectLen; i > edge; i--)
	{
		for (int j = CrossDetectLen; j < height - CrossDetectLen; j++)
		{
			if (imageBytes[i + j * width] == 255)
			{
				CrossType type = GetCrossFromRight(width, imageBytes, i, j);
				if (type != None)
				{
					AddPoint(crossDic, type, i, j);
					if (first)
					{
						first = false;
						edge = i - MinGridWidth;
					}
					//并将之后的一片区域设为0，避免重复检查
					for (int ii = 0; ii < CrossDetectLen; ii++)
					{
						for (int jj = 0; jj < CrossDetectLen; jj++)
						{
							imageBytes[i + ii + (j + jj) * width] = 0;
						}
					}
				}
			}
		}
	}
	return crossDic;
	//}
	//catch (Exception ex)
	//{
	//    throw ex;
	//}
}
#pragma endregion


/// <summary>
/// 找到棋盘四个顶点
/// </summary>
/// <returns>从左上角顺时针的点</returns>
Point2f  *FindConor(Point2f *directionLeft, Point2f *directionRight, Point2f *directionUp, Point2f *directionDown)
{
	// *directionLeft =  Point2f(0, 0);
	//directionRight = Point2f.Empty;
	//directionUp = Point2f.Empty;
	//directionDown = Point2f.Empty;

	//左
	vector<Point2f> leftPoints;//因为fitline只能vector，这里不用list
	TypePointMap::iterator iterLeft = CrossPoints.find(Left);
	if (iterLeft != CrossPoints.end)
	{
		for each (Point2f p in iterLeft->second)
		{
			leftPoints.push_back(Point2f(p.x, p.y));
		}
	}
	if (leftPoints.size() < 2)
	{
		return nullptr;
	}
	Point2f *pointOnLineLeft;
	LineFit(leftPoints, directionLeft, pointOnLineLeft);

	//右
	vector<Point2f> rightPoints;
	TypePointMap::iterator iterRight = CrossPoints.find(Right);
	if (iterRight != CrossPoints.end)
	{
		for each (Point2f p in iterRight->second)
		{
			rightPoints.push_back(Point2f(p.x, p.y));
		}
	}
	if (rightPoints.size() < 2)
	{
		return nullptr;
	}
	Point2f *pointOnLineRight;
	LineFit(rightPoints, directionRight, pointOnLineRight);

	//上
	vector<Point2f> upPoints;
	TypePointMap::iterator iterUp = CrossPoints.find(Up);
	if (iterUp != CrossPoints.end)
	{
		for each (Point2f p in iterUp->second)
		{
			upPoints.push_back(Point2f(p.x, p.y));
		}
	}
	TypePointMap::iterator iterLeftUp = CrossPoints.find(LeftUp);
	if (iterLeftUp != CrossPoints.end)
	{
		for each (Point2f p in iterLeftUp->second)
		{
			upPoints.push_back(Point2f(p.x, p.y));
		}
	}
	TypePointMap::iterator iterRightUp = CrossPoints.find(RightUp);
	if (iterRightUp != CrossPoints.end)
	{
		for each (Point2f p in iterRightUp->second)
		{
			upPoints.push_back(Point2f(p.x, p.y));
		}
	}
	if (upPoints.size < 2)
	{
		return nullptr;
	}
	Point2f *pointOnLineUp;
	LineFit(upPoints, directionUp, pointOnLineUp);

	//下
	vector<Point2f> downPoints;
	TypePointMap::iterator iterDown = CrossPoints.find(Down);
	if (iterDown != CrossPoints.end)
	{
		for each (Point2f p in iterDown->second)
		{
			downPoints.push_back(Point2f(p.x, p.y));
		}
	}
	TypePointMap::iterator iterLeftDown = CrossPoints.find(LeftDown);
	if (iterLeftDown != CrossPoints.end)
	{
		for each (Point2f p in iterLeftDown->second)
		{
			downPoints.push_back(Point2f(p.x, p.y));
		}
	}
	TypePointMap::iterator iterRightDown = CrossPoints.find(RightDown);
	if (iterRightDown != CrossPoints.end)
	{
		for each (Point2f p in iterRightDown->second)
		{
			downPoints.push_back(Point2f(p.x, p.y));
		}
	}
	if (downPoints.size() < 2)
	{
		return nullptr;
	}
	Point2f *pointOnLineDown;
	LineFit(downPoints, directionDown, pointOnLineDown);

	//求交点
	Point2f leftTop = FindLineCross(*directionLeft, *pointOnLineLeft, *directionUp, *pointOnLineUp);
	Point2f rightTop = FindLineCross(*directionRight, *pointOnLineRight, *directionUp, *pointOnLineUp);
	Point2f leftDown = FindLineCross(*directionLeft, *pointOnLineLeft, *directionDown, *pointOnLineDown);
	Point2f rightDown = FindLineCross(*directionRight, *pointOnLineRight, *directionDown, *pointOnLineDown);
	/*if (leftTop == nullptr || rightTop == nullptr || leftDown == nullptr || rightDown == nullptr)
	{
		return nullptr;
	}*/
	Point2f *result = new Point2f[4];// (*leftTop, *rightTop, *rightDown, *leftDown);
	result[0] = leftTop;
	result[1] = rightTop;
	result[2] = rightDown;
	result[3] = leftDown;
	return result;
}

#pragma region 计算Grid点
//通过四个角，矫正获得等分线
void GetEvenDevideLines(Point2f *conors, Point2f directionLeft, Point2f directionRight, Point2f directionUp, Point2f directionDown, LineSegment2DF *horizontalLines, LineSegment2DF * verticalLines)
{
	Point2f leftUpPoint = conors[0];
	Point2f rightUpPoint = conors[1];
	Point2f rightDownPoint = conors[2];
	Point2f leftDownPoint = conors[3];

	Point2f  *upPoints;
	Point2f *downPoints;
	Point2f *leftPoints;
	Point2f *rightPoints;

	horizontalLines = new LineSegment2DF[BoardSize];
	verticalLines = new LineSegment2DF[BoardSize];
	horizontalLines[0] = LineSegment2DF(leftUpPoint, rightUpPoint);
	horizontalLines[BoardSize - 1] = LineSegment2DF(leftDownPoint, rightDownPoint);
	verticalLines[0] = LineSegment2DF(leftUpPoint, leftDownPoint);
	verticalLines[BoardSize - 1] = LineSegment2DF(rightUpPoint, rightDownPoint);

	//分三种情况：1、都不平行。2、有一对平行。3、都平行
	//平行的判定条件，tanα <0.005
	float parallelAngle = 0.005f;

	float tanUp = directionUp.y / directionUp.x; //已考虑除数不会为零
	float tanDown = directionDown.y / directionDown.x;
	bool horizontalParallel = abs((tanUp - tanDown) / (1 + tanUp * tanDown)) < parallelAngle; //判断上下是否平行，根据三角公式tan(a-b)

	float tanLeft = directionLeft.x / directionLeft.y;
	float tanRight = directionRight.x / directionRight.y;
	bool verticalParallel = abs((tanLeft - tanRight) / (1 + tanLeft * tanRight)) < parallelAngle;  //判断左右是否平行

	if (horizontalParallel)
	{
		upPoints = new Point2f[BoardSize];
		for (int i = 0; i < BoardSize; i++)
		{
			leftPoints[i] = Point2f();
			leftPoints[i].x = leftUpPoint.x + (rightUpPoint.x - leftUpPoint.x) * i / (BoardSize - 1);
			leftPoints[i].y = leftUpPoint.y + (rightUpPoint.y - leftUpPoint.y) * i / (BoardSize - 1);
		}
		downPoints = new Point2f[BoardSize];
		for (int i = 0; i < BoardSize; i++)
		{
			rightPoints[i] = Point2f();
			rightPoints[i].x = leftDownPoint.x + (rightDownPoint.x - leftDownPoint.x) * i / (BoardSize - 1);
			rightPoints[i].y = leftDownPoint.y + (rightDownPoint.y - leftDownPoint.y) * i / (BoardSize - 1);
		}
		verticalLines = new LineSegment2DF[BoardSize];
		for (int i = 0; i < BoardSize; i++)
		{
			verticalLines[i] = LineSegment2DF(upPoints[i], downPoints[i]);
		}
	}
	if (verticalParallel)
	{
		leftPoints = new Point2f[BoardSize];
		for (int i = 0; i < BoardSize; i++)
		{
			leftPoints[i] = Point2f();
			leftPoints[i].x = leftUpPoint.x + (leftDownPoint.x - leftUpPoint.x) * i / (BoardSize - 1);
			leftPoints[i].y = leftUpPoint.y + (leftDownPoint.y - leftUpPoint.y) * i / (BoardSize - 1);
		}
		rightPoints = new Point2f[BoardSize];
		for (int i = 0; i < BoardSize; i++)
		{
			rightPoints[i] = Point2f();
			rightPoints[i].x = rightUpPoint.x + (rightDownPoint.x - rightUpPoint.x) * i / (BoardSize - 1);
			rightPoints[i].y = rightUpPoint.y + (rightDownPoint.y - rightUpPoint.y) * i / (BoardSize - 1);
		}
		horizontalLines = new LineSegment2DF[BoardSize];
		for (int i = 0; i < BoardSize; i++)
		{
			horizontalLines[i] = LineSegment2DF(leftPoints[i], rightPoints[i]);
		}
	}
	if (horizontalParallel && !verticalParallel)
	{
		leftPoints = new Point2f[BoardSize];
		rightPoints = new Point2f[BoardSize];
		leftPoints[0] = leftUpPoint;
		leftPoints[BoardSize - 1] = leftDownPoint;
		rightPoints[0] = rightUpPoint;
		rightPoints[BoardSize - 1] = rightDownPoint;
		//渐进作对角线，这样点在两边，会准确一点。如果boardsize(n)是偶数，n/2成为下部的第一条线，如果是奇数，n/2为中线
		//从上面画线时，都从1画到n/2-1，从下面画线时，从n-2画到n/2。当奇偶不同时，从下划线条数不同，但代码一致。
		for (int i = 0; i < BoardSize / 2 - 1; i++)
		{
			//从左上和右上开始，往对边画对角线，然后求平行的第1、2...n/2-1条线
			LineSegment2DF diagonalLineUp1 = LineSegment2DF(leftPoints[i], downPoints[BoardSize - i]);
			LineSegment2DF diagonalLineUp2 = LineSegment2DF(rightPoints[i], downPoints[i]);
			//和第1,n-2条垂直线交点
			Point2f pLeft = FindLineCross(diagonalLineUp1.Direction, diagonalLineUp1.P1, verticalLines[1].Direction, verticalLines[1].P1);
			Point2f pRight = FindLineCross(diagonalLineUp2.Direction, diagonalLineUp2.P1, verticalLines[BoardSize - 2].Direction, verticalLines[BoardSize - 2].P1);
			horizontalLines[i + 1] = LineSegment2DF(pLeft, pRight);
			leftPoints[i + 1] = FindLineCross(horizontalLines[i + 1].Direction, pLeft, verticalLines[0].Direction, verticalLines[0].P1);
			rightPoints[i + 1] = FindLineCross(horizontalLines[i + 1].Direction, pRight, verticalLines[BoardSize - 1].Direction, verticalLines[BoardSize - 1].P1);
		}
		for (int i = BoardSize - 1; i > BoardSize / 2; i--)
		{
			//从左下和右下开始，往对边画对角线，然后求平行的第n-2、n-3..n/2条线
			LineSegment2DF diagonalLineDown1 = LineSegment2DF(leftPoints[i], upPoints[i]);
			LineSegment2DF diagonalLineDown2 = LineSegment2DF(rightPoints[i], upPoints[BoardSize - 1 - i]);
			//和第1,n-2条垂直线交点
			Point2f pLeft = FindLineCross(diagonalLineDown1.Direction, diagonalLineDown1.P1, verticalLines[1].Direction, verticalLines[1].P1);
			Point2f pRight = FindLineCross(diagonalLineDown2.Direction, diagonalLineDown2.P1, verticalLines[BoardSize - 2].Direction, verticalLines[BoardSize - 2].P1);
			horizontalLines[i - 1] = LineSegment2DF(pLeft, pRight);
			leftPoints[i - 1] = FindLineCross(horizontalLines[i - 1].Direction, pLeft, verticalLines[0].Direction, verticalLines[0].P1);
			rightPoints[i - 1] = FindLineCross(horizontalLines[i - 1].Direction, pRight, verticalLines[BoardSize - 1].Direction, verticalLines[BoardSize - 1].P1);
		}
	}
	if (verticalParallel && !horizontalParallel)
	{
		upPoints = new Point2f[BoardSize];
		downPoints = new Point2f[BoardSize];
		upPoints[0] = leftUpPoint;
		upPoints[BoardSize - 1] = rightUpPoint;
		downPoints[0] = leftDownPoint;
		downPoints[BoardSize - 1] = rightDownPoint;
		//渐进作对角线，这样点在两边，会准确一点。如果boardsize(n)是偶数，n/2成为下部的第一条线，如果是奇数，n/2为中线
		//从上面画线时，都从1画到n/2-1，从下面画线时，从n-2画到n/2。当奇偶不同时，从下划线条数不同，但代码一致。
		for (int i = 0; i < BoardSize / 2 - 1; i++)
		{
			//从左上和右上开始，往对边画对角线，然后求平行的第1、2...n/2-1条线
			LineSegment2DF diagonalLineLeft1 = LineSegment2DF(upPoints[i], rightPoints[BoardSize - 1 - i]);
			LineSegment2DF diagonalLineLeft2 = LineSegment2DF(downPoints[i], rightPoints[i]);
			//和第1,n-2条平行线交点
			Point2f pUp = FindLineCross(diagonalLineLeft1.Direction, diagonalLineLeft1.P1, horizontalLines[1].Direction, horizontalLines[1].P1);
			Point2f pDown = FindLineCross(diagonalLineLeft2.Direction, diagonalLineLeft2.P1, horizontalLines[BoardSize - 2].Direction, horizontalLines[BoardSize - 2].P1);
			verticalLines[i + 1] = LineSegment2DF(pUp, pDown);
			upPoints[i + 1] = FindLineCross(verticalLines[i + 1].Direction, pUp, horizontalLines[0].Direction, horizontalLines[0].P1);
			downPoints[i + 1] = FindLineCross(verticalLines[i + 1].Direction, pDown, horizontalLines[BoardSize - 1].Direction, horizontalLines[BoardSize - 1].P1);
		}
		for (int i = BoardSize - 1; i > BoardSize / 2; i--)
		{
			//从左下和右下开始，往对边画对角线，然后求平行的第n-2、n-3..n/2条线
			LineSegment2DF diagonalLineRight1 = LineSegment2DF(upPoints[i], leftPoints[i]);
			LineSegment2DF diagonalLineRight2 = LineSegment2DF(downPoints[i], leftPoints[BoardSize - 1 - i]);
			//和第1,n-2条垂直线交点
			Point2f pUp = FindLineCross(diagonalLineRight1.Direction, diagonalLineRight1.P1, horizontalLines[1].Direction, horizontalLines[1].P1);
			Point2f pDown = FindLineCross(diagonalLineRight2.Direction, diagonalLineRight2.P1, horizontalLines[BoardSize - 2].Direction, horizontalLines[BoardSize - 2].P1);
			verticalLines[i - 1] = LineSegment2DF(pUp, pDown);
			upPoints[i - 1] = FindLineCross(verticalLines[i - 1].Direction, pUp, horizontalLines[0].Direction, horizontalLines[0].P1);
			downPoints[i - 1] = FindLineCross(verticalLines[i - 1].Direction, pDown, horizontalLines[BoardSize - 1].Direction, horizontalLines[BoardSize - 1].P1);
		}
	}
	if (!horizontalParallel && !verticalParallel)
	{
		//1.寻找两个边的交点，及上平行线
		Point2f horizontalCross = FindLineCross(directionUp, leftUpPoint, directionDown, leftDownPoint);  //水平相交点
		Point2f verticalCross = FindLineCross(directionLeft, leftUpPoint, directionRight, rightUpPoint);  //垂直相交点
		LineSegment2DF l1 = LineSegment2DF(horizontalCross, verticalCross);
		//2、过任意一点作l1的平行线 （找leftUpPoint和距leftUpPoint距离为1000的一点）
		Point2f pointOnL2 = Point2f(leftUpPoint.x + 1000 * l1.Direction.x, leftUpPoint.y + 1000 * l1.Direction.y);
		LineSegment2DF l2 = LineSegment2DF(leftUpPoint, pointOnL2);
		//3、让两边交于l2，并平分，作平分点和顶点交点连线，这些连线就是中间的格子线。
		//横端
		Point2f end1 = FindLineCross(horizontalLines[0].Direction, horizontalLines[0].P1, l2.Direction, l2.P1);
		Point2f end2 = FindLineCross(horizontalLines[BoardSize - 1].Direction, horizontalLines[BoardSize - 1].P1, l2.Direction, l2.P1);
		for (int i = 1; i < BoardSize - 1; i++)
		{
			Point2f gridPoint = Point2f();
			gridPoint.x = end1.x + (end2.x - end1.x) * i / (BoardSize - 1);
			gridPoint.y = end1.y + (end2.y - end1.y) * i / (BoardSize - 1);
			horizontalLines[i] = LineSegment2DF(horizontalCross, gridPoint);
		}
		//竖端
		Point2f end3 = FindLineCross(verticalLines[0].Direction, verticalLines[0].P1, l2.Direction, l2.P1);
		Point2f end4 = FindLineCross(verticalLines[BoardSize - 1].Direction, verticalLines[BoardSize - 1].P1, l2.Direction, l2.P1);
		for (int i = 1; i < BoardSize - 1; i++)
		{
			Point2f gridPoint;
			gridPoint.x = end3.x + (end4.x - end3.x) * i / (BoardSize - 1);
			gridPoint.y = end3.y + (end4.y - end3.y) * i / (BoardSize - 1);
			verticalLines[i] = LineSegment2DF(verticalCross, gridPoint);
		}
		//妹的，怎么有一边平行的反而算法和代码更复杂
	}
}

Point[] GetGridCoordinate(LineSegment2DF[] horizontalLines, LineSegment2DF[] verticalLines)
{
	Point[] coordinates = new Point[BoardSize * BoardSize];
	for (int i = 0; i < BoardSize; i++)
	{
		for (int j = 0; j < BoardSize; j++)
		{
			Point2f pointf = FindLineCross(verticalLines[i].Direction, verticalLines[i].P1, horizontalLines[j].Direction, horizontalLines[j].P1);
			coordinates[i + j * BoardSize] = new Point()
			{
				X = (int)pointf.x + 1,//因为检测的时候都偏小，这里补偿1像素
				Y = (int)pointf.y + 1//因为检测的时候都偏小，这里补偿1像素
			};
		}
	}
	return coordinates;
}

#pragma endregion
