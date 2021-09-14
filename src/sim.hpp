#include "point_particle.hpp"

#include <cmath>

#include <algorithm>
#include <chrono>
#include <execution>
#include <iostream>
#include <optional>
#include <random>
#include <vector>

#include <fmt/format.h>

#include <SFML\Graphics.hpp>
#include <SFML\Main.hpp>
#include <SFML\Window.hpp>

class point_particle_simulator {
public:

	point_particle_simulator();
	void select(sf::Vector2f end_pos, sf::Vector2f start_pos);

	bool clear_current_selection();

	void reserve(size_t desired_capacity) {
		manager.reserve(desired_capacity);
	}

	void generate_pairs();

	std::thread interact_in_separate_thread();

	void sort_pairs();

	void draw();

	void spawn_particles();

	void run();

	EntityManagerType manager;
private:

	void physical_interaction();

	float gen_random_float();

	void draw_function(std::vector<GraphicComponent*>& graphical_representations);


	static constexpr size_t width = 1280;
	static constexpr size_t height = 720;
	static constexpr size_t smaller_dimension = std::min(width, height);

	static constexpr size_t num_dots = 1500;
	static constexpr float particle_display_size = 5.f;
	static constexpr float placement_scale_factor = 1.f;

	std::random_device rd;
	std::mt19937 mt;
	std::uniform_real_distribution<float> delta_dist;
	std::chrono::high_resolution_clock clock;


	sf::RenderWindow window;
	sf::View v;
	sf::Font font;

	float approx_fps;
	float zoom_factor;

	std::vector<std::pair<point_particle*, point_particle*>> distinct_pairs;
	std::vector<point_particle*> current_selection;
	std::mutex interaction_lock;
	std::mutex selection_lock;
	std::mutex draw_lock;
};