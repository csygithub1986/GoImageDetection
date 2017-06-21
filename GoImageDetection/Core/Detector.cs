using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using Emgu.CV;
using Emgu.CV.CvEnum;
using Emgu.CV.Structure;
using System.Collections;

namespace GoImageDetection.Core
{
    /// <summary>
    /// 检测图形算法
    /// 其中有2个假设：
    /// 1、标准棋盘为0.9375:1，这里采用正方形输入。假定棋盘每一边都在(0.7~1) *(图宽)范围内
    /// 2、棋盘交叉点误差应该小于最大格宽的正负5%；圆心坐标相差应小于最大格宽的正负25%
    /// 目前采用整个棋盘全部检测模式
    /// </summary>
    public class Detector : IDetect
    {
        double minWidthRate = 0.7;
        double cannyThreshold = 100;    // 参数5：边缘检测阈值（30~180）
        double circleAccumulatorThreshold = 30;       // 参数6：累加器阈值（圆心重合点，越低的时候圆弧就越容易当成圆）

        double dp = 1;    // 参数3：dp，不懂

        int maxGridWidth;//最大格宽
        int minGridWidth;//最小格宽

        public Detector()
        {

        }

        public CircleF[] Circles;
        public List<Point> CrossPoints;

        public int[] Detect(Bitmap bitmap, int boardSize)
        {
            UMat uimage = InitImage(bitmap);
            maxGridWidth = uimage.Size.Width / (boardSize - 1);
            minGridWidth = (int)(uimage.Size.Width / (boardSize - 1) * minWidthRate);

            Circles = DetectCircle(uimage, boardSize);

            //List<LineSegment2D> horizontalLines;
            //List<LineSegment2D> verticalLines;
            //DetectLine(uimage, boardSize, out horizontalLines, out verticalLines);
            //CrossPoints = CalculateCross(horizontalLines, verticalLines);

            UMat cannyEdges = new UMat();
            //第三、四个参数分别为边缘检测阈值和连接阈值（大于第一个作为边界，小于第二个舍弃，介于之间时看该点是否连接着其他边界点）
            CvInvoke.Canny(uimage, cannyEdges, cannyThreshold, cannyThreshold * 0.8);
            CrossPoints = FindCross(cannyEdges.GetMat(AccessType.Read));

            return null;
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
            CircleF[] circles = CvInvoke.HoughCircles(uimage, HoughType.Gradient, dp, minDistance, cannyThreshold, circleAccumulatorThreshold, minRadius, maxRadius);
            return circles;
        }

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
            CvInvoke.Canny(uimage, cannyEdges, cannyThreshold, cannyThreshold * 0.8);
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


        //判断两条线是否相交

        double determinant(double v1, double v2, double v3, double v4)  // 行列式
        {
            return (v1 * v3 - v2 * v4);
        }

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



        private List<Point> FindCross(Mat uimage)
        {
            List<Point> crossList = new List<Point>();
            //TODO:跳过一些不用查找的点
            //因为使用边缘来处理，所以不管线宽是多少，这里统一用一边为3*12像素的十字来处理
            for (int i = 12; i < uimage.Height - 12; i++)
            {
                for (int j = 12; j < uimage.Width - 12; j++)
                {
                    if (uimage.GetData(new int[] { i, j })[0] == 255)
                    {
                        if (IsCross(uimage, i, j))
                        {
                            crossList.Add(new Point(j, i));
                        }
                    }
                }
            }
            return crossList;
        }

        private bool IsCross(Mat uimage, int x, int y)
        {
            try
            {
                //因为使用边缘来处理，所以不管线宽是多少，这里统一用一边为3*12像素的十字来处理
                byte[] whiteBytes = new byte[3 * (22 + 22 + 3)];//141
                int index = 0;
                //横排
                for (int i = -12; i <= 12; i++)
                {
                    for (int j = -1; j <= 1; j++)
                    {
                        whiteBytes[index++] = uimage.GetData(new int[] { x + i, y + j })[0];
                    }
                }
                //竖排
                for (int i = -1; i <= 1; i++)
                {
                    for (int j = -12; j <= 12; j++)
                    {
                        if (j >= -1 && j <= 1) { continue; }
                        whiteBytes[index++] = uimage.GetData(new int[] { x + i, y + j })[0];
                    }
                }
                int whiteCount = whiteBytes.Count(b => b == 255);
                //达到百分之30，即通过
                bool hasHoriAndVerti = whiteCount > whiteBytes.Length * 0.3;


                //计算45度斜边 1*12
                byte[] blackBytes = new byte[12 * 4];//48
                int index45 = 0;
                for (int i = -12; i <= 12; i++)
                {
                    if (i == 0) continue;
                    blackBytes[index45++] = uimage.GetData(new int[] { x + i, y + i })[0];
                    blackBytes[index45++] = uimage.GetData(new int[] { x + i, y - i })[0];
                }
                //大于92%通过
                int blackCount = blackBytes.Count(b => b == 0);
                bool not45 = blackCount > blackBytes.Length * 0.92;
                return hasHoriAndVerti && not45;
            }
            catch (Exception ex)
            {

                throw ex;
            }
        }



        private List<Point> FindCross(UMat uimage)
        {
            byte[] imageBytes = uimage.Bytes;
            List<Point> crossList = new List<Point>();
            //TODO:跳过一些不用查找的点
            //因为使用边缘来处理，所以不管线宽是多少，这里统一用一边为3*12像素的十字来处理
            for (int i = 12; i < uimage.Cols - 12; i++)
            {
                for (int j = 12; j < uimage.Rows - 12; j++)
                {
                    if (imageBytes[i + j * uimage.Rows] == 255)
                    {
                        if (IsCross(uimage, i, j))
                        {
                            crossList.Add(new Point(j, i));
                        }
                    }
                }
            }
            return crossList;
        }

        private bool IsCross(UMat uimage, int x, int y)
        {
            try
            {
                byte[] imageBytes = uimage.Bytes;
                //因为使用边缘来处理，所以不管线宽是多少，这里统一用一边为3*12像素的十字来处理
                byte[] whiteBytes = new byte[3 * (22 + 22 + 3)];//141
                int index = 0;
                //横排
                for (int i = -12; i <= 12; i++)
                {
                    for (int j = -1; j <= 1; j++)
                    {
                        whiteBytes[index++] = imageBytes[x + i + (y + j) * uimage.Rows];
                    }
                }
                //竖排
                for (int i = -1; i <= 1; i++)
                {
                    for (int j = -12; j <= 12; j++)
                    {
                        if (j >= -1 && j <= 1) { continue; }
                        whiteBytes[index++] = imageBytes[x + i + (y + j) * uimage.Rows];
                    }
                }
                int whiteCount = whiteBytes.Count(b => b == 255);
                //达到百分之30，即通过
                bool hasHoriAndVerti = whiteCount > whiteBytes.Length * 0.3;


                //计算45度斜边 1*12
                byte[] blackBytes = new byte[12 * 4];//48
                int index45 = 0;
                for (int i = -12; i <= 12; i++)
                {
                    if (i == 0) continue;
                    blackBytes[index45++] = imageBytes[x + i + (y + i) * uimage.Rows];
                    blackBytes[index45++] = imageBytes[x + i + (y - i) * uimage.Rows];
                }
                //大于92%通过
                int blackCount = blackBytes.Count(b => b == 0);
                bool not45 = blackCount > blackBytes.Length * 0.92;
                return hasHoriAndVerti && not45;
            }
            catch (Exception ex)
            {

                throw ex;
            }
        }
    }
}
