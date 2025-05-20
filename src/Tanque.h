#ifndef __TANQUE_H__
#define __TANQUE_H__

#include "gl_canvas2d.h"
#include "Vector2.h"
#include <cmath>
#include <vector>
#include "BSplineTrack.h"
#include "Projectile.h"
#include "Target.h"
#include "ExplosionManager.h" 

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
    static const int COLLISION_REBOUND_FRAMES = 90; 
    static constexpr float REBOUND_SPEED_FACTOR = 0.3f; 

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
    static const int INVULNERABILITY_FRAMES = 60; 
    bool isShieldInvulnerable; 

    // Add shield property
    bool hasShield;

    // Add explosion manager
    ExplosionManager explosions;

    Tanque(float x, float y, float initialSpeed = 1.0f, float initialRotationRate = 0.03f);

    void Update(float mouseX, float mouseY, bool rotateLeft, bool rotateRight, BSplineTrack* track);
    void Render();
    
    // Projectile related methods
    bool FireProjectile();
    Vector2 GetCannonTipPosition() const;
    void UpdateProjectiles(BSplineTrack* track);

    // Check if a projectile hits any targets and returns the index of hit target or -1
    int CheckProjectileTargetCollision(const Projectile& proj, std::vector<Target>& targets);
    
    // Check all projectiles against all targets
    bool CheckAllProjectilesAgainstTargets(std::vector<Target>& targets, int& hitTargetIndex, int& hitProjectileIndex);

    // Change the return type from void to int
    int CheckTargetCollisions(std::vector<Target>& targets);

private:
    void CheckCollisionAndRespond(BSplineTrack* track);
};

#endif
