cmake_minimum_required(VERSION 2.8)
project(clientcache)

include_directories(
  "${PROJECT_SOURCE_DIR}/../boost_1_52_0"
  "${PROJECT_SOURCE_DIR}/../openssl-1.0.1c/inc32"
)

link_directories(
  "${PROJECT_SOURCE_DIR}/../boost_1_52_0/stage/lib"
  "${PROJECT_SOURCE_DIR}/../openssl-1.0.1c/out32dll"
)

add_executable(clientcache
  cache.hpp
  cacheimpl.hpp
  crypt.hpp
  scoped_handle.hpp
  os.hpp
  stdinc.hpp
  cacheimpl.cpp
  crypt.cpp
  os.cpp
  unittest.cpp
)

target_link_libraries(clientcache
  libeay32.lib
)