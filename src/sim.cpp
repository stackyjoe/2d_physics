#include "sim.hpp"

#include <tuple>
#include <numbers>

#include <fmt/format.h>

point_particle_simulator::point_particle_simulator()
	: mt(rd()),
	delta_dist(-1.0, 1.0),
	window(sf::VideoMode(width, height), "Particle simulator"),
	v(sf::FloatRect(0, 0, width, height)),
	approx_fps(0.f),
	zoom_factor(1.0f)
	{
	
	if (!font.loadFromFile("sansation.ttf"))
		fmt::print("Font failed to load\n");


	v.zoom(zoom_factor);


}

void point_particle_simulator::select(sf::Vector2f end_pos, sf::Vector2f start_pos) {
	std::unique_lock l(selection_lock, std::try_to_lock);

	if (!l.owns_lock())
		return;

	std::atomic<size_t> hits = 0;

	auto is_in_the_box = [start_pos, end_pos, this](point_particle const* e) mutable -> bool {
		auto const& nc = *e->get_value<NewtonianBody>();

		auto lx = std::min(start_pos.x, end_pos.x);
		auto bx = std::max(start_pos.x, end_pos.x);
		auto ly = std::min(start_pos.y, end_pos.y);
		auto by = std::max(start_pos.y, end_pos.y);

		if (lx <= nc.position[0] and nc.position[0] <= bx and ly <= nc.position[1] and nc.position[1] <= by) {
			return true;
		}
		return false;
	};

	auto const select = [this, is_in_the_box](std::unique_ptr<point_particle>const& e) mutable {
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
	std::thread get_statistics([this, l = std::move(l)]() {
		float total_mass = 0;
		float total_charge = 0;
		float total_scalar_momentum = 0;
		for (auto* p : current_selection) {
			auto& nc = *p->get_component<PhysicalComponent>();
			auto& ec = *p->get_component<ElectricalComponent>();
			total_mass += nc.mass;
			total_charge += ec.charge;
			total_scalar_momentum += nc.mass * (std::hypotf(nc.velocity[0], nc.velocity[1]));
		}

		fmt::print("Total mass is {}, charge is {}, and avg scalar momentum {}\n", total_mass, total_charge, total_scalar_momentum / current_selection.size());

	});
	get_statistics.detach();
}


bool point_particle_simulator::clear_current_selection() {
	std::unique_lock l(selection_lock, std::try_to_lock);
	if (l.owns_lock()) {
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
		auto& [container, callable] = pair;
		std::for_each(std::execution::par, container.begin(), container.end(), callable);
	};

	auto const clear_it = [](std::unique_ptr<point_particle>& p) {
		auto& nc = *p->get_value<PhysicalComponent>();
		auto& force_vector = *(nc.shared_force);

		for (auto& val : force_vector)
			val = 0;
		for (auto& val : nc.acceleration)
			val = 0;
	};


	auto const wiggle = [this](std::unique_ptr<point_particle>& owning_ptr) {
		auto& p = *owning_ptr;

		auto& nc = *p.get_value<NewtonianBody>();
		auto& shape = *p.get_value<sf::CircleShape>();

		/*
		nc.x += wiggle_factor * this->gen_random_float();
		nc.y += wiggle_factor * this->gen_random_float();
		nc.dx_dt += wiggle_factor * this->gen_random_float();
		nc.dy_dt += wiggle_factor * this->gen_random_float();
		*/
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

		mathematics::vector<float, 2> total_force;
		for(auto i = 0; i < 2; ++i)
			total_force[i] = (*nc.shared_force)[i];

		nc.acceleration += (1/m) * total_force;
		nc.velocity += (dt * 0.5f) * nc.acceleration;
		nc.position += dt * nc.velocity;

		shape.setPosition(nc.position[0], nc.position[1]);
	};

	auto& particles = manager.get_storage_for_entities();

	auto const linear_interaction = [&](auto&) mutable {
		auto& list = manager.get_storage_for_entities();
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
		//std::make_pair(std::reference_wrapper(particles), wiggle),
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

	auto& gc = manager.get_storage_for_component<GraphicComponent>();
	draw_function(gc);
}

void point_particle_simulator::spawn_particles() {


	for (size_t i = 0; i < num_dots; ++i) {
		sf::CircleShape particle(particle_display_size);

		float const theta = static_cast<float>(std::numbers::pi) * delta_dist(mt);
		float const r = delta_dist(mt);

		float const mass = 1836.f;
		float const charge = 1.f;

		float const x = width / 2 + placement_scale_factor * r * cos(theta) * smaller_dimension / 3;
		float const y = height / 2 + placement_scale_factor * r * sin(theta) * smaller_dimension / 3;

		particle.setPosition(x, y);


		if (charge == 1.f)
			particle.setFillColor(sf::Color::Red);
		else if (charge == -1.f)
			particle.setFillColor(sf::Color::Blue);
		else if (charge == 0.f)
			particle.setFillColor(sf::Color(200, 200, 200));
		else
			particle.setFillColor(sf::Color::Yellow);

		manager.push_back(std::make_unique<point_particle>(PhysicalComponent(x, y, mass), ElectricalComponent(charge), Selectable(), std::move(particle)));
	}

	for (size_t i = 0; i < num_dots * 3; ++i) {
		sf::CircleShape particle(particle_display_size);

		float const theta = static_cast<float>(std::numbers::pi) * delta_dist(mt);
		float const r = delta_dist(mt);

		float const mass = 1837.f;
		float const charge = 0.f;

		float const x = width / 2 + placement_scale_factor * (1 - std::copysignf(r, r)) * cos(theta) * smaller_dimension / 4;
		float const y = height / 2 + placement_scale_factor * (1 - std::copysignf(r, r)) * sin(theta) * smaller_dimension / 4;

		particle.setPosition(x, y);

		if (charge == 1.f)
			particle.setFillColor(sf::Color::Red);
		else if (charge == -1.f)
			particle.setFillColor(sf::Color::Blue);
		else if (charge == 0.f)
			particle.setFillColor(sf::Color(150, 150, 150));
		else
			particle.setFillColor(sf::Color::Yellow);

		auto p = std::make_unique<point_particle>(PhysicalComponent(x, y, mass), ElectricalComponent(charge), Selectable(), std::move(particle));

		manager.push_back(std::move(p));
	}


	for (size_t i = 0; i < num_dots; ++i) {
		sf::CircleShape particle(particle_display_size);

		float const theta = static_cast<float>(std::numbers::pi) * delta_dist(mt);
		float const raw_r = delta_dist(mt);
		float const r = std::copysignf(std::powf(raw_r, 2.f), raw_r);

		float const mass = 1.f;
		float const charge = -1.f;

		float const x = width / 2 + placement_scale_factor * (1 - std::copysignf(r * r, r)) * cos(theta) * smaller_dimension;
		float const y = height / 2 + placement_scale_factor * (1 - std::copysignf(r * r, r)) * sin(theta) * smaller_dimension;

		particle.setPosition(x, y);

		if (charge == 1.f)
			particle.setFillColor(sf::Color::Red);
		else if (charge == -1.f)
			particle.setFillColor(sf::Color::Blue);
		else if (charge == 0.f)
			particle.setFillColor(sf::Color(200, 200, 200));
		else
			particle.setFillColor(sf::Color::Yellow);

		manager.push_back(std::make_unique<point_particle>(PhysicalComponent(x, y, mass), charge, Selectable(), std::move(particle)));
	}
}

float point_particle_simulator::gen_random_float() {
	return delta_dist(mt);
}

void point_particle_simulator::draw_function(std::vector<GraphicComponent*>& graphical_representations) {

		window.clear();

		auto draw_a_single_component = [this](auto* gc) {
			this->window.draw(*gc);
		};

		std::for_each(std::execution::seq, graphical_representations.begin(), graphical_representations.end(), draw_a_single_component);


		sf::Text txt;
		txt.setFillColor(sf::Color::Green);
		txt.setOutlineColor(sf::Color::Magenta);
		txt.setOutlineThickness(4.f);
		txt.setFont(font);
		txt.setString(std::to_string(approx_fps));
		txt.setCharacterSize(32);

		auto base_pos = window.getView().getViewport().getPosition();

		txt.setPosition(base_pos);
		window.draw(txt);


		window.display();
}

void point_particle_simulator::run() {

	window.setFramerateLimit(60);
	window.setView(v);
	window.display();

	auto start_pos_right = window.mapPixelToCoords(sf::Mouse::getPosition(window), v);
	auto end_pos_right = start_pos_right;

	auto start_pos_left = window.mapPixelToCoords(sf::Mouse::getPosition(window), v);
	auto end_pos_left = start_pos_right;


	bool run = false;

	//size_t cycle_count = 0;

	while (window.isOpen())
	{
		auto const tick = clock.now();

		if (run)
			physical_interaction();

		sf::Event event;
		while (window.pollEvent(event))
		{

			if (event.type == sf::Event::Closed)
				window.close();
			else if (event.type == sf::Event::KeyPressed) {
				if (event.key.code == sf::Keyboard::Space) {
					run = !run;
				}
			}
			else if (event.type == sf::Event::MouseWheelScrolled) {
				if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {

					if (event.mouseWheelScroll.delta <= 0)
						zoom_factor /= 1.1f;
					else
						zoom_factor *= 1.1f;


					v.setSize(width * zoom_factor, height * zoom_factor);
					std::cout << "Zoom factor is: " << zoom_factor << "\n";
					v.zoom(zoom_factor);
					window.setView(v);
				}
			}
			else if (event.type == sf::Event::MouseButtonPressed) {
				if (event.mouseButton.button == sf::Mouse::Right) {
					start_pos_right = window.mapPixelToCoords(sf::Mouse::getPosition(window), v);
				}
				if (event.mouseButton.button == sf::Mouse::Left) {
					clear_current_selection();
					start_pos_left = window.mapPixelToCoords(sf::Mouse::getPosition(window), v);
				}
				v.getCenter();

			}

			else if (event.type == sf::Event::MouseButtonReleased) {
				if (event.mouseButton.button == sf::Mouse::Right) {
					end_pos_right = window.mapPixelToCoords(sf::Mouse::getPosition(window), v);

					sf::Vector2f translation;
					translation.x = -1.f * (end_pos_right.x - start_pos_right.x);
					translation.y = -1.f * (end_pos_right.y - start_pos_right.y);


					if (translation.x != 0.f and translation.y != 0.f) {
						v.move(translation);
						window.setView(v);
					}
				}

				if (event.mouseButton.button == sf::Mouse::Left) {
					end_pos_left = window.mapPixelToCoords(sf::Mouse::getPosition(window), v);


					fmt::print("Selected the region from ({},{}) to ({},{})\n", end_pos_left.x, end_pos_left.y, start_pos_left.x, start_pos_left.y);
					select(end_pos_left, start_pos_left);

				}
			}
		}

		draw();


		auto const tock = clock.now();
		auto const diff = tock - tick;

		approx_fps = 1000.f / std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
		//fmt::print("Approx fps: {}\n", approx_fps);
	}

}

