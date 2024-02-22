if (APPLE AND (NOT CMAKE_COMPILER_IS_GNUCC))
  message(FATAL_ERROR "Use gcc from brew")
endif ()

if ((NOT CMAKE_COMPILER_IS_GNUCC) OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.2)
  message(FATAL_ERROR "Install gcc-11.2 or newer")
endif ()

set(CMAKE_CXX_FLAGS_ASAN "-g -fsanitize=address,undefined -fno-sanitize-recover=all"
  CACHE STRING "Compiler flags in asan build"
  FORCE)

set(CMAKE_CXX_FLAGS_TSAN "-g -fsanitize=thread -fno-sanitize-recover=all"
  CACHE STRING "Compiler flags in tsan build"
  FORCE)
