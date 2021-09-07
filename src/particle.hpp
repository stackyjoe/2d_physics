#pragma once

#include <atomic>
#include <functional>
#include <random>
#include <vector>

#include <SFML/Graphics.hpp>
#include <SFML/Main.hpp>
#include <SFML/Window.hpp>

struct point_particle
{
	point_particle() = delete;
	point_particle(float mass, float charge, sf::CircleShape &&shape) :  mass(mass), charge(charge), shape(shape) { }

	std::pair<float,float> get_position() const {
		return std::make_pair(shape.getPosition().x, shape.getPosition().y);
	}

	virtual void act_on(point_particle& other) const;

	float mass;
	float charge;
	std::pair<float, float> dx_dt;
	std::pair<std::atomic<float>, std::atomic<float>> dx_d2t;
	// This will hold the position
	sf::CircleShape shape;

};

struct neutron : public point_particle {
	neutron(float mass, float charge, sf::CircleShape&& shape) : point_particle(mass, charge, std::move(shape)) { }

	void act_on(point_particle& other) const override;
};



struct point_particle_simulator {
	point_particle_simulator() = default;
	point_particle_simulator(std::function<float(void)> &&gen_random_float) : gen_random_float(gen_random_float) {}

	template<typename ...Args>
	void add_point_particle(Args &&...args) {
		particles.emplace_back(std::make_unique<point_particle>(std::move(args)...));
	}

	template<typename ...Args>
	void  add_neutron(Args&& ...args) {
		particles.emplace_back(std::make_unique<neutron>(std::move(args)...));
	}

	void generate_pairs();

	void interact();

	void sort_pairs();


	std::function<float(void)> gen_random_float;
	std::vector<std::unique_ptr<point_particle>> particles;
	std::vector<std::pair<std::reference_wrapper<point_particle>, std::reference_wrapper<point_particle>>> distinct_pairs;

};
