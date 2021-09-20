add_library(src STATIC)

set( FILE_LIST 2d_physics.cpp
	entity.hpp
	sim.hpp sim.cpp
	mathematics.hpp
	point_particle.cpp point_particle.hpp
	tuple_of_optionals.hpp
	TypeList.hpp)

list(TRANSFORM FILE_LIST PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")

target_sources(src PUBLIC ${FILE_LIST} )
target_link_libraries(src PUBLIC libs)
target_include_directories(src PUBLIC ${fmt_headers} ${sfml_headers})