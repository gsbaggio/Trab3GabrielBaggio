#ifndef __PROJECTILE_H__
#define __PROJECTILE_H__

#include "Vector2.h"
#include "gl_canvas2d.h"
#include "BSplineTrack.h"

class Projectile {
public:
    Vector2 position;
    Vector2 velocity;
    bool active;
    float radius;
    float maxLifeTime;
    float currentLifeTime;
    
    Projectile();
    Projectile(const Vector2& startPos, const Vector2& startVel, float size = 3.0f);
    
    void Update();
    void Render();
    bool CheckCollisionWithTrack(BSplineTrack* track);
};

#endif
