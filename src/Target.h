#ifndef __TARGET_H__
#define __TARGET_H__

#include "Vector2.h"
#include "gl_canvas2d.h"
#include <cmath>
#include <algorithm> // For std::max/std::min

class Target {
public:
    Vector2 position;
    bool active;
    float radius;
    int health;        // Health points (default = 2 hits to destroy)
    int maxHealth;     // Maximum health
    
    Target();

    Target(const Vector2& pos);

    void Render();
    
    bool CheckCollision(const Vector2& point);

    bool CheckCollisionWithTank(const Vector2& tankPos, float tankWidth, float tankHeight, float tankAngle);

    void TakeDamage(int amount);
};

#endif
