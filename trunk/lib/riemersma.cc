/* Riemersma dither
 * Based uppon:
 *   C/C++ Users Journal, December 1998: A Balanced Dithering Technique
 *                                       Thiadmer Riemersma
 */

#include <malloc.h>
#include <math.h>
#include <string.h>

enum {
  NONE,
  UP,
  LEFT,
  DOWN,
  RIGHT,
};

/* variables needed for the Riemersma dither algorithm */
static int cur_x=0, cur_y=0;
static int img_width=0, img_height=0;
static unsigned char *img_ptr;

#define SIZE 16                 /* queue size: number of pixels remembered */
#define MAX  16                 /* relative weight of youngest pixel in the
                                 * queue, versus the oldest pixel */

static int weights[SIZE];       /* weights for the errors of recent pixels */

static void init_weights(int a[],int size,int max)
{
  double m = exp(log(max)/(size-1));
  double v;
  int i;

  for (i=0, v=1.0; i<size; i++) {
    a[i]=(int)(v+0.5);  /* store rounded value */
    v*=m;               /* next value */
  } /*for */
}

static void dither_pixel(unsigned char *pixel)
{
static int error[SIZE]; /* queue with error values of recent pixels */
  int i,pvalue,err;

  for (i=0,err=0L; i<SIZE; i++)
    err+=error[i]*weights[i];
  pvalue=*pixel + err/MAX;
  pvalue = (pvalue>=128) ? 255 : 0;
  memmove(error,error+1,(SIZE-1)*sizeof error[0]);    /* shift queue */
  error[SIZE-1] = *pixel - pvalue;
  *pixel=(unsigned char)pvalue;
}

static void move(int direction)
{
  /* dither the current pixel */
  if (cur_x>=0 && cur_x<img_width && cur_y>=0 && cur_y<img_height)
    dither_pixel(img_ptr);

  /* move to the next pixel */
  switch (direction) {
  case LEFT:
    cur_x--;
    img_ptr--;
    break;
  case RIGHT:
    cur_x++;
    img_ptr++;
    break;
  case UP:
    cur_y--;
    img_ptr-=img_width;
    break;
  case DOWN:
    cur_y++;
    img_ptr+=img_width;
    break;
  } /* switch */
}

void hilbert_level(int level,int direction)
{
  if (level==1) {
    switch (direction) {
    case LEFT:
      move(RIGHT);
      move(DOWN);
      move(LEFT);
      break;
    case RIGHT:
      move(LEFT);
      move(UP);
      move(RIGHT);
      break;
    case UP:
      move(DOWN);
      move(RIGHT);
      move(UP);
      break;
    case DOWN:
      move(UP);
      move(LEFT);
      move(DOWN);
      break;
    } /* switch */
  } else {
    switch (direction) {
    case LEFT:
      hilbert_level(level-1,UP);
      move(RIGHT);
      hilbert_level(level-1,LEFT);
      move(DOWN);
      hilbert_level(level-1,LEFT);
      move(LEFT);
      hilbert_level(level-1,DOWN);
      break;
    case RIGHT:
      hilbert_level(level-1,DOWN);
      move(LEFT);
      hilbert_level(level-1,RIGHT);
      move(UP);
      hilbert_level(level-1,RIGHT);
      move(RIGHT);
      hilbert_level(level-1,UP);
      break;
    case UP:
      hilbert_level(level-1,LEFT);
      move(DOWN);
      hilbert_level(level-1,UP);
      move(RIGHT);
      hilbert_level(level-1,UP);
      move(UP);
      hilbert_level(level-1,RIGHT);
      break;
    case DOWN:
      hilbert_level(level-1,RIGHT);
      move(UP);
      hilbert_level(level-1,DOWN);
      move(LEFT);
      hilbert_level(level-1,DOWN);
      move(DOWN);
      hilbert_level(level-1,LEFT);
      break;
    } /* switch */
  } /* if */
}

void Riemersma(unsigned char *image,int width,int height)
{
  int level,size;

  /* determine the required order of the Hilbert curve */
  size=width > height ? width : height;
  level=(int)log2(size);
  if ((1L << level) < size)
    level++;

  init_weights(weights,SIZE,MAX);
  img_ptr=image;
  img_width=width;
  img_height=height;
  cur_x=0;
  cur_y=0;
  if (level>0)
    hilbert_level(level,UP);
  move(NONE);
}

