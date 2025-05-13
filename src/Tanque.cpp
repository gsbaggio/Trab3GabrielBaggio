#include "Tanque.h"
#include "gl_canvas2d.h"
#include <cmath> // For M_PI, cos, sin, atan2

// Define M_PI if not available (common on Windows with MSVC)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Tanque::Tanque(float x, float y, float initialSpeed, float initialRotationRate) {
    position.set(x, y);
    baseAngle = 0.0f; // Pointing right initially
    topAngle = 0.0f;
    speed = initialSpeed; // Pixels per second
    rotationRate = initialRotationRate; // Radians per second
    turretRotationSpeed = 2.5f; // Radians per second for turret, adjust as needed

    baseWidth = 60.0f;
    baseHeight = 40.0f;
    turretRadius = 15.0f;
    cannonLength = 40.0f;
    cannonWidth = 6.0f;

    forwardVector.set(cos(baseAngle), sin(baseAngle));
}

void Tanque::Update(float mouseX, float mouseY, bool rotateLeft, bool rotateRight, float deltaTime) {
    // Update base rotation
    if (rotateLeft) {
        baseAngle -= rotationRate * deltaTime;
    }
    if (rotateRight) {
        baseAngle += rotationRate * deltaTime;
    }

    // Keep baseAngle within 0 to 2*PI
    if (baseAngle > 2 * M_PI) baseAngle -= 2 * M_PI;
    if (baseAngle < 0) baseAngle += 2 * M_PI;

    // Update forward vector based on baseAngle
    forwardVector.set(cos(baseAngle), sin(baseAngle));

    // Move the tank forward
    position.x += forwardVector.x * speed * deltaTime;
    position.y += forwardVector.y * speed * deltaTime;

    // Update top (turret) angle to point towards the mouse
    float dx = mouseX - position.x;
    float dy = mouseY - position.y;
    float targetAngle = atan2(dy, dx);

    // Normalize angles to be between -PI and PI for easier comparison
    float currentTopAngle = fmod(topAngle + M_PI, 2 * M_PI) - M_PI;
    if (currentTopAngle < -M_PI) currentTopAngle += 2 * M_PI; // Ensure it's in [-PI, PI]

    float angleDifference = targetAngle - currentTopAngle;

    // Normalize the angle difference to the shortest path (-PI to PI)
    if (angleDifference > M_PI) {
        angleDifference -= 2 * M_PI;
    } else if (angleDifference < -M_PI) {
        angleDifference += 2 * M_PI;
    }

    float maxRotationThisFrame = turretRotationSpeed * deltaTime;

    if (std::abs(angleDifference) <= maxRotationThisFrame) {
        topAngle = targetAngle; // Snap to target if close enough
    } else {
        if (angleDifference > 0) {
            topAngle += maxRotationThisFrame; // Rotate counter-clockwise
        } else {
            topAngle -= maxRotationThisFrame; // Rotate clockwise
        }
    }

    // Normalize topAngle to be within 0 to 2*PI (or -PI to PI, consistently)
    topAngle = fmod(topAngle, 2 * M_PI);
    if (topAngle < 0) {
        topAngle += 2 * M_PI;
    }
}

void Tanque::Render() {
    CV::color(0.2f, 0.5f, 0.2f); // Dark green for base

    // Render Base (Rectangle)
    // We need to draw the rectangle rotated around its center (position)
    // 1. Translate to the tank's position
    // 2. Rotate by baseAngle
    // 3. Draw the rectangle centered at (0,0) in local coordinates
    // 4. Undo rotation and translation (or use CV::pushMatrix/popMatrix if available)

    // For simplicity, we'll calculate the 4 corners of the rotated rectangle manually.
    // This is equivalent to applying a rotation matrix to the local corner coordinates.
    float halfW = baseWidth / 2.0f;
    float halfH = baseHeight / 2.0f;

    // Local corners of the base rectangle (before rotation)
    Vector2 p1_local(-halfW, -halfH);
    Vector2 p2_local( halfW, -halfH);
    Vector2 p3_local( halfW,  halfH);
    Vector2 p4_local(-halfW,  halfH);

    // Rotated corners
    float cosB = cos(baseAngle);
    float sinB = sin(baseAngle);

    Vector2 p1_world(p1_local.x * cosB - p1_local.y * sinB + position.x, p1_local.x * sinB + p1_local.y * cosB + position.y);
    Vector2 p2_world(p2_local.x * cosB - p2_local.y * sinB + position.x, p2_local.x * sinB + p2_local.y * cosB + position.y);
    Vector2 p3_world(p3_local.x * cosB - p3_local.y * sinB + position.x, p3_local.x * sinB + p3_local.y * cosB + position.y);
    Vector2 p4_world(p4_local.x * cosB - p4_local.y * sinB + position.x, p4_local.x * sinB + p4_local.y * cosB + position.y);

    // Draw the base as a polygon (or multiple lines/triangles if rectFill takes only axis-aligned)
    // Assuming CV::rectFill might not support rotated rectangles directly, we draw lines.
    // If CV::polygon or similar exists, that would be better.
    // For now, let's use lines to form the rectangle.
    CV::line(p1_world.x, p1_world.y, p2_world.x, p2_world.y);
    CV::line(p2_world.x, p2_world.y, p3_world.x, p3_world.y);
    CV::line(p3_world.x, p3_world.y, p4_world.x, p4_world.y);
    CV::line(p4_world.x, p4_world.y, p1_world.x, p1_world.y);

    // Draw a line indicating the front of the base
    CV::color(1,1,1); // White line for direction
    float frontMarkerLength = baseWidth * 0.6f;
    CV::line(position.x, position.y, position.x + forwardVector.x * frontMarkerLength, position.y + forwardVector.y * frontMarkerLength);


    // Render Top (Turret - Circle and Cannon - Rectangle)
    CV::color(0.1f, 0.3f, 0.1f); // Darker green for turret
    CV::circleFill(position.x, position.y, turretRadius, 20); // Draw turret base

    // Cannon
    // Calculate cannon end points based on topAngle
    float cannonStartX = position.x + cos(topAngle) * (turretRadius * 0.5f); // Start a bit from center
    float cannonStartY = position.y + sin(topAngle) * (turretRadius * 0.5f);
    float cannonEndX = position.x + cos(topAngle) * cannonLength;
    float cannonEndY = position.y + sin(topAngle) * cannonLength;

    // To draw the cannon as a rectangle, we need its 4 corners.
    // The cannon points along topAngle. Its width is cannonWidth.
    float halfCW = cannonWidth / 2.0f;
    float cosT = cos(topAngle);
    float sinT = sin(topAngle);
    float cosT_perp = cos(topAngle + M_PI / 2.0f); // Perpendicular to cannon direction
    float sinT_perp = sin(topAngle + M_PI / 2.0f); // Perpendicular to cannon direction

    // Points relative to tank's position, then add tank's position
    // Start of cannon (closer to turret center)
    Vector2 c1_local(0, -halfCW); // Local to cannon's orientation
    Vector2 c2_local(0,  halfCW);
    // End of cannon
    Vector2 c3_local(cannonLength, -halfCW);
    Vector2 c4_local(cannonLength,  halfCW);

    // Rotate these local points by topAngle and add to tank's position
    Vector2 c1(position.x + (c1_local.x * cosT - c1_local.y * sinT), position.y + (c1_local.x * sinT + c1_local.y * cosT));
    Vector2 c2(position.x + (c2_local.x * cosT - c2_local.y * sinT), position.y + (c2_local.x * sinT + c2_local.y * cosT));
    Vector2 c3(position.x + (c3_local.x * cosT - c3_local.y * sinT), position.y + (c3_local.x * sinT + c3_local.y * cosT));
    Vector2 c4(position.x + (c4_local.x * cosT - c4_local.y * sinT), position.y + (c4_local.x * sinT + c4_local.y * cosT));

    // Draw cannon using lines (assuming no rotated rectFill)
    CV::line(c1.x, c1.y, c3.x, c3.y); // One side
    CV::line(c2.x, c2.y, c4.x, c4.y); // Other side
    CV::line(c3.x, c3.y, c4.x, c4.y); // Tip
    // CV::line(c1.x, c1.y, c2.x, c2.y); // Base (optional, might be covered by turret)

    // Draw the movement vector line
    CV::color(1, 0, 0); // Red for movement vector
    CV::line(position.x, position.y, position.x + forwardVector.x * 50, position.y + forwardVector.y * 50); // Length 50 for visibility
}
