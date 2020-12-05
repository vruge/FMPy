cmake_minimum_required (VERSION 3.2)

project (FMUContainer)

if (WIN32)
  set(FMI_PLATFORM win)
elseif (APPLE)
  set(FMI_PLATFORM darwin)
else ()
  set(FMI_PLATFORM linux)
endif ()

if ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
  set (FMI_PLATFORM ${FMI_PLATFORM}64)
else ()
  set (FMI_PLATFORM ${FMI_PLATFORM}32)
endif ()

if (MSVC)
  # link statically against the the Visual C runtime
  string(REPLACE "/MD"  "/MT"  CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
  string(REPLACE "/MDd" "/MTd" CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG}")

  # disable compiler warnings
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
endif ()

add_library(FMUContainer SHARED
  ../c-code/fmi2Functions.h
  ../c-code/fmi2FunctionTypes.h
  ../c-code/fmi2TypesPlatform.h
  sources/FMUContainer.c
  sources/mpack.h
  sources/mpack.c
  sources/cvode.c
  sources/cvode_direct.c
  sources/cvode_io.c
  sources/cvode_ls.c
  sources/cvode_nls.c
  sources/cvode_proj.c
  sources/nvector_serial.c
  sources/sundials_band.c
  sources/sundials_dense.c
  sources/sundials_linearsolver.c
  sources/sundials_math.c
  sources/sundials_matrix.c
  sources/sundials_nonlinearsolver.c
  sources/sundials_nvector.c
  sources/sundials_nvector_senswrapper.c
  sources/sunlinsol_band.c
  sources/sunlinsol_dense.c
  sources/sunmatrix_dense.c
  sources/sunmatrix_band.c
  sources/sunnonlinsol_fixedpoint.c
  sources/sunnonlinsol_newton.c
)

SET_TARGET_PROPERTIES(FMUContainer PROPERTIES PREFIX "")

target_include_directories(FMUContainer PUBLIC
  sources
  ../c-code
)

target_link_libraries(FMUContainer
  ${CMAKE_DL_LIBS}
)

add_custom_command(TARGET FMUContainer POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
  "$<TARGET_FILE:FMUContainer>"
  "${CMAKE_CURRENT_SOURCE_DIR}/binaries/${FMI_PLATFORM}"
)
