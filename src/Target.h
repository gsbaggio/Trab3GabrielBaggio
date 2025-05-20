#ifndef __TARGET_H__
#define __TARGET_H__

#include "Vector2.h"
#include "gl_canvas2d.h"
#include <cmath>
#include <algorithm> 
#include <vector>   

// Forward declaration for collision detection
class BSplineTrack;

// Define a projectile class for enemy projectiles
class EnemyProjectile {
public:
    Vector2 position;
    Vector2 velocity;
    bool active;
    float radius;
    
    EnemyProjectile() : active(false), radius(5.0f) {}
    
    EnemyProjectile(const Vector2& pos, const Vector2& vel) 
        : position(pos), velocity(vel), active(true), radius(5.0f) {}
    
    void Update() {
        if (!active) return;
        position = position + velocity;
    }
    
    void Render() {
        if (!active) return;
        CV::color(1.0f, 0.5f, 0.0f); // Bright orange for better visibility
        CV::circleFill(position.x, position.y, radius, 8);
        CV::color(1.0f, 0.2f, 0.0f); // Red outline
        CV::circle(position.x, position.y, radius, 8);
    }
    
    bool CheckCollisionWithTrack(BSplineTrack* track);
};

// Define target types
enum class TargetType {
    Basic = 0,    // Original stationary circular target
    Shooter = 1,  // Triangle shooter that fires at the tank
    Star = 2      // Star that chases the tank when in range
};

class Target {
public:
    Vector2 position;
    bool active;
    float radius;
    int health;        // Health points
    int maxHealth;     // Maximum health
    TargetType type;   // Type of target
    
    // Shooter-specific properties
    float aimAngle;                // Angle at which the shooter is aiming
    float shootingRadius;          // Range within which the shooter will fire
    int firingCooldown;            // Current cooldown timer
    int firingCooldownReset;       // Time between shots
    std::vector<EnemyProjectile> projectiles; // Enemy projectiles
    
    // Star-specific properties
    float detectionRadius;    // Range within which the star starts chasing
    float moveSpeed;          // Movement speed of the star
    bool isChasing;           // Whether the star is currently chasing the tank
    float rotationAngle;      // Current rotation angle of the star
    float rotationSpeed;      // How fast the star rotates
    
    Target();
    Target(const Vector2& pos, TargetType targetType = TargetType::Basic);

    void Update(const Vector2& tankPosition, BSplineTrack* track);
    void Render();
    bool CheckCollision(const Vector2& point);
    bool CheckCollisionWithTank(const Vector2& tankPos, float tankWidth, float tankHeight, float tankAngle);
    void TakeDamage(int amount);
    bool FireAtTarget(const Vector2& targetPos);

private:
    void RenderBasicTarget();
    void RenderShooterTarget();
    void RenderStarTarget();
    void UpdateProjectiles(BSplineTrack* track);
};

#endif
