using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace GoImageDetection
{
    public class DllImporter
    {
        const string DLLNAME = "OpenCVNativeConsoleTest.dll";

        [DllImport(DLLNAME, EntryPoint = "Detect", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Detect(IntPtr data, int width, int height, int channels, int boardSize, int[] result);

        [DllImport(DLLNAME, EntryPoint = "SetConfig", CallingConvention = CallingConvention.Cdecl)]
        public static extern void SetConfig(double minWidthRate, double cannyThreshold, double cannyThreshold2, double circleAccumulatorThreshold, double circleCannyThresh, double crossFillRate);

    }
}
