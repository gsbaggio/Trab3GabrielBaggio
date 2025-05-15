#ifndef __TANQUE_H__
#define __TANQUE_H__

#include "gl_canvas2d.h"
#include "Vector2.h" // Assuming this file exists and provides a Vector2 class
#include <cmath>     // For M_PI, cos, sin, atan2

// Define M_PI if not available (common on Windows with MSVC)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward declaration to avoid circular dependency if BSplineTrack.h includes Tanque.h
class BSplineTrack; 

class Tanque {
public:
    Vector2 position;
    float baseAngle; // Radians, 0 is along positive X-axis
    float topAngle;  // Radians, 0 is along positive X-axis, relative to world
    float speed;
    float rotationRate; // Radians per frame for base rotation
    float turretRotationSpeed; // Radians per frame for turret rotation

    Vector2 forwardVector; // Direction the base is pointing

    // Dimensions
    float baseWidth;
    float baseHeight;
    float turretRadius;
    float cannonLength;
    float cannonWidth;

    // Collision related members
    Vector2 lastSafePosition;
    bool isColliding;
    int collisionTimer; // Counts down frames for rebound

    // Collision constants
    static const int COLLISION_REBOUND_FRAMES = 90; // Approx 1.5s at 60fps
    static constexpr float REBOUND_SPEED_FACTOR = 0.5f; // 50% of normal speed for rebound

    Tanque(float x, float y, float initialSpeed = 1.0f, float initialRotationRate = 0.03f); // Adjusted defaults for per-frame

    void Update(float mouseX, float mouseY, bool rotateLeft, bool rotateRight, BSplineTrack* track);
    void Render();

private:
    void CheckCollisionAndRespond(BSplineTrack* track);
};

#endif
