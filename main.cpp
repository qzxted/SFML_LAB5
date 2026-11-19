#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {

    constexpr unsigned int kWindowWidth = 800;
    constexpr unsigned int kWindowHeight = 600;
    constexpr float kPi = 3.14159265358979323846f;

    struct PendulumState {
        sf::Vector2f pivot{ 400.f, 130.f };
        float length = 240.f;
        float initialAngleDeg = 45.f;
        float angleRad = 45.f * kPi / 180.f;
        float angularVel = 0.0f;
        float gravity = 650.f;
        float damping = 0.0f;
        bool paused = false;
        float elapsedSinceCrossing = 0.f;
        int halfSwings = 0;
        float lastPeriodSec = 0.f;
    };

    void resetPendulum(PendulumState& s, std::vector<sf::Vector2f>& trail) {
        s.angleRad = s.initialAngleDeg * kPi / 180.f;
        s.angularVel = 0.0f;
        s.elapsedSinceCrossing = 0.f;
        s.halfSwings = 0;
        trail.clear();
    }

    void handleInput(sf::RenderWindow& window, PendulumState& s, std::vector<sf::Vector2f>& trail) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                switch (key->code) {
                case sf::Keyboard::Key::Space:
                    s.paused = !s.paused;
                    break;
                case sf::Keyboard::Key::R:
                    resetPendulum(s, trail);
                    break;
                case sf::Keyboard::Key::Up:
                    s.length = std::clamp(s.length + 20.f, 80.f, 380.f);
                    resetPendulum(s, trail);
                    break;
                case sf::Keyboard::Key::Down:
                    s.length = std::clamp(s.length - 20.f, 80.f, 380.f);
                    resetPendulum(s, trail);
                    break;
                case sf::Keyboard::Key::Right:
                    s.initialAngleDeg = std::clamp(s.initialAngleDeg + 5.f, -85.f, 85.f);
                    resetPendulum(s, trail);
                    break;
                case sf::Keyboard::Key::Left:
                    s.initialAngleDeg = std::clamp(s.initialAngleDeg - 5.f, -85.f, 85.f);
                    resetPendulum(s, trail);
                    break;
                default:
                    break;
                }
            }
        }
    }

    void updatePhysics(PendulumState& s, float dt, std::vector<sf::Vector2f>& trail) {
        if (s.paused) {
            return;
        }

        const float prevAngle = s.angleRad;
        const float angularAcc = -(s.gravity / s.length) * std::sin(s.angleRad) - s.damping * s.angularVel;
        s.angularVel += angularAcc * dt;
        s.angleRad += s.angularVel * dt;

        s.elapsedSinceCrossing += dt;
        if ((prevAngle < 0.f && s.angleRad >= 0.f) || (prevAngle > 0.f && s.angleRad <= 0.f)) {
            ++s.halfSwings;
            if (s.halfSwings >= 2) {
                s.lastPeriodSec = s.elapsedSinceCrossing;
                s.elapsedSinceCrossing = 0.f;
                s.halfSwings = 0;
            }
        }

        const sf::Vector2f bobPos{
            s.pivot.x + s.length * std::sin(s.angleRad),
            s.pivot.y + s.length * std::cos(s.angleRad)
        };

        trail.push_back(bobPos);
        constexpr std::size_t kMaxTrail = 160;
        if (trail.size() > kMaxTrail) {
            trail.erase(trail.begin());
        }
    }

    std::string formatHudText(const PendulumState& s) {
        const float currentDeg = s.angleRad * 180.f / kPi;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);
        oss << "Current Angle: " << currentDeg << " deg (" << std::setprecision(2) << s.angleRad << " rad)\n";
        oss << std::setprecision(1);
        oss << "Length (L): " << s.length << " px\n";
        oss << "Initial Angle: " << s.initialAngleDeg << " deg\n";
        oss << "Measured Period (T): " << std::setprecision(2) << s.lastPeriodSec << " s\n";
        oss << "State: " << (s.paused ? "PAUSED" : "RUNNING") << "\n";
        oss << "[Up/Down] Length | [Left/Right] Init Angle | [Space] Pause | [R] Restart";
        return oss.str();
    }

    void drawScene(
        sf::RenderWindow& window,
        const PendulumState& s,
        const std::vector<sf::Vector2f>& trail,
        sf::Text* hudText
    ) {
        window.clear(sf::Color(20, 22, 28));

        const sf::Vector2f bobPos{
            s.pivot.x + s.length * std::sin(s.angleRad),
            s.pivot.y + s.length * std::cos(s.angleRad)
        };

        for (std::size_t i = 0; i < trail.size(); ++i) {
            const float ratio = static_cast<float>(i + 1) / static_cast<float>(trail.size());
            sf::CircleShape dot(2.5f * ratio + 1.f);
            dot.setOrigin({ dot.getRadius(), dot.getRadius() });
            dot.setPosition(trail[i]);
            dot.setFillColor(sf::Color(90, 180, 255, static_cast<std::uint8_t>(140.f * ratio)));
            window.draw(dot);
        }

        sf::VertexArray rod(sf::PrimitiveType::Lines, 2);
        rod[0].position = s.pivot;
        rod[0].color = sf::Color(190, 195, 210);
        rod[1].position = bobPos;
        rod[1].color = sf::Color(190, 195, 210);
        window.draw(rod);

        sf::CircleShape anchor(6.f);
        anchor.setOrigin({ 6.f, 6.f });
        anchor.setPosition(s.pivot);
        anchor.setFillColor(sf::Color(220, 90, 90));
        window.draw(anchor);

        constexpr float bobRadius = 22.f;
        sf::CircleShape bob(bobRadius);
        bob.setOrigin({ bobRadius, bobRadius });
        bob.setPosition(bobPos);
        bob.setFillColor(sf::Color(90, 180, 255));
        window.draw(bob);

        if (hudText != nullptr) {
            hudText->setString(formatHudText(s));
            window.draw(*hudText);
        }

        window.display();
    }

}

int main() {
    sf::RenderWindow window(sf::VideoMode({ kWindowWidth, kWindowHeight }), "SFML Lab");
    window.setFramerateLimit(60);

    sf::Font font;
    bool fontLoaded = false;
    for (const char* path : {
             "../resources/fonts/arial.ttf",
             "../../resources/fonts/arial.ttf",
             "resources/fonts/arial.ttf"
        }) {
        if (font.openFromFile(path)) {
            fontLoaded = true;
            break;
        }
    }

    if (!fontLoaded) {
        std::cerr << "Warning: failed to load arial.ttf from resources/fonts/\n";
    }

    sf::Text hudText(font, "", 16);
    hudText.setPosition({ 16.f, 14.f });
    hudText.setFillColor(sf::Color(235, 240, 248));

    PendulumState state;
    std::vector<sf::Vector2f> trail;
    resetPendulum(state, trail);

    sf::Clock clock;

    while (window.isOpen()) {
        handleInput(window, state, trail);

        float dt = clock.restart().asSeconds();
        dt = std::min(dt, 0.05f);

        updatePhysics(state, dt, trail);
        drawScene(window, state, trail, fontLoaded ? &hudText : nullptr);
    }

    return 0;
}