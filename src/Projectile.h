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
    
    Projectile()
        : position(0, 0), previousPosition(0, 0), velocity(0, 0), 
          active(false), lifetime(300), collisionRadius(8.0f) { // Increased from 8.0f to 12.0f
    }

    Projectile(const Vector2& pos, const Vector2& vel, float radius = 4.0f) // Increased default from 4.0f to 8.0f
        : position(pos), previousPosition(pos), velocity(vel), 
          active(true), lifetime(300), collisionRadius(radius) {
    }
    
    void Update() {
        if (!active) return;
        
        previousPosition = position; // Store current position before updating
        position = position + velocity;
        
        if (lifetime > 0) {
            lifetime--;
            if (lifetime <= 0) active = false;
        }
    }
    
    void Render() {
        if (!active) return;
        CV::color(1.0f, 0.7f, 0.0f); // Orange-yellow for projectiles
        CV::circleFill(position.x, position.y, 8.0f, 10); // Increased from 4.0f to 8.0f to match larger collision
    }
    
    void CheckCollisionWithTrack(BSplineTrack* track) {
        if (!active || !track) return;
        
        // Get closest points on both track boundaries for current position
        ClosestPointInfo cpiLeft = track->findClosestPointOnCurve(position, CurveSide::Left);
        ClosestPointInfo cpiRight = track->findClosestPointOnCurve(position, CurveSide::Right);

        // Check collision with left boundary using normal projection
        if (cpiLeft.isValid) {
            Vector2 vec_proj_to_cl_point = position - cpiLeft.point;
            float projection = vec_proj_to_cl_point.x * cpiLeft.normal.x + vec_proj_to_cl_point.y * cpiLeft.normal.y;
            
            // If projection is positive, projectile is outside the left boundary.
            // If the projection is less than the collision radius, it's colliding with the boundary
            if (projection > 0.0f && projection < collisionRadius) {
                active = false;
                return;
            }
        }

        // Check collision with right boundary using normal projection
        if (cpiRight.isValid) {
            Vector2 vec_proj_to_cr_point = position - cpiRight.point;
            float projection = vec_proj_to_cr_point.x * cpiRight.normal.x + vec_proj_to_cr_point.y * cpiRight.normal.y;
            
            // If projection is negative, projectile is outside the right boundary.
            // If the absolute value of the projection is less than the collision radius, it's colliding
            if (projection < 0.0f && std::abs(projection) < collisionRadius) {
                active = false;
                return;
            }
        }
        
        // If moving fast, check for "tunneling" through boundaries by sampling points along movement path
        float movementLength = (position - previousPosition).length();
        if (movementLength > collisionRadius) {
            // The faster the projectile, the more samples we need
            int numSamples = std::max(5, static_cast<int>(movementLength / (collisionRadius * 0.5f)));
            
            for (int i = 1; i < numSamples; i++) {
                float t = static_cast<float>(i) / numSamples;
                Vector2 samplePos = previousPosition + (position - previousPosition) * t;
                
                // Check sample points against both boundaries using same method as above
                ClosestPointInfo cpiLeftSample = track->findClosestPointOnCurve(samplePos, CurveSide::Left);
                if (cpiLeftSample.isValid) {
                    Vector2 vecSample = samplePos - cpiLeftSample.point;
                    float projSample = vecSample.x * cpiLeftSample.normal.x + vecSample.y * cpiLeftSample.normal.y;
                    if (projSample > 0.0f && projSample < collisionRadius) {
                        active = false;
                        return;
                    }
                }
                
                ClosestPointInfo cpiRightSample = track->findClosestPointOnCurve(samplePos, CurveSide::Right);
                if (cpiRightSample.isValid) {
                    Vector2 vecSample = samplePos - cpiRightSample.point;
                    float projSample = vecSample.x * cpiRightSample.normal.x + vecSample.y * cpiRightSample.normal.y;
                    if (projSample < 0.0f && std::abs(projSample) < collisionRadius) {
                        active = false;
                        return;
                    }
                }
            }
        }
    }
};

#endif
