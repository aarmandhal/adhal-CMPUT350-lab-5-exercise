#include <cmath>
#include <cstddef>
#include <iostream>
#include <optional>
#include <vector>

#include <SFML/Graphics.hpp>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
const int FPS_LIMIT = 30;
// How much of the curve the square covers each frame
// 1 / 90 at 30 FPS means the square takes 3 seconds to travel the curve
const float STEP = 1.0f / 90.0f;

using Point2D = sf::Vector2f;

// TODO: (Part 1) Define a function that samples a cubic Bezier curve at t in [0, 1].

// Cubic Bezier Equation
// B(t) = (1-t)^3 * P0 + 3(1-t)^2 * t * P1 + 3(1-t) * t^2 * P2 + t^3 * P3
Point2D getPoint(const std::vector<sf::Vector2f>& pts, float t) {
    // Calculate (1 - t), which is used in every term of the equation
    float temp = 1 - t;
    // Blend the four control points using the cubic Bezier weights
    return (temp * temp * temp) * pts[0] + (3 * (temp * temp) * t) * pts[1] +
           (3 * temp * (t * t)) * pts[2] + (t * t * t) * pts[3];
}

// TODO: (Part 2) Define a function that returns the curve's slope at t in [0, 1].

// Cubic Bezier Slope Equation (the derivative of the Bezier equation)
// B'(t) = 3(1-t)^2 * (P1-P0) + 6(1-t) * t * (P2-P1) + 3t^2 * (P3-P2)
// Returns a direction vector
Point2D getSlope(const std::vector<sf::Vector2f>& pts, float t) {
    // Calculate (1 - t)
    float temp = 1 - t;
    // Blend the three differences between the control points using the derivative weights
    return 3 * (temp * temp) * (pts[1] - pts[0]) + 6 * (temp * t) * (pts[2] - pts[1]) +
           3 * (t * t) * (pts[3] - pts[2]);
}

// TODO: (Part 1) Store four control points for the curve.
// Every group of four points (sharing an endpoint with the next group) is one segment
// The number of points is always 3 * (number of segments) + 1
std::vector<sf::Vector2f> points = {{100.f, 600.f}, {250.f, 200.f}, {550.f, 200.f}, {700.f, 600.f}};

void drawCurveSegment(sf::RenderWindow& window, const std::vector<sf::Vector2f>& pts) {
    // Draw one Bezier segment by connecting sampled points with lines
    // pts holds the four control points of the segment
    // Create a line strip, which connects each vertex to the next one
    sf::VertexArray lines(sf::PrimitiveType::LineStrip);
    // Number of samples used to approximate the curve
    const int n_steps = 100;
    for (int i = 0; i <= n_steps; i++) {
        // Convert the step number into a value of t between 0 and 1
        float t = static_cast<float>(i) / static_cast<float>(n_steps);
        // Add the point on the curve at t to the line strip
        lines.append(sf::Vertex{getPoint(pts, t), sf::Color::White});
    }
    // Draw the whole strip at once
    window.draw(lines);
}

// TODO: (Part 2) Track animation time for the square moving along the curve.
// Progress of the square along the curve, from 0 (start) to 1 (end)
float animationPos = 0;

// TODO: (Part 3) Track the index of the control point being dragged.
// -1 means no point is being dragged
int dragIndex = -1;

float squaredDistance(const Point2D& a, const Point2D& b) {
    // Squared Distance Equation
    // d^2 = (x2 - x1)^2 + (y2 - y1)^2
    // (x1, y1) -> a.x, a.y
    // (x2, y2) -> b.x, b.y
    // Calculate the difference between the two points
    Point2D d = a - b;
    // Add the squared differences of each axis
    return d.x * d.x + d.y * d.y;
}

void handleInput(sf::Window& window, bool& shouldQuit) {
    while (const std::optional<sf::Event> event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            // Close the window and tell the main loop to stop
            window.close();
            shouldQuit = true;
        } else if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
            // TODO: (Part 3) On left-click, select the closest control point
            // using mouse->position and start dragging it.
            if (mouse->button == sf::Mouse::Button::Left) {
                // Convert the integer mouse position into a float point
                Point2D m = sf::Vector2f(mouse->position);
                // Index of the closest point found so far
                std::size_t best = 0;
                // Distance to the closest point found so far
                float bestDist = INFINITY;
                for (std::size_t i = 0; i < points.size(); i++) {
                    // Calculate how far this control point is from the mouse
                    float d = squaredDistance(m, points[i]);
                    // Remember this point if it is closer than any so far
                    if (d < bestDist) {
                        best = i;
                        bestDist = d;
                    }
                }
                // Start dragging the closest point
                dragIndex = static_cast<int>(best);
            }
        } else if (const auto* mouse = event->getIf<sf::Event::MouseButtonReleased>()) {
            // TODO: (Part 3) On left-button release, stop dragging.
            if (mouse->button == sf::Mouse::Button::Left) {
                // No point is selected anymore
                dragIndex = -1;
            }
        } else if (const auto* mouse = event->getIf<sf::Event::MouseMoved>()) {
            // TODO: (Part 3) Move the selected control point to mouse->position.
            // TODO: (Part 4) Maintain matching slopes at shared endpoints.
            // When moving point 3, move point 5 without changing its distance
            // from point 4 (point numbers here start at 1).
            if (dragIndex != -1) {
                // Get the index of the point being dragged
                std::size_t idx = static_cast<std::size_t>(dragIndex);
                // Move the dragged point to the mouse position
                points[idx] = sf::Vector2f(mouse->position);

                // Points at index 2, 5, 8... are the handle just before a shared endpoint
                // The handle after that endpoint is 2 points further along
                if (idx % 3 == 2 && idx + 2 < points.size()) {
                    // The shared endpoint sits between the two handles
                    Point2D joint = points[idx + 1];
                    // Keep the partner handle the same distance from the endpoint as before
                    float len = (points[idx + 2] - joint).length();
                    // Direction from the dragged handle through the endpoint
                    Point2D dir = joint - points[idx];
                    // Only normalize if the direction is not the zero vector
                    if (dir.length() > 0.f) {
                        // Place the partner on the opposite side, in a straight line
                        points[idx + 2] = joint + dir.normalized() * len;
                    }
                }
            }
        } else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            // TODO: (Part 4) '+' adds three control points; '-' removes three,
            // keeping at least four points.
            if (key->code == sf::Keyboard::Key::Equal || key->code == sf::Keyboard::Key::Add) {
                // Start the new segment from the last point
                Point2D last = points.back();
                // Grow to the left if the last point is on the right half of the window
                // (and to the right otherwise) so the new points stay on screen
                float side = (last.x > static_cast<float>(WINDOW_WIDTH) / 2.f) ? -1.f : 1.f;
                // Add the first handle, second handle, and end point of the new segment
                points.push_back(last + Point2D{40.f * side, -80.f});
                points.push_back(last + Point2D{100.f * side, -80.f});
                points.push_back(last + Point2D{140.f * side, 0.f});
            } else if (key->code == sf::Keyboard::Key::Hyphen ||
                       key->code == sf::Keyboard::Key::Subtract) {
                // Never go below one segment (four points)
                if (points.size() > 4) {
                    // Remove the last segment's three points
                    points.resize(points.size() - 3);
                    // Stop dragging if the dragged point was just removed
                    if (dragIndex >= static_cast<int>(points.size())) {
                        dragIndex = -1;
                    }
                }
            }
        }
    }
}

std::size_t segmentCount() {
    // Segment Count Equation
    // segments = (points - 1) / 3
    // Each segment adds three points after the first four
    return (points.size() - 1) / 3;
}

std::vector<sf::Vector2f> getSegment(std::size_t s) {
    // Copy the four control points of one segment into their own vector
    // getPoint and getSlope expect exactly four points
    // Segment s uses points 3s to 3s + 3
    // Index of the first point of this segment
    std::size_t base = 3 * s;
    return {points[base], points[base + 1], points[base + 2], points[base + 3]};
}

void render(sf::RenderWindow& window) {
    window.clear(sf::Color::Black);
    // ====== ====== ======
    // TODO: (Part 1) Sample GetPoint over t in [0, 1] and connect samples using the line-drawing
    // code from your project. Draw all four control points as circles after drawing the curve.
    // ====== ====== ======
    // Draw every segment of the curve
    for (std::size_t s = 0; s < segmentCount(); s++) {
        drawCurveSegment(window, getSegment(s));
    }

    // Create one circle and reuse it for every control point
    sf::CircleShape controlPoint(5.f);
    // Put the origin at the circle's center so its position is its center
    controlPoint.setOrigin({5.f, 5.f});
    controlPoint.setFillColor(sf::Color::Red);
    for (const auto& pt : points) {
        // Move the circle onto this control point and draw it
        controlPoint.setPosition(pt);
        window.draw(controlPoint);
    }

    // ====== ====== ======
    // TODO: (Part 2) Draw a small square moving repeatedly along the curve.
    // Use GetSlope to orient it to the curve at each time step.
    // ====== ====== ======
    // Move the square forward along the curve
    animationPos += STEP;
    // Start over once it reaches the end of the curve
    if (animationPos >= 1.0f) {
        animationPos = 0.0f;
    }

    // Find where the square is on the curve (this uses the first four points)
    Point2D pos = getPoint(points, animationPos);
    // Find the direction the curve is heading at that spot
    Point2D slope = getSlope(points, animationPos);
    // Convert the direction into an angle (y first, then x)
    float angle = std::atan2(slope.y, slope.x);
    // Create the square
    sf::RectangleShape square;
    square.setSize({20.f, 20.f});
    // Rotate around the square's center instead of its top-left corner
    square.setOrigin({10.f, 10.f});
    square.setPosition(pos);
    // SFML rotation takes an angle object, not a plain number
    square.setRotation(sf::radians(angle));
    square.setFillColor(sf::Color::Blue);
    window.draw(square);

    // ====== ====== ======
    // TODO: (Part 3) Draw control handles from point 1 to 2 and point 3 to 4.
    // TODO: (Part 4) Draw all connected cubic Bezier segments and their handles.
    // ====== ====== ======
    // Every two vertices form one separate line
    sf::VertexArray handles(sf::PrimitiveType::Lines);
    for (std::size_t s = 0; s < segmentCount(); s++) {
        // Index of the first point of this segment
        std::size_t base = 3 * s;
        // Handle from the start point to the first control handle
        handles.append(sf::Vertex{points[base], sf::Color::Yellow});
        handles.append(sf::Vertex{points[base + 1], sf::Color::Yellow});
        // Handle from the second control handle to the end point
        handles.append(sf::Vertex{points[base + 2], sf::Color::Yellow});
        handles.append(sf::Vertex{points[base + 3], sf::Color::Yellow});
    }
    window.draw(handles);

    // ====== ====== ======
    // TODO: (Bonus) Support multiple curves, a Galaga screen overlay at a 1:2 ratio, and exporting
    // curve points as C++ code for Project 1b.
    // ====== ====== ======

    // Show everything that was drawn this frame
    window.display();
}

int main() {
    sf::RenderWindow window;

    try {
        // Initialize window
        window.create(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Bezier Curve Editor");
        window.setFramerateLimit(FPS_LIMIT);
        // Prevent key repeats.
        window.setKeyRepeatEnabled(false);

        bool shouldQuit = false;
        // Main game loop
        while (window.isOpen()) {
            handleInput(window, shouldQuit);
            if (shouldQuit) {
                break;
            }
            render(window);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}