using Emgu.CV.UI;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Forms;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Emgu.CV;
using Emgu.CV.CvEnum;
using Emgu.CV.Structure;
using System.Diagnostics;
using Emgu.CV.Util;
using GoImageDetection.Core;
using System.Drawing;
using System.Runtime.InteropServices;

namespace GoImageDetection
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {
        #region 字段
        double dp = 1;    // 参数3：dp，不懂
        double minDist = 40;     // 参数4：两个圆中心的最小距离
        double cannyThreshold = 80;    // 参数5：边缘检测阈值（30~180）
        double circleAccumulatorThreshold = 30;       // 参数6：累加器阈值（圆心重合点，越低的时候圆弧就越容易当成圆）
        int minRadius = 20;
        int maxRadius = 60;

        double lineDp = 2;
        double lineAngleReso = 180;
        int lineThreshold = 50;
        double lineMinLen = 50;
        double lineMaxGap = 10;

        double cannyThresh1 = 100;
        double cannyThresh2 = 50;
        bool cannyI2Gradient = false;

        //cross检测
        double crossFillRate = 0.2;
        #endregion

        Detector detector;
        Bitmap _bitmap;
        public MainWindow()
        {
            InitializeComponent();

            txtDp.Text = dp.ToString();
            txtCanny.Text = cannyThreshold.ToString();
            txtAccThr.Text = circleAccumulatorThreshold.ToString();
            txtMinDist.Text = minDist.ToString();
            txtMinCir.Text = minRadius.ToString();
            txtMaxCir.Text = maxRadius.ToString();
            txtLineDp.Text = lineDp.ToString();

            txtLineDp.Text = lineDp.ToString();
            txtLineAngleReso.Text = lineAngleReso.ToString();
            txtLineThresh.Text = lineThreshold.ToString();
            txtLineMinLen.Text = lineMinLen.ToString();
            txtLineMaxGap.Text = lineMaxGap.ToString();
            txtCannyThresh1.Text = cannyThresh1.ToString();
            txtCannyThresh2.Text = cannyThresh2.ToString();
            txtCannyGradient.Text = cannyI2Gradient.ToString();

            txtCrossFillRate.Text = crossFillRate.ToString();
        }

        private void BtnOpenFile_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog openFileDialog1 = new OpenFileDialog();
            DialogResult result = openFileDialog1.ShowDialog();
            if (result == System.Windows.Forms.DialogResult.OK || result == System.Windows.Forms.DialogResult.Yes)
            {
                fileNameTextBox.Text = openFileDialog1.FileName;
            }
        }

        private void BtnFormalDetect_Click(object sender, RoutedEventArgs e)
        {
            crossFillRate = double.Parse(txtCrossFillRate.Text);

            detector = new Detector(crossFillRate);
            _bitmap = new Bitmap(fileNameTextBox.Text);//加载图片

            int[] finalResult = detector.Detect(_bitmap, 19);//检测，完成后detector中带有Circles和CrossPoints信息

            //绘制灰度图
            //Image<Bgr, Byte> img = new Image<Bgr, byte>(bitmap);
            UMat cannyEdges = detector.cannyEdges;
            ////灰度，第三、四个参数分别为边缘检测阈值和连接阈值（大于第一个作为边界，小于第二个舍弃，介于之间时看该点是否连接着其他边界点）
            //CvInvoke.Canny(img, cannyEdges, cannyThreshold, cannyThreshold * 0.8);
            imageOrigin.Image = cannyEdges;

            #region 鼠标事件
            imageOrigin.MouseDown += ImageOrigin_MouseDown;
            #endregion

            //绘制圆和焦点图
            Mat circleImage = new Mat(cannyEdges.Size, DepthType.Cv8U, 3);
            circleImage.SetTo(new MCvScalar(0));
            foreach (CircleF circle in detector.Circles)
                CvInvoke.Circle(circleImage, System.Drawing.Point.Round(circle.Center), (int)circle.Radius, new Bgr(System.Drawing.Color.Brown).MCvScalar, 2);
            foreach (var points in detector.CrossPoints.Values)
            {
                foreach (var point in points)
                {
                    CvInvoke.Circle(circleImage, point, 2, new Bgr(System.Drawing.Color.Green).MCvScalar, 2);
                }
            }
            imageResult.Image = circleImage;


            //canny图和十字圆圈重合图
            Emgu.CV.Image<Rgb, Byte> im = cannyEdges.ToImage<Rgb, Byte>();
            Mat twoImage = im.Mat;
            foreach (CircleF circle in detector.Circles)
            {
                CvInvoke.Circle(twoImage, System.Drawing.Point.Round(circle.Center), (int)circle.Radius, new Bgr(System.Drawing.Color.Red).MCvScalar, 2);
            }
            System.Drawing.Color[] colors = new System.Drawing.Color[] {
                System.Drawing.Color.Red,//Left
                System.Drawing.Color.Green,//Up
                System.Drawing.Color.Red,//Right
                System.Drawing.Color.Green,//Down
                System.Drawing.Color.GreenYellow,//LeftUp
                System.Drawing.Color.Pink,//RightUp
                System.Drawing.Color.Pink,//RightDown
                System.Drawing.Color.GreenYellow//LeftDown 
            };
            int index = 0;
            foreach (var pair in detector.CrossPoints)
            {
                foreach (var cross in pair.Value)
                {
                    CvInvoke.Circle(twoImage, cross, 4, new Bgr(colors[(int)pair.Key - 1]).MCvScalar, 3);
                    index++;
                    if (index == colors.Length)
                    {
                        index = 0;
                    }
                }
            }
            dupliImage.Image = twoImage;

            #region canny和直线图
            if (detector.conors != null)
            {
                PointF? leftTop = detector.conors[0];
                PointF? rightTop = detector.conors[1];
                PointF? rightDown = detector.conors[2];
                PointF? leftDown = detector.conors[3];

                Emgu.CV.Image<Rgb, Byte> im2 = cannyEdges.ToImage<Rgb, Byte>();
                Mat twoImage2 = im2.Mat;
                CvInvoke.Line(twoImage2, new System.Drawing.Point((int)leftTop.Value.X, (int)leftTop.Value.Y), new System.Drawing.Point((int)rightTop.Value.X, (int)rightTop.Value.Y), new MCvScalar(0, 255, 0), 1);
                CvInvoke.Line(twoImage2, new System.Drawing.Point((int)leftDown.Value.X, (int)leftDown.Value.Y), new System.Drawing.Point((int)rightDown.Value.X, (int)rightDown.Value.Y), new MCvScalar(0, 255, 0), 1);
                CvInvoke.Line(twoImage2, new System.Drawing.Point((int)leftTop.Value.X, (int)leftTop.Value.Y), new System.Drawing.Point((int)leftDown.Value.X, (int)leftDown.Value.Y), new MCvScalar(0, 255, 0), 1);
                CvInvoke.Line(twoImage2, new System.Drawing.Point((int)rightTop.Value.X, (int)rightTop.Value.Y), new System.Drawing.Point((int)rightDown.Value.X, (int)rightDown.Value.Y), new MCvScalar(0, 255, 0), 1);
                imageCannyAndLine.Image = twoImage2;
            }


            #endregion

            #region imageFinal
            if (finalResult != null)
            {
                //Emgu.CV.Image<Rgb, Byte> imFinal = cannyEdges.ToImage<Rgb, Byte>();
                Image<Bgr, Byte> img = new Image<Bgr, byte>(_bitmap);
                Mat matFinal = img.Mat;
                for (int i = 0; i < finalResult.Length; i++)
                {
                    if (finalResult[i] > 0)
                    {
                        CvInvoke.Circle(matFinal, detector.allCoordinate[i], detector.minGridWidth / 2, new Bgr(finalResult[i] == 2 ? System.Drawing.Color.Green : System.Drawing.Color.Red).MCvScalar, 5);
                        //CvInvoke.Circle(matFinal, detector.allCoordinate[i], 5, new Bgr(System.Drawing.Color.DarkGreen ).MCvScalar, 3);
                    }
                }
                imageFinal.Image = matFinal;
            }
            #endregion
        }

        private void ImageOrigin_MouseDown(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            int x = (int)((e.Location.X / imageOrigin.ZoomScale + imageOrigin.HorizontalScrollBar.Value));
            int y = (int)((e.Location.Y / imageOrigin.ZoomScale + imageOrigin.VerticalScrollBar.Value));
            Console.WriteLine(x + "  " + y);
            double[] rates = detector.CheckCross(x, y);
            if (rates != null)
            {
                txtConsole.Text = rates[0].ToString();
                txtConsole.Text += "\r\n" + rates[1].ToString();
                txtConsole.Text += "\r\n" + rates[2].ToString();
                txtConsole.Text += "\r\n" + rates[3].ToString();
            }
        }


        #region 无用了
        //chess棋盘检测测试
        private void BtnChessBoard_Click(object sender, RoutedEventArgs e)
        {
            Bitmap bitmap = new Bitmap(fileNameTextBox.Text);
            Image<Bgr, Byte> img = new Image<Bgr, byte>(bitmap);
            UMat cornerMat = new UMat();

            CvInvoke.FindChessboardCorners(img, new System.Drawing.Size(9, 9), cornerMat);
            //CvInvoke.Find4QuadCornerSubpix(img, cornerMat, new System.Drawing.Size(9, 9));

            imageOrigin.Image = img;
            imageResult.Image = cornerMat;
        }

        //直线拟合测试
        private void BtnLineFit_Click(object sender, RoutedEventArgs e)
        {
            //float[] x = new float[] { 0, 0, 0, 0, 0 };
            //float[] y = new float[] { 1, 2, 3, 4, 5 };
            //int n = 5;
            //float[] result = new float[3];
            //MathImport.LineFit(x, y, n, result);
            ////int c = MathImport.Add(1, 2);
            PointF[] points = new PointF[] {
                new PointF(0,1),
                new PointF(0,2),
                new PointF(0,3),
            };
            PointF direction;
            PointF pointOnLine;
            //方向(a,b)和所在点(x0,y0)  代表着直线 a*y=b*x+(ay0-bx0)
            //直线(a1,b1)(x1,y1)和(a2,b2)(x2,y2)的交点为 
            //x =[a1a2(y2-y1)+a2b1x1-a1b2x2]/(a2b1-a1b2)   
            //y =[b1b2(x1-x2)+a2b1y2-a1b2y1]/(a2b1-a1b2)
            CvInvoke.FitLine(points, out direction, out pointOnLine, DistType.L1, 0, 0.01, 0.01);
        }

        private void BtnReDetect_Click(object sender, RoutedEventArgs e)
        {
            dp = int.Parse(txtDp.Text.Trim());
            cannyThreshold = int.Parse(txtCanny.Text.Trim());
            circleAccumulatorThreshold = int.Parse(txtAccThr.Text.Trim());
            minDist = int.Parse(txtMinDist.Text.Trim());
            minRadius = int.Parse(txtMinCir.Text.Trim());
            maxRadius = int.Parse(txtMaxCir.Text.Trim());
            lineDp = double.Parse(txtLineDp.Text.Trim());

            lineDp = double.Parse(txtLineDp.Text.Trim());
            lineAngleReso = double.Parse(txtLineAngleReso.Text.Trim());
            lineThreshold = int.Parse(txtLineThresh.Text.Trim());
            lineMinLen = double.Parse(txtLineMinLen.Text.Trim());
            lineMaxGap = double.Parse(txtLineMaxGap.Text.Trim());
            cannyThresh1 = double.Parse(txtCannyThresh1.Text.Trim());
            cannyThresh2 = double.Parse(txtCannyThresh2.Text.Trim());
            cannyI2Gradient = bool.Parse(txtCannyGradient.Text.Trim());

            PerformShapeDetection();
        }

        public void PerformShapeDetection()
        {
            try
            {
                if (fileNameTextBox.Text != String.Empty)
                {
                    #region 预处理

                    StringBuilder msgBuilder = new StringBuilder("Performance: ");

                    //Load the image from file and resize it for display
                    Image<Bgr, Byte> img = new Image<Bgr, byte>(fileNameTextBox.Text);
                    //.Resize(2000, 2000, Emgu.CV.CvEnum.Inter.Linear, true);
                    //labelSize.Text = img.Width.ToString() + " , " + img.Height.ToString();
                    //转为灰度级图像
                    UMat uimage = new UMat();
                    CvInvoke.CvtColor(img, uimage, ColorConversion.Bgr2Gray);

                    //use image pyr to remove noise 降噪，为了更准确的做边缘检测
                    UMat pyrDown = new UMat();
                    CvInvoke.PyrDown(uimage, pyrDown);
                    CvInvoke.PyrUp(pyrDown, uimage);
                    #endregion


                    #region 检测圆
                    Stopwatch watch = Stopwatch.StartNew();
                    CircleF[] circles = CvInvoke.HoughCircles(uimage, HoughType.Gradient, dp, minDist, cannyThreshold, circleAccumulatorThreshold, minRadius, maxRadius);

                    watch.Stop();
                    msgBuilder.Append(String.Format("Hough circles - {0} ms; ", watch.ElapsedMilliseconds));
                    #endregion

                    #region Canny and edge detection
                    watch.Reset(); watch.Start();
                    UMat cannyEdges = new UMat();
                    CvInvoke.Canny(uimage, cannyEdges, cannyThresh1, cannyThresh2, 3, cannyI2Gradient);

                    LineSegment2D[] lines = CvInvoke.HoughLinesP(
                       cannyEdges,
                       lineDp, //Distance resolution in pixel-related units
                       Math.PI / lineAngleReso, //Angle resolution measured in radians.
                       lineThreshold, //threshold
                       lineMinLen, //min Line width
                       lineMaxGap); //gap between lines

                    watch.Stop();
                    msgBuilder.Append(String.Format("Canny & Hough lines - {0} ms; ", watch.ElapsedMilliseconds));
                    imageOrigin.Image = img;
                    #endregion

                    #region draw circles
                    Mat circleImage = new Mat(img.Size, DepthType.Cv8U, 3);
                    circleImage.SetTo(new MCvScalar(0));
                    foreach (CircleF circle in circles)
                        CvInvoke.Circle(circleImage, System.Drawing.Point.Round(circle.Center), (int)circle.Radius, new Bgr(System.Drawing.Color.Brown).MCvScalar, 2);

                    imageCircles.Image = circleImage;
                    #endregion

                    #region draw lines
                    Mat lineImage = new Mat(img.Size, DepthType.Cv8U, 3);
                    lineImage.SetTo(new MCvScalar(0));
                    System.Drawing.Color[] colors = {
                        System.Drawing.Color.Red,
                        System.Drawing.Color.Orange,
                        System.Drawing.Color.Yellow,
                        System.Drawing.Color.Green,
                        System.Drawing.Color.Blue,
                        System.Drawing.Color.Purple
                    };
                    int index = 0;
                    foreach (LineSegment2D line in lines)
                    {
                        if (index > colors.Length - 1)
                        {
                            index = 0;
                        }
                        CvInvoke.Line(circleImage, line.P1, line.P2, new Bgr(colors[index++]).MCvScalar, 2);
                    }

                    txtLineCount.Text = "LineCount: " + lines.Length;
                    imageOrigin.Image = cannyEdges;
                    imageResult.Image = circleImage;
                    #endregion
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex);
            }
        }
        #endregion

        private void bhd_Click(object sender, RoutedEventArgs e)
        {
            TestWindow w = new TestWindow(new Bitmap(fileNameTextBox.Text));
            w.Show();
        }

        //调试C++
        private void Button_Click(object sender, RoutedEventArgs e)
        {
            _bitmap = new Bitmap(fileNameTextBox.Text);
            Image<Bgr, Byte> img = new Image<Bgr, byte>(_bitmap);
            GCHandle hObject = GCHandle.Alloc(img.Bytes, GCHandleType.Pinned);
            IntPtr pObject = hObject.AddrOfPinnedObject();
            if (hObject.IsAllocated)
                hObject.Free();

            //DllImporter.SetConfig(0.7, 90, 90 * 0.6, 30, 90, 0.2);

            int[] result = new int[19 * 19];
            DllImporter.Detect(pObject, img.Width, img.Height, 3, 19, result);


            //绘图
            Mat matFinal = img.Mat;
            for (int i = 0; i < result.Length; i++)
            {
                if (result[i] > 0)
                {
                    CvInvoke.Circle(matFinal, detector.allCoordinate[i], detector.minGridWidth / 2, new Bgr(result[i] == 2 ? System.Drawing.Color.Green : System.Drawing.Color.Red).MCvScalar, 5);
                    //CvInvoke.Circle(matFinal, detector.allCoordinate[i], 5, new Bgr(System.Drawing.Color.DarkGreen ).MCvScalar, 3);
                }
            }
            imageFinal.Image = matFinal;
        }
    }
}
