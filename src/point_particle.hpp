#pragma once

#include <functional>
#include <mutex>
#include <thread>

#include <SFML/Graphics.hpp>
#include <SFML/Main.hpp>
#include <SFML/Window.hpp>

#include "entity.hpp"
#include "math_vectors.hpp"


constexpr float g = 0.00981f;
constexpr float k = -89755.1f;
constexpr float dt = 0.1f;

struct NewtonianBody {
	NewtonianBody() = delete;
	NewtonianBody(NewtonianBody const&) = delete;
	NewtonianBody& operator=(NewtonianBody const&) = delete;

	NewtonianBody(NewtonianBody&&) = default;
	NewtonianBody& operator=(NewtonianBody&&) = default;

	NewtonianBody(float x, float y, float mass)
		: mass(mass), shared_force(std::make_unique<std::array<std::atomic<float>,2>>()) {
		position[0] = x;
		position[1] = y;
		(*shared_force)[0] = 0;
		(*shared_force)[1] = 0;
	}

	template<mathematics::concepts::FieldLike T>
	using coordinate = typename mathematics::vector<T,2>;

	coordinate<float> position;
	coordinate<float> velocity;
	coordinate<float> acceleration;

	const float mass;

	std::unique_ptr<std::array<std::atomic<float>,2>> shared_force;
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

std::tuple < float, mathematics::vector<float,2>, mathematics::vector<float, 2> > distance_between_and_difference(point_particle const& p1, point_particle const& p2);
bool compare_by_distance(std::pair<point_particle *,point_particle *> const& pair1, std::pair<point_particle *, point_particle *> const& pair2);
void mass_interaction(point_particle* p1, point_particle* p2);
void electrical_interaction(point_particle* p1, point_particle* p2);
