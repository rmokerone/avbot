
add_subdirectory(avproto)

add_library(libavim STATIC src/libavim.cpp src/avim_impl.cpp)
target_link_libraries(libavim avim++)

target_include_directories(libavim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
