#include "particle.hpp"

#include <algorithm>
#include <cmath>
#include <execution>
#include <iostream>

constexpr float g = -0.000981f;
constexpr float k = 89755.1f;
constexpr float dt = 0.01f;


auto distance_between_and_difference(point_particle const& p1, point_particle const& p2) {
	auto pos1 = p1.get_position();
	auto pos2 = p2.get_position();

	auto dist = std::hypotf(pos2.first - pos1.first, pos2.second - pos1.second);
	auto diff = std::make_pair(pos2.second - pos1.second, pos2.first - pos1.first);
	auto unit_vector_of_diff = std::make_pair(diff.first/dist, diff.second/dist);

	return std::make_tuple( dist, diff, unit_vector_of_diff);
}

auto compare_by_distance = [](auto const& pair1, auto const& pair2) -> bool {
	auto [dist1, diff1, unit_diff1] = distance_between_and_difference(pair1.first, pair1.second);
	auto [dist2, diff2, unit_diff2] = distance_between_and_difference(pair2.first, pair2.second);
	return dist1 < dist2;
};


void mass_interaction(point_particle& p1, point_particle& p2) {
	auto [dist, dir, unit_dir] = distance_between_and_difference(p1, p2);

	auto force = (g * p1.mass * p2.mass) / (dist * dist);

	if (force < 0.001)
		return;

	p1.dx_d2t.first += (force*unit_dir.first)/p1.mass;
	p1.dx_d2t.second += (force*unit_dir.second)/p1.mass;
	p2.dx_d2t.first -= (force*unit_dir.first)/p2.mass;
	p2.dx_d2t.second -= (force*unit_dir.second)/p2.mass;
};

void electrical_interaction(point_particle &p1, point_particle &p2) {
	auto [dist, dir, unit_dir] = distance_between_and_difference(p1, p2);

	auto force = (k * p1.charge * p2.charge) / (dist * dist);

	if (force < 0.001)
		return;

	p1.dx_d2t.first += (force * unit_dir.first) / p1.mass;
	p1.dx_d2t.second += (force * unit_dir.second) / p1.mass;
	p2.dx_d2t.first -= (force * unit_dir.first) / p2.mass;
	p2.dx_d2t.second -= (force * unit_dir.second) / p2.mass;


}

void point_particle::act_on(point_particle &other) const {
	auto [dist, dir, unit_dir] = distance_between_and_difference(*this, other);

	auto grav_force = (g * mass * other.mass) / (dist * dist);
	auto elec_force = (k * charge * other.charge) / (dist * dist);

	if (grav_force >= 0.001) {
		other.dx_d2t.first -= (grav_force * unit_dir.first) / other.mass;
		other.dx_d2t.second -= (grav_force * unit_dir.second) / other.mass;
	}

	if (elec_force >= 0.001) {
		other.dx_d2t.first -= (elec_force * unit_dir.first) / other.mass;
		other.dx_d2t.second -= (elec_force * unit_dir.second) / other.mass;
	}
		

}

void neutron::act_on(point_particle& other) const {
	auto [dist, dir, unit_dir] = distance_between_and_difference(*this, other);

	auto grav_force = (g * mass * other.mass) / (dist * dist);
	auto elec_force = k * (std::sinf(std::expf(dist))/std::expf(dist)) * other.charge;

	if (grav_force >= 0.001) {
		other.dx_d2t.first -= (grav_force * unit_dir.first) / other.mass;
		other.dx_d2t.second -= (grav_force * unit_dir.second) / other.mass;
	}

}




void point_particle_simulator::sort_pairs() {
	std::cout << "Begin sort" << std::endl;
	std::sort(distinct_pairs.begin(), distinct_pairs.end(), compare_by_distance);
	std::cout << "End sort" << std::endl;
}

void point_particle_simulator::generate_pairs() {
	distinct_pairs.clear();
	distinct_pairs.reserve(particles.size() * particles.size());
	for (auto first = particles.begin(); first != particles.end(); ++first) {
		for (auto second = first + 1; second != particles.end(); ++second) {
			distinct_pairs.push_back(std::make_pair(std::reference_wrapper(**first), std::reference_wrapper(**second)));
		}
	}
}

void point_particle_simulator::interact() {

	constexpr float wiggle_factor = 1.f;

	auto perform = [](auto& pair) {
		auto [container, callable] = pair;
		std::for_each(std::execution::par, container.begin(), container.end(), callable);
	};

	auto wiggle = [this](auto& p) {
		auto pos = p->shape.getPosition();
		p->shape.setPosition(pos.x + wiggle_factor * this->gen_random_float(), pos.y + wiggle_factor * this->gen_random_float());
		p->dx_dt = std::make_pair(p->dx_dt.first + wiggle_factor * this->gen_random_float(), p->dx_dt.second + wiggle_factor * this->gen_random_float());
	};

	auto interaction = [](auto& pair) {
		point_particle& p1 = pair.first;
		point_particle& p2 = pair.second;
		p1.act_on(p2);
		p2.act_on(p1);
		//std::apply(mass_interaction, pair);
		//std::apply(electrical_interaction, pair);
	};

	auto move_it = [](auto& p) {
		p->dx_dt = std::make_pair(p->dx_dt.first + dt * 0.5f * p->dx_d2t.first, p->dx_dt.second + dt * 0.5f * p->dx_d2t.second);
		p->shape.setPosition(p->shape.getPosition().x + dt * p->dx_dt.first, p->shape.getPosition().y + dt * p->dx_dt.second);
	};

	auto pairs = std::tuple(std::make_pair(std::reference_wrapper(particles), wiggle), std::make_pair(std::reference_wrapper(distinct_pairs), interaction), std::make_pair(std::reference_wrapper(particles), move_it));

	// Using ... first gets correct expansion order
	std::apply([perform](auto ... pair) {(...,perform(pair)); }, pairs);

	/*
	std::for_each(std::execution::par, particles.begin(), particles.end(), wiggle);
	

	
	std::for_each(std::execution::par, distinct_pairs.begin(), distinct_pairs.end(), interaction);


	std::for_each(std::execution::par, particles.begin(), particles.end(), move_it);
	*/


}