add_library(libs INTERFACE)

set(BUILD_SHARED_LIBS FALSE)
add_subdirectory(SFML)
target_link_libraries(libs INTERFACE 
    sfml-graphics
    sfml-network
    sfml-audio
    sfml-window
    sfml-system
  )

set(sfml_headers "${SFML_ABS_PATH}/include")
  
target_include_directories(libs INTERFACE ${sfml_headers})


add_subdirectory(fmt)
find_package(fmt)
set(fmt_headers "${CMAKE_CURRENT_SOURCE_DIR}/fmt/include/")
set(fmt_PATH "${CMAKE_CURRENT_SOURCE_DIR}/fmt")

target_link_libraries(libs INTERFACE fmt::fmt)
target_include_directories(libs INTERFACE fmt_headers)

set(lib_headers ${sfml_headers} ${fmt_headers})