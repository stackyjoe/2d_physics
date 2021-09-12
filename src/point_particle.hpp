#pragma once

#include <functional>
#include <mutex>
#include <thread>

#include <SFML/Graphics.hpp>
#include <SFML/Main.hpp>
#include <SFML/Window.hpp>

#include "entity.hpp"


constexpr float g = -0.000981f;
constexpr float k = 897.551f;
constexpr float dt = 0.1f;

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

std::tuple < float, std::pair<float, float>, std::pair<float, float> > distance_between_and_difference(point_particle const& p1, point_particle const& p2);
bool compare_by_distance(std::pair<point_particle *,point_particle *> const& pair1, std::pair<point_particle *, point_particle *> const& pair2);
void mass_interaction(point_particle* p1, point_particle* p2);
void electrical_interaction(point_particle* p1, point_particle* p2);
