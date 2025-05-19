#include "Tanque.h"
#include "gl_canvas2d.h"
#include <cmath> // For M_PI, cos, sin, atan2
#include <algorithm> // For std::remove_if
#include "BSplineTrack.h" // Added this include

// Define M_PI if not available (common on Windows with MSVC)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Tanque::Tanque(float x, float y, float initialSpeed, float initialRotationRate) {
    position.set(x, y);
    baseAngle = 0.0f;
    topAngle = 0.0f;
    speed = initialSpeed;
    rotationRate = initialRotationRate;
    turretRotationSpeed = 0.05f; // Assuming this is now per-frame if deltaTime was globally removed
    baseWidth = 60.0f;
    baseHeight = 40.0f;
    turretRadius = 15.0f;
    cannonLength = 40.0f;
    cannonWidth = 6.0f;

    forwardVector.set(cos(baseAngle), sin(baseAngle));

    // Initialize collision members
    lastSafePosition = position;
    isColliding = false;
    collisionTimer = 0;

    // Initialize projectile related members
    firingCooldown = 0;
    firingCooldownReset = 45; // ~0.75 seconds at 60fps
    projectileSpeed = 3.5f;  // Decreased from 7.0f to 4.0f

    // Initialize health related members
    health = 100;
    maxHealth = 100;
    isInvulnerable = false;
    invulnerabilityTimer = 0;
    isShieldInvulnerable = false;

    // Initialize shield
    hasShield = false;
}

void Tanque::Update(float mouseX, float mouseY, bool rotateLeft, bool rotateRight, BSplineTrack* track) {
    // Decrease firing cooldown if active
    if (firingCooldown > 0) {
        firingCooldown--;
    }

    // Update projectiles
    UpdateProjectiles(track);

    // Update explosions
    explosions.Update();

    if (isColliding) {
        // Rebound movement
        position.x -= forwardVector.x * speed * REBOUND_SPEED_FACTOR;
        position.y -= forwardVector.y * speed * REBOUND_SPEED_FACTOR;
        collisionTimer--;
        if (collisionTimer <= 0) {
            isColliding = false;
            printf("Rebound finished.\n");
        }

        // Turret can still aim during rebound
        float dx_turret = mouseX - position.x;
        float dy_turret = mouseY - position.y;
        float targetAngle_turret = atan2(dy_turret, dx_turret);
        float currentTopAngle_turret = fmod(topAngle + M_PI, 2 * M_PI) - M_PI;
        if (currentTopAngle_turret < -M_PI) currentTopAngle_turret += 2 * M_PI;
        float angleDifference_turret = targetAngle_turret - currentTopAngle_turret;
        if (angleDifference_turret > M_PI) angleDifference_turret -= 2 * M_PI;
        else if (angleDifference_turret < -M_PI) angleDifference_turret += 2 * M_PI;

        float maxRotationThisFrame_turret = turretRotationSpeed; // Assuming per-frame

        if (std::abs(angleDifference_turret) <= maxRotationThisFrame_turret) {
            topAngle = targetAngle_turret;
        } else {
            topAngle += (angleDifference_turret > 0 ? maxRotationThisFrame_turret : -maxRotationThisFrame_turret);
        }
        topAngle = fmod(topAngle, 2 * M_PI);
        if (topAngle < 0) topAngle += 2 * M_PI;

        return; // Skip normal movement and new collision checks
    }

    // Not currently colliding and rebounding: Normal update logic
    lastSafePosition = position;

    // Update base rotation
    if (rotateLeft) {
        baseAngle -= rotationRate;
    }
    if (rotateRight) {
        baseAngle += rotationRate;
    }

    // Keep baseAngle within 0 to 2*PI
    if (baseAngle > 2 * M_PI) baseAngle -= 2 * M_PI;
    if (baseAngle < 0) baseAngle += 2 * M_PI;

    // Update forward vector based on baseAngle
    forwardVector.set(cos(baseAngle), sin(baseAngle));

    // Tentative new position (no deltaTime)
    Vector2 tentativePosition = position;
    tentativePosition.x += forwardVector.x * speed;
    tentativePosition.y += forwardVector.y * speed;

    // Store current position, set to tentative for collision check
    Vector2 currentActualPosition = position;
    position = tentativePosition;

    CheckCollisionAndRespond(track); // This method might revert position to lastSafePosition and set isColliding

    // If a collision occurred, position was reset by CheckCollisionAndRespond.
    // The rebound will start in the next Update call.
    // If no collision, position remains the new tentativePosition.

    // Update top (turret) angle to point towards the mouse
    float dx = mouseX - position.x; // Use the (potentially reverted) position
    float dy = mouseY - position.y;
    float targetAngle = atan2(dy, dx);

    float currentTopAngle = fmod(topAngle + M_PI, 2 * M_PI) - M_PI;
    if (currentTopAngle < -M_PI) currentTopAngle += 2 * M_PI; // Ensure it's in [-PI, PI]
    float angleDifference = targetAngle - currentTopAngle;

    // Normalize the angle difference to the shortest path (-PI to PI)
    if (angleDifference > M_PI) {
        angleDifference -= 2 * M_PI;
    } else if (angleDifference < -M_PI) {
        angleDifference += 2 * M_PI;
    }

    float maxRotationThisFrame = turretRotationSpeed; // Assuming turretRotationSpeed is now per-frame

    if (std::abs(angleDifference) <= maxRotationThisFrame) {
        topAngle = targetAngle;
    } else {
        topAngle += (angleDifference > 0 ? maxRotationThisFrame : -maxRotationThisFrame);
    }

    // Normalize topAngle to be within 0 to 2*PI (or -PI to PI, consistently)
    topAngle = fmod(topAngle, 2 * M_PI);
    if (topAngle < 0) {
        topAngle += 2 * M_PI;
    }
}

void Tanque::Render() {
    // Render projectiles first (so tank appears above them)
    for (auto& proj : projectiles) {
        proj.Render();
    }

    // Render explosions before tank
    explosions.Render();

    // Draw health bar below the tank (changed from above)
    float healthBarWidth = baseWidth * 1.2f;  // Make it slightly wider than the tank
    float healthBarHeight = 5.0f;
    float healthBarY = position.y + baseHeight / 2.0f + 5.0f; // Position below the tank instead of above
    float healthPercent = static_cast<float>(health) / maxHealth;

    // Health bar background (red)
    CV::color(1.0f, 0.2f, 0.2f);
    CV::rectFill(position.x - healthBarWidth / 2.0f, healthBarY,
                position.x + healthBarWidth / 2.0f, healthBarY + healthBarHeight);

    // Health bar fill (green)
    CV::color(0.2f, 0.8f, 0.2f);
    CV::rectFill(position.x - healthBarWidth / 2.0f, healthBarY,
                position.x - healthBarWidth / 2.0f + healthBarWidth * healthPercent, healthBarY + healthBarHeight);

    // Use a flash effect when tank is invulnerable (hit)
    if (isInvulnerable) {
        if ((invulnerabilityTimer / 5) % 2 == 0) {
            if (isShieldInvulnerable) {
                CV::color(0.3f, 0.3f, 1.0f); // Blue flash for shield
            } else {
                CV::color(1.0f, 0.2f, 0.2f); // Red flash for damage
            }
        } else {
            CV::color(0.2f, 0.5f, 0.2f); // Normal green
        }
    } else {
        CV::color(0.2f, 0.5f, 0.2f); // Normal green
    }

    // Render Base (Rectangle)
    float halfW = baseWidth / 2.0f;
    float halfH = baseHeight / 2.0f;

    // Local corners of the base rectangle (before rotation)
    Vector2 p1_local(-halfW, -halfH);
    Vector2 p2_local( halfW, -halfH);
    Vector2 p3_local( halfW,  halfH);
    Vector2 p4_local(-halfW,  halfH);

    // Rotated corners
    float cosB = cos(baseAngle);
    float sinB = sin(baseAngle);

    Vector2 p1_world(p1_local.x * cosB - p1_local.y * sinB + position.x, p1_local.x * sinB + p1_local.y * cosB + position.y);
    Vector2 p2_world(p2_local.x * cosB - p2_local.y * sinB + position.x, p2_local.x * sinB + p2_local.y * cosB + position.y);
    Vector2 p3_world(p3_local.x * cosB - p3_local.y * sinB + position.x, p3_local.x * sinB + p3_local.y * cosB + position.y);
    Vector2 p4_world(p4_local.x * cosB - p4_local.y * sinB + position.x, p4_local.x * sinB + p4_local.y * cosB + position.y);

    // Draw the base as a filled polygon instead of lines
    float vx_base[4] = {p1_world.x, p2_world.x, p3_world.x, p4_world.x};
    float vy_base[4] = {p1_world.y, p2_world.y, p3_world.y, p4_world.y};
    CV::polygonFill(vx_base, vy_base, 4);

    // Draw shield visualization if tank has a shield
    CV::color(0.0f, 0.0f, 1.0f); // Blue color for shield
    if (hasShield) {
        for(int side = 0; side < 4; side++){
            CV::line(vx_base[side], vy_base[side], vx_base[(side + 1) % 4], vy_base[(side + 1) % 4]);
        }
    }

    // Render Top (Turret - Circle and Cannon - Rectangle)
    CV::color(0.1f, 0.3f, 0.1f); // Darker green for turret
    CV::circleFill(position.x, position.y, turretRadius, 20); // Draw turret base

    // Cannon calculations
    float halfCW = cannonWidth / 2.0f;
    float cosT = cos(topAngle);
    float sinT = sin(topAngle);

    // Points relative to tank's position
    Vector2 c1_local(0, -halfCW);
    Vector2 c2_local(0,  halfCW);
    Vector2 c3_local(cannonLength, -halfCW);
    Vector2 c4_local(cannonLength,  halfCW);

    // Rotate these local points by topAngle and add to tank's position
    Vector2 c1(position.x + (c1_local.x * cosT - c1_local.y * sinT), position.y + (c1_local.x * sinT + c1_local.y * cosT));
    Vector2 c2(position.x + (c2_local.x * cosT - c2_local.y * sinT), position.y + (c2_local.x * sinT + c2_local.y * cosT));
    Vector2 c3(position.x + (c3_local.x * cosT - c3_local.y * sinT), position.y + (c3_local.x * sinT + c3_local.y * cosT));
    Vector2 c4(position.x + (c4_local.x * cosT - c4_local.y * sinT), position.y + (c4_local.x * sinT + c4_local.y * cosT));

    // Draw cannon as a filled polygon instead of lines
    float vx_cannon[4] = {c1.x, c2.x, c4.x, c3.x}; // Order matters for convex polygon
    float vy_cannon[4] = {c1.y, c2.y, c4.y, c3.y};
    CV::polygonFill(vx_cannon, vy_cannon, 4);


    // Draw cooldown indicator if cooling down
    if (firingCooldown > 0) {
        float cooldownFraction = (float)firingCooldown / firingCooldownReset;
        float barLength = 25.0f * cooldownFraction;
        CV::color(1.0f, 0.3f, 0.3f); // Red cooldown bar
        CV::rectFill(position.x - 12.5f, position.y - turretRadius - 10, position.x - 12.5f + barLength, position.y - turretRadius - 5);
    }
}

// New method for firing projectiles
bool Tanque::FireProjectile() {
    if (firingCooldown > 0) {
        return false; // Can't fire yet
    }

    // Get the position of the cannon tip
    Vector2 cannonTip = GetCannonTipPosition();

    // Create velocity vector based on cannon direction
    Vector2 projectileVelocity(cos(topAngle) * projectileSpeed, sin(topAngle) * projectileSpeed);

    // Create and add the projectile
    Projectile newProjectile(cannonTip, projectileVelocity);
    projectiles.push_back(newProjectile);

    // Reset cooldown
    firingCooldown = firingCooldownReset;

    return true;
}

// Helper to calculate the position of the cannon tip
Vector2 Tanque::GetCannonTipPosition() const {
    return Vector2(
        position.x + cos(topAngle) * cannonLength,
        position.y + sin(topAngle) * cannonLength
    );
}

// New method to update projectiles
void Tanque::UpdateProjectiles(BSplineTrack* track) {
    // Update each projectile and check for collisions
    for (auto& proj : projectiles) {
        if (proj.active) {
            proj.Update();
            // Pass the explosions manager for collision effects
            if (proj.CheckCollisionWithTrack(track, &explosions)) {
                // Projectile is now inactive due to collision
            }
        }
    }

    // Remove inactive projectiles (using erase-remove idiom)
    projectiles.erase(
        std::remove_if(
            projectiles.begin(),
            projectiles.end(),
            [](const Projectile& p) { return !p.active; }
        ),
        projectiles.end()
    );
}

void Tanque::CheckCollisionAndRespond(BSplineTrack* track) {
    if (!track) {
        this->isColliding = false;
        return;
    }

    // Skip health damage if the tank is already invulnerable
    bool canTakeDamage = !isInvulnerable;

    Vector2 currentTankPosition = this->position; // Current tentative position of the tank's center

    // Calculate tank's world corners based on currentTankPosition and baseAngle
    float halfW = this->baseWidth / 2.0f;
    float halfH = this->baseHeight / 2.0f;
    float cosB = cos(this->baseAngle);
    float sinB = sin(this->baseAngle);

    Vector2 local_corners[4] = {
        Vector2(-halfW, -halfH), Vector2(halfW, -halfH),
        Vector2(halfW,  halfH), Vector2(-halfW,  halfH)
    };
    Vector2 world_corners[4];
    for (int i = 0; i < 4; ++i) {
        world_corners[i].x = local_corners[i].x * cosB - local_corners[i].y * sinB + currentTankPosition.x;
        world_corners[i].y = local_corners[i].x * sinB + local_corners[i].y * cosB + currentTankPosition.y;
    }

    ClosestPointInfo cpiLeft = track->findClosestPointOnCurve(currentTankPosition, CurveSide::Left);
    ClosestPointInfo cpiRight = track->findClosestPointOnCurve(currentTankPosition, CurveSide::Right);

    bool collisionThisFrame = false;
    // Store projections for debugging if needed, ensure they are initialized.
    // float max_proj_left_debug = -FLT_MAX;
    // float max_proj_right_debug = -FLT_MAX;

    // Check collision with Left Curve
    if (cpiLeft.isValid) {
        float max_projection_left = -FLT_MAX;
        for (int i = 0; i < 4; ++i) {
            Vector2 vec_corner_to_cl_point = world_corners[i] - cpiLeft.point;
            float projection = vec_corner_to_cl_point.x * cpiLeft.normal.x + vec_corner_to_cl_point.y * cpiLeft.normal.y;
            if (projection > max_projection_left) {
                max_projection_left = projection;
            }
        }
        // Assuming cpiLeft.normal points "outward" from the track (to the left of the LeftCurve).
        // If any corner's projection is positive, it's on the "outside".
        if (max_projection_left > 0.0f) {
            collisionThisFrame = true;
        }
    }

    // Check collision with Right Curve
    if (!collisionThisFrame && cpiRight.isValid) {
        // cpiRight.normal is the "left normal" of the RightCurve.
        // A collision occurs if any tank corner is to the "right" of the RightCurve.
        // This means the projection of (corner - closest_point_on_curve) onto normal_R should be negative.
        // We look for the most negative projection (min_projection_right).
        float min_projection_right = FLT_MAX;
        for (int i = 0; i < 4; ++i) {
            Vector2 vec_corner_to_cr_point = world_corners[i] - cpiRight.point;
            float projection = vec_corner_to_cr_point.x * cpiRight.normal.x + vec_corner_to_cr_point.y * cpiRight.normal.y;
            if (projection < min_projection_right) {
                min_projection_right = projection;
            }
        }
        // If the smallest projection (most "to the right" relative to N_R) is negative,
        // it means a part of the tank is to the right of the RightCurve (outside the track).
        if (min_projection_right < 0.0f) {
            collisionThisFrame = true;
        }
    }

    if (collisionThisFrame) {
        this->isColliding = true;
        this->collisionTimer = COLLISION_REBOUND_FRAMES;
        this->position = this->lastSafePosition; // Revert to last known safe position

        // Only apply damage if the tank isn't already invulnerable
        if (canTakeDamage) {
            // Check if we have a shield to block the damage
            if (hasShield) {
                hasShield = false; // Consume the shield
                // Still make the tank temporarily invulnerable and flash
                isInvulnerable = true;
                isShieldInvulnerable = true; // Set flag for shield invulnerability
                invulnerabilityTimer = INVULNERABILITY_FRAMES;
                printf("Shield blocked damage from track collision!\n");
            } else {
                // Tank takes damage - 25% of max health (same as target collision)
                int damageTaken = maxHealth / 4;  // 25% of max health
                health -= damageTaken;
                if (health < 0) health = 0;

                // Make tank temporarily invulnerable and flash (just like with target collisions)
                isInvulnerable = true;
                isShieldInvulnerable = false; // Regular damage
                invulnerabilityTimer = INVULNERABILITY_FRAMES;
            }
        }
    } else {
        // No new collision detected by this check.
        // If isColliding was true due to an ongoing rebound, Update() handles the timer.
        // If timer runs out, Update() sets isColliding = false.
    }
}

// Changed return type from void to int to match the header declaration
int Tanque::CheckTargetCollisions(std::vector<Target>& targets) {
    // Skip if tank is currently invulnerable
    if (isInvulnerable) {
        invulnerabilityTimer--;
        if (invulnerabilityTimer <= 0) {
            isInvulnerable = false;
            isShieldInvulnerable = false; // Reset shield invulnerability flag too
        }
        return -1;
    }

    // Check each target for collision with the tank
    for (size_t i = 0; i < targets.size(); i++) {
        // Skip Star type targets as they are handled separately in the main render loop
        if (targets[i].type == TargetType::Star) {
            continue;
        }

        if (targets[i].active && targets[i].CheckCollisionWithTank(position, baseWidth, baseHeight, baseAngle)) {
            // Check if we have a shield to block the damage
            if (hasShield) {
                hasShield = false; // Consume the shield
                // Still make the tank temporarily invulnerable and flash
                isInvulnerable = true;
                isShieldInvulnerable = true; // Set flag for shield invulnerability
                invulnerabilityTimer = INVULNERABILITY_FRAMES;
                printf("Shield blocked damage from target collision!\n");

                // Target takes damage and should be destroyed when hitting a shield
                targets[i].TakeDamage(targets[i].health); // Kill the target by dealing its full health

                // Check if target was destroyed by this collision
                if (!targets[i].active) {
                    return i; // Return the index of the destroyed target
                }
            } else {
                // Apply normal damage
                int damageTaken = maxHealth / 4;  // 25% of max health
                health -= damageTaken;
                if (health < 0) health = 0;

                // Make tank temporarily invulnerable to prevent multiple rapid hits
                isInvulnerable = true;
                isShieldInvulnerable = false; // Regular damage
                invulnerabilityTimer = INVULNERABILITY_FRAMES;

                // Target takes damage too
                targets[i].TakeDamage(1);

                // Check if target was destroyed by this collision
                if (!targets[i].active) {
                    return i; // Return the index of the destroyed target
                }
            }

            // If we hit any target, we can break the loop since we're now invulnerable
            break;
        }
    }

    return -1; // Return -1 if no target was destroyed
}

// Add the missing implementations from Tanque.h
int Tanque::CheckProjectileTargetCollision(const Projectile& proj, std::vector<Target>& targets) {
    if (!proj.active) return -1;

    for (size_t i = 0; i < targets.size(); i++) {
        if (targets[i].active && targets[i].CheckCollision(proj.position)) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

bool Tanque::CheckAllProjectilesAgainstTargets(std::vector<Target>& targets, int& hitTargetIndex, int& hitProjectileIndex) {
    for (size_t i = 0; i < projectiles.size(); i++) {
        int targetIdx = CheckProjectileTargetCollision(projectiles[i], targets);
        if (targetIdx >= 0) {
            hitTargetIndex = targetIdx;
            hitProjectileIndex = static_cast<int>(i);

            // Create explosion on target hit
            projectiles[i].CreateExplosionOnCollision(&explosions);

            return true;
        }
    }

    hitTargetIndex = -1;
    hitProjectileIndex = -1;
    return false;
}
