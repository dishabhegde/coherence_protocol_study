project(coherentCache)
add_library(coherentCache SHARED cache.c stree.c)
target_include_directories(coherentCache PRIVATE ../common)
