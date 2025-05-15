#include "Projectile.h"
#include <cmath>

Projectile::Projectile() : position(0, 0), velocity(0, 0), active(false), 
                          radius(3.0f), maxLifeTime(5.0f), currentLifeTime(0.0f) {
}

Projectile::Projectile(const Vector2& startPos, const Vector2& startVel, float size)
    : position(startPos), velocity(startVel), active(true), 
      radius(size), maxLifeTime(5.0f), currentLifeTime(0.0f) {
}

void Projectile::Update() {
    if (!active) return;
    
    // Update position based on velocity
    position.x += velocity.x;
    position.y += velocity.y;
    
    // Track lifetime
    currentLifeTime += 1.0f/60.0f; // Assuming 60fps
    if (currentLifeTime >= maxLifeTime) {
        active = false;
    }
}

void Projectile::Render() {
    if (!active) return;
    
    // Draw projectile as a small filled circle
    CV::color(1.0f, 0.8f, 0.2f); // Yellow-orange color
    CV::circleFill(position.x, position.y, radius, 8);
}

bool Projectile::CheckCollisionWithTrack(BSplineTrack* track) {
    if (!active || !track) return false;

    // Check collision with Left Curve
    ClosestPointInfo cpiLeft = track->findClosestPointOnCurve(position, CurveSide::Left);
    if (cpiLeft.isValid) {
        Vector2 vec_to_point = position - cpiLeft.point;
        float projection = vec_to_point.x * cpiLeft.normal.x + vec_to_point.y * cpiLeft.normal.y;
        if (projection > 0.0f) { // Projectile is "outside" the left curve
            active = false;
            return true;
        }
    }

    // Check collision with Right Curve
    ClosestPointInfo cpiRight = track->findClosestPointOnCurve(position, CurveSide::Right);
    if (cpiRight.isValid) {
        Vector2 vec_to_point = position - cpiRight.point;
        float projection = vec_to_point.x * cpiRight.normal.x + vec_to_point.y * cpiRight.normal.y;
        if (projection < 0.0f) { // Projectile is "outside" the right curve
            active = false;
            return true;
        }
    }

    return false;
}
