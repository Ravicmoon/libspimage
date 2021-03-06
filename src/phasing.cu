#include <spimage.h>

__global__ void CUDA_module_projection(cufftComplex* g, const float* amp, const float* amp_min, const float* amp_max, const int * pixel_flags,const  int size, const SpPhasingConstraints constraints);
__global__ void CUDA_support_projection_hio(cufftComplex* g1, const cufftComplex* g0, const cufftComplex* gp, const int * pixel_flags,const  int size, const float beta);
__global__ void CUDA_support_projection_er(cufftComplex* g1, cufftComplex *gp, const int * pixel_flags, const  int size);
__global__ void CUDA_complex_scale(cufftComplex * a, int size ,float scale);
__global__ void CUDA_support_projection_raar(cufftComplex* g1, const cufftComplex* g0, const cufftComplex* gp, const int * pixel_flags,const  int size, const float beta);
__global__ void CUDA_apply_constraints(cufftComplex* g, const int * pixel_flags,const  int size,const SpPhasingConstraints constraints);
__global__ void CUDA_apply_fourier_constraints(cufftComplex* g, const  int size,const SpPhasingConstraints constraints);
__global__ void CUDA_phased_amplitudes_projection(cufftComplex* g, const cufftComplex* phased_amp,const int * pixel_flags, const  int size);
__global__ void CUDA_diff_map_f1(cufftComplex* f1, const cufftComplex* g0,const int * pixel_flags,const float gamma1,const  int size);
__global__ void CUDA_diff_map(cufftComplex* Pi2f1,cufftComplex* Pi2rho, const cufftComplex* g0,cufftComplex* g1,const int * pixel_flags,const float gamma2,const float beta,const  int size);

int phaser_iterate_er_cuda(SpPhaser * ph,int iterations){
  SpPhasingERParameters * params = (SpPhasingERParameters *)ph->algorithm->params;
  for(int i = 0;i<iterations;i++){
    cufftComplex * swap = ph->d_g0;
    ph->d_g0 = ph->d_g1;
    ph->d_g1 = swap;
    /* executes FFT processes */
    cufftExecC2C(ph->cufft_plan, ph->d_g0, ph->d_g1, CUFFT_FORWARD);
    CUDA_apply_fourier_constraints<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1,ph->image_size,params->constraints);    
    if(ph->phasing_objective == SpRecoverPhases){
      CUDA_module_projection<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1,ph->d_amplitudes,ph->d_amplitudes_min,ph->d_amplitudes_max,ph->d_pixel_flags,ph->image_size,params->constraints);
    }else if(ph->phasing_objective == SpRecoverAmplitudes){
      CUDA_phased_amplitudes_projection<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1,ph->d_phased_amplitudes,ph->d_pixel_flags,ph->image_size);
    }else{
      abort();
    }
    sp_cuda_check_errors();
    sp_cuda_check_errors();
    cufftExecC2C(ph->cufft_plan, ph->d_g1, ph->d_gp, CUFFT_INVERSE);
    /* normalize */
    CUDA_complex_scale<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_gp,ph->image_size, 1.0f / (ph->image_size));
    sp_cuda_check_errors();
    CUDA_support_projection_er<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1, ph->d_gp, ph->d_pixel_flags,ph->image_size);
    sp_cuda_check_errors();
    if(params->constraints != SpNoConstraints){
      CUDA_apply_constraints<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1,ph->d_pixel_flags,ph->image_size,params->constraints);
    }
    sp_cuda_check_errors();
  }
  ph->iteration += iterations;
  return 0;
}

int phaser_iterate_hio_cuda(SpPhaser * ph,int iterations){
  SpPhasingHIOParameters * params = (SpPhasingHIOParameters *)ph->algorithm->params;
  for(int i = 0;i<iterations;i++){
    real beta = sp_smap_interpolate(params->beta,ph->iteration);
    cufftComplex * swap = ph->d_g0;
    ph->d_g0 = ph->d_g1;
    ph->d_g1 = swap;
    /* executes FFT processes */
    cufftExecC2C(ph->cufft_plan, ph->d_g0, ph->d_g1, CUFFT_FORWARD);
    /* The fourier constraints have to be applied before the amplitude projection otherwise the algorithm never converges,
     probably because there is a deficit of power after the constraints */
    CUDA_apply_fourier_constraints<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1,ph->image_size,params->constraints);
    sp_cuda_check_errors();
    if(ph->phasing_objective == SpRecoverPhases){
      CUDA_module_projection<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1,ph->d_amplitudes,ph->d_amplitudes_min,ph->d_amplitudes_max,ph->d_pixel_flags,ph->image_size,params->constraints);
    }else if(ph->phasing_objective == SpRecoverAmplitudes){
      CUDA_phased_amplitudes_projection<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1,ph->d_phased_amplitudes,ph->d_pixel_flags,ph->image_size);
    }else{
      abort();
    }
    sp_cuda_check_errors();

    /* The fourier constraints cannot be applied in this location! See comment above */
    cufftExecC2C(ph->cufft_plan, ph->d_g1, ph->d_gp, CUFFT_INVERSE);
    /* normalize */
    CUDA_complex_scale<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_gp,ph->image_size, 1.0f / (ph->image_size));
    sp_cuda_check_errors();
    CUDA_support_projection_hio<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1,ph->d_g0,ph->d_gp,ph->d_pixel_flags,ph->image_size,beta);
    sp_cuda_check_errors();
    if(params->constraints != SpNoConstraints){
      CUDA_apply_constraints<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1,ph->d_pixel_flags,ph->image_size,params->constraints);
    }
    sp_cuda_check_errors();
    ph->iteration++; 
  }
  return 0;
}

int phaser_iterate_diff_map_cuda(SpPhaser * ph,int iterations){
  SpPhasingDiffMapParameters * params = (SpPhasingDiffMapParameters *)ph->algorithm->params;
  const real gamma1 = params->gamma1;
  const real gamma2 = params->gamma2;
  cufftComplex * f1;
  cudaMalloc((void **)&f1,sizeof(cufftComplex)*ph->image_size);
  for(int i = 0;i<iterations;i++){
    real beta = sp_smap_interpolate(params->beta,ph->iteration);
    cufftComplex * swap = ph->d_g0;
    ph->d_g0 = ph->d_g1;
    ph->d_g1 = swap;
    /* executes FFT processes */
    cufftExecC2C(ph->cufft_plan, ph->d_g0, ph->d_g1, CUFFT_FORWARD);
    CUDA_apply_fourier_constraints<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1,ph->image_size,params->constraints);
    CUDA_diff_map_f1<<<ph->number_of_blocks, ph->threads_per_block>>>(f1,ph->d_g0,ph->d_pixel_flags,gamma1,ph->image_size);
    cufftExecC2C(ph->cufft_plan, f1, f1, CUFFT_FORWARD);
    CUDA_module_projection<<<ph->number_of_blocks, ph->threads_per_block>>>(f1,ph->d_amplitudes,ph->d_amplitudes_min,ph->d_amplitudes_max,ph->d_pixel_flags,ph->image_size,params->constraints); 
    cufftExecC2C(ph->cufft_plan, f1, f1, CUFFT_INVERSE);
    CUDA_module_projection<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1,ph->d_amplitudes,ph->d_amplitudes_min,ph->d_amplitudes_max,ph->d_pixel_flags,ph->image_size,params->constraints);
    cufftExecC2C(ph->cufft_plan, ph->d_g1, ph->d_gp, CUFFT_INVERSE);
    sp_cuda_check_errors();
    /* normalize */
    CUDA_complex_scale<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_gp,ph->image_size, 1.0f / (ph->image_size));
    CUDA_complex_scale<<<ph->number_of_blocks, ph->threads_per_block>>>(f1,ph->image_size, 1.0f / (ph->image_size));
    sp_cuda_check_errors();
    CUDA_diff_map<<<ph->number_of_blocks, ph->threads_per_block>>>(f1,ph->d_gp,ph->d_g0,ph->d_g1,ph->d_pixel_flags,gamma2,beta,ph->image_size);
    sp_cuda_check_errors();
    if(params->constraints != SpNoConstraints){
      CUDA_apply_constraints<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1,ph->d_pixel_flags,ph->image_size,params->constraints);
    }
    sp_cuda_check_errors();
    ph->iteration++; 
  }
  cudaFree(f1);
  return 0;
}

int phaser_iterate_raar_cuda(SpPhaser * ph,int iterations){
  SpPhasingRAARParameters * params = (SpPhasingRAARParameters *)ph->algorithm->params;
  for(int i = 0;i<iterations;i++){
    real beta = sp_smap_interpolate(params->beta,ph->iteration);
    cufftComplex * swap = ph->d_g0;
    ph->d_g0 = ph->d_g1;
    ph->d_g1 = swap;
    /* executes FFT processes */
    cufftExecC2C(ph->cufft_plan, ph->d_g0, ph->d_g1, CUFFT_FORWARD);
    sp_cuda_check_errors();
    CUDA_apply_fourier_constraints<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1,ph->image_size,params->constraints);
    sp_cuda_check_errors();
    if(ph->phasing_objective == SpRecoverPhases){
      CUDA_module_projection<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1,ph->d_amplitudes,ph->d_amplitudes_min,ph->d_amplitudes_max,ph->d_pixel_flags,ph->image_size,params->constraints);
    }else if(ph->phasing_objective == SpRecoverAmplitudes){
      CUDA_phased_amplitudes_projection<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1,ph->d_phased_amplitudes,ph->d_pixel_flags,ph->image_size);
    }else{
      abort();
    }
    sp_cuda_check_errors();
    cufftExecC2C(ph->cufft_plan, ph->d_g1, ph->d_gp, CUFFT_INVERSE);
    /* normalize */
    CUDA_complex_scale<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_gp,ph->image_size, 1.0f / (ph->image_size));
    sp_cuda_check_errors();
    CUDA_support_projection_raar<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1,ph->d_g0,ph->d_gp,ph->d_pixel_flags,ph->image_size,beta);
    sp_cuda_check_errors();
    if(params->constraints != SpNoConstraints){
      CUDA_apply_constraints<<<ph->number_of_blocks, ph->threads_per_block>>>(ph->d_g1,ph->d_pixel_flags,ph->image_size,params->constraints);
    }
    sp_cuda_check_errors();
    ph->iteration++;
  }
  return 0;

}

