// 2d_physics.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "sim.hpp"

int main()
{


    point_particle_simulator sim;

    sim.spawn_particles();

    sim.generate_pairs();

    sim.run();

    return EXIT_SUCCESS;
}
