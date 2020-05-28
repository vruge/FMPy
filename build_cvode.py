import tarfile
import requests
import os
import shutil
from subprocess import check_call

url = 'https://computing.llnl.gov/projects/sundials/download/cvode-5.3.0.tar.gz'
filename = os.path.basename(url)

response = requests.get(url)

with open(filename, 'wb') as f:
    f.write(response.content)

with tarfile.open(filename, "r:gz") as tar:
    tar.extractall()
    tar.close()

os.mkdir('cvode-5.3.0/static')

# build CVode as static library
check_call([
    'cmake',
    '-DEXAMPLES_ENABLE_C=OFF',
    '-DBUILD_SHARED_LIBS=OFF',
    '-DCMAKE_INSTALL_PREFIX=cvode-5.3.0/static/install',
    '-DCMAKE_USER_MAKE_RULES_OVERRIDE=../OverrideMSVCFlags.cmake',
    '-S', 'cvode-5.3.0',
    '-B', 'cvode-5.3.0/static'
])

check_call(['cmake', '--build', 'cvode-5.3.0/static', '--target', 'install', '--config', 'Release'])

# build CVode as dynamic library
check_call([
    'cmake',
    '-DEXAMPLES_ENABLE_C=OFF',
    '-DBUILD_STATIC_LIBS=OFF',
    '-DCMAKE_INSTALL_PREFIX=cvode-5.3.0/dynamic/install',
    '-DCMAKE_USER_MAKE_RULES_OVERRIDE=../OverrideMSVCFlags.cmake',
    '-S', 'cvode-5.3.0',
    '-B', 'cvode-5.3.0/dynamic'
])

check_call(['cmake', '--build', 'cvode-5.3.0/dynamic', '--target', 'install', '--config', 'Release'])

os.mkdir('fmpy/sundials/x86_64-windows')

shutil.copy('cvode-5.3.0/dynamic/install/lib/sundials_cvode.dll',          'fmpy/sundials/x86_64-windows')
shutil.copy('cvode-5.3.0/dynamic/install/lib/sundials_nvecserial.dll',     'fmpy/sundials/x86_64-windows')
shutil.copy('cvode-5.3.0/dynamic/install/lib/sundials_sunlinsoldense.dll', 'fmpy/sundials/x86_64-windows')
shutil.copy('cvode-5.3.0/dynamic/install/lib/sundials_sunmatrixdense.dll', 'fmpy/sundials/x86_64-windows')

# build cswrapper
os.mkdir('cswrapper/build')

check_call([
    'cmake',
    '-DCVODE_INSTALL_DIR=../cvode-5.3.0/static/install',
    '-S', 'cswrapper',
    '-B', 'cswrapper/build'
])

check_call(['cmake', '--build', 'cswrapper/build', '--config', 'Release'])
