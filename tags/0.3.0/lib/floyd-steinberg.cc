
/*
 * Floyd Steinberg dithering based on web publications and so on
 */

#include <stdlib.h>
#include <string.h>	// memcpy
#include <math.h>	// floor
#include <inttypes.h>	// uint8_t

void
FloydSteinberg (uint8_t *image, int width, int height, int shades)
{
  uint8_t *src_row;
  uint8_t *dest_row;
  float *error, *nexterror, *tmp;
  int row;
  int bytes = 1;

  /*  allocate row/error buffers  */
  src_row = image;
  dest_row = (uint8_t *) malloc (width * bytes);

  error = (float *) malloc (width * bytes * sizeof (float));
  nexterror = (float *) malloc (width * bytes * sizeof (float));

  /*  initialize the error buffers  */
  for (row = 0; row < width * bytes; row++)
    error[row] = nexterror[row] = 0;

  for (row = 0; row < height; row++)
    {
      int col;
      int channel = 0;
      static int direction = 1;
      int start, end;

      float newval, factor;
      float cerror;

      for (col = 0; col < width * bytes; col++)
	nexterror[col] = 0;

      if (direction == 1)
	{
	  start = 0;
	  end = width;
	}
      else
	{
	  direction = -1;
	  start = width - 1;
	  end = -1;
	}

      factor = (float) (shades - 1) / (float) 255;

      for (col = start; col != end; col += direction)
	{
	  newval =
	    src_row[col * bytes + channel] + error[col * bytes + channel];

          newval = floor (newval*factor + 0.5) / factor;
	  if (newval > 255)
	    newval = 255;
	  else if (newval < 0)
	    newval = 0;

	  dest_row[col * bytes + channel] = (unsigned int) (newval + 0.5);

	  cerror = src_row[col * bytes + channel] + error[col * bytes +
							  channel] -
	    dest_row[col * bytes + channel];

	  nexterror[col * bytes + channel] += cerror * 5 / 16;
	  if (col + direction >= 0 && col + direction < width)
	    {
	      error[(col + direction) * bytes + channel] += cerror * 7 / 16;
	      nexterror[(col + direction) * bytes + channel] +=
		cerror * 1 / 16;
	    }
	  if (col - direction >= 0 && col - direction < width)
	    nexterror[(col - direction) * bytes + channel] += cerror * 3 / 16;
	}

      /* store in the data buffer we got */
      memcpy (src_row, dest_row, width*bytes);
      src_row += width*bytes;

      /* next row in the opposite direction */
      direction = -direction;

      /* swap error/nexerror */
      tmp = error;
      error = nexterror;
      nexterror = tmp;
    }

  free (dest_row);
  free (error);
  free (nexterror);
}
