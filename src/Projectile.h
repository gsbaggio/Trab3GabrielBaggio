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
          active(false), lifetime(300), collisionRadius(12.0f) { // Increased from 8.0f to 12.0f
    }

    Projectile(const Vector2& pos, const Vector2& vel, float radius = 8.0f) // Increased default from 4.0f to 8.0f
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
        ClosestPointInfo cpiLeftCurrent = track->findClosestPointOnCurve(position, CurveSide::Left);
        ClosestPointInfo cpiRightCurrent = track->findClosestPointOnCurve(position, CurveSide::Right);
        
        // Check collision with current position and boundaries
        if (cpiLeftCurrent.isValid && cpiLeftCurrent.distance < collisionRadius) {
            active = false;
            return;
        }
        
        if (cpiRightCurrent.isValid && cpiRightCurrent.distance < collisionRadius) {
            active = false;
            return;
        }
        
        // If moving fast, also check for "tunneling" through boundaries by sampling points along movement path
        float movementLength = (position - previousPosition).length();
        if (movementLength > collisionRadius * 1.5f) {
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
};

#endif
