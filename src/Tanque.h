#ifndef __TANQUE_H__
#define __TANQUE_H__

#include "gl_canvas2d.h"
#include "Vector2.h" // Assuming this file exists and provides a Vector2 class
#include <cmath>     // For M_PI, cos, sin, atan2

// Define M_PI if not available (common on Windows with MSVC)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Tanque {
public:
    Vector2 position;
    float baseAngle; // Radians, 0 is along positive X-axis
    float topAngle;  // Radians, 0 is along positive X-axis, relative to world
    float speed;
    float rotationRate; // Radians per second for base rotation
    float turretRotationSpeed; // Radians per second for turret rotation

    Vector2 forwardVector; // Direction the base is pointing

    // Dimensions
    float baseWidth;
    float baseHeight;
    float turretRadius;
    float cannonLength;
    float cannonWidth;

    Tanque(float x, float y, float initialSpeed = 75.0f, float initialRotationRate = 2.0f);

    void Update(float mouseX, float mouseY, bool rotateLeft, bool rotateRight, float deltaTime);
    void Render();
};

#endif
