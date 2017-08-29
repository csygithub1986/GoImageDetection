#pragma once
#define EX __declspec(dllexport)

extern "C" {
int EX *Detect(unsigned char*, int , int , int);
}
