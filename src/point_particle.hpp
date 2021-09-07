#pragma once

#include <functional>
#include <mutex>
#include <thread>

#include <SFML/Graphics.hpp>
#include <SFML/Main.hpp>
#include <SFML/Window.hpp>

#include "entity.hpp"


struct NewtonianBody {
	NewtonianBody() = delete;
	NewtonianBody(NewtonianBody const&) = delete;
	NewtonianBody& operator=(NewtonianBody const&) = delete;

	NewtonianBody(NewtonianBody&&) = default;
	NewtonianBody& operator=(NewtonianBody&&) = default;

	NewtonianBody(float x, float y, float mass)
		: x(x), y(y), dx_dt(0), dy_dt(0), d2x_dt2(0), d2y_dt2(0), mass(mass), shared_force(std::make_unique<thread_safe_data>()) { }

	float x;
	float y;
	float dx_dt;
	float dy_dt;
	float d2x_dt2;
	float d2y_dt2;
	const float mass;

	struct thread_safe_data {
		thread_safe_data() noexcept : x(0), y(0) { }
		thread_safe_data(float x, float y) noexcept : x(x), y(y) { }

		void clear() noexcept {
			x = 0;
			y = 0;
		}

		std::atomic<float> x;
		std::atomic<float> y;
	};

	std::unique_ptr<thread_safe_data> shared_force;

};

struct PointCharge {
	PointCharge(float charge) noexcept : charge(charge) { }

	const float charge;
};

struct Selectable {
	Selectable() noexcept : selected(false), highlight_color(sf::Color::Yellow) { }
	bool selected;
	sf::Color highlight_color;
};

using PhysicalComponent = NewtonianBody;

using ElectricalComponent = PointCharge;

using GraphicComponent = sf::CircleShape;

using point_particle = Entity<ListsViaTypes::TypeList<PhysicalComponent, ElectricalComponent, Selectable, GraphicComponent>>;
using EntityManagerType = EntityManager<point_particle, std::vector>;

class point_particle_simulator {
public:

	point_particle_simulator() = delete;
	point_particle_simulator(std::function<float(void)> && gen_random_float, std::function<void(std::vector<GraphicComponent*> &)> && draw_particles) : gen_random_float(gen_random_float), draw_function(draw_particles) {}

	void select(sf::Vector2f end_pos, sf::Vector2f start_pos);

	bool clear_current_selection();

	void reserve(size_t desired_capacity) {
		manager.reserve(desired_capacity);
	}

	void generate_pairs();

	std::thread interact_in_separate_thread();

	void sort_pairs();

	void draw();


	EntityManagerType manager;
private:

	void physical_interaction();

	std::function<float(void)> gen_random_float;



	std::function<void(std::vector<GraphicComponent*>&)> draw_function;

	std::vector<std::pair<point_particle*, point_particle*>> distinct_pairs;
	std::vector<point_particle*> current_selection; 
	std::mutex interaction_lock;
	std::mutex selection_lock;
	std::mutex draw_lock;
};

