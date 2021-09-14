#include "point_particle.hpp"

#include <tuple>

#include <fmt/format.h>

using mathematics::vector;

std::tuple < float, vector<float,2>, vector<float,2> > distance_between_and_difference(point_particle const& p1, point_particle const& p2) {
	auto& pos1 = p1.get_value<NewtonianBody>()->position;
	auto& pos2 = p2.get_value<NewtonianBody>()->position;

	auto dist = std::hypotf(pos2[0] - pos1[0], pos2[1] - pos1[1]);
	auto diff = pos2-pos1;
	auto unit_vector_of_diff = (1/dist)*diff;

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
	vector<float, 2> vector_force = force * unit_dir;

	auto& vector_force_array1 = *(pc1.shared_force);
	auto& vector_force_array2 = *(pc2.shared_force);

	for (auto i = 0; i < 2; ++i)
		vector_force_array1[i] += vector_force[i];
	for (auto i = 0; i < 2; ++i)
		vector_force_array2[i] -= vector_force[i];
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


	auto const scalar_force = (k * c1 * c2) / (dist * dist);

	vector<float, 2> vector_force = scalar_force * unit_dir;

	for (auto i = 0; i < 2; ++i)
		(*pc1.shared_force)[i] += vector_force[i];
	for (auto i = 0; i < 2; ++i)
		(*pc2.shared_force)[i] -= vector_force[i];
}

