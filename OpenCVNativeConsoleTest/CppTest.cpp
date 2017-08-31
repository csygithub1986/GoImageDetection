#include "CppTest.h"

typedef map<CrossType, list<Point>> TypePointMap;
//using namespace cv;

double MinWidthRate = 0.7;
double CannyThreshold = 90;   // ����5����Ե�����ֵ��30~180��
double CannyThreshold2 = 90 * 0.6;
double CircleAccumulatorThreshold = 30;       // ����6���ۼ�����ֵ��Բ���غϵ㣬Խ�͵�ʱ��Բ����Խ���׵���Բ��
double CircleCannyThresh = 90;    // Բ�ı�Ե�����ֵ��30~180��
double CrossFillRate = 0.2; //ʮ�ּ��


int BoardSize;
int ImageWidth;
int ImageHeight;
int MaxGridWidth;
int MinGridWidth;
int CrossDetectLen;

//�м����
vector<CircleF> Circles;
TypePointMap CrossPoints;
Point2f *Conors;
Mat CannyEdges;
Mat GrayBlurImage;
Point *AllCoordinate;

void Detect(unsigned char* src, int w, int h, int channel, int boardSize, int result[])
{
	BoardSize = boardSize;
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

	//��ʼ�����Ҷȡ�����
	GrayBlurImage = InitImage(img);

	//��Բ
	Circles = DetectCircle(GrayBlurImage, BoardSize);
	int size = Circles.size();

	//1���ҽ���
	//�������ĸ������ֱ�Ϊ��Ե�����ֵ��������ֵ�����ڵ�һ����Ϊ�߽磬С�ڵڶ�������������֮��ʱ���õ��Ƿ������������߽�㣩
	cv::Canny(GrayBlurImage, CannyEdges, CannyThreshold, CannyThreshold2);

	/*cv::namedWindow("camera", CV_WINDOW_NORMAL);
	cv::imshow("camera", cannyEdges);
	return;*/
	uchar *psrc = (uchar*)CannyEdges.data;
	uchar *pdst = new uchar[CannyEdges.total()];
	//for (int i = 0; i < h; i++) //������  
	//{
	//	const uchar* p0 = psrc + i*w;
	//	uchar* p1 = pdst + i*w;
	//	for (int j = 0; j < w; j++) //������  
	//	{
	//		*p1++ = p0[j];
	//	}
	//}   //��ʱ��0.7ms  
	memcpy(pdst, psrc, CannyEdges.total() * sizeof(uchar));  //��ʱ��0.5ms  

	/*Mat test(h, w, CV_8UC1, pdst);
	cv::namedWindow("camera1", CV_WINDOW_NORMAL);
	cv::imshow("camera1", test);
	return;
*/

	CrossPoints = DetectCross(pdst, w, h);
	//return;
	//2���ҽ�
	Point2f *directionLeft = &Point2f();
	Point2f *directionRight = &Point2f();
	Point2f *directionUp = &Point2f();
	Point2f *directionDown = &Point2f();
	Conors = FindConor(directionLeft, directionRight, directionUp, directionDown);

	//3��͸����������������
	LineSegment2DF *horizontalLines = new LineSegment2DF[BoardSize];
	LineSegment2DF *verticalLines = new LineSegment2DF[BoardSize];
	GetEvenDevideLines(Conors, *directionLeft, *directionRight, *directionUp, *directionDown, horizontalLines, verticalLines);
	AllCoordinate = GetGridCoordinate(horizontalLines, verticalLines);


	//4��������ɫ
	//int[] stones = new int[BoardSize * BoardSize];
	uchar *imageByte = CannyEdges.data;
	uchar *grayImageData = GrayBlurImage.data;
	for (int i = 0; i < BoardSize*BoardSize; i++)
	{
		result[i] = FindStone(i, imageByte, grayImageData);
	}

	//for (size_t i = 0; i < Circles.size(); i++)
	//{
	//	Point center(cvRound(Circles[i][0]), cvRound(Circles[i][1]));
	//	int radius = cvRound(Circles[i][2]);
	//	//����Բ��  
	//	circle(img, center, 3, Scalar(0, 255, 0), -1, 8, 0);
	//	//����Բ����  
	//	circle(img, center, radius, Scalar(155, 50, 255), 3, 8, 0);
	//	//Scalar(55,100,195)������G��B��R��ɫֵ����ֵ���õ���Ҫ����ɫ  
	//}
}


/// <summary>
/// ͼ��Ԥ�����ҶȺͽ���
/// </summary>
Mat InitImage(Mat mat)
{
	//תΪ�Ҷȼ�ͼ��
	Mat	grayMat;
	cvtColor(mat, grayMat, COLOR_BGR2GRAY);
	//use image pyr to remove noise ���룬Ϊ�˸�׼ȷ������Ե���
	Mat pyrDownMat;
	pyrDown(grayMat, pyrDownMat);//��֪����GaussianBlur��ʲô����
	pyrUp(pyrDownMat, grayMat);
	return grayMat;
}

void  SetConfig(double minWidthRate, double cannyThreshold, double cannyThreshold2, double circleAccumulatorThreshold, double circleCannyThresh, double crossFillRate)
{
	MinWidthRate = minWidthRate;
	CannyThreshold = cannyThreshold;   // ����5����Ե�����ֵ��30~180��
	CannyThreshold2 = cannyThreshold2;   // 
	CircleAccumulatorThreshold = circleAccumulatorThreshold;       // ����6���ۼ�����ֵ��Բ���غϵ㣬Խ�͵�ʱ��Բ����Խ���׵���Բ��
	CircleCannyThresh = circleCannyThresh;    // Բ�ı�Ե�����ֵ��30~180��
	CrossFillRate = crossFillRate; //ʮ�ּ��
}

/// <summary>
/// ���Բ
/// </summary>
vector<CircleF> DetectCircle(Mat uimage, int BoardSize)
{
	//�������뾶
	int maxRadius = MaxGridWidth / 2;//���������Ϊ��Ķ���֮һ									 //������С�뾶Ϊ���뾶*0.7
	int minRadius = (int)(maxRadius * MinWidthRate);
	//��С���Ϊ��Сֱ��
	int minDistance = minRadius * 2;
	vector<Vec3f> vecs;//����ʸ��
	int dp = 1;//��̫���������
	HoughCircles(uimage, vecs, CV_HOUGH_GRADIENT, dp, minDistance, CircleCannyThresh, CircleAccumulatorThreshold, minRadius, maxRadius);
	vector<CircleF> circles;
	for each (Vec3f vec in vecs)
	{
		circles.push_back(CircleF(Point2f(vec[0], vec[1]), vec[2]));//0,1��ʾcenter��x��y��2��ʾ�뾶
	}
	return circles;
}

bool first = true;

#pragma region CrossDetection
/// <summary>
/// ����ĳ�㣬�ж�ĳ���������Ƿ���ֱ��
/// </summary>
/// <param name="width">ͼ�εĿ��</param>
/// <param name="imageBytes">ͼ������</param>
/// <param name="x">Ҫ����ĵ�����x</param>
/// <param name="y">Ҫ����ĵ�����y</param>
/// <param name="directionX">x����ȡ-1,0,1</param>
/// <param name="directionY">y����ȡ-1,0,1</param>
/// <returns></returns>
bool IsLineOnDirection(int width, uchar imageBytes[], int x, int y, int directionX, int directionY)
{
	//try
	//{
	int iMin = 0, iMax = 0, jMin = 0, jMax = 0;
	//����Ҫ�жϵķ��򣬼�������
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

	vector<uchar> whiteBytes(5 * CrossDetectLen);
	//uchar *whiteBytes = new  uchar[5 * CrossDetectLen];
	int index = 0;
	for (int i = iMin; i <= iMax; i++)
	{
		for (int j = jMin; j <= jMax; j++)
		{
			if (x + i + (y + j) * width >= 0 && x + i + (y + j) * width < ImageWidth*ImageHeight)
			{
				whiteBytes[index++] = imageBytes[x + i + (y + j) * width];
			}
		}
	}
	int whiteCount = 0;

	if (first)
	{
		Mat test(ImageHeight, width, CV_8UC1, imageBytes);
		cv::namedWindow("camera1", CV_WINDOW_NORMAL);
		cv::imshow("camera1", test);
		first = false;
	}


	for (size_t i = 0; i < 5 * CrossDetectLen; i++)
	{
		if (whiteBytes[i] == (uchar)255)
		{
			whiteCount++;
		}
	}
	return whiteCount > whiteBytes.size() * CrossFillRate;
	//}
	//catch (Exception ex)
	//{
	//    throw ex;
	//}
}



void AddPoint(TypePointMap &crossDic, CrossType type, int x, int y)
{
	TypePointMap::iterator iter = crossDic.find(type);
	if (iter != crossDic.end())
	{
		list<Point> *pList = &(iter->second);
		(*pList).push_back(Point(x, y));
	}
	else
	{
		list<Point> pList;
		pList.push_front(Point(x, y));
		crossDic.insert(TypePointMap::value_type(type, pList));
	}
}


/// <summary>
/// ���ϼ�齻���
/// </summary>
/// <param name="width"></param>
/// <param name="imageBytes"></param>
/// <param name="x"></param>
/// <param name="y"></param>
/// <returns>0��ʾ�ǽ���㣬1,2,3,4�ֱ��ʾ��������T�֣�5,6,7,8�ֱ��ʾ�����Ͻǿ�ʼ˳ʱ�뷽��Ľǡ�������������ܻ���0,2,5,6�������</returns>
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
/// ���¼�齻���
/// </summary>
/// <param name="width"></param>
/// <param name="imageBytes"></param>
/// <param name="x"></param>
/// <param name="y"></param>
/// <returns>0��ʾ�ǽ���㣬1,2,3,4�ֱ��ʾ��������T�֣�5,6,7,8�ֱ��ʾ�����Ͻǿ�ʼ˳ʱ�뷽��Ľǡ�������������ܻ���0,4,7,8�������</returns>
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
/// �����齻���
/// </summary>
/// <param name="width"></param>
/// <param name="imageBytes"></param>
/// <param name="x"></param>
/// <param name="y"></param>
/// <returns>0��ʾ�ǽ���㣬1,2,3,4�ֱ��ʾ��������T�֣�5,6,7,8�ֱ��ʾ�����Ͻǿ�ʼ˳ʱ�뷽��Ľǡ�������������ܻ���0,1�������</returns>
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
/// ���Ҽ�齻���
/// </summary>
/// <param name="width"></param>
/// <param name="imageBytes"></param>
/// <param name="x"></param>
/// <param name="y"></param>
/// <returns>0��ʾ�ǽ���㣬1,2,3,4�ֱ��ʾ��������T�֣�5,6,7,8�ֱ��ʾ�����Ͻǿ�ʼ˳ʱ�뷽��Ľǡ�������������ܻ���0,3�������</returns>
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
/// ֻ����ıߵ�T�ͽ������Ľǵ�L�ͽ����
/// �ӵ�һ�п�ʼɨ�詰 ������״����ɨ����һ���п�ʼ��������ɨ������С������ɨ1/10��ͼ�θ߶ȡ�
/// �ӵ���ɨ�詸 ������״������ͬ��
/// �����ɨ��������ұ�ɨ��ȣ�����ͬ��
/// </summary>
/// <param name="width"></param>
/// <param name="imageBytes"></param>
/// <param name="x"></param>
/// <param name="y"></param>
/// <returns></returns>
TypePointMap DetectCross(uchar *imageBytes, int width, int height)
{
	//Mat test(height, width, CV_8UC1, imageBytes);
	//cv::namedWindow("camera1", CV_WINDOW_NORMAL);
	//cv::imshow("camera1", test);
	//return TypePointMap();

	TypePointMap crossDic;
	int scanRowCount = height / 7;//���ɨ�������//TODO ����������
	int scanColCount = width / 7;
	//try
	//{
	//�� //TODO���Ż���ɨ�赽���Ժ����Сɨ������������ɨ�赽19�����Ժ�ͽ��������߼���Բ��һ����19���˾ͽ���
	int edge = scanRowCount + CrossDetectLen;
	bool first = true;

	for (int j = CrossDetectLen; j < edge; j++)
	{
		for (int i = CrossDetectLen; i < width - CrossDetectLen; i++)
		{
			if (imageBytes[i + j * width] == (uchar)255)
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
					//����֮���һƬ������Ϊ0�������ظ����
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
	//��
	edge = height - scanRowCount - CrossDetectLen;
	first = true;
	for (int j = height - 1 - CrossDetectLen; j > edge; j--)
	{
		for (int i = CrossDetectLen; i < width - CrossDetectLen; i++)//��CrossDetectLen��Ϊ���߻�����
		{
			if (imageBytes[i + j * width] == (uchar)255)
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
					//����֮���һƬ������Ϊ0�������ظ����
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
	//��
	edge = scanColCount + CrossDetectLen;
	first = true;
	for (int i = CrossDetectLen; i < edge; i++)
	{
		for (int j = CrossDetectLen; j < height - CrossDetectLen; j++)
		{
			if (imageBytes[i + j * width] == (uchar)255)
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
					//����֮���һƬ������Ϊ0�������ظ����
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
	//��
	edge = width - scanColCount - CrossDetectLen;
	first = true;
	for (int i = width - 1 - CrossDetectLen; i > edge; i--)
	{
		for (int j = CrossDetectLen; j < height - CrossDetectLen; j++)
		{
			if (imageBytes[i + j * width] == (uchar)255)
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
					//����֮���һƬ������Ϊ0�������ظ����
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
/// �ҵ������ĸ�����
/// </summary>
/// <returns>�����Ͻ�˳ʱ��ĵ�</returns>
Point2f  *FindConor(Point2f *directionLeft, Point2f *directionRight, Point2f *directionUp, Point2f *directionDown)
{
	// *directionLeft =  Point2f(0, 0);
	//directionRight = Point2f.Empty;
	//directionUp = Point2f.Empty;
	//directionDown = Point2f.Empty;

	//��
	vector<Point2f> leftPoints;//��Ϊfitlineֻ��vector�����ﲻ��list
	TypePointMap::iterator iterLeft = CrossPoints.find(Left);
	if (iterLeft != CrossPoints.end())
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
	Point2f *pointOnLineLeft = &Point2f();
	LineFit(leftPoints, directionLeft, pointOnLineLeft);

	//��
	vector<Point2f> rightPoints;
	TypePointMap::iterator iterRight = CrossPoints.find(Right);
	if (iterRight != CrossPoints.end())
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
	Point2f *pointOnLineRight = &Point2f();
	LineFit(rightPoints, directionRight, pointOnLineRight);

	//��
	vector<Point2f> upPoints;
	TypePointMap::iterator iterUp = CrossPoints.find(Up);
	if (iterUp != CrossPoints.end())
	{
		for each (Point2f p in iterUp->second)
		{
			upPoints.push_back(Point2f(p.x, p.y));
		}
	}
	TypePointMap::iterator iterLeftUp = CrossPoints.find(LeftUp);
	if (iterLeftUp != CrossPoints.end())
	{
		for each (Point2f p in iterLeftUp->second)
		{
			upPoints.push_back(Point2f(p.x, p.y));
		}
	}
	TypePointMap::iterator iterRightUp = CrossPoints.find(RightUp);
	if (iterRightUp != CrossPoints.end())
	{
		for each (Point2f p in iterRightUp->second)
		{
			upPoints.push_back(Point2f(p.x, p.y));
		}
	}
	if (upPoints.size() < 2)
	{
		return nullptr;
	}
	Point2f *pointOnLineUp = &Point2f();
	LineFit(upPoints, directionUp, pointOnLineUp);

	//��
	vector<Point2f> downPoints;
	TypePointMap::iterator iterDown = CrossPoints.find(Down);
	if (iterDown != CrossPoints.end())
	{
		for each (Point2f p in iterDown->second)
		{
			downPoints.push_back(Point2f(p.x, p.y));
		}
	}
	TypePointMap::iterator iterLeftDown = CrossPoints.find(LeftDown);
	if (iterLeftDown != CrossPoints.end())
	{
		for each (Point2f p in iterLeftDown->second)
		{
			downPoints.push_back(Point2f(p.x, p.y));
		}
	}
	TypePointMap::iterator iterRightDown = CrossPoints.find(RightDown);
	if (iterRightDown != CrossPoints.end())
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
	Point2f *pointOnLineDown = &Point2f();
	LineFit(downPoints, directionDown, pointOnLineDown);

	//�󽻵�
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

#pragma region ����Grid��
//ͨ���ĸ��ǣ�������õȷ���
void GetEvenDevideLines(Point2f *conors, Point2f directionLeft, Point2f directionRight, Point2f directionUp, Point2f directionDown, LineSegment2DF *horizontalLines, LineSegment2DF *verticalLines)
{
	Point2f leftUpPoint = conors[0];
	Point2f rightUpPoint = conors[1];
	Point2f rightDownPoint = conors[2];
	Point2f leftDownPoint = conors[3];

	Point2f  *upPoints;
	Point2f *downPoints;
	Point2f *leftPoints;
	Point2f *rightPoints;

	//horizontalLines = new LineSegment2DF[BoardSize];
	//verticalLines = new LineSegment2DF[BoardSize];
	horizontalLines[0] = LineSegment2DF(leftUpPoint, rightUpPoint);
	horizontalLines[BoardSize - 1] = LineSegment2DF(leftDownPoint, rightDownPoint);
	verticalLines[0] = LineSegment2DF(leftUpPoint, leftDownPoint);
	verticalLines[BoardSize - 1] = LineSegment2DF(rightUpPoint, rightDownPoint);

	//�����������1������ƽ�С�2����һ��ƽ�С�3����ƽ��
	//ƽ�е��ж�������tan�� <0.005
	float parallelAngle = 0.005f;

	float tanUp = directionUp.y / directionUp.x; //�ѿ��ǳ�������Ϊ��
	float tanDown = directionDown.y / directionDown.x;
	bool horizontalParallel = abs((tanUp - tanDown) / (1 + tanUp * tanDown)) < parallelAngle; //�ж������Ƿ�ƽ�У��������ǹ�ʽtan(a-b)

	float tanLeft = directionLeft.x / directionLeft.y;
	float tanRight = directionRight.x / directionRight.y;
	bool verticalParallel = abs((tanLeft - tanRight) / (1 + tanLeft * tanRight)) < parallelAngle;  //�ж������Ƿ�ƽ��

	if (horizontalParallel)
	{
		upPoints = new Point2f[BoardSize];
		for (int i = 0; i < BoardSize; i++)
		{
			upPoints[i] = Point2f();
			upPoints[i].x = leftUpPoint.x + (rightUpPoint.x - leftUpPoint.x) * i / (BoardSize - 1);
			upPoints[i].y = leftUpPoint.y + (rightUpPoint.y - leftUpPoint.y) * i / (BoardSize - 1);
		}
		downPoints = new Point2f[BoardSize];
		for (int i = 0; i < BoardSize; i++)
		{
			downPoints[i] = Point2f();
			downPoints[i].x = leftDownPoint.x + (rightDownPoint.x - leftDownPoint.x) * i / (BoardSize - 1);
			downPoints[i].y = leftDownPoint.y + (rightDownPoint.y - leftDownPoint.y) * i / (BoardSize - 1);
		}
		//verticalLines = new LineSegment2DF[BoardSize];
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
		//horizontalLines = new LineSegment2DF[BoardSize];
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
		//�������Խ��ߣ������������ߣ���׼ȷһ�㡣���boardsize(n)��ż����n/2��Ϊ�²��ĵ�һ���ߣ������������n/2Ϊ����
		//�����滭��ʱ������1����n/2-1�������滭��ʱ����n-2����n/2������ż��ͬʱ�����»���������ͬ��������һ�¡�
		for (int i = 0; i < BoardSize / 2 - 1; i++)
		{
			//�����Ϻ����Ͽ�ʼ�����Ա߻��Խ��ߣ�Ȼ����ƽ�еĵ�1��2...n/2-1����
			LineSegment2DF diagonalLineUp1 = LineSegment2DF(leftPoints[i], downPoints[BoardSize - i]);
			LineSegment2DF diagonalLineUp2 = LineSegment2DF(rightPoints[i], downPoints[i]);
			//�͵�1,n-2����ֱ�߽���
			Point2f pLeft = FindLineCross(diagonalLineUp1.Direction, diagonalLineUp1.P1, verticalLines[1].Direction, verticalLines[1].P1);
			Point2f pRight = FindLineCross(diagonalLineUp2.Direction, diagonalLineUp2.P1, verticalLines[BoardSize - 2].Direction, verticalLines[BoardSize - 2].P1);
			horizontalLines[i + 1] = LineSegment2DF(pLeft, pRight);
			leftPoints[i + 1] = FindLineCross(horizontalLines[i + 1].Direction, pLeft, verticalLines[0].Direction, verticalLines[0].P1);
			rightPoints[i + 1] = FindLineCross(horizontalLines[i + 1].Direction, pRight, verticalLines[BoardSize - 1].Direction, verticalLines[BoardSize - 1].P1);
		}
		for (int i = BoardSize - 1; i > BoardSize / 2; i--)
		{
			//�����º����¿�ʼ�����Ա߻��Խ��ߣ�Ȼ����ƽ�еĵ�n-2��n-3..n/2����
			LineSegment2DF diagonalLineDown1 = LineSegment2DF(leftPoints[i], upPoints[i]);
			LineSegment2DF diagonalLineDown2 = LineSegment2DF(rightPoints[i], upPoints[BoardSize - 1 - i]);
			//�͵�1,n-2����ֱ�߽���
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
		//�������Խ��ߣ������������ߣ���׼ȷһ�㡣���boardsize(n)��ż����n/2��Ϊ�²��ĵ�һ���ߣ������������n/2Ϊ����
		//�����滭��ʱ������1����n/2-1�������滭��ʱ����n-2����n/2������ż��ͬʱ�����»���������ͬ��������һ�¡�
		for (int i = 0; i < BoardSize / 2 - 1; i++)
		{
			//�����Ϻ����Ͽ�ʼ�����Ա߻��Խ��ߣ�Ȼ����ƽ�еĵ�1��2...n/2-1����
			LineSegment2DF diagonalLineLeft1 = LineSegment2DF(upPoints[i], rightPoints[BoardSize - 1 - i]);
			LineSegment2DF diagonalLineLeft2 = LineSegment2DF(downPoints[i], rightPoints[i]);
			//�͵�1,n-2��ƽ���߽���
			Point2f pUp = FindLineCross(diagonalLineLeft1.Direction, diagonalLineLeft1.P1, horizontalLines[1].Direction, horizontalLines[1].P1);
			Point2f pDown = FindLineCross(diagonalLineLeft2.Direction, diagonalLineLeft2.P1, horizontalLines[BoardSize - 2].Direction, horizontalLines[BoardSize - 2].P1);
			verticalLines[i + 1] = LineSegment2DF(pUp, pDown);
			upPoints[i + 1] = FindLineCross(verticalLines[i + 1].Direction, pUp, horizontalLines[0].Direction, horizontalLines[0].P1);
			downPoints[i + 1] = FindLineCross(verticalLines[i + 1].Direction, pDown, horizontalLines[BoardSize - 1].Direction, horizontalLines[BoardSize - 1].P1);
		}
		for (int i = BoardSize - 1; i > BoardSize / 2; i--)
		{
			//�����º����¿�ʼ�����Ա߻��Խ��ߣ�Ȼ����ƽ�еĵ�n-2��n-3..n/2����
			LineSegment2DF diagonalLineRight1 = LineSegment2DF(upPoints[i], leftPoints[i]);
			LineSegment2DF diagonalLineRight2 = LineSegment2DF(downPoints[i], leftPoints[BoardSize - 1 - i]);
			//�͵�1,n-2����ֱ�߽���
			Point2f pUp = FindLineCross(diagonalLineRight1.Direction, diagonalLineRight1.P1, horizontalLines[1].Direction, horizontalLines[1].P1);
			Point2f pDown = FindLineCross(diagonalLineRight2.Direction, diagonalLineRight2.P1, horizontalLines[BoardSize - 2].Direction, horizontalLines[BoardSize - 2].P1);
			verticalLines[i - 1] = LineSegment2DF(pUp, pDown);
			upPoints[i - 1] = FindLineCross(verticalLines[i - 1].Direction, pUp, horizontalLines[0].Direction, horizontalLines[0].P1);
			downPoints[i - 1] = FindLineCross(verticalLines[i - 1].Direction, pDown, horizontalLines[BoardSize - 1].Direction, horizontalLines[BoardSize - 1].P1);
		}
	}
	if (!horizontalParallel && !verticalParallel)
	{
		//1.Ѱ�������ߵĽ��㣬����ƽ����
		Point2f horizontalCross = FindLineCross(directionUp, leftUpPoint, directionDown, leftDownPoint);  //ˮƽ�ཻ��
		Point2f verticalCross = FindLineCross(directionLeft, leftUpPoint, directionRight, rightUpPoint);  //��ֱ�ཻ��
		LineSegment2DF l1 = LineSegment2DF(horizontalCross, verticalCross);
		//2��������һ����l1��ƽ���� ����leftUpPoint�;�leftUpPoint����Ϊ1000��һ�㣩
		Point2f pointOnL2 = Point2f(leftUpPoint.x + 1000 * l1.Direction.x, leftUpPoint.y + 1000 * l1.Direction.y);
		LineSegment2DF l2 = LineSegment2DF(leftUpPoint, pointOnL2);
		//3�������߽���l2����ƽ�֣���ƽ�ֵ�Ͷ��㽻�����ߣ���Щ���߾����м�ĸ����ߡ�
		//���
		Point2f end1 = FindLineCross(horizontalLines[0].Direction, horizontalLines[0].P1, l2.Direction, l2.P1);
		Point2f end2 = FindLineCross(horizontalLines[BoardSize - 1].Direction, horizontalLines[BoardSize - 1].P1, l2.Direction, l2.P1);
		for (int i = 1; i < BoardSize - 1; i++)
		{
			Point2f gridPoint = Point2f();
			gridPoint.x = end1.x + (end2.x - end1.x) * i / (BoardSize - 1);
			gridPoint.y = end1.y + (end2.y - end1.y) * i / (BoardSize - 1);
			horizontalLines[i] = LineSegment2DF(horizontalCross, gridPoint);
		}
		//����
		Point2f end3 = FindLineCross(verticalLines[0].Direction, verticalLines[0].P1, l2.Direction, l2.P1);
		Point2f end4 = FindLineCross(verticalLines[BoardSize - 1].Direction, verticalLines[BoardSize - 1].P1, l2.Direction, l2.P1);
		for (int i = 1; i < BoardSize - 1; i++)
		{
			Point2f gridPoint;
			gridPoint.x = end3.x + (end4.x - end3.x) * i / (BoardSize - 1);
			gridPoint.y = end3.y + (end4.y - end3.y) * i / (BoardSize - 1);
			verticalLines[i] = LineSegment2DF(verticalCross, gridPoint);
		}
		//�õģ���ô��һ��ƽ�еķ����㷨�ʹ��������
	}
}

Point *GetGridCoordinate(LineSegment2DF *horizontalLines, LineSegment2DF *verticalLines)
{
	Point *coordinates = new Point[BoardSize * BoardSize];
	for (int i = 0; i < BoardSize; i++)
	{
		for (int j = 0; j < BoardSize; j++)
		{
			Point2f pointf = FindLineCross(verticalLines[i].Direction, verticalLines[i].P1, horizontalLines[j].Direction, horizontalLines[j].P1);
			coordinates[i + j * BoardSize] = Point((int)pointf.x + 1, (int)pointf.y + 1);//��Ϊ����ʱ��ƫС�����ﲹ��1����
		}
	}
	return coordinates;
}

#pragma endregion


//points ֻ����vector
void LineFit(vector<Point2f> points, Point2f *direction, Point2f *pointOnLine)
{
	Vec4f line_para;
	cv::fitLine(points, line_para, cv::DIST_L2, 0, 1e-2, 1e-2);
	(*direction).x = line_para[0];
	(*direction).y = line_para[1];
	(*pointOnLine).x = line_para[2];
	(*pointOnLine).y = line_para[3];
}

/// <summary>
/// ����ֱ�߽��㣨ͨ�������ߵ�б�ʺ͵㣩
/// </summary>
/// <param name="direction1"></param>
/// <param name="pointOnLine1"></param>
/// <param name="direction2"></param>
/// <param name="pointOnLine2"></param>
/// <returns></returns>
Point2f FindLineCross(Point2f direction1, Point2f pointOnLine1, Point2f direction2, Point2f pointOnLine2)
{
	if (direction1.x * direction2.y == direction1.y * direction2.x)//ƽ��
	{
		return NULL;
	}
	float x, y;
	x = (direction1.x * direction2.x * (pointOnLine2.y - pointOnLine1.y) + direction1.y * direction2.x * pointOnLine1.x - direction1.x * direction2.y * pointOnLine2.x) / (direction2.x * direction1.y - direction1.x * direction2.y);
	y = (direction1.y * direction2.y * (pointOnLine2.x - pointOnLine1.x) + direction1.x * direction2.y * pointOnLine1.y - direction1.y * direction2.x * pointOnLine2.y) / (direction2.y * direction1.x - direction1.y * direction2.x);
	return Point2f(x, y);
}

/// <summary>
/// ������
/// </summary>
/// <returns>0:empty��1:black��2:white��-1:����</returns>
int FindStone(int index, uchar *cannyBytes, uchar *grayImageData)
{
	int indexX = index % BoardSize;
	int indexY = index / BoardSize;

	int x = AllCoordinate[index].x;
	int y = AllCoordinate[index].y;
	//�������ַ��������ŶȴӸߵ���

#pragma region ����xy����25%������Ƿ���Բ��Ȼ��Բ�ڵĻҶȴ���ƽ��ֵ���������Ϊ��128����0.5�����Ǻ��壬С�����ǰ���
	{
		int minX = x - MaxGridWidth / 4;
		minX = minX < 0 ? 0 : minX;
		int maxX = x + MaxGridWidth / 4;
		maxX = maxX >= ImageWidth ? ImageWidth - 1 : maxX;

		int minY = y - MaxGridWidth / 4;
		minY = minY < 0 ? 0 : minY;
		int maxY = y + MaxGridWidth / 4;
		maxY = maxY >= ImageHeight ? ImageHeight - 1 : maxY;

		CircleF circleStone = CircleF(Point2f(), 0);
		for each(CircleF circle in Circles)
		{
			//if (index==2)
			//{

			//}
			if (circle.Center.x >= minX && circle.Center.x <= maxX && circle.Center.y >= minY && circle.Center.y <= maxY)
			{
				circleStone = circle;
				break;
			}
		}
		if (circleStone.Radius == 0)
		{
			//������0.25����С���Ϊ�뾶��Բ���ٷ�֮99����ֵ���������Ǻ�ɫ���ж�Ϊ��Բ
			int totalCannyCount = 0;
			int blackCount = 0;
			float littleRadius = 0.25f * MinGridWidth;
			for (int i = (int)(-littleRadius) + 1; i < littleRadius; i++)
			{
				for (int j = (int)(-littleRadius) + 1; j < littleRadius; j++)
				{
					if (i * i + j * j < littleRadius * littleRadius)
					{
						if (x + i >= 0 && x + i < ImageWidth && y + j >= 0 && y + j < ImageHeight)
						{
							totalCannyCount++;
							blackCount += cannyBytes[x + i + (y + j) * ImageWidth] == 0 ? 1 : 0;
						}
					}
				}
			}
			if ((float)blackCount / totalCannyCount >= 0.99)
			{
				circleStone = CircleF(Point2f(x, y), littleRadius);
				//Console.Write("��Բ�����Ŀն�   ");
			}
		}
		else
		{
			//Console.Write("��Բ   ");
		}

		if (circleStone.Radius != 0)
		{
			//��Բ�ڻҶ�
			float totalGray = 0;
			int totalCount = 0;
			for (int i = (int)(circleStone.Center.x - circleStone.Radius) + 1; i < circleStone.Center.x + circleStone.Radius; i++)
			{
				for (int j = (int)(circleStone.Center.y - circleStone.Radius) + 1; j < circleStone.Center.y + circleStone.Radius; j++)
				{
					if ((i - circleStone.Center.x) * (i - circleStone.Center.x) + (j - circleStone.Center.y) * (j - circleStone.Center.y) < circleStone.Radius * circleStone.Radius)
					{
						if (i >= 0 && i < ImageWidth && j >= 0 && j < ImageHeight)
						{
							totalGray += grayImageData[i + j * ImageWidth] / 255.0f;
							totalCount++;
						}
					}
				}
			}
			float averageGray = totalGray / totalCount;
			if (averageGray > 0.45)
			{
				/*Console.Write("  Ϊ��" + "  �Ҷ�" + averageGray.ToString("F2"));
				Console.WriteLine("  (" + indexX + "," + indexY + ")");*/
				return 2;//��
			}
			else if (averageGray < 0.45)
			{
				/*Console.Write("  Ϊ��" + "  �Ҷ�" + averageGray.ToString("F2"));
				Console.WriteLine("  (" + indexX + "," + indexY + ")");*/
				return 1;//��
			}
			else
			{
				//Console.WriteLine("����");
				return -1;//����
			}
		}
	}
#pragma endregion

#pragma endregion ����xy����10%������Ƿ���ʮ�֣�����У���Ϊ��
	{
		int minX = x - MaxGridWidth / 10;
		minX = minX < 0 ? 0 : minX;
		int maxX = x + MaxGridWidth / 10;
		maxX = maxX >= ImageWidth ? ImageWidth - 1 : maxX;

		int minY = x - MaxGridWidth / 10;
		minY = minY < 0 ? 0 : minY;
		int maxY = x + MaxGridWidth / 10;
		maxY = maxY >= ImageHeight ? ImageHeight - 1 : maxY;

		for (int i = minX; i <= maxX; i++)
		{
			for (int j = minY; j <= maxY; j++)
			{
				CrossType type = GetCrossFromCenter(ImageWidth, cannyBytes, i, j);
				if (type == Center)
				{
					//Console.Write("  �ҵ�ʮ��");
					//Console.WriteLine("  (" + indexX + "," + indexY + ")");
					return 0;//��
				}
			}
		}
	}
#pragma endregion

#pragma endregion �������ԲҲ��ʮ�֣�����0.4����С���Ϊ�뾶��Բ��Ҷȣ�����Ҷ�<0.2����>0.75����ֵ��������֮����Ϊ��
	{
		float totalGray = 0;
		int totalCount = 0;
		float littleRadius = 0.4f * MinGridWidth;

		for (int i = (int)(-littleRadius) + 1; i < littleRadius; i++)
		{
			for (int j = (int)(-littleRadius) + 1; j < littleRadius; j++)
			{
				if (i * i + j * j < littleRadius * littleRadius)
				{
					if (x + i >= 0 && x + i < ImageWidth && y + j >= 0 && y + j < ImageHeight)
					{
						totalCount++;
						totalGray += grayImageData[x + i + (y + j) * ImageWidth] / 255.0f;
					}
				}
			}
		}
		float averageGray = totalGray / totalCount;
		if (averageGray > 0.75)//�׵ĻҶ�һ����0.6����
		{
			//Console.Write("  ǿ�лҶ�Ϊ��" + "  �Ҷ�" + averageGray.ToString("F2"));
			//Console.WriteLine("  (" + indexX + "," + indexY + ")");
			return 2;//��
		}
		else if (averageGray < 0.15)//�ڵĻҶ�һ����0.2����
		{
			//Console.Write("  ǿ�лҶ�Ϊ��" + "  �Ҷ�" + averageGray.ToString("F2"));
			//Console.WriteLine("  (" + indexX + "," + indexY + ")");
			return 1;//��
		}
		else
		{
			//Console.Write("  ǿ�лҶ�Ϊ��");
			//Console.WriteLine("  (" + indexX + 1 + "," + indexY + 1 + ")");
			return 0;//��
		}
	}
#pragma endregion

}

void GetCoordinate(int x[], int y[])
{
	for (size_t i = 0; i < BoardSize*BoardSize; i++)
	{
		x[i] = AllCoordinate[i].x;
		y[i] = AllCoordinate[i].y;
	}
}

