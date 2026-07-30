# add_library(utilities INTERFACE)
# target_include_directories(utilities INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
# target_compile_definitions(utilities INTERFACE LOG_LEVEL=LOG_LEVEL_INFO)

add_library(utilities STATIC log.c)
target_include_directories(utilities PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(utilities PUBLIC ${CMAKE_SOURCE_DIR}/Core/Inc)
target_compile_definitions(utilities PUBLIC LOG_LEVEL=LOG_LEVEL_INFO)
target_link_libraries(utilities PUBLIC stm32cubemx)