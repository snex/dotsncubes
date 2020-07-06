#ifndef __TGA_H_
#define __TGA_H_

#include <windows.h>
#include <GL/gl.h>

typedef struct{
  GLbyte b,g,r;
} RGBPixel;


typedef struct {
  int rows, cols, bpp;
  RGBPixel **data;
} RGBImg;

RGBImg* allocRGBImg(int rows, int cols);
void deallocRGBImg(RGBImg *img);
RGBImg* readTGAImg(char *filename);

#endif