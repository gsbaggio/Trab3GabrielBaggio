#include "Projectile.h"
#include <cmath>

Projectile::Projectile()
    : position(0, 0), previousPosition(0, 0), velocity(0, 0), 
      active(false), lifetime(300), collisionRadius(12.0f) { // Increased from 8.0f to 12.0f
}

Projectile::Projectile(const Vector2& pos, const Vector2& vel, float radius) // Increased default from 4.0f to 8.0f
    : position(pos), previousPosition(pos), velocity(vel), 
      active(true), lifetime(300), collisionRadius(radius) {
}

void Projectile::Update() {
    if (!active) return;
    
    previousPosition = position; // Store current position before updating
    position = position + velocity;
    
    if (lifetime > 0) {
        lifetime--;
        if (lifetime <= 0) active = false; // Added active = false;
    }
}

void Projectile::Render() {
    if (!active) return;
    CV::color(1.0f, 0.7f, 0.0f); // Orange-yellow for projectiles
    CV::circleFill(position.x, position.y, 8.0f, 10); // Increased from 4.0f to 8.0f to match larger collision
}

void Projectile::CheckCollisionWithTrack(BSplineTrack* track) {
    if (!active || !track) return;
    
    // Get closest points on both track boundaries for current position
    ClosestPointInfo cpiLeftCurrent = track->findClosestPointOnCurve(position, CurveSide::Left); // Renamed cpiLeft to cpiLeftCurrent
    ClosestPointInfo cpiRightCurrent = track->findClosestPointOnCurve(position, CurveSide::Right); // Renamed cpiRight to cpiRightCurrent
    
    // Check collision with current position and boundaries
    if (cpiLeftCurrent.isValid) { // Used cpiLeftCurrent
        Vector2 vec_proj_to_cl_point = position - cpiLeftCurrent.point; // Used cpiLeftCurrent
        float projection = vec_proj_to_cl_point.x * cpiLeftCurrent.normal.x + vec_proj_to_cl_point.y * cpiLeftCurrent.normal.y; // Used cpiLeftCurrent
            
        // If projection is positive, projectile is outside the left boundary.
        // If the projection is less than the collision radius, it's colliding with the boundary
        if (projection > 0.0f && projection < collisionRadius) { // Added missing code block
            active = false;
            return;
        }
    }
    
    if (cpiRightCurrent.isValid) { // Used cpiRightCurrent
        Vector2 vec_proj_to_cr_point = position - cpiRightCurrent.point; // Used cpiRightCurrent
        float projection = vec_proj_to_cr_point.x * cpiRightCurrent.normal.x + vec_proj_to_cr_point.y * cpiRightCurrent.normal.y; // Used cpiRightCurrent
            
        // If projection is negative, projectile is outside the right boundary.
        // If the absolute value of projection is less than the collision radius, it's colliding
        if (projection < 0.0f && std::abs(projection) < collisionRadius) { // Added missing code block and std::abs
            active = false;
            return;
        }
    }
    
    // If moving fast, also check for "tunneling" through boundaries by sampling points along movement path
    float movementLength = (position - previousPosition).length();
    if (movementLength > collisionRadius) { // Removed * 1.5f
        const int numSamples = 5; // Sample a few points along the movement path
        
        for (int i = 1; i < numSamples; i++) {
            float t = static_cast<float>(i) / numSamples;
            Vector2 samplePos = previousPosition + (position - previousPosition) * t;
            
            // Check sample point against both boundaries
            ClosestPointInfo cpiLeftSample = track->findClosestPointOnCurve(samplePos, CurveSide::Left);
            if (cpiLeftSample.isValid && cpiLeftSample.distance < collisionRadius) {
                active = false;
                return;
            }
            
            ClosestPointInfo cpiRightSample = track->findClosestPointOnCurve(samplePos, CurveSide::Right);
            if (cpiRightSample.isValid && cpiRightSample.distance < collisionRadius) {
                active = false;
                return;
            }
        }
    }
}
