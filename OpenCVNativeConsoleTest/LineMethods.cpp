#include "CppTest.h"

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

