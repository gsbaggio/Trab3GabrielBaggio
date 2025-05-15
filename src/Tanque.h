#ifndef __TANQUE_H__
#define __TANQUE_H__

#include "gl_canvas2d.h"
#include "Vector2.h" // Assuming this file exists and provides a Vector2 class
#include <cmath>     // For M_PI, cos, sin, atan2
#include <vector>    // For std::vector
#include "BSplineTrack.h" // Include full header instead of forward declaration

// Define M_PI if not available (common on Windows with MSVC)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Projectile class for tank firing
class Projectile {
public:
    Vector2 position;
    Vector2 previousPosition; // Store previous position for continuous collision detection
    Vector2 velocity;
    bool active;
    int lifetime; // Optional: limit projectile lifetime
    float collisionRadius; // Explicit collision radius value
    
    Projectile(const Vector2& pos, const Vector2& vel) 
        : position(pos), previousPosition(pos), velocity(vel), active(true), lifetime(300), collisionRadius(8.0f) {}
    
    void Update() {
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
        CV::circleFill(position.x, position.y, 4.0f, 10); // Visual radius stays at 4.0f
    }
    
    // Check if projectile hits track boundaries - improved collision detection
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

// Target class for explosive barrels
class Target {
public:
    Vector2 position;
    bool active;
    float radius;
    int health;        // Health points (default = 2 hits to destroy)
    int maxHealth;     // Maximum health
    
    Target() : active(false), radius(12.0f), health(2), maxHealth(2) {}
    
    Target(const Vector2& pos) 
        : position(pos), active(true), radius(12.0f), health(2), maxHealth(2) {}
    
    void Render() {
        if (!active) return;
        
        // Draw barrel (orange-red circle with dark border)
        float healthRatio = static_cast<float>(health) / maxHealth;
        CV::color(0.8f * healthRatio, 0.3f * healthRatio, 0.0f);
        CV::circleFill(position.x, position.y, radius, 15);
        CV::color(0.4f * healthRatio, 0.15f * healthRatio, 0.0f);
        CV::circle(position.x, position.y, radius, 15);
        
        // Add a simple "X" marking
        CV::color(0.2f, 0.2f, 0.2f);
        CV::line(position.x - radius/1.5f, position.y - radius/1.5f, 
                position.x + radius/1.5f, position.y + radius/1.5f);
        CV::line(position.x + radius/1.5f, position.y - radius/1.5f, 
                position.x - radius/1.5f, position.y + radius/1.5f);
        
        // Draw health bar if target has less than max health
        if (health < maxHealth) {
            float barWidth = radius * 2.0f;
            float barHeight = 4.0f;
            float fillWidth = barWidth * static_cast<float>(health) / maxHealth;
            
            // Health bar background
            CV::color(0.3f, 0.3f, 0.3f);
            CV::rectFill(position.x - radius, position.y - radius - 10, 
                         position.x - radius + barWidth, position.y - radius - 10 + barHeight);
            
            // Health bar fill
            CV::color(1.0f - healthRatio, healthRatio, 0.0f); // Red to green
            CV::rectFill(position.x - radius, position.y - radius - 10, 
                         position.x - radius + fillWidth, position.y - radius - 10 + barHeight);
        }
    }
    
    bool CheckCollision(const Vector2& point) {
        if (!active) return false;
        
        float dx = point.x - position.x;
        float dy = point.y - position.y;
        float distanceSquared = dx*dx + dy*dy;
        
        return distanceSquared <= radius * radius;
    }
    
    // Check collision with a tank (rectangle)
    bool CheckCollisionWithTank(const Vector2& tankPos, float tankWidth, float tankHeight, float tankAngle) {
        if (!active) return false;
        
        // Convert tank bounds to world coordinates
        float halfW = tankWidth / 2.0f;
        float halfH = tankHeight / 2.0f;
        float cosB = cos(tankAngle);
        float sinB = sin(tankAngle);
        
        Vector2 localCorners[4] = {
            Vector2(-halfW, -halfH), Vector2(halfW, -halfH),
            Vector2(halfW, halfH), Vector2(-halfW, halfH)
        };
        
        Vector2 worldCorners[4];
        for (int i = 0; i < 4; ++i) {
            worldCorners[i].x = localCorners[i].x * cosB - localCorners[i].y * sinB + tankPos.x;
            worldCorners[i].y = localCorners[i].x * sinB + localCorners[i].y * cosB + tankPos.y;
        }
        
        // Check if any corner of the tank is inside the target circle
        for (int i = 0; i < 4; ++i) {
            if (CheckCollision(worldCorners[i])) {
                return true;
            }
        }
        
        // Check if any edge of the tank intersects the target circle
        for (int i = 0; i < 4; ++i) {
            int j = (i + 1) % 4;
            
            // Line segment from worldCorners[i] to worldCorners[j]
            Vector2 lineStart = worldCorners[i];
            Vector2 lineEnd = worldCorners[j];
            Vector2 lineDir = lineEnd - lineStart;
            float lineLength = lineDir.length();
            
            // Normalize direction
            if (lineLength > 0.0001f) {
                lineDir = lineDir * (1.0f / lineLength);
                
                // Vector from lineStart to circle center
                Vector2 startToCircle = position - lineStart;
                
                // Projection of startToCircle onto lineDir
                float projection = startToCircle.x * lineDir.x + startToCircle.y * lineDir.y;
                projection = std::max(0.0f, std::min(lineLength, projection));
                
                // Closest point on line segment to circle
                Vector2 closestPoint = lineStart + lineDir * projection;
                
                // Check if closest point is within circle radius
                float distSq = (position - closestPoint).lengthSq();
                if (distSq <= radius * radius) {
                    return true;
                }
            }
        }
        
        return false;
    }
    
    void TakeDamage(int amount) {
        health -= amount;
        if (health <= 0) {
            health = 0;
            active = false;
        }
    }
};

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
    static constexpr float REBOUND_SPEED_FACTOR = 0.3f; // 30% of normal speed for rebound

    // Projectile related members
    int firingCooldown;
    int firingCooldownReset;
    float projectileSpeed;
    std::vector<Projectile> projectiles;

    // Health related members
    int health;
    int maxHealth;
    bool isInvulnerable;
    int invulnerabilityTimer;
    static const int INVULNERABILITY_FRAMES = 60; // 1 second at 60fps

    Tanque(float x, float y, float initialSpeed = 1.0f, float initialRotationRate = 0.03f); // Adjusted defaults for per-frame

    void Update(float mouseX, float mouseY, bool rotateLeft, bool rotateRight, BSplineTrack* track);
    void Render();
    
    // Projectile related methods
    bool FireProjectile();
    Vector2 GetCannonTipPosition() const;
    void UpdateProjectiles(BSplineTrack* track);

    // Check if a projectile hits any targets and returns the index of hit target or -1
    int CheckProjectileTargetCollision(const Projectile& proj, std::vector<Target>& targets) {
        if (!proj.active) return -1;
        
        for (size_t i = 0; i < targets.size(); i++) {
            if (targets[i].active && targets[i].CheckCollision(proj.position)) {
                return static_cast<int>(i);
            }
        }
        
        return -1;
    }
    
    // Check all projectiles against all targets
    bool CheckAllProjectilesAgainstTargets(std::vector<Target>& targets, int& hitTargetIndex, int& hitProjectileIndex) {
        for (size_t i = 0; i < projectiles.size(); i++) {
            int targetIdx = CheckProjectileTargetCollision(projectiles[i], targets);
            if (targetIdx >= 0) {
                hitTargetIndex = targetIdx;
                hitProjectileIndex = static_cast<int>(i);
                return true;
            }
        }
        
        hitTargetIndex = -1;
        hitProjectileIndex = -1;
        return false;
    }

    void CheckTargetCollisions(std::vector<Target>& targets);

private:
    void CheckCollisionAndRespond(BSplineTrack* track);
};

#endif
