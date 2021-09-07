// 2d_physics.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#define _USE_MATH_DEFINES
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

#include "point_particle.hpp"
#include "tuple_of_optionals.hpp"

int main()
{

    constexpr size_t width = 1280;
    constexpr size_t height = 720;
    constexpr size_t smaller_dimension = std::min(width, height);


    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> delta_dist(-1.0, 1.0);
    std::chrono::high_resolution_clock clock;


    sf::RenderWindow window(sf::VideoMode(width, height), "Particle simulator");

    sf::View v(sf::FloatRect(0,0,width,height));

    sf::Font font;
    if (!font.loadFromFile("sansation.ttf"))
        fmt::print("Font failed to load\n");

    std::atomic<float> approx_fps = 0.f;
    float zoom_factor = 1.0f;
    constexpr size_t num_dots = 1000;
    v.zoom(zoom_factor);

    constexpr float particle_display_size = 5.f;
    constexpr float placement_scale_factor = 1.f;

    auto gen_random_float = [&delta_dist, &mt]() {return delta_dist(mt); };

    auto draw_function = [&font, &clock, &window, &approx_fps](auto &gfx_components) {
        window.clear();

        auto const draw_a_single_component = [&window](auto *gc) {
            window.draw(*gc);
        };

        std::for_each(std::execution::seq, gfx_components.begin(), gfx_components.end(), draw_a_single_component);

        
        sf::Text txt;
        txt.setFillColor(sf::Color::Black);
        txt.setOutlineColor(sf::Color::Magenta);
        txt.setOutlineThickness(4.f);
        txt.setFont(font);
        txt.setString(std::to_string(approx_fps));
        txt.setCharacterSize(32);
        txt.setPosition(window.getView().getViewport().getPosition());
        window.draw(txt);
        

        window.display();
        

    };

    point_particle_simulator sim(std::move(gen_random_float), std::move(draw_function));
    sim.manager.get_storage_for_entities().reserve(num_dots*5);

    
    for (size_t i = 0; i < num_dots; ++i) {
        sf::CircleShape particle(particle_display_size);

        float const theta = static_cast<float>(M_PI) * delta_dist(mt);
        float const r = delta_dist(mt);

        float const mass = 1836.f;
        float const charge = 1.f;

        float const x = width / 2 + placement_scale_factor * r * cos(theta) * smaller_dimension / 3;
        float const y = height / 2 + placement_scale_factor * r * sin(theta) * smaller_dimension / 3;

        particle.setPosition(x, y);

        
        if(charge == 1.f)
            particle.setFillColor(sf::Color::Red);
        else if(charge == -1.f)
            particle.setFillColor(sf::Color::Blue);
        else if(charge == 0.f)
            particle.setFillColor(sf::Color(200,200,200));
        else
            particle.setFillColor(sf::Color::Yellow);
        
        sim.manager.push_back(std::make_unique<point_particle>(PhysicalComponent(x, y, mass), ElectricalComponent(charge), Selectable(), std::move(particle)));
    }

    for (size_t i = 0; i < num_dots*3; ++i) {
        sf::CircleShape particle(particle_display_size);

        float const theta = static_cast<float>(M_PI) * delta_dist(mt);
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

        sim.manager.push_back(std::move(p));
    }

    
    for (size_t i = 0; i < num_dots; ++i) {
        sf::CircleShape particle(particle_display_size);

        float const theta = static_cast<float>(M_PI) * delta_dist(mt);
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

        sim.manager.push_back(std::make_unique<point_particle>(PhysicalComponent(x,y,mass), charge, Selectable(),std::move(particle)));
    }

    sim.generate_pairs();

    //window.setFramerateLimit(65);
    window.setView(v);
    window.display();

    auto start_pos_right = window.mapPixelToCoords(sf::Mouse::getPosition(window),v);
    auto end_pos_right = start_pos_right;

    auto start_pos_left = window.mapPixelToCoords(sf::Mouse::getPosition(window),v);
    auto end_pos_left = start_pos_right;


    bool run = false;

    //size_t cycle_count = 0;

	while (window.isOpen())
	{
        auto const tick = clock.now();

        std::optional<std::thread> t;
        if (run)
            t.emplace(sim.interact_in_separate_thread());

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
                if(event.mouseButton.button == sf::Mouse::Right)
				    start_pos_right = window.mapPixelToCoords(sf::Mouse::getPosition(window),v);
                if (event.mouseButton.button == sf::Mouse::Left)
                    start_pos_left = window.mapPixelToCoords(sf::Mouse::getPosition(window),v);
                v.getCenter();
                    
			}

			else if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Right) {
                    end_pos_right = window.mapPixelToCoords(sf::Mouse::getPosition(window), v);

                    sf::Vector2f translation;
                    translation.x = -1.f*(end_pos_right.x - start_pos_right.x);
                    translation.y = -1.f*(end_pos_right.y - start_pos_right.y);


                    if (translation.x != 0.f and translation.y != 0.f) {
                        v.move(translation);
                        window.setView(v);
                    }
                }

                if (event.mouseButton.button == sf::Mouse::Left) {
                    end_pos_left = window.mapPixelToCoords(sf::Mouse::getPosition(window),v);


                    fmt::print("Selected the region from ({},{}) to ({},{})\n", end_pos_left.x, end_pos_left.y, start_pos_left.x, start_pos_left.y);
                    sim.clear_current_selection();
                    sim.select(end_pos_left , start_pos_left);
                    
                }
			}
		}

        if (t.has_value()) {
            t->join();
            t.reset();
        }

		
        sim.draw();


        auto const tock = clock.now();
        auto const diff = tock - tick;

        approx_fps = 1000.f / std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
        fmt::print("Approx fps: {}\n", approx_fps);
	}


    return EXIT_SUCCESS;
}
