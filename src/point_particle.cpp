#include "point_particle.hpp"

#include <tuple>

#include <fmt/format.h>



std::tuple < float, std::pair<float,float>, std::pair<float,float> > distance_between_and_difference(point_particle const& p1, point_particle const& p2) {
	auto& pos1 = *p1.get_value<NewtonianBody>();
	auto& pos2 = *p2.get_value<NewtonianBody>();

	auto dist = std::hypotf(pos2.x - pos1.x, pos2.y - pos1.y);
	auto diff = std::make_pair(pos2.x - pos1.x, pos2.y - pos1.y);
	auto unit_vector_of_diff = std::make_pair(diff.first / dist, diff.second / dist);

	return std::make_tuple(dist, diff, unit_vector_of_diff);
}

bool compare_by_distance(std::pair<point_particle*, point_particle*> const & pair1, std::pair<point_particle*, point_particle*> const & pair2) {
	auto [dist1, diff1, unit_diff1] = distance_between_and_difference(* pair1.first, * pair1.second);
	auto [dist2, diff2, unit_diff2] = distance_between_and_difference(* pair2.first, * pair2.second);
	return dist1 < dist2;
};


void mass_interaction(point_particle* p1, point_particle * p2) {
	auto [dist, dir, unit_dir] = distance_between_and_difference(*p1, *p2);

	auto& pc1 = *p1->get_value<PhysicalComponent>();
	auto& pc2 = *p2->get_value<PhysicalComponent>();

	auto const m1 = pc1.mass;
	auto const m2 = pc2.mass;

	auto const force = (g * m1 * m2) / (dist * dist);

	pc1.shared_force->x += force * unit_dir.first;
	pc1.shared_force->y += force * unit_dir.second;
	pc2.shared_force->x -= force * unit_dir.first;
	pc2.shared_force->y -= force * unit_dir.second;
};

void electrical_interaction(point_particle * p1, point_particle* p2) {
	auto [dist, dir, unit_dir] = distance_between_and_difference(*p1, *p2);

	auto& pc1 = *p1->get_value<PhysicalComponent>();
	auto& pc2 = *p2->get_value<PhysicalComponent>();

	auto const m1 = pc1.mass;
	auto const m2 = pc2.mass;

	auto& ec1 = *p1->get_value<ElectricalComponent>();
	auto& ec2 = *p2->get_value<ElectricalComponent>();

	auto const c1 = ec1.charge;
	auto const c2 = ec2.charge;


	auto const force = (k * c1 * c2) / (dist * dist);


	pc1.shared_force->x += force * unit_dir.first;
	pc1.shared_force->y += force * unit_dir.second;
	pc2.shared_force->x -= force * unit_dir.first;
	pc2.shared_force->y -= force * unit_dir.second;

}

