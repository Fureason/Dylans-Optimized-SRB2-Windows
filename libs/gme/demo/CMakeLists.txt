



include_directories(${CMAKE_SOURCE_DIR}/gme ${CMAKE_SOURCE_DIR})
link_directories(${CMAKE_BINARY_DIR}/gme)

add_executable(demo Wave_Writer.cpp basics.c)


add_custom_command(TARGET demo
    POST_BUILD
    COMMAND cmake -E copy "${CMAKE_SOURCE_DIR}/test.nsf" ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Add convenience copy of test.nsf file for demo application"
    VERBATIM)  

target_link_libraries(demo gme)
