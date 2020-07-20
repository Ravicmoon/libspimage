# - Find numpy
# Find the native numpy includes
# This module defines
#  PYTHON_NUMPY_INCLUDE_DIR, where to find numpy/arrayobject.h, etc.
#  PYTHON_NUMPY_FOUND, If false, do not try to use numpy headers.

if(PYTHON_NUMPY_INCLUDE_DIR)
  # in cache already
  set (PYTHON_NUMPY_FIND_QUIETLY TRUE)
endif()

if(PYTHON_FOUND)
  execute_process(
    COMMAND
    ${PYTHON_EXECUTABLE} -c "import numpy as np; print(np.get_include())"
    OUTPUT_VARIABLE PYTHON_NUMPY_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(PYTHON_NUMPY_INCLUDE_DIR)
    set(PYTHON_NUMPY_FOUND TRUE)
    set(PYTHON_NUMPY_INCLUDE_DIR ${PYTHON_NUMPY_INCLUDE_DIR} CACHE PATH "Numpy include path")
  else()
    set(PYTHON_NUMPY_FOUND FALSE)
  endif()
endif()

if(PYTHON_NUMPY_FOUND)
  if(NOT PYTHON_NUMPY_FIND_QUIETLY)
    message (STATUS "Numpy headers found")
  endif()
else()
  if(PYTHON_NUMPY_FIND_REQUIRED)
    message (FATAL_ERROR "Numpy headers missing")
  endif()
endif()

MARK_AS_ADVANCED (PYTHON_NUMPY_INCLUDE_DIR)
