import os
import shutil
import distutils
from distutils import dir_util

sundials_dir = '/Users/tors10/Downloads/sundials-5.5.0'

source_dir = '/Users/tors10/Development/FMPy/fmpy/fmucontainer/sources'

for component in ['cvode', 'nvector', 'sundials', 'sunlinsol', 'sunlinsol', 'sunmatrix', 'sunnonlinsol']:
    dir_util.copy_tree(src=os.path.join(sundials_dir, 'src', component), dst=os.path.join(source_dir, component))
    dir_util.copy_tree(src=os.path.join(sundials_dir, 'include', component), dst=os.path.join(source_dir, component))

dir_util.copy_tree(src=os.path.join(sundials_dir, 'build', 'include', 'sundials'), dst=os.path.join(source_dir, 'sundials'))

for src, dst in [
    (('cvode', 'cvode.c'), ('cvode.c',)),
    (('cvode', 'cvode_direct.c'), ('cvode_direct.c',)),
    (('cvode', 'cvode_io.c'), ('cvode_io.c',)),
    (('cvode', 'cvode_ls.c'), ('cvode_ls.c',)),
    (('cvode', 'cvode_impl.h'), ('cvode_impl.h',)),
    (('cvode', 'cvode_ls_impl.h'), ('cvode_ls_impl.h',)),
    (('cvode', 'cvode_nls.c'), ('cvode_nls.c',)),
    (('cvode', 'cvode_proj.c'), ('cvode_proj.c',)),
    (('cvode', 'cvode_proj_impl.h'), ('cvode_proj_impl.h',)),
    (('sundials', 'sundials_debug.h'), ('sundials_debug.h',)),
    (('nvector', 'serial', 'nvector_serial.c'), ('nvector_serial.c',)),
    (('sundials', 'sundials_band.c'), ('sundials_band.c',)),
    (('sundials', 'sundials_dense.c'), ('sundials_dense.c',)),
    (('sundials', 'sundials_linearsolver.c'), ('sundials_linearsolver.c',)),
    (('sundials', 'sundials_math.c'), ('sundials_math.c',)),
    (('sundials', 'sundials_matrix.c'), ('sundials_matrix.c',)),
    (('sundials', 'sundials_nonlinearsolver.c'), ('sundials_nonlinearsolver.c',)),
    (('sundials', 'sundials_nvector.c'), ('sundials_nvector.c',)),
    (('sundials', 'sundials_nvector_senswrapper.c'), ('sundials_nvector_senswrapper.c',)),
    (('sunlinsol', 'band', 'sunlinsol_band.c'), ('sunlinsol_band.c',)),
    (('sunlinsol', 'dense', 'sunlinsol_dense.c'), ('sunlinsol_dense.c',)),
    (('sunmatrix', 'dense', 'sunmatrix_dense.c'), ('sunmatrix_dense.c',)),
    (('sunmatrix', 'band', 'sunmatrix_band.c'), ('sunmatrix_band.c',)),
    (('sunnonlinsol', 'newton', 'sunnonlinsol_newton.c'), ('sunnonlinsol_newton.c',)),
    (('sunnonlinsol', 'fixedpoint', 'sunnonlinsol_fixedpoint.c'), ('sunnonlinsol_fixedpoint.c',)),
]:
    os.rename(src=os.path.join(source_dir, *src), dst=os.path.join(source_dir, *dst))

