#pragma once
#include<cmath>
#include <stdio.h>
#include <tchar.h>
#include<Windows.h>

#define _EXTERN_C_  extern  _declspec(dllexport)

_EXTERN_C_ 	 void LineFit(float *, float *, int, float*); 
_EXTERN_C_ 	 int Add(int a,int b);