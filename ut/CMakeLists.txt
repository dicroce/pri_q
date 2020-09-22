cmake_minimum_required (VERSION 2.8.12)

add_executable (pri_ut include/framework.h
                       include/test_pri_q.h
                       source/framework.cpp
                       source/test_pri_q.cpp)
                      
target_include_directories (pri_ut PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(pri_ut pri cppkit bzip2 pthread)
