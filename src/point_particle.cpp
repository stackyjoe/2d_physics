#include "point_particle.hpp"

#include <execution>
#include <tuple>

#include <fmt/format.h>

constexpr float g = -0.000981f;
constexpr float k = 897.551f;
constexpr float dt = 0.1f;


auto distance_between_and_difference(point_particle const & p1, point_particle const& p2) {
	auto& pos1 = *p1.get_value<NewtonianBody>();
	auto& pos2 = *p2.get_value<NewtonianBody>();

	auto dist = std::hypotf(pos2.x - pos1.x, pos2.y - pos1.y);
	auto diff = std::make_pair(pos2.x - pos1.x, pos2.y - pos1.y);
	auto unit_vector_of_diff = std::make_pair(diff.first / dist, diff.second / dist);

	return std::make_tuple(dist, diff, unit_vector_of_diff);
}

auto compare_by_distance = [](auto const & pair1, auto const & pair2) -> bool {
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

void point_particle_simulator::select(sf::Vector2f end_pos, sf::Vector2f start_pos) {
	std::unique_lock l(selection_lock, std::try_to_lock);

	if (!l.owns_lock())
		return;

	std::atomic<size_t> hits = 0;

	auto is_in_the_box = [start_pos, end_pos, this](point_particle const *e) mutable -> bool{
		auto const & nc = *e->get_value<NewtonianBody>();

		auto lx = std::min(start_pos.x, end_pos.x);
		auto bx = std::max(start_pos.x, end_pos.x);
		auto ly = std::min(start_pos.y, end_pos.y);
		auto by = std::max(start_pos.y, end_pos.y);

		if (lx <= nc.x and nc.x <= bx and ly <= nc.y and nc.y <= by) {
			return true;
		}
		return false;
	};

	auto const select = [ this, is_in_the_box](std::unique_ptr<point_particle>const & e) mutable {
		if (is_in_the_box(e.get())) {
			auto& nc = *e->get_value<NewtonianBody>();
			auto& sel = *e->get_value<Selectable>();
			auto& gfx_cmp = *e->get_value<GraphicComponent>();
			sel.selected = true;
			auto cur_color = gfx_cmp.getFillColor();
			auto hl_color = sel.highlight_color;
			std::swap(cur_color, hl_color);
			gfx_cmp.setFillColor(cur_color);
			sel.highlight_color = hl_color;
			
			current_selection.push_back(e.get());
		}
	};


	std::for_each(std::execution::seq, manager.get_storage_for_entities().begin(), manager.get_storage_for_entities().end(), select);
	fmt::print("Selected {} particles\n", current_selection.size());
	std::thread get_statistics([this, l=std::move(l)]() {
		float total_mass = 0;
		float total_charge = 0;
		float total_scalar_momentum = 0;
		for (auto* p: current_selection) {
			auto &nc = *p->get_component<PhysicalComponent>();
			auto& ec = *p->get_component<ElectricalComponent>();
			total_mass += nc.mass;
			total_charge += ec.charge;
			total_scalar_momentum += nc.mass * (std::hypotf(nc.dx_dt,nc.dy_dt));
		}

		fmt::print("Total mass is {}, charge is {}, and avg scalar momentum {}\n", total_mass, total_charge, total_scalar_momentum/current_selection.size());

		for (auto* p : current_selection) {
			auto& sel = *p->get_component<Selectable>();
			auto& gfx_cmp = *p->get_component<GraphicComponent>();
			auto cur_color = gfx_cmp.getFillColor();
			auto hl_color = sel.highlight_color;
			std::swap(cur_color, hl_color);
			gfx_cmp.setFillColor(cur_color);
			sel.highlight_color = hl_color;
			sel.selected = false;
		}
		
	});
	get_statistics.detach();
}


bool point_particle_simulator::clear_current_selection() {
	std::unique_lock l(selection_lock, std::try_to_lock);
	if (l.owns_lock()) {
		current_selection.clear();
		return true;
	}

	return false;
}


void point_particle_simulator::sort_pairs() {
	fmt::print("Begin sort\n");
	std::sort(distinct_pairs.begin(), distinct_pairs.end(), compare_by_distance);
	fmt::print("End sort\n");
}

void point_particle_simulator::generate_pairs() {
	distinct_pairs.clear();

	auto& particles = manager.get_storage_for_entities();

	distinct_pairs.reserve(particles.size() * particles.size());
	for (auto first = particles.begin(); first != particles.end(); ++first) {
		for (auto second = first + 1; second != particles.end(); ++second) {
			distinct_pairs.push_back(std::make_pair(first->get(), second->get()));
		}
	}
}

std::thread point_particle_simulator::interact_in_separate_thread() {
	return std::thread([this]() mutable -> void {this->physical_interaction(); });
}

void point_particle_simulator::physical_interaction() {
	std::unique_lock l(interaction_lock, std::try_to_lock);

	if (!l.owns_lock())
		return;

	constexpr float wiggle_factor = 1.f;

	auto const perform = [](auto& pair) {
		auto &[container, callable] = pair;
		std::for_each(std::execution::par, container.begin(), container.end(), callable);
	};

	auto const clear_it = [](std::unique_ptr<point_particle>& p) {
		auto& nc = *p->get_value<PhysicalComponent>();
		nc.shared_force->clear();
	};



	auto const wiggle = [this](std::unique_ptr<point_particle>& owning_ptr) {
		auto& p = *owning_ptr;

		auto& nc = *p.get_value<NewtonianBody>();
		auto& shape = *p.get_value<sf::CircleShape>();
		nc.x += wiggle_factor * this->gen_random_float();
		nc.y += wiggle_factor * this->gen_random_float();
		nc.dx_dt += wiggle_factor * this->gen_random_float();
		nc.dy_dt += wiggle_factor * this->gen_random_float();
	};

	auto const interaction = [](std::pair<point_particle*, point_particle*>& pair) {
		std::apply(mass_interaction, pair);
		std::apply(electrical_interaction, pair);
	};

	auto const move_it = [](std::unique_ptr<point_particle>& owning_ptr) {
		auto& p = *owning_ptr;
		auto& nc = *p.get_value<NewtonianBody>();
		auto& shape = *p.get_value<sf::CircleShape>();

		auto const m = nc.mass;
		
		nc.d2x_dt2 += nc.shared_force->x / m;
		nc.d2y_dt2 += nc.shared_force->y / m;

		nc.dx_dt += dt * 0.5f * nc.d2x_dt2;
		nc.dy_dt += dt * 0.5f * nc.d2y_dt2;

		nc.x += dt * nc.dx_dt;
		nc.y += dt * nc.dy_dt;

		shape.setPosition(nc.x, nc.y);
	};

	auto& particles = manager.get_storage_for_entities();

	auto const linear_interaction = [&](auto &) mutable {
		auto & list = manager.get_storage_for_entities();
		size_t cycles = 0;
		for (auto entity_itr = list.begin(); entity_itr != list.end(); ++entity_itr) {
			for (auto entity_itr2 = entity_itr + 1; entity_itr2 != list.end(); ++entity_itr2) {
				++cycles;
				std::pair<point_particle*, point_particle*> a = std::make_pair(entity_itr->get(), entity_itr2->get());
				interaction(a);
			}
		}
		fmt::print("linear_interaction took {} cycles\n", cycles);
	};


	//std::vector<int> dummy(1, 0);

	auto const pairs = std::make_tuple(
		std::make_pair(std::reference_wrapper(particles), clear_it),
		std::make_pair(std::reference_wrapper(particles), wiggle),
		//std::make_pair(std::reference_wrapper(dummy), linear_interaction),
		std::make_pair(std::reference_wrapper(distinct_pairs), interaction),
		std::make_pair(std::reference_wrapper(particles), move_it));

	auto const perform_each_arg = [perform](auto const & ... pair) {(..., perform(pair)); };

	// Using ... first gets correct expansion order
	std::apply(perform_each_arg, pairs);

}

void point_particle_simulator::draw() {
	std::unique_lock l(draw_lock, std::try_to_lock);

	if (!l.owns_lock())
		return;

	auto & gc = manager.get_storage_for_component<GraphicComponent>();
	draw_function(gc);
}
