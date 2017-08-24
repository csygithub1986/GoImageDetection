using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using Emgu.CV;
using Emgu.CV.CvEnum;
using Emgu.CV.Structure;
using System.Collections;
using System.Runtime.InteropServices;

namespace GoImageDetection.Core
{
    /// <summary>
    /// 检测图形算法
    /// 其中有2个假设：
    /// 1、标准棋盘为（0.92~0.96）:1，为了简便，这里图像像素强制为正方形输入。假定棋盘每一边都在(0.7~1) *(图宽)范围内
    /// 2、棋盘交叉点误差应该小于最大格宽的正负10%；圆心坐标相差应小于最大格宽的正负25%
    /// 目前采用整个棋盘全部检测模式
    /// </summary>
    public class Detector : IDetect
    {
        double minWidthRate = 0.7;
        double cannyThreshold = 80;   // 参数5：边缘检测阈值（30~180）
        double circleAccumulatorThreshold = 30;       // 参数6：累加器阈值（圆心重合点，越低的时候圆弧就越容易当成圆）

        double circleCannyThresh = 100;    // 圆的边缘检测阈值（30~180）

        #region 可调参数

        #region 十字检测
        double crossFillRate = 0.2;
        #endregion

        #endregion

        double dp = 1;    // 参数3：dp，不懂

        int maxGridWidth;//最大格宽
        public int minGridWidth;//最小格宽
        int crossDetectLen;//十字检测像素数(纵向)，也作为四周扫描的起点边界
        //int crossDetectWidth;//十字检测偏差像素数

        public CircleF[] Circles;
        public Dictionary<CrossType, List<Point>> CrossPoints;

        public UMat cannyEdges;
        Bitmap bitmap;
        int boardSize;
        int imageSize;

        public PointF[] conors;
        public Point[] allCoordinate;

        public Detector(double crossFillRate)
        {
            this.crossFillRate = crossFillRate;
        }

        /// <summary>
        /// 检测
        /// </summary>
        /// <param name="bitmap">图像，必须为正方形</param>
        /// <param name="boardSize">大小</param>
        /// <returns></returns>
        public int[] Detect(Bitmap bitmap, int boardSize)
        {
            this.bitmap = bitmap;
            this.boardSize = boardSize;
            UMat uimage = InitImage(bitmap);
            imageSize = uimage.Cols;
            maxGridWidth = uimage.Size.Width / (boardSize - 1);
            minGridWidth = (int)(uimage.Size.Width / (boardSize - 1) * minWidthRate);
            crossDetectLen = minGridWidth / 4;
            //crossDetectWidth = crossDetectWidth = crossDetectLen / 4;

            Circles = DetectCircle(uimage, boardSize);

            //List<LineSegment2D> horizontalLines;
            //List<LineSegment2D> verticalLines;
            //DetectLine(uimage, boardSize, out horizontalLines, out verticalLines);
            //CrossPoints = CalculateCross(horizontalLines, verticalLines);

            cannyEdges = new UMat();
            //第三、四个参数分别为边缘检测阈值和连接阈值（大于第一个作为边界，小于第二个舍弃，介于之间时看该点是否连接着其他边界点）
            CvInvoke.Canny(uimage, cannyEdges, cannyThreshold, cannyThreshold * 0.6);
            DateTime t1 = DateTime.Now;
            CrossPoints = DetectCross(cannyEdges.Bytes, cannyEdges.Cols);
            DateTime t2 = DateTime.Now;
            Console.WriteLine((t2 - t1).Milliseconds + " ms");

            //foreach (var item in CrossPoints)
            //{
            //    Console.WriteLine(item.Key + "  " + item.Value.Count());
            //}


            //找交点
            PointF directionLeft = PointF.Empty;
            PointF directionRight = PointF.Empty;
            PointF directionUp = PointF.Empty;
            PointF directionDown = PointF.Empty;
            conors = FindConor(out directionLeft, out directionRight, out directionUp, out directionDown);
            if (conors == null)
            {
                return null;
            }

            //TODO：加入矫正

            //未校正之前不做一下操作
            allCoordinate = CalculateAllCoordinate(conors);

            int[] stones = new int[boardSize * boardSize];
            for (int i = 0; i < stones.Length; i++)
            {
                stones[i] = FindStone(allCoordinate[i].X, allCoordinate[i].Y);
            }
            return stones;
        }

        /// <summary>
        /// 图形预处理
        /// </summary>
        private UMat InitImage(Bitmap bitmap)
        {
            Image<Bgr, Byte> img = new Image<Bgr, byte>(bitmap);
            //转为灰度级图像
            UMat uimage = new UMat();
            CvInvoke.CvtColor(img, uimage, ColorConversion.Bgr2Gray);
            //use image pyr to remove noise 降噪，为了更准确的做边缘检测
            UMat pyrDown = new UMat();
            CvInvoke.PyrDown(uimage, pyrDown);
            CvInvoke.PyrUp(pyrDown, uimage);
            return uimage;
        }

        /// <summary>
        /// 检测圆
        /// </summary>
        private CircleF[] DetectCircle(UMat uimage, int boardSize)
        {
            //棋子最大半径
            int maxRadius = maxGridWidth / 2;//棋子最大宽度为格的二分之一
            //棋子最小半径为最大半径*0.7
            int minRadius = (int)(maxRadius * minWidthRate);
            //最小间距为最小直径
            int minDistance = minRadius * 2;
            CircleF[] circles = CvInvoke.HoughCircles(uimage, HoughType.Gradient, dp, minDistance, circleCannyThresh, circleAccumulatorThreshold, minRadius, maxRadius);
            return circles;
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
        private Dictionary<CrossType, List<Point>> DetectCross(byte[] imageBytes, int width)
        {
            Dictionary<CrossType, List<Point>> crossDic = new Dictionary<CrossType, List<Point>>();
            int height = imageBytes.Length / width;
            int scanRowCount = height / 7;//最多扫描的行数
            int scanColCount = width / 7;
            //try
            //{
            //上 //TODO：优化，扫描到了以后就缩小扫描行数，或者扫描到19个了以后就结束，或者加上圆形一共有19个了就结束
            int edge = scanRowCount + crossDetectLen;
            bool first = true;

            for (int j = crossDetectLen; j < edge; j++)
            {
                for (int i = crossDetectLen; i < width - crossDetectLen; i++)
                {
                    if (imageBytes[i + j * width] == 255)
                    {
                        CrossType type = GetCrossFromUp(width, imageBytes, i, j);
                        if (type != CrossType.None)
                        {
                            AddPoint(crossDic, type, i, j);
                            if (first)
                            {
                                first = false;
                                edge = j + minGridWidth;
                            }
                            //并将之后的一片区域设为0，避免重复检查
                            for (int ii = 0; ii < crossDetectLen; ii++)
                            {
                                for (int jj = 0; jj < crossDetectLen; jj++)
                                {
                                    imageBytes[i + ii + (j + jj) * width] = 0;
                                }
                            }
                        }
                    }
                }
            }
            //下
            edge = height - scanRowCount - crossDetectLen;
            first = true;
            for (int j = height - 1 - crossDetectLen; j > edge; j--)
            {
                for (int i = crossDetectLen; i < width - crossDetectLen; i++)//用crossDetectLen作为两边缓冲区
                {
                    if (imageBytes[i + j * width] == 255)
                    {
                        CrossType type = GetCrossFromDown(width, imageBytes, i, j);
                        if (type != CrossType.None)
                        {
                            AddPoint(crossDic, type, i, j);
                            if (first)
                            {
                                first = false;
                                edge = j - minGridWidth;
                            }
                            //并将之后的一片区域设为0，避免重复检查
                            for (int ii = 0; ii < crossDetectLen; ii++)
                            {
                                for (int jj = 0; jj < crossDetectLen; jj++)
                                {
                                    imageBytes[i + ii + (j + jj) * width] = 0;
                                }
                            }
                        }
                    }
                }
            }
            //左
            edge = scanColCount + crossDetectLen;
            first = true;
            for (int i = crossDetectLen; i < edge; i++)
            {
                for (int j = crossDetectLen; j < height - crossDetectLen; j++)
                {
                    if (imageBytes[i + j * width] == 255)
                    {
                        CrossType type = GetCrossFromLeft(width, imageBytes, i, j);
                        if (type != CrossType.None)
                        {
                            AddPoint(crossDic, type, i, j);
                            if (first)
                            {
                                first = false;
                                edge = i + minGridWidth;
                            }
                            //并将之后的一片区域设为0，避免重复检查
                            for (int ii = 0; ii < crossDetectLen; ii++)
                            {
                                for (int jj = 0; jj < crossDetectLen; jj++)
                                {
                                    imageBytes[i + ii + (j + jj) * width] = 0;
                                }
                            }
                        }
                    }
                }
            }
            //右
            edge = width - scanColCount - crossDetectLen;
            first = true;
            for (int i = width - 1 - crossDetectLen; i > edge; i--)
            {
                for (int j = crossDetectLen; j < height - crossDetectLen; j++)
                {
                    if (imageBytes[i + j * width] == 255)
                    {
                        CrossType type = GetCrossFromRight(width, imageBytes, i, j);
                        if (type != CrossType.None)
                        {
                            AddPoint(crossDic, type, i, j);
                            if (first)
                            {
                                first = false;
                                edge = i - minGridWidth;
                            }
                            //并将之后的一片区域设为0，避免重复检查
                            for (int ii = 0; ii < crossDetectLen; ii++)
                            {
                                for (int jj = 0; jj < crossDetectLen; jj++)
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


        /// <summary>
        /// 从上检查交叉点
        /// </summary>
        /// <param name="width"></param>
        /// <param name="imageBytes"></param>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <returns>0表示非交叉点，1,2,3,4分别表示左上右下T字，5,6,7,8分别表示从左上角开始顺时针方向的角。所以在这里可能会有0,2,5,6四种情况</returns>
        private CrossType GetCrossFromUp(int width, byte[] imageBytes, int x, int y)
        {
            bool toDown = IsLineOnDirection(width, imageBytes, x, y, 0, 1);
            if (toDown == false)
            {
                return CrossType.None;
            }
            bool toUp = IsLineOnDirection(width, imageBytes, x, y, 0, -1);
            if (toUp)
            {
                return CrossType.None;
            }
            bool toLeft = IsLineOnDirection(width, imageBytes, x, y, -1, 0);
            bool toRight = IsLineOnDirection(width, imageBytes, x, y, 1, 0);
            if (toLeft && toRight)
            {
                return CrossType.Up;
            }
            if (toLeft && toRight == false)
            {
                return CrossType.RightUp;
            }
            if (toLeft == false && toRight)
            {
                return CrossType.LeftUp;
            }
            return CrossType.None;
        }

        /// <summary>
        /// 从下检查交叉点
        /// </summary>
        /// <param name="width"></param>
        /// <param name="imageBytes"></param>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <returns>0表示非交叉点，1,2,3,4分别表示左上右下T字，5,6,7,8分别表示从左上角开始顺时针方向的角。所以在这里可能会有0,4,7,8四种情况</returns>
        private CrossType GetCrossFromDown(int width, byte[] imageBytes, int x, int y)
        {
            bool toUp = IsLineOnDirection(width, imageBytes, x, y, 0, -1);
            if (toUp == false)
            {
                return CrossType.None;
            }
            bool toDown = IsLineOnDirection(width, imageBytes, x, y, 0, 1);
            if (toDown)
            {
                return CrossType.None;
            }
            bool toLeft = IsLineOnDirection(width, imageBytes, x, y, -1, 0);
            bool toRight = IsLineOnDirection(width, imageBytes, x, y, 1, 0);
            if (toLeft && toRight)
            {
                return CrossType.Down;
            }
            if (toLeft && toRight == false)
            {
                return CrossType.RightDown;
            }
            if (toLeft == false && toRight)
            {
                return CrossType.LeftDown;
            }
            return CrossType.None;
        }

        /// <summary>
        /// 从左检查交叉点
        /// </summary>
        /// <param name="width"></param>
        /// <param name="imageBytes"></param>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <returns>0表示非交叉点，1,2,3,4分别表示左上右下T字，5,6,7,8分别表示从左上角开始顺时针方向的角。所以在这里可能会有0,1两种情况</returns>
        private CrossType GetCrossFromLeft(int width, byte[] imageBytes, int x, int y)
        {
            bool toRight = IsLineOnDirection(width, imageBytes, x, y, 1, 0);
            if (toRight == false)
            {
                return CrossType.None;
            }
            bool toLeft = IsLineOnDirection(width, imageBytes, x, y, -1, 0);
            if (toLeft)
            {
                return CrossType.None;
            }
            bool toUp = IsLineOnDirection(width, imageBytes, x, y, 0, -1);
            if (toUp == false)
            {
                return CrossType.None;
            }
            bool toDown = IsLineOnDirection(width, imageBytes, x, y, 0, 1);
            if (toDown == false)
            {
                return CrossType.None;
            }
            return CrossType.Left;
        }

        /// <summary>
        /// 从右检查交叉点
        /// </summary>
        /// <param name="width"></param>
        /// <param name="imageBytes"></param>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <returns>0表示非交叉点，1,2,3,4分别表示左上右下T字，5,6,7,8分别表示从左上角开始顺时针方向的角。所以在这里可能会有0,3两种情况</returns>
        private CrossType GetCrossFromRight(int width, byte[] imageBytes, int x, int y)
        {
            bool toLeft = IsLineOnDirection(width, imageBytes, x, y, -1, 0);
            if (toLeft == false)
            {
                return CrossType.None;
            }
            bool toRight = IsLineOnDirection(width, imageBytes, x, y, 1, 0);
            if (toRight)
            {
                return CrossType.None;
            }
            bool toUp = IsLineOnDirection(width, imageBytes, x, y, 0, -1);
            if (toUp == false)
            {
                return CrossType.None;
            }
            bool toDown = IsLineOnDirection(width, imageBytes, x, y, 0, 1);
            if (toDown == false)
            {
                return CrossType.None;
            }
            return CrossType.Right;
        }

        private CrossType GetCrossFromCenter(int width, byte[] imageBytes, int x, int y)
        {
            bool toLeft = IsLineOnDirection(width, imageBytes, x, y, -1, 0);
            if (toLeft == false)
            {
                return CrossType.None;
            }
            bool toRight = IsLineOnDirection(width, imageBytes, x, y, 1, 0);
            if (toRight == false)
            {
                return CrossType.None;
            }
            bool toUp = IsLineOnDirection(width, imageBytes, x, y, 0, -1);
            if (toUp == false)
            {
                return CrossType.None;
            }
            bool toDown = IsLineOnDirection(width, imageBytes, x, y, 0, 1);
            if (toDown == false)
            {
                return CrossType.None;
            }
            return CrossType.Center;
        }


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
        private bool IsLineOnDirection(int width, byte[] imageBytes, int x, int y, int directionX, int directionY)
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
                    jMax = crossDetectLen;
                }
                else
                {
                    jMin = -crossDetectLen;
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
                    iMax = crossDetectLen;
                }
                else
                {
                    iMin = -crossDetectLen;
                    iMax = -1;
                }
            }

            byte[] whiteBytes = new byte[5 * crossDetectLen];
            int index = 0;
            for (int i = iMin; i <= iMax; i++)
            {
                for (int j = jMin; j <= jMax; j++)
                {
                    if (x + i + (y + j) * width >= 0 && x + i + (y + j) * width < imageBytes.Length)
                    {
                        whiteBytes[index++] = imageBytes[x + i + (y + j) * width];
                    }
                }
            }
            int whiteCount = whiteBytes.Count(b => b == 255);
            rate = (double)whiteCount / whiteBytes.Length;
            return whiteCount > whiteBytes.Length * crossFillRate;
            //}
            //catch (Exception ex)
            //{
            //    throw ex;
            //}
        }

        private void AddPoint(Dictionary<CrossType, List<Point>> crossDic, CrossType type, int x, int y)
        {
            if (crossDic.Keys.Contains(type))
            {
                crossDic[type].Add(new Point(x, y));
            }
            else
            {
                List<Point> pList = new List<Point>();
                pList.Add(new Point(x, y));
                crossDic.Add(type, pList);
            }
        }

        public enum CrossType
        {
            None = 0, Left, Up, Right, Down, LeftUp, RightUp, RightDown, LeftDown, Center
        }

        #region canny和直线图

        /// <summary>
        /// 找到棋盘四个顶点
        /// 因为如果一边偏宽，那么它会使它两边的格子不均匀，所以顶点如果不方正，后面的找全部坐标考虑矫正，如果矫正效果不好，考虑放弃不方正的顶角。（待定）
        /// </summary>
        /// <returns>从左上角顺时针的点</returns>
        private PointF[] FindConor(out PointF directionLeft, out PointF directionRight, out PointF directionUp, out PointF directionDown)
        {
            directionLeft = PointF.Empty;
            directionRight = PointF.Empty;
            directionUp = PointF.Empty;
            directionDown = PointF.Empty;
            List<PointF> leftPoints = new List<PointF>();
            if (CrossPoints.Keys.Contains(CrossType.Left))
            {
                foreach (var item in CrossPoints[CrossType.Left])
                {
                    leftPoints.Add(new PointF(item.X, item.Y));
                }
            }
            if (leftPoints.Count < 2)
            {
                return null;
            }
            PointF pointOnLineLeft;
            LineMethods.LineFit(leftPoints.ToArray(), out directionLeft, out pointOnLineLeft);

            List<PointF> rightPoints = new List<PointF>();
            if (CrossPoints.Keys.Contains(CrossType.Right))
            {
                foreach (var item in CrossPoints[CrossType.Right])
                {
                    rightPoints.Add(new PointF(item.X, item.Y));
                }
            }
            if (rightPoints.Count < 2)
            {
                return null;
            }
            PointF pointOnLineRight;
            LineMethods.LineFit(rightPoints.ToArray(), out directionRight, out pointOnLineRight);

            List<PointF> upPoints = new List<PointF>();
            if (CrossPoints.Keys.Contains(CrossType.Up))
            {
                foreach (var item in CrossPoints[CrossType.Up])
                {
                    upPoints.Add(new PointF(item.X, item.Y));
                }
            }
            if (CrossPoints.Keys.Contains(CrossType.LeftUp))
            {
                foreach (var item in CrossPoints[CrossType.LeftUp])
                {
                    upPoints.Add(new PointF(item.X, item.Y));
                }
            }
            if (CrossPoints.Keys.Contains(CrossType.RightUp))
            {
                foreach (var item in CrossPoints[CrossType.RightUp])
                {
                    upPoints.Add(new PointF(item.X, item.Y));
                }
            }
            if (upPoints.Count < 2)
            {
                return null;
            }

            PointF pointOnLineUp;
            LineMethods.LineFit(upPoints.ToArray(), out directionUp, out pointOnLineUp);

            List<PointF> downPoints = new List<PointF>();
            if (CrossPoints.Keys.Contains(CrossType.Down))
            {
                foreach (var item in CrossPoints[CrossType.Down])
                {
                    downPoints.Add(new PointF(item.X, item.Y));
                }
            }
            if (CrossPoints.Keys.Contains(CrossType.LeftDown))
            {
                foreach (var item in CrossPoints[CrossType.LeftDown])
                {
                    downPoints.Add(new PointF(item.X, item.Y));
                }
            }
            if (CrossPoints.Keys.Contains(CrossType.RightDown))
            {
                foreach (var item in CrossPoints[CrossType.RightDown])
                {
                    downPoints.Add(new PointF(item.X, item.Y));
                }
            }
            if (downPoints.Count < 2)
            {
                return null;
            }

            PointF pointOnLineDown;
            LineMethods.LineFit(downPoints.ToArray(), out directionDown, out pointOnLineDown);//拟合

            //求交点
            PointF? leftTop = LineMethods.FindLineCross(directionLeft, pointOnLineLeft, directionUp, pointOnLineUp);
            PointF? rightTop = LineMethods.FindLineCross(directionRight, pointOnLineRight, directionUp, pointOnLineUp);
            PointF? leftDown = LineMethods.FindLineCross(directionLeft, pointOnLineLeft, directionDown, pointOnLineDown);
            PointF? rightDown = LineMethods.FindLineCross(directionRight, pointOnLineRight, directionDown, pointOnLineDown);
            if (leftTop == null || rightTop == null || leftDown == null || rightDown == null)
            {
                return null;
            }
            PointF[] result = new PointF[] { leftTop.Value, rightTop.Value, rightDown.Value, leftDown.Value };
            return result;
        }

        //通过四个角，矫正获得等分线
        private void GetEvenDevidePoint(PointF[] conors, PointF directionLeft, PointF directionRight, PointF directionUp, PointF directionDown, out LineSegment2DF[] horizontalLines, out LineSegment2DF[] verticalLines)
        {
            PointF leftUpPoint = conors[0];
            PointF rightUpPoint = conors[1];
            PointF rightDownPoint = conors[2];
            PointF leftDownPoint = conors[3];

            PointF[] upPoints = null;
            PointF[] downPoints = null;
            PointF[] leftPoints = null;
            PointF[] rightPoints = null;

            horizontalLines = null;
            verticalLines = null;
            //分三种情况：1、都不平行。2、有一对平行。3、都平行
            //平行的判定条件，tanα <0.005
            float parallelAngle = 0.005f;

            float tanUp = directionUp.Y / directionUp.X; //已考虑除数不会为零
            float tanDown = directionDown.Y / directionDown.X;
            bool horizontalParallel = Math.Abs((tanUp - tanDown) / (1 + tanUp * tanDown)) < parallelAngle; //判断上下是否平行，根据三角公式tan(a-b)

            float tanLeft = directionLeft.X / directionLeft.Y;
            float tanRight = directionRight.X / directionRight.Y;
            bool verticalParallel = Math.Abs((tanLeft - tanRight) / (1 + tanLeft * tanRight)) < parallelAngle;  //判断左右是否平行

            if (horizontalParallel)
            {
                upPoints = new PointF[boardSize];
                for (int i = 0; i < boardSize; i++)
                {
                    leftPoints[i] = new PointF();
                    leftPoints[i].X = leftUpPoint.X + (rightUpPoint.X - leftUpPoint.X) * i / (boardSize - 1);
                    leftPoints[i].Y = leftUpPoint.Y + (rightUpPoint.Y - leftUpPoint.Y) * i / (boardSize - 1);
                }
                downPoints = new PointF[boardSize];
                for (int i = 0; i < boardSize; i++)
                {
                    rightPoints[i] = new PointF();
                    rightPoints[i].X = leftDownPoint.X + (rightDownPoint.X - leftDownPoint.X) * i / (boardSize - 1);
                    rightPoints[i].Y = leftDownPoint.Y + (rightDownPoint.Y - leftDownPoint.Y) * i / (boardSize - 1);
                }
                verticalLines = new LineSegment2DF[boardSize];
                for (int i = 0; i < boardSize; i++)
                {
                    verticalLines[i] = new LineSegment2DF(upPoints[i], downPoints[i]);
                }
            }
            if (verticalParallel)
            {
                leftPoints = new PointF[boardSize];
                for (int i = 0; i < boardSize; i++)
                {
                    leftPoints[i] = new PointF();
                    leftPoints[i].X = leftUpPoint.X + (leftDownPoint.X - leftUpPoint.X) * i / (boardSize - 1);
                    leftPoints[i].Y = leftUpPoint.Y + (leftDownPoint.Y - leftUpPoint.Y) * i / (boardSize - 1);
                }
                rightPoints = new PointF[boardSize];
                for (int i = 0; i < boardSize; i++)
                {
                    rightPoints[i] = new PointF();
                    rightPoints[i].X = rightUpPoint.X + (rightDownPoint.X - rightUpPoint.X) * i / (boardSize - 1);
                    rightPoints[i].Y = rightUpPoint.Y + (rightDownPoint.Y - rightUpPoint.Y) * i / (boardSize - 1);
                }
                horizontalLines = new LineSegment2DF[boardSize];
                for (int i = 0; i < boardSize; i++)
                {
                    horizontalLines[i] = new LineSegment2DF(leftPoints[i], rightPoints[i]);
                }
            }
            if (horizontalParallel && !verticalParallel)
            {
                leftPoints = new PointF[boardSize];
                rightPoints = new PointF[boardSize];
                leftPoints[0] = leftUpPoint;
                leftPoints[boardSize - 1] = leftDownPoint;
                rightPoints[0] = rightUpPoint;
                rightPoints[boardSize - 1] = rightDownPoint;
                horizontalLines = new LineSegment2DF[boardSize];
                horizontalLines[0] = new LineSegment2DF(leftPoints[0], rightPoints[0]);//顶线
                horizontalLines[boardSize - 1] = new LineSegment2DF(leftPoints[boardSize - 1], rightPoints[boardSize - 1]);//底线
                //渐进作对角线，这样点在两边，会准确一点。如果boardsize(n)是偶数，n/2成为下部的第一条线，如果是奇数，n/2为中线
                //从上面画线时，都从1画到n/2-1，从下面画线时，从n-2画到n/2。当奇偶不同时，从下划线条数不同，但代码一致。
                for (int i = 0; i < boardSize / 2 - 1; i++)
                {
                    //从左上和右上开始，往对边画对角线，然后求平行的第1、2...n/2-1条线
                    LineSegment2DF diagonalLineUp1 = new LineSegment2DF(leftPoints[i], downPoints[boardSize - i]);
                    LineSegment2DF diagonalLineUp2 = new LineSegment2DF(rightPoints[i], downPoints[i]);
                    //和第1,n-2条垂直线交点
                    PointF pLeft = (PointF)LineMethods.FindLineCross(diagonalLineUp1.Direction, diagonalLineUp1.P1, verticalLines[1].Direction, verticalLines[1].P1);
                    PointF pRight = (PointF)LineMethods.FindLineCross(diagonalLineUp2.Direction, diagonalLineUp2.P1, verticalLines[boardSize - 2].Direction, verticalLines[1].P1);
                    horizontalLines[i + 1] = new LineSegment2DF(pLeft, pRight);
                    leftPoints[i + 1] = (PointF)LineMethods.FindLineCross(horizontalLines[i + 1].Direction, pLeft, verticalLines[0].Direction, verticalLines[0].P1);
                    rightPoints[i + 1] = (PointF)LineMethods.FindLineCross(horizontalLines[i + 1].Direction, pRight, verticalLines[boardSize - 1].Direction, verticalLines[boardSize - 1].P1);
                }
                for (int i = boardSize - 1; i > boardSize / 2; i--)
                {
                    //从左下和右下开始，往对边画对角线，然后求平行的第n-2、n-3..n/2条线
                    LineSegment2DF diagonalLineDown1 = new LineSegment2DF(leftPoints[i], upPoints[i]);
                    LineSegment2DF diagonalLineDown2 = new LineSegment2DF(rightPoints[i], upPoints[boardSize - 1 - i]);
                    //和第1,n-2条垂直线交点
                    PointF pLeft = (PointF)LineMethods.FindLineCross(diagonalLineDown1.Direction, diagonalLineDown1.P1, verticalLines[1].Direction, verticalLines[1].P1);
                    PointF pRight = (PointF)LineMethods.FindLineCross(diagonalLineDown2.Direction, diagonalLineDown2.P1, verticalLines[boardSize - 2].Direction, verticalLines[1].P1);
                    horizontalLines[i - 1] = new LineSegment2DF(pLeft, pRight);
                    leftPoints[i - 1] = (PointF)LineMethods.FindLineCross(horizontalLines[i - 1].Direction, pLeft, verticalLines[0].Direction, verticalLines[0].P1);
                    rightPoints[i - 1] = (PointF)LineMethods.FindLineCross(horizontalLines[i - 1].Direction, pRight, verticalLines[boardSize - 1].Direction, verticalLines[boardSize - 1].P1);
                }
            }
            if (verticalParallel && !horizontalParallel)
            {
                upPoints = new PointF[boardSize];
                downPoints = new PointF[boardSize];
                upPoints[0] = leftUpPoint;
                upPoints[boardSize - 1] = rightUpPoint;
                downPoints[0] = leftDownPoint;
                downPoints[boardSize - 1] = rightDownPoint;
                verticalLines = new LineSegment2DF[boardSize];
                verticalLines[0] = new LineSegment2DF(upPoints[0], downPoints[0]);//左线
                verticalLines[boardSize - 1] = new LineSegment2DF(upPoints[boardSize - 1], downPoints[boardSize - 1]);//右线
                //渐进作对角线，这样点在两边，会准确一点。如果boardsize(n)是偶数，n/2成为下部的第一条线，如果是奇数，n/2为中线
                //从上面画线时，都从1画到n/2-1，从下面画线时，从n-2画到n/2。当奇偶不同时，从下划线条数不同，但代码一致。
                for (int i = 0; i < boardSize / 2 - 1; i++)
                {
                    //从左上和右上开始，往对边画对角线，然后求平行的第1、2...n/2-1条线
                    LineSegment2DF diagonalLineLeft1 = new LineSegment2DF(upPoints[i], rightPoints[boardSize - i]);
                    LineSegment2DF diagonalLineLeft2 = new LineSegment2DF(downPoints[i], rightPoints[i]);
                    //和第1,n-2条平行线交点
                    PointF pUp = (PointF)LineMethods.FindLineCross(diagonalLineLeft1.Direction, diagonalLineLeft1.P1, horizontalLines[1].Direction, horizontalLines[1].P1);
                    PointF pDown = (PointF)LineMethods.FindLineCross(diagonalLineLeft2.Direction, diagonalLineLeft2.P1, horizontalLines[boardSize - 2].Direction, horizontalLines[1].P1);
                    verticalLines[i + 1] = new LineSegment2DF(pUp, pDown);
                    upPoints[i + 1] = (PointF)LineMethods.FindLineCross(verticalLines[i + 1].Direction, pUp, horizontalLines[0].Direction, horizontalLines[0].P1);
                    downPoints[i + 1] = (PointF)LineMethods.FindLineCross(verticalLines[i + 1].Direction, pDown, horizontalLines[boardSize - 1].Direction, horizontalLines[boardSize - 1].P1);
                }
                for (int i = boardSize - 1; i > boardSize / 2; i--)
                {
                    //从左下和右下开始，往对边画对角线，然后求平行的第n-2、n-3..n/2条线
                    LineSegment2DF diagonalLineRight1 = new LineSegment2DF(upPoints[i], leftPoints[i]);
                    LineSegment2DF diagonalLineRight2 = new LineSegment2DF(downPoints[i], leftPoints[boardSize - 1 - i]);
                    //和第1,n-2条垂直线交点
                    PointF pUp = (PointF)LineMethods.FindLineCross(diagonalLineRight1.Direction, diagonalLineRight1.P1, horizontalLines[1].Direction, horizontalLines[1].P1);
                    PointF pDown = (PointF)LineMethods.FindLineCross(diagonalLineRight2.Direction, diagonalLineRight2.P1, horizontalLines[boardSize - 2].Direction, horizontalLines[1].P1);
                    verticalLines[i - 1] = new LineSegment2DF(pUp, pDown);
                    upPoints[i - 1] = (PointF)LineMethods.FindLineCross(verticalLines[i - 1].Direction, pUp, horizontalLines[0].Direction, horizontalLines[0].P1);
                    downPoints[i - 1] = (PointF)LineMethods.FindLineCross(verticalLines[i - 1].Direction, pDown, horizontalLines[boardSize - 1].Direction, horizontalLines[boardSize - 1].P1);
                }
            }
            if (!horizontalParallel && !verticalParallel)
            {
                //1.寻找两个边的交点，及上平行线
                PointF horizontalCross = (PointF)LineMethods.FindLineCross(directionUp, leftUpPoint, directionDown, leftDownPoint);  //水平相交点
                PointF verticalCross = (PointF)LineMethods.FindLineCross(directionLeft, leftUpPoint, directionRight, rightUpPoint);  //垂直相交点
                LineSegment2DF l1 = new LineSegment2DF(horizontalCross, verticalCross);
                //2、过任意一点作l1的平行线 （找leftUpPoint和距leftUpPoint距离为1000的一点）
                PointF pointOnL2 = new PointF(leftUpPoint.X + 1000 * l1.Direction.X, leftUpPoint.Y + 1000 * l1.Direction.Y);
                LineSegment2DF l2 = new LineSegment2DF(leftUpPoint, pointOnL2);
                //TODO:
                //3、让两边交于l2，并平分
                //4、作平分点和顶点交点连线，这些连线就是中间的格子线。
            }
        }


        private Point[] CalculateAllCoordinate(PointF[] conors)
        {
            PointF leftTop = conors[0];
            PointF rightTop = conors[1];
            PointF rightDown = conors[2];
            PointF leftDown = conors[3];

            //先获得左右的等分
            PointF[] lefts = new PointF[boardSize];
            for (int i = 0; i < boardSize; i++)
            {
                lefts[i] = new PointF();
                lefts[i].X = leftTop.X + (leftDown.X - leftTop.X) * i / (boardSize - 1);
                lefts[i].Y = leftTop.Y + (leftDown.Y - leftTop.Y) * i / (boardSize - 1);
            }
            PointF[] rights = new PointF[boardSize];
            for (int i = 0; i < boardSize; i++)
            {
                rights[i] = new PointF();
                rights[i].X = rightTop.X + (rightDown.X - rightTop.X) * i / (boardSize - 1);
                rights[i].Y = rightTop.Y + (rightDown.Y - rightTop.Y) * i / (boardSize - 1);
            }

            //求所有
            Point[] coordinates = new Point[boardSize * boardSize];
            for (int i = 0; i < boardSize; i++)
            {
                for (int j = 0; j < boardSize; j++)
                {
                    coordinates[i + j * boardSize] = new Point()
                    {
                        X = (int)(lefts[j].X + (rights[j].X - lefts[j].X) * i / (boardSize - 1)) + 1,//因为检测的时候都偏小，这里补偿1像素
                        Y = (int)(lefts[j].Y + (rights[j].Y - lefts[j].Y) * i / (boardSize - 1)) + 1,//因为检测的时候都偏小，这里补偿1像素
                    };
                }
            }
            return coordinates;
        }

        /// <summary>
        /// 找棋子
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <returns>0:empty，1:black，2:white，-1:错误</returns>
        private int FindStone(int x, int y)
        {
            byte[] imageByte = cannyEdges.Bytes;//如果直接随机访问cannyEdges.Bytes中的元素，会非常耗时
            Console.Write("FindStone    ");
            //下面三种方法，可信度从高到低
            #region 先找xy附近25%最大格宽是否有圆，然后圆内的灰度大于平均值（这里简单认为是128，或0.5）则是黑棋，小于则是白棋
            {
                int minX = x - maxGridWidth / 4;
                minX = minX < 0 ? 0 : minX;
                int maxX = x + maxGridWidth / 4;
                maxX = maxX >= imageSize ? imageSize - 1 : maxX;

                int minY = x - maxGridWidth / 4;
                minY = minY < 0 ? 0 : minY;
                int maxY = x + maxGridWidth / 4;
                maxY = maxY >= imageSize ? imageSize - 1 : maxY;

                CircleF circleStone = new CircleF(PointF.Empty, 0);
                foreach (var circle in Circles)
                {
                    if (circle.Center.X >= minX && circle.Center.X <= maxX && circle.Center.Y >= minY && circle.Center.Y <= maxY)
                    {
                        circleStone = circle;
                        break;
                    }
                }
                if (circleStone.Radius == 0)
                {
                    //或者以0.25倍最小格宽为半径的圆，百分之99（数值待定）都是黑色，判定为有圆
                    int totalCannyCount = 0;
                    int blackCount = 0;
                    float littleRadius = 0.25f * minGridWidth;
                    for (int i = (int)(-littleRadius) + 1; i < littleRadius; i++)
                    {
                        for (int j = (int)(-littleRadius) + 1; j < littleRadius; j++)
                        {
                            if (i * i + j * j < littleRadius * littleRadius)
                            {
                                totalCannyCount++;
                                blackCount += imageByte[x + i + (y + j) * imageSize] == 0 ? 1 : 0;
                            }
                        }
                    }
                    if ((float)blackCount / totalCannyCount >= 0.99)
                    {
                        circleStone = new CircleF(new PointF(x, y), littleRadius);
                    }
                }

                if (circleStone.Radius != 0)
                {
                    //求圆内灰度
                    float totalGray = 0;
                    int totalCount = 0;
                    for (int i = (int)(circleStone.Center.X - circleStone.Radius) + 1; i < circleStone.Center.X + circleStone.Radius; i++)
                    {
                        for (int j = (int)(circleStone.Center.Y - circleStone.Radius) + 1; j < circleStone.Center.Y + circleStone.Radius; j++)
                        {
                            if ((i - circleStone.Center.X) * (i - circleStone.Center.X) + (j - circleStone.Center.Y) * (j - circleStone.Center.Y) < circleStone.Radius * circleStone.Radius)
                            {
                                totalGray += bitmap.GetPixel(i, j).GetBrightness();
                                totalCount++;
                            }
                        }
                    }
                    float averageGray = totalGray / totalCount;
                    if (averageGray > 0.5)
                    {
                        return 2;//白
                    }
                    else if (averageGray < 0.5)
                    {
                        return 1;//黑
                    }
                    else
                    {
                        Console.WriteLine("错误");
                        return -1;//错误
                    }
                }
            }
            #endregion

            #region 再找xy附近10%最大格宽是否有十字，如果有，则为空
            {
                Console.WriteLine("找十字    ");

                int minX = x - maxGridWidth / 10;
                minX = minX < 0 ? 0 : minX;
                int maxX = x + maxGridWidth / 10;
                maxX = maxX >= imageSize ? imageSize - 1 : maxX;

                int minY = x - maxGridWidth / 10;
                minY = minY < 0 ? 0 : minY;
                int maxY = x + maxGridWidth / 10;
                maxY = maxY >= imageSize ? imageSize - 1 : maxY;

                for (int i = minX; i <= maxX; i++)
                {
                    for (int j = minY; j <= maxY; j++)
                    {
                        CrossType type = GetCrossFromCenter(imageSize, imageByte, i, j);
                        if (type == CrossType.Center)
                        {
                            return 0;//空
                        }
                    }
                }
            }
            #endregion

            #region 如果既无圆也无十字，最后对0.4倍最小格宽为半径的圆求灰度，如果灰度<0.2或者>0.75（数值待定），之间则为空
            {
                Console.WriteLine("最后小圆    ");
                float totalGray = 0;
                int totalCount = 0;
                float littleRadius = 0.4f * minGridWidth;

                for (int i = (int)(-littleRadius) + 1; i < littleRadius; i++)
                {
                    for (int j = (int)(-littleRadius) + 1; j < littleRadius; j++)
                    {
                        if (i * i + j * j < littleRadius * littleRadius)
                        {
                            totalCount++;
                            totalGray += bitmap.GetPixel(x + i, y + j).GetBrightness();
                        }
                    }
                }
                float averageGray = totalGray / totalCount;
                if (averageGray > 0.75)
                {
                    return 2;//白
                }
                else if (averageGray < 0.2)
                {
                    return 1;//黑
                }
                else
                {
                    return 0;//空
                }
            }
            #endregion
        }
        #endregion

        #region 验证
        double rate;
        /// <summary>
        ///  验证十字
        /// </summary>
        /// <param name="x">坐标x</param>
        /// <param name="y">坐标y</param>
        /// <returns>左上右下的比例</returns>
        public double[] CheckCross(int x, int y)
        {
            if (x < crossDetectLen || x >= cannyEdges.Cols - crossDetectLen || y < crossDetectLen || y >= cannyEdges.Rows - crossDetectLen)
            {
                return null;
            }
            double[] rates = new double[4];
            //左
            IsLineOnDirection(cannyEdges.Cols, cannyEdges.Bytes, x, y, -1, 0);
            rates[0] = rate;
            //上
            IsLineOnDirection(cannyEdges.Cols, cannyEdges.Bytes, x, y, 0, -1);
            rates[1] = rate;
            //右
            IsLineOnDirection(cannyEdges.Cols, cannyEdges.Bytes, x, y, 1, 0);
            rates[2] = rate;
            //下
            IsLineOnDirection(cannyEdges.Cols, cannyEdges.Bytes, x, y, 0, 1);
            rates[3] = rate;
            return rates;
        }

        #endregion

        #region 弃用
        private List<Point> FindCross_Old(byte[] imageBytes, int width, int height)
        {
            List<Point> crossList = new List<Point>();
            //byte[] imageBytes = (byte[])uimage.Data;

            //IntPtr ptr = uimage.DataPointer;
            //int size = uimage.Width * uimage.Height;
            //byte[] imageBytes = new byte[size];
            //Marshal.Copy(ptr, imageBytes, 0, size);

            //int height = uimage.Height;
            //int width = uimage.Width;
            //TODO:跳过一些不用查找的点
            //因为使用边缘来处理，所以不管线宽是多少，这里统一用一边为3*12像素的十字来处理
            for (int i = 12; i < width - 12; i++)
            {
                for (int j = 12; j < height - 12; j++)
                {
                    if (imageBytes[i + j * width] == 255)
                    {
                        if (IsCross(width, imageBytes, i, j))
                        {
                            crossList.Add(new Point(i, j));
                            //并将右下角的12*12设为0，避免重复检查
                            for (int ii = 0; ii < 10; ii++)
                            {
                                for (int jj = 0; jj < 50; jj++)
                                {
                                    imageBytes[i + ii + (j + jj) * width] = 0;
                                }
                            }
                            for (int ii = 10; ii < 50; ii++)
                            {
                                for (int jj = 0; jj < 10; jj++)
                                {
                                    imageBytes[i + ii + (j + jj) * width] = 0;
                                }
                            }
                        }
                    }
                }
            }
            return crossList;
        }

        private bool IsCross(int width, byte[] imageBytes, int x, int y)
        {
            //IntPtr ptr = uimage.DataPointer;
            //int size = uimage.Width * uimage.Height;
            //byte[] imageBytes = new byte[size];
            //Marshal.Copy(ptr, imageBytes, 0, size);
            try
            {
                //因为使用边缘来处理，所以不管线宽是多少，这里统一用一边为3*12像素的十字来处理
                //byte[] whiteBytes = new byte[3 * (22 + 22 + 3)];//141
                byte[] whiteBytes = new byte[141];//141
                int index = 0;
                //横排
                for (int i = -12; i <= 12; i++)
                {
                    for (int j = -1; j <= 1; j++)
                    {
                        whiteBytes[index++] = imageBytes[x + i + (y + j) * width];// uimage.GetData(new int[] { x + i, y + j })[0];
                    }
                }
                //竖排
                for (int i = -1; i <= 1; i++)
                {
                    for (int j = -12; j <= 12; j++)
                    {
                        if (j >= -1 && j <= 1) { continue; }
                        whiteBytes[index++] = imageBytes[x + i + (y + j) * width];
                    }
                }
                int whiteCount = whiteBytes.Count(b => b == 255);
                //达到百分之30，即通过
                bool hasHoriAndVerti = whiteCount > whiteBytes.Length * 0.3;

                ////计算45度斜边 1*12
                //byte[] blackBytes = new byte[12 * 4];//48
                //int index45 = 0;
                //for (int i = -12; i <= 12; i++)
                //{
                //    if (i == 0) continue;
                //    blackBytes[index45++] = imageBytes[x + i + (y + i) * width];
                //    blackBytes[index45++] = imageBytes[x + i + (y - i) * width];
                //}
                ////大于92%通过
                //int blackCount = blackBytes.Count(b => b == 0);
                //bool not45 = blackCount > blackBytes.Length * 0.92;
                //return hasHoriAndVerti && not45;
                return hasHoriAndVerti;
            }
            catch (Exception ex)
            {
                throw ex;
            }
        }

        private List<Point> CalculateCross(List<LineSegment2D> horizontalLines, List<LineSegment2D> verticalLines)
        {
            //去掉重复的线 TODO：这里就直接简单粗暴的认为5%内是同一根线，并取其中一根就可以，以后改进
            List<LineSegment2D> distinctHorLines = new List<LineSegment2D>();
            List<LineSegment2D> distinctVerLines = new List<LineSegment2D>();
            foreach (var line in horizontalLines)
            {
                if (!distinctHorLines.Contains(line, new LineEqualityComparer<LineSegment2D>(minGridWidth * 0.05)))
                {
                    distinctHorLines.Add(line);
                }
            }
            foreach (var line in verticalLines)
            {
                if (!distinctVerLines.Contains(line, new LineEqualityComparer<LineSegment2D>(minGridWidth * 0.05)))
                {
                    distinctVerLines.Add(line);
                }
            }
            //计算交叉
            List<Point> crossPointList = new List<Point>();
            foreach (var horLine in distinctHorLines)
            {
                foreach (var verLine in distinctVerLines)
                {
                    PointF crossCenter = GetIntersection(horLine.P1, horLine.P2, verLine.P1, verLine.P2);
                    if (!crossCenter.Equals(PointF.Empty))
                    {
                        crossPointList.Add(new Point((int)crossCenter.X, (int)crossCenter.Y));
                    }
                }
            }
            return crossPointList;
        }
        #endregion

        #region 检测直线相关方法 弃用
        /// <summary>
        /// 检测线
        /// </summary>
        /// <param name="uimage"></param>
        /// <param name="boardSize"></param>
        /// <param name="horizontalLines"></param>
        /// <param name="verticalLines"></param>
        private void DetectLine(UMat uimage, int boardSize, out List<LineSegment2D> horizontalLines, out List<LineSegment2D> verticalLines)
        {
            UMat cannyEdges = new UMat();
            //第三、四个参数分别为边缘检测阈值和连接阈值（大于第一个作为边界，小于第二个舍弃，介于之间时看该点是否连接着其他边界点）
            CvInvoke.Canny(uimage, cannyEdges, circleCannyThresh, circleCannyThresh * 0.8);
            LineSegment2D[] lines = CvInvoke.HoughLinesP(
               cannyEdges,
               1, //像素分辨率，取1
               Math.PI / 180, //角度分辨率，取1度
               (int)(uimage.Size.Width / (boardSize - 1) * minWidthRate * 0.8), //最小像素累计值，取最小线长*0.8
               uimage.Size.Width / (boardSize - 1) * minWidthRate, //最小线长，取最小的棋子直径（如果一行当中只有一个空点，并且该点被左右两个棋子挤到不剩最小直径，有可能检测不到）
               2); //找连续线，不允许间距，暂时允许2像素间距

            //这里假设线的角度在14度以内，即边长1:4（已经很夸张了）
            horizontalLines = new List<LineSegment2D>();
            verticalLines = new List<LineSegment2D>();
            foreach (var line in lines)
            {
                if (line.Direction.Y == 0 || Math.Abs(line.Direction.X / line.Direction.Y) > 4)
                {
                    horizontalLines.Add(line);
                }
                else if (line.Direction.X == 0 || Math.Abs(line.Direction.Y / line.Direction.X) > 4)
                {
                    verticalLines.Add(line);
                }
            }
        }


        double determinant(double v1, double v2, double v3, double v4)  // 行列式
        {
            return (v1 * v3 - v2 * v4);
        }

        //判断两条线是否相交
        bool IsIntersect(Point aP1, Point aP2, Point bP1, Point bP2)
        {
            double delta = determinant(aP2.X - aP1.X, bP1.X - bP2.X, aP2.Y - aP1.Y, bP1.Y - bP2.Y);
            if (delta <= (1e-6) && delta >= -(1e-6))  // delta=0，表示两线段重合或平行
            {
                return false;
            }
            double namenda = determinant(bP1.X - aP1.X, bP1.X - bP2.X, bP1.Y - aP1.Y, bP1.Y - bP2.Y) / delta;
            if (namenda > 1 || namenda < 0)
            {
                return false;
            }
            double miu = determinant(aP2.X - aP1.X, bP1.X - aP1.X, aP2.Y - aP1.Y, bP1.Y - aP1.Y) / delta;
            if (miu > 1 || miu < 0)
            {
                return false;
            }
            return true;
        }

        /// <summary>
        /// 计算两条直线的交点
        /// </summary>
        /// <param name="lineFirstStar">L1的点1坐标</param>
        /// <param name="lineFirstEnd">L1的点2坐标</param>
        /// <param name="lineSecondStar">L2的点1坐标</param>
        /// <param name="lineSecondEnd">L2的点2坐标</param>
        /// <returns></returns>
        public PointF GetIntersection(PointF lineFirstStar, PointF lineFirstEnd, PointF lineSecondStar, PointF lineSecondEnd)
        {
            /*
             * L1，L2都存在斜率的情况：
             * 直线方程L1: ( y - y1 ) / ( y2 - y1 ) = ( x - x1 ) / ( x2 - x1 ) 
             * => y = [ ( y2 - y1 ) / ( x2 - x1 ) ]( x - x1 ) + y1
             * 令 a = ( y2 - y1 ) / ( x2 - x1 )
             * 有 y = a * x - a * x1 + y1   .........1
             * 直线方程L2: ( y - y3 ) / ( y4 - y3 ) = ( x - x3 ) / ( x4 - x3 )
             * 令 b = ( y4 - y3 ) / ( x4 - x3 )
             * 有 y = b * x - b * x3 + y3 ..........2
             * 
             * 如果 a = b，则两直线平等，否则， 联解方程 1,2，得:
             * x = ( a * x1 - b * x3 - y1 + y3 ) / ( a - b )
             * y = a * x - a * x1 + y1
             * 
             * L1存在斜率, L2平行Y轴的情况：
             * x = x3
             * y = a * x3 - a * x1 + y1
             * 
             * L1 平行Y轴，L2存在斜率的情况：
             * x = x1
             * y = b * x - b * x3 + y3
             * 
             * L1与L2都平行Y轴的情况：
             * 如果 x1 = x3，那么L1与L2重合，否则平等
             * 
            */
            float a = 0, b = 0;
            int state = 0;
            if (lineFirstStar.X != lineFirstEnd.X)
            {
                a = (lineFirstEnd.Y - lineFirstStar.Y) / (lineFirstEnd.X - lineFirstStar.X);
                state |= 1;
            }
            if (lineSecondStar.X != lineSecondEnd.X)
            {
                b = (lineSecondEnd.Y - lineSecondStar.Y) / (lineSecondEnd.X - lineSecondStar.X);
                state |= 2;
            }
            switch (state)
            {
                case 0: //L1与L2都平行Y轴
                    {
                        if (lineFirstStar.X == lineSecondStar.X)
                        {
                            //throw new Exception("两条直线互相重合，且平行于Y轴，无法计算交点。");
                            return PointF.Empty;
                        }
                        else
                        {
                            //throw new Exception("两条直线互相平行，且平行于Y轴，无法计算交点。");
                            return PointF.Empty;
                        }
                    }
                case 1: //L1存在斜率, L2平行Y轴
                    {
                        float x = lineSecondStar.X;
                        float y = (lineFirstStar.X - x) * (-a) + lineFirstStar.Y;
                        return new PointF(x, y);
                    }
                case 2: //L1 平行Y轴，L2存在斜率
                    {
                        float x = lineFirstStar.X;
                        //网上有相似代码的，这一处是错误的。你可以对比case 1 的逻辑 进行分析
                        //源code:lineSecondStar * x + lineSecondStar * lineSecondStar.X + p3.Y;
                        float y = (lineSecondStar.X - x) * (-b) + lineSecondStar.Y;
                        return new PointF(x, y);
                    }
                case 3: //L1，L2都存在斜率
                    {
                        if (a == b)
                        {
                            // throw new Exception("两条直线平行或重合，无法计算交点。");
                            return PointF.Empty;
                        }
                        float x = (a * lineFirstStar.X - b * lineSecondStar.X - lineFirstStar.Y + lineSecondStar.Y) / (a - b);
                        float y = a * x - a * lineFirstStar.X + lineFirstStar.Y;
                        return new PointF(x, y);
                    }
            }
            // throw new Exception("不可能发生的情况");
            return PointF.Empty;
        }
        class LineEqualityComparer<T> : IEqualityComparer<T> where T : struct
        {
            double minDiff;//端点最小距离
            public LineEqualityComparer(double minDiff)
            {
                this.minDiff = minDiff;
            }
            public bool Equals(T x, T y)
            {
                object objP1 = x;
                LineSegment2D p1 = (LineSegment2D)objP1;
                object objP2 = y;
                LineSegment2D p2 = (LineSegment2D)objP2;
                return ((p1.P1.X - p2.P1.X) * (p1.P1.X - p2.P1.X) + (p1.P1.Y - p2.P1.Y) * (p1.P1.Y - p2.P1.Y) < minDiff * minDiff &&
                    (p1.P2.X - p2.P2.X) * (p1.P2.X - p2.P2.X) + (p1.P2.Y - p2.P2.Y) * (p1.P2.Y - p2.P2.Y) < minDiff * minDiff);
            }

            public int GetHashCode(T obj)
            {
                return 0;
            }
        }
        #endregion
    }
}
