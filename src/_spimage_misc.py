import numpy as np
import scipy as sp
__all__ = ['grid', 'rgrid']

def grid(shape, center=None):
    if center is None: center = (0,0)
    yy, xx = np.indices(shape)
    yy = yy.astype(np.float) - ( center[1] + shape[1]/2. - 0.5 )
    xx = xx.astype(np.float) - ( center[0] + shape[0]/2. - 0.5 )
    return yy, xx

def rgrid(shape, center=None):
    if center is None: center = (0, 0)
    yy, xx = grid(shape, center)
    rr = np.sqrt( (xx**2) + (yy**2) )
    rr[np.where(rr == 0)] = 1e-20
    return rr
