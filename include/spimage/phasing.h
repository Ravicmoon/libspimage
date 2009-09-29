#ifndef _SP_PHASING_H_
#define _SP_PHASING_H_ 1

#include "image.h"
#include "cuda_util.h"
#include "support_update.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef enum{SpModelRandomPhases=1,SpModelZeroPhases=2,SpModelRandomValues=4,SpModelMaskedOutZeroed=256}SpModelInitialization;
typedef enum{SpSupportFromPatterson=1}SpSupportInitialization;
typedef enum{SpHIO=1,SpRAAR,SpDiffMap}SpPhasingAlgorithmType;
typedef enum{SpNoConstraints=0,SpRealObject=1,SpPositiveRealObject=2,SpPositiveComplexObject=4,SpPositivityFlipping=8}SpPhasingConstraints;
typedef enum{SpEngineAutomatic=0,SpEngineCPU=1,SpEngineCUDA=2}SpPhasingEngine;
typedef enum{SpPixelInsideSupport=1,SpPixelMeasuredAmplitude=2}SpPhasingPixelFlags;
/*! This structure is private */
typedef struct{
  real beta;
  SpPhasingConstraints constraints;
}SpPhasingHIOParameters;


typedef SpPhasingHIOParameters SpPhasingRAARParameters;

/*! This structure is private */
typedef struct{
  real beta;
  real gamma1;
  real gamma2;
  SpPhasingConstraints constraints;
}SpPhasingDiffMapParameters;

/*! This structure is private */
typedef struct{
  SpPhasingAlgorithmType type;
  void * params;
}SpPhasingAlgorithm;

/*! This structure is private */
typedef struct{
  sp_3matrix * amplitudes;
  sp_i3matrix * pixel_flags;
  SpPhasingAlgorithm * algorithm;
  SpSupportAlgorithm * sup_algorithm;
  int iteration;
  int image_size;

  /* These are images that are exposed to the user
   when sp_phaser_model() sp_phaser_model_change()
   and sp_phaser_support() are called */
  Image * model;
  int model_iteration;
  Image * model_change;
  int model_change_iteration;
  Image * support;
  int support_iteration;
    

  SpPhasingEngine engine;

  Image * g0;
  Image * g1;

#ifdef _USE_CUDA
  cufftHandle cufft_plan;
  float * d_amplitudes;
  int * d_pixel_flags;
  cufftComplex * d_g0;
  cufftComplex * d_g1;
  int threads_per_block;
  int number_of_blocks;
#endif
}SpPhaser;


spimage_EXPORT SpPhasingAlgorithm * sp_phasing_hio_alloc(real beta, SpPhasingConstraints constraints);
spimage_EXPORT SpPhasingAlgorithm * sp_phasing_raar_alloc(real beta, SpPhasingConstraints constraints);

spimage_EXPORT SpPhaser * sp_phaser_alloc();
spimage_EXPORT void sp_phaser_free(SpPhaser * ph);

spimage_EXPORT const Image * sp_phaser_model(SpPhaser * ph);
  spimage_EXPORT void sp_phaser_set_model(SpPhaser * ph,const Image * model);
  spimage_EXPORT void sp_phaser_set_support(SpPhaser * ph,const Image * support);
spimage_EXPORT Image * sp_phaser_model_change(SpPhaser * ph);
spimage_EXPORT const Image * sp_phaser_support(SpPhaser * ph);
spimage_EXPORT int sp_phaser_init(SpPhaser * ph, SpPhasingAlgorithm * alg, SpSupportAlgorithm * sup_alg,Image * amplitudes, SpPhasingEngine engine);
  spimage_EXPORT int sp_phaser_init_model(SpPhaser * ph,const Image * model, int flags);
  spimage_EXPORT int sp_phaser_init_support(SpPhaser * ph,const Image * support, int flags, real value);
spimage_EXPORT int sp_phaser_iterate(SpPhaser * ph, int iterations);

#ifdef _USE_CUDA
  int phaser_iterate_hio_cuda(SpPhaser * ph,int iterations);  
  int phaser_iterate_raar_cuda(SpPhaser * ph,int iterations);  
  int sp_support_threshold_update_support_cuda(SpPhaser * ph);
  int sp_support_area_update_support_cuda(SpPhaser * ph);
  int sp_proj_module_cuda(Image * a, Image * amp);
#endif

  int sp_support_area_update_support(SpPhaser * ph);
  int sp_support_threshold_update_support(SpPhaser * ph);


#ifdef __cplusplus
}  /* extern "C" */
#endif /* __cplusplus */

  
#endif
