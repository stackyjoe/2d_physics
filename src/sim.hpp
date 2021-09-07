class sim {
private:
    constexpr size_t width = 1280;
    constexpr size_t height = 720;
    constexpr size_t smaller_dimension = std::min(width, height);


    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> delta_dist(-1.0, 1.0);
    std::chrono::high_resolution_clock clock;

    sf::RenderWindow window;
    sf::View v;
    sf::Font font;

    std::atomic<float> approx_fps = 0.f;
    float zoom_factor = 1.0f;
    constexpr size_t num_dots = 1000;

    point_particle_simulator sim;

public:
    void spawn_particle(float x, float y, float mass, float charge, sf::Color fill_color) noexcept;
    void draw() noexcept;
    void handle_input() noexcept;
};