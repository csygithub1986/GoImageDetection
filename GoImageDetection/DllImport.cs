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

        //[DllImport(DLLNAME, EntryPoint = "?Detect@@YAPEAHPEAEHHH@Z", CallingConvention = CallingConvention.Cdecl)]
        [DllImport(DLLNAME, EntryPoint = "Detect", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Detect(IntPtr data, int width, int height, int channels);

    }
}
