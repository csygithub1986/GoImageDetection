#pragma once
#define EX __declspec(dllexport)


double MinWidthRate = 0.7;
double CannyThreshold = 90;   // 参数5：边缘检测阈值（30~180）
double CircleAccumulatorThreshold = 30;       // 参数6：累加器阈值（圆心重合点，越低的时候圆弧就越容易当成圆）
double CircleCannyThresh = 90;    // 圆的边缘检测阈值（30~180）
double CrossFillRate = 0.2; //十字检测




extern "C" {
	int EX *Detect(unsigned char*, int, int, int,int);

	void EX SetConfig(double, double, double, double, double);
}
