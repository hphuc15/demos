add_library(bh1750 STATIC bh1750.c)
target_include_directories(bh1750 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})