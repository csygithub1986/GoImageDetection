#pragma once
#define EX __declspec(dllexport)


double MinWidthRate = 0.7;
double CannyThreshold = 90;   // ����5����Ե�����ֵ��30~180��
double CircleAccumulatorThreshold = 30;       // ����6���ۼ�����ֵ��Բ���غϵ㣬Խ�͵�ʱ��Բ����Խ���׵���Բ��
double CircleCannyThresh = 90;    // Բ�ı�Ե�����ֵ��30~180��
double CrossFillRate = 0.2; //ʮ�ּ��




extern "C" {
	int EX *Detect(unsigned char*, int, int, int,int);

	void EX SetConfig(double, double, double, double, double);
}
