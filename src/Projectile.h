#ifndef __PROJECTILE_H__
#define __PROJECTILE_H__

#include "Vector2.h"
#include "gl_canvas2d.h"
#include "BSplineTrack.h"
#include <cmath>

class Projectile {
public:
    Vector2 position;
    Vector2 previousPosition; // Store previous position for continuous collision detection
    Vector2 velocity;
    bool active;
    int lifetime; // Optional: limit projectile lifetime
    float collisionRadius; // Explicit collision radius value
    
    Projectile();

    Projectile(const Vector2& pos, const Vector2& vel, float radius = 4.0f);
    
    void Update();
    
    void Render();
    
    void CheckCollisionWithTrack(BSplineTrack* track);
};

#endif
