#ifndef _COLORMAP_H_
#define _COLORMAP_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "spimage.h"

/** @defgroup Color Colors and Colormaps 
 *  This module is only for internal use.
 *  @{
 */

/*!
  This structure is use to store color information.
  \internal
*/
  typedef struct{
    real r;
    real g;
    real b;
  }sp_rgb;
  
  /*! 
    This enum controls how an image is mapped to colors.
   */
  typedef enum{SpColormapFirstColorScheme=1,SpColormapGrayScale=1,SpColormapTraditional=2,
	       SpColormapHot=4,SpColormapRainbow=8,SpColormapJet=16,
	       SpColormapWheel=32,SpColormapLastColorScheme=64,SpColormapLogScale=128,SpColormapPhase=256,
	       SpColormapWeightedPhase=512,SpColormapMask=1024,SpColormapShadedMask=2048,SpColormapInvertedPhase=4096,
	       SpColormapPosNeg=8192,SpColormapInvertedPosNeg=16384}SpColormap;

  /*! 
    \internal
    Calculates the color corresponding to the given value using a certain colormap.
    value must fall in the interval [0,1[.
  */
  spimage_EXPORT sp_rgb sp_colormap_rgb_from_value(real value, int colormap);

  /*!
  \internal
    Creates a color table given a colormap.
    It's only for internal use.
   */
  spimage_EXPORT void sp_colormap_create_table(sp_rgb color_table[256],int colormap);

  /*!
    \internal
    Takes an input value and outputs a vale between [0,1[ depending on the colormap.
    
    For a linear colormap it corresponds to linear interpolation between min_v and max_v as if min_v was 0 and max_v was 1.
    For a log colormap the interpolation is logarithmic.
   */
  spimage_EXPORT real sp_colormap_scale_value(real input,int colormap,real max_v, real min_v);

  /*!
    \internal
    Writes the ARGB 32 bit color pixel corresponding to the input img at position (x,y,z) to the out buffer.
    
    The buffer has to be preallocated.
    If min_v and max_v are the same then the minimum and maximum of the image will be used.
    The colormap has to have been initialized using sp_colormap_create_table.
    If red_blue_swap is different than 0 the image is written as ABGR.
   */
  spimage_EXPORT void sp_colormap_write_rgb(unsigned char * out,Image * img, int colormap,sp_rgb * color_table,real max_v, real min_v, int x, int y, int z, int red_blue_swap);

/*@}*/

#ifdef __cplusplus
}  /* extern "C" */
#endif /* __cplusplus */


#endif 
