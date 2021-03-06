#include <spimage.h>

/* Helper function for find_center. Mask should be in img->mask */
Image * sp_find_center_refine_calculate_mask(Image *img, const int search_space)
{
  const int x_size = sp_image_x(img);
  const int y_size = sp_image_y(img);
  const int i_size = sp_image_size(img);
  Image *res = sp_image_alloc(x_size,y_size,1);
  for (int i = 0; i < i_size; i++) {
    res->mask->data[i] = 0;
  }

  int x1,x2,y1,y2;
  for (int trans_x = -search_space; trans_x <= search_space; trans_x++) {
    for (int trans_y = -search_space; trans_y <= search_space; trans_y++) {
      for (int x = 0; x < x_size; x++) {
	for (int y = 0; y < y_size; y++) {
	  x1 = x+trans_x; y1 = y+trans_y;
	  if (x1 > 0 && x1 < x_size &&
	      y1 > 0 && y1 < y_size &&
	      sp_image_mask_get(img,x1,y1,0) == 0) {
	    sp_image_mask_set(res,x,y,0,1);
	  }
	  x2 = x_size - 1 - x - trans_x; y2 = y_size - 1 - y - trans_y;
	  if (x2 > 0 && x2 < x_size &&
	      y2 > 0 && y2 < y_size &&
	      sp_image_mask_get(img,x2,y2,0) == 0) {
	    sp_image_mask_set(res,x,y,0,1);
	  }
	}
      }
    }
  }
  return res;
}

/* Expects diffraction patterns. Uses centrosymmetry. Only handles 2D patterns at the moment. Can take a precalculated mask calculated by the sp_find_center_refine_calculate_mask function. If this parameter is set to NULL the mask will be calculated. */
float sp_find_center_refine(Image *img, const int search_radius, int crop_radius, Image *precalculated_mask)
{

  int min_crop_radius = MIN(MIN((int)ceil(img->detector->image_center[0]) - search_radius,
				sp_image_x(img) - (int)ceil(img->detector->image_center[0]) - search_radius - 1),
			    MIN((int)ceil(img->detector->image_center[1]) - search_radius,
				sp_image_y(img) - (int)ceil(img->detector->image_center[1]) - search_radius - 1));
  /*
  if ((int)ceil(img->detector->image_center[0]) - crop_radius - search_radius < 0 ||
      (int)ceil(img->detector->image_center[0]) + crop_radius + search_radius >= sp_image_x(img) ||
      (int)ceil(img->detector->image_center[1]) - crop_radius - search_radius < 0 ||
      (int)ceil(img->detector->image_center[1]) + crop_radius + search_radius >= sp_image_y(img)) {
  */
  if (crop_radius > min_crop_radius) {
    return 0;
  }

  if (crop_radius == 0) {
    crop_radius = min_crop_radius;
  }

  Image *mask;
  if (precalculated_mask == NULL) {
    mask = sp_find_center_refine_calculate_mask(img, search_radius);
  } else {
    mask = precalculated_mask;
  }

  // Find number of positions in the region used.
  int counter = 0;
  for (int x = (int)ceil(img->detector->image_center[0])-crop_radius; x < (int)ceil(img->detector->image_center[0])+crop_radius; x++) {
    for (int y = (int)ceil(img->detector->image_center[1])-crop_radius; y < (int)ceil(img->detector->image_center[0]); y++) {
      if (x < 0 || x >= sp_image_x(img) || y < 0 || y >= sp_image_y(img)) {
	return 0;
      }
      if (sp_image_mask_get(mask, x, y, 0) == 0) {
	counter++;
      }
    }
  }
  const int number_of_pixels_in_comparison = counter;
  if (number_of_pixels_in_comparison == 0)
    return 0;

  // Prepare coordinates to make main loop faster
  int *x_pos = malloc(number_of_pixels_in_comparison*sizeof(int));
  int *y_pos = malloc(number_of_pixels_in_comparison*sizeof(int));
  int *x_pos_inv = malloc(number_of_pixels_in_comparison*sizeof(int));
  int *y_pos_inv = malloc(number_of_pixels_in_comparison*sizeof(int));

  int index_1d = 0;
  for (int x = (int)ceil(img->detector->image_center[0])-crop_radius; x < (int)ceil(img->detector->image_center[0])+crop_radius; x++) {
    for (int y = (int)ceil(img->detector->image_center[1])-crop_radius; y < (int)ceil(img->detector->image_center[0]); y++) {
      if (sp_image_mask_get(mask, x, y, 0) == 0) {
	x_pos[index_1d] = x;
	y_pos[index_1d] = y;
	x_pos_inv[index_1d] = (int)round(2.*img->detector->image_center[0]) - x;
	y_pos_inv[index_1d] = (int)round(2.*img->detector->image_center[1]) - y;
	index_1d++;
      }
    }
  }
  if (precalculated_mask != NULL) {
    sp_image_free(mask);
  }
  
  // This is the comparison.
  Image *result_map = sp_image_alloc(2*search_radius+1, 2*search_radius+1, 1);
  real sum;
  real a, b;
  for (int cx = -search_radius; cx <= search_radius; cx++) {
    for (int cy = -search_radius; cy <= search_radius; cy++) {
      sum = 0.;
      for (int i = 0; i < number_of_pixels_in_comparison; i++) {
	a = sp_real(sp_image_get(img, x_pos[i] + cx, y_pos[i] + cy, 0));
	b = sp_real(sp_image_get(img, x_pos_inv[i] + cx, y_pos_inv[i] + cy, 0));
	//sum += fabs(a-b) / fabs(a+b);
	sum += fabs(a-b);
      }
      sp_image_set(result_map, search_radius+cx, search_radius+cy, 0, sp_cinit(sum, 0.0));
    }
  }
  free(x_pos);
  free(y_pos);
  free(x_pos_inv);
  free(y_pos_inv);

  // Find the minimum (best fit) of the fits calculated before.
  real minimum = 1e10;
  int min_x = -1000;
  int min_y = -1000;
  for (int cx = -search_radius; cx <= search_radius; cx++) {
    for (int cy = -search_radius; cy <= search_radius; cy++) {
      if (sp_real(sp_image_get(result_map,search_radius+cx,search_radius+cy,0)) < minimum) {
	minimum = sp_real(sp_image_get(result_map,search_radius+cx,search_radius+cy,0));
	min_x = cx;
	min_y = cy;
      }
    }
  }

  //sp_image_free(result_map);
  img->detector->image_center[0] += (real)min_x;
  img->detector->image_center[1] += (real)min_y;
  return minimum;
}


/* Expects diffraction patterns. Uses centrosymmetry. Only handles 2D patterns at the moment. Can take a precalculated mask calculated by the sp_find_center_refine_calculate_mask function. If this parameter is set to NULL the mask will be calculated. */
/*
int sp_find_center_refine2(Image *img, const int search_radius, int crop_radius)
{

  int min_crop_radius = MIN(MIN((int)ceil(img->detector->image_center[0]) - search_radius,
				sp_image_x(img) - (int)ceil(img->detector->image_center[0]) - search_radius - 1),
			    MIN((int)ceil(img->detector->image_center[1]) - search_radius,
				sp_image_y(img) - (int)ceil(img->detector->image_center[1]) - search_radius - 1));
  if (crop_radius > min_crop_radius) {
    return 0;
  }

  if (crop_radius == 0) {
    crop_radius = min_crop_radius;
  }

  // Find number of positions in the region used.
  int counter = 0;
  for (int x = (int)ceil(img->detector->image_center[0])-crop_radius; x < (int)ceil(img->detector->image_center[0])+crop_radius; x++) {
    for (int y = (int)ceil(img->detector->image_center[1])-crop_radius; y < (int)ceil(img->detector->image_center[0]); y++) {
      if (x < 0 || x >= sp_image_x(img) || y < 0 || y >= sp_image_y(img)) {
	return 0;
      }
      counter++;
    }
  }
  const int number_of_pixels_in_comparison = counter;
  if (number_of_pixels_in_comparison == 0)
    return 0;

  // Prepare coordinates to make main loop faster
  int *x_pos = malloc(number_of_pixels_in_comparison*sizeof(int));
  int *y_pos = malloc(number_of_pixels_in_comparison*sizeof(int));
  int *x_pos_inv = malloc(number_of_pixels_in_comparison*sizeof(int));
  int *y_pos_inv = malloc(number_of_pixels_in_comparison*sizeof(int));

  int index_1d = 0;
  for (int x = (int)ceil(img->detector->image_center[0])-crop_radius; x < (int)ceil(img->detector->image_center[0])+crop_radius; x++) {
    for (int y = (int)ceil(img->detector->image_center[1])-crop_radius; y < (int)ceil(img->detector->image_center[0]); y++) {
      x_pos[index_1d] = x;
      y_pos[index_1d] = y;
      x_pos_inv[index_1d] = (int)round(2.*img->detector->image_center[0]) - x;
      y_pos_inv[index_1d] = (int)round(2.*img->detector->image_center[1]) - y;
      index_1d++;
    }
  }
  
  // This is the comparison.
  Image *result_map = sp_image_alloc(2*search_radius+1, 2*search_radius+1, 1);
  real sum2;
  real sum;
  real a, b;
  for (int cx = -search_radius; cx <= search_radius; cx++) {
    for (int cy = -search_radius; cy <= search_radius; cy++) {
      sum = 0.;
      sum2 = 0.;
      for (int i = 0; i < number_of_pixels_in_comparison; i++) {
	if(sp_image_mask_get(img, x_pos[i] + cx, y_pos[i] + cy, 0) && 
	   sp_image_mask_get(img, x_pos_inv[i] + cx, y_pos_inv[i] + cy, 0)){
	  a = sp_real(sp_image_get(img, x_pos[i] + cx, y_pos[i] + cy, 0));
	  b = sp_real(sp_image_get(img, x_pos_inv[i] + cx, y_pos_inv[i] + cy, 0));
	//sum += fabs(a-b) / fabs(a+b);
	//	sum += fabs(a-b);
	  sum += a+b;
	  sum2 += (a-b)*(a-b);
	}
      }
      printf("pos [%f, %f] error = %g\n",img->detector->image_center[0]+cx,img->detector->image_center[1]+cy,(sum2)/sum);
      sp_image_set(result_map, search_radius+cx, search_radius+cy, 0, sp_cinit((sum2)/sum, 0.0));
    }
  }
  free(x_pos);
  free(y_pos);
  free(x_pos_inv);
  free(y_pos_inv);

  // Find the minimum (best fit) of the fits calculated before.
  real minimum = 1e10;
  int min_x = -1000;
  int min_y = -1000;
  for (int cx = -search_radius; cx <= search_radius; cx++) {
    for (int cy = -search_radius; cy <= search_radius; cy++) {
      if (sp_real(sp_image_get(result_map,search_radius+cx,search_radius+cy,0)) < minimum) {
	minimum = sp_real(sp_image_get(result_map,search_radius+cx,search_radius+cy,0));
	min_x = cx;
	min_y = cy;
      }
    }
  }

  //sp_image_free(result_map);
  img->detector->image_center[0] += (real)min_x;
  img->detector->image_center[1] += (real)min_y;
  printf("pos [%f, %f] error = %g\n", img->detector->image_center[0],img->detector->image_center[1],minimum);
  return 1;
}
*/

/* Expects diffraction patterns. Uses centrosymmetry. Only handles 2D patterns at the moment. */
float sp_find_center_refine_minimal_mask(Image *img, const int search_radius, int crop_radius)
{
  // This is the comparison.
  //  Image *result_map = sp_image_alloc(2*search_radius+1, 2*search_radius+1, 1);
  real min_score = 1e10;
  int min_cx;
  int min_cy;
  for (int cx = -search_radius+img->detector->image_center[0]; cx <= search_radius+img->detector->image_center[0]; cx++) {
    int comp_radius_x = MIN(cx, sp_image_x(img) - cx - 1);
    if(comp_radius_x <= 0){
      return -1;
    }
    for (int cy = -search_radius+img->detector->image_center[1]; cy <= search_radius+img->detector->image_center[1]; cy++) {
      int comp_radius_y = MIN(cy, sp_image_y(img) - cy - 1);
      if(comp_radius_y <= 0){
	return -2;
      }
      
      real sum = 0.;
      real sum2 = 0.;
      for (int y = -comp_radius_y; y < 0; y++) {
	for (int x = -comp_radius_x; x <= comp_radius_x; x++) {
	  if(sp_image_mask_get(img, cx + x, cy + y, 0) &&
	     sp_image_mask_get(img, cx - x, cy - y, 0)){
	    real a, b;	  
	    a = sp_real(sp_image_get(img, cx + x, cy + y, 0));
	    b = sp_real(sp_image_get(img, cx - x, cy - y, 0));
	    sum += a+b;
	    sum2 += (a-b)*(a-b);
	  }
	}
      }
      //            printf("pos [%d, %d] error = %g\n",cx,cy,(sum2)/sum);
      //      sp_image_set(result_map, search_radius+cx, search_radius+cy, 0, sp_cinit((sum2)/sum, 0.0));
      if((sum2)/sum < min_score){
	min_score = (sum2)/sum;
	min_cx = cx;
	min_cy = cy;
      }
    }
  }  
  //  sp_image_free(result_map);
  img->detector->image_center[0] = min_cx;
  img->detector->image_center[1] = min_cy;
  //printf("pos [%f, %f] error = %g\n", img->detector->image_center[0],img->detector->image_center[1],min_score);
  return min_score;
}
