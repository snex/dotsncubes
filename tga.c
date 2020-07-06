#include <stdlib.h>
#include <stdio.h>
#include "tga.h"

RGBImg* allocRGBImg(int rows, int cols)
{
  RGBPixel **arr;
  RGBPixel *data;
  int i;
  RGBImg *img = calloc(1, sizeof(RGBImg));

  img->rows=rows;
  img->cols=cols;
  img->bpp=24;

  if( !(arr = calloc(rows, sizeof(RGBPixel)))) return 0;

  if( !(data = calloc(rows * cols, sizeof(RGBPixel)))){
    free(arr);
    return 0;
  }

  for(i=0;i<rows;i++) arr[i] = data + i*cols;

  img->data=arr;
  return(img);
}

void deallocRGBImg(RGBImg *img)
{
  free(img->data[0]);
  free(img->data);
}

RGBImg* readTGAImg(char *filename)
{
  int size, i;
  char	head[18] ;
  FILE	*tga_in;
  int originFlag;
  int rows, cols, bpp;
  RGBImg *img;

  if(!(tga_in = fopen(filename, "rb" ))) return 0;
    
  fread( head, sizeof(char), 18, tga_in ) ;
  cols = (int)((unsigned int)head[12]&0xFF | (unsigned int)head[13]*256);
  rows = (int)((unsigned int)head[14]&0xFF | (unsigned int)head[15]*256);
  bpp  = (int)(head[16]);
  originFlag = head[17]&0x20;

  printf("TGA Image size %d x %d\n", cols, rows);
  if(head[2] != 2){
    printf("Image type is %d, it is not true color, uncompressed\n", head[2]);
    fclose(tga_in);
    return(NULL);
  }
  
  size = (cols) * (rows);
  if( !(img = allocRGBImg(rows, cols)) )
    {
      fprintf(stderr, "Unable to allocate (%d x %d) array\n", cols, rows);
      fclose(tga_in);
      return(NULL);
    }

  if(originFlag){
    for(i = 0; i < rows; i++)
      fread(img->data[i], 3*sizeof(char), cols, tga_in);
  }
  else {
    for(i = 0; i < rows; i++)
      fread(img->data[rows-1-i], 3*sizeof(char), cols, tga_in);
  }

  fclose(tga_in) ;

  return(img);
}