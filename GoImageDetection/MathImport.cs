using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;

namespace GoImageDetection
{
    public class MathImport
    {
        const string DLLNAME = "Math.dll";

        [DllImport(DLLNAME, EntryPoint = "?LineFit@@YAXPAM0H0@Z", CallingConvention = CallingConvention.Cdecl)]
        public static extern void LineFit(Single[] dataX, Single[] dataY, int dataCount, Single[] result);


        [DllImport(DLLNAME, EntryPoint = "?Add@@YAHHH@Z", CallingConvention = CallingConvention.Cdecl)]
        public static extern int Add(int a,int b);
    }
}
