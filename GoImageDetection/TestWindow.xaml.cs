using Emgu.CV;
using Emgu.CV.CvEnum;
using Emgu.CV.Structure;
using Emgu.CV.Util;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace GoImageDetection
{
    /// <summary>
    /// TestWindow.xaml 的交互逻辑
    /// </summary>
    public partial class TestWindow : Window
    {
        Bitmap _bitmap;
        public TestWindow(Bitmap bitmap)
        {
            _bitmap = bitmap;
            InitializeComponent();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            Image<Hsv, Byte> img = new Image<Hsv, byte>(_bitmap);
            VectorOfMat mats = new VectorOfMat();
            CvInvoke.Split(img, mats);
            //imageH.Image = mats[0];
            //imageS.Image = mats[1];
            //imageB.Image = mats[2];


            UMat uimage1 = new UMat();
            UMat pyrDown1 = new UMat();
            CvInvoke.PyrDown(mats[0], pyrDown1);
            CvInvoke.PyrUp(pyrDown1, uimage1);
            UMat cannyEdges1 = new UMat();
            CvInvoke.Canny(uimage1, cannyEdges1, 80, 80 * 0.6);
            imageHCanny.Image = cannyEdges1;

            UMat uimage2 = new UMat();
            UMat pyrDown2 = new UMat();
            CvInvoke.PyrDown(mats[1], pyrDown2);
            CvInvoke.PyrUp(pyrDown2, uimage2);
            UMat cannyEdges2 = new UMat();
            CvInvoke.Canny(uimage2, cannyEdges2, 80, 80 * 0.6);
            imageSCanny.Image = cannyEdges2;

            UMat uimage3 = new UMat();
            UMat pyrDown3 = new UMat();
            CvInvoke.PyrDown(mats[2], pyrDown3);
            CvInvoke.PyrUp(pyrDown3, uimage3);
            UMat cannyEdges3 = new UMat();
            CvInvoke.Canny(uimage3, cannyEdges3, 80, 80 * 0.6);
            imageBCanny.Image = cannyEdges3;


            imageH.Image = uimage1;
            imageS.Image = uimage2;
            imageB.Image = uimage3;
        }
    }
}
