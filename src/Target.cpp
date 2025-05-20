#include "Target.h"
#include "BSplineTrack.h"
#include <cmath>
#include <algorithm> 

Target::Target()
    : active(false), radius(12.0f), health(2), maxHealth(2), type(TargetType::Basic),
      aimAngle(0.0f), shootingRadius(250.0f), firingCooldown(0), firingCooldownReset(120),
      detectionRadius(200.0f), moveSpeed(0.8f), isChasing(false), rotationAngle(0.0f), rotationSpeed(0.05f) {}

Target::Target(const Vector2& pos, TargetType targetType)
    : position(pos), active(true), radius(12.0f), health(2), maxHealth(2), type(targetType),
      aimAngle(0.0f), shootingRadius(200.0f), firingCooldown(0), firingCooldownReset(90),
      detectionRadius(200.0f), moveSpeed(0.8f), isChasing(false), rotationAngle(0.0f), rotationSpeed(0.05f) {}

void Target::Update(const Vector2& tankPosition, BSplineTrack* track) {
    if (!active) return;
    
    // Update behavior based on target type
    if (type == TargetType::Shooter) {
        // Calculate aim angle toward tank
        float dx = tankPosition.x - position.x;
        float dy = tankPosition.y - position.y;
        aimAngle = atan2(dy, dx);

        // Calculate distance to tank
        float distSq = Vector2(dx, dy).lengthSq();

        // Decrease firing cooldown
        if (firingCooldown > 0) {
            firingCooldown--;
        }

        // Fire at tank if in range and cooldown elapsed
        if (distSq <= shootingRadius * shootingRadius && firingCooldown <= 0) {
            if (FireAtTarget(tankPosition)) {
                firingCooldown = firingCooldownReset;
            }
        }

        // Update existing projectiles
        UpdateProjectiles(track);
    }
    else if (type == TargetType::Star) {
        // Always update the rotation angle for spinning effect
        rotationAngle += rotationSpeed;
        if (rotationAngle > 2 * M_PI) {
            rotationAngle -= 2 * M_PI;
        }
        
        // Calculate distance to tank
        float dx = tankPosition.x - position.x;
        float dy = tankPosition.y - position.y;
        float distSq = dx*dx + dy*dy;
        
        // If within detection radius, start chasing
        if (distSq <= detectionRadius * detectionRadius) {
            isChasing = true;
            
            // Calculate direction to tank
            float dist = sqrt(distSq);
            
            // Only move if not right on top of the tank
            if (dist > 0.1f) {
                // Normalize and scale by speed
                float dirX = dx / dist;
                float dirY = dy / dist;
                
                // Move toward tank (slower than before)
                position.x += dirX * moveSpeed;
                position.y += dirY * moveSpeed;
            }
        }
    }
}

void Target::Render() {
    if (!active) return;
    
    if (type == TargetType::Basic) {
        RenderBasicTarget();
    } else if (type == TargetType::Shooter) {
        RenderShooterTarget();
        
        // Render projectiles
        for (auto& proj : projectiles) {
            proj.Render();
        }
    } else if (type == TargetType::Star) {
        RenderStarTarget();
    }
    
    // Draw health bar for all target types if health < maxHealth
    if (health < maxHealth) {
        float healthRatio = static_cast<float>(health) / maxHealth;
        float barWidth = radius * 2.0f;
        float barHeight = 4.0f;
        float fillWidth = barWidth * healthRatio;

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

void Target::RenderBasicTarget() {
    // Original circular target rendering
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
}

void Target::RenderShooterTarget() {
    // Triangle target that aims at the tank
    float size = radius * 1.5f; // Slightly larger than basic target

    // Calculate triangle vertices
    float cosA = cos(aimAngle);
    float sinA = sin(aimAngle);

    // Point 1: front point (aimed at tank)
    float x1 = position.x + cosA * size;
    float y1 = position.y + sinA * size;

    // Point 2 and 3: back corners (perpendicular to aim direction)
    float x2 = position.x - cosA * size * 0.5f + sinA * size * 0.7f;
    float y2 = position.y - sinA * size * 0.5f - cosA * size * 0.7f;
    float x3 = position.x - cosA * size * 0.5f - sinA * size * 0.7f;
    float y3 = position.y - sinA * size * 0.5f + cosA * size * 0.7f;

    // Draw the triangle
    float vx[3] = {x1, x2, x3};
    float vy[3] = {y1, y2, y3};

    float healthRatio = static_cast<float>(health) / maxHealth;
    CV::color(1.0f * healthRatio, 0.9f * healthRatio, 0.0f); // Yellow
    CV::polygonFill(vx, vy, 3);

    CV::color(0.7f * healthRatio, 0.6f * healthRatio, 0.0f); // Border
    CV::line(x1, y1, x2, y2);
    CV::line(x2, y2, x3, y3);
    CV::line(x3, y3, x1, y1);

    // Draw shooting range indicator when in cooldown
    if (firingCooldown > 0) {
        float cooldownRatio = static_cast<float>(firingCooldown) / firingCooldownReset;
        CV::color(1.0f, 0.0f, 0.0f, 0.3f); // Red with transparency
        CV::circle(position.x, position.y, shootingRadius * cooldownRatio * 0.1f, 30);
    }
}

// Render a star shape with proper geometry
void Target::RenderStarTarget() {
    // Star parameters
    float outerRadius = radius * 1.5f;
    float innerRadius = radius * 0.6f;
    const int numPoints = 5;  // 5-pointed star
    
    // We need 10 points total (5 outer points and 5 inner points)
    Vector2 points[numPoints * 2];
    float angle = rotationAngle - M_PI/2; 
    
    // Draw points in a clockwise order, alternating between outer and inner points
    for (int i = 0; i < numPoints * 2; i++) {
        float r = (i % 2 == 0) ? outerRadius : innerRadius;
        
        points[i].x = position.x + r * cos(angle);
        points[i].y = position.y + r * sin(angle);
        
        angle += M_PI / numPoints;
    }

    
    // Gray color with health tint
    float healthRatio = static_cast<float>(health) / maxHealth;
    float shade = 0.5f + 0.3f * (1.0f - healthRatio); // Darker when damaged
    
    // Draw outline
    CV::color(shade * 0.7f, shade * 0.2f, shade * 0.2f); // Darker gray outline
    for (int i = 0; i < numPoints * 2; i++) {
        int next = (i + 1) % (numPoints * 2);
        CV::line(points[i].x, points[i].y, points[next].x, points[next].y);
    }
    
    // Draw indicator when chasing
    if (isChasing) {
        CV::color(1.0f, 0.2f, 0.2f); // Red indicator
        CV::circleFill(position.x, position.y, radius * 0.3f, 8);
    }
}

bool Target::CheckCollision(const Vector2& point) {
    if (!active) return false;

    float dx = point.x - position.x;
    float dy = point.y - position.y;
    float distanceSquared = dx*dx + dy*dy;

    return distanceSquared <= radius * radius;
}

bool Target::CheckCollisionWithTank(const Vector2& tankPos, float tankWidth, float tankHeight, float tankAngle) {
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
        if (CheckCollision(worldCorners[i])) return true;
    }

    // For simplicity, we're keeping the same collision check for all target types
    return false;
}

void Target::TakeDamage(int amount) {
    health -= amount;
    if (health <= 0) {
        health = 0;
        active = false;
    }
}

bool Target::FireAtTarget(const Vector2& targetPos) {
    if (!active || type != TargetType::Shooter) return false;

    // Calculate shooting direction
    Vector2 direction(targetPos.x - position.x, targetPos.y - position.y);
    if (direction.lengthSq() > 0.001f) {
        direction.normalize();

        // Create bullet with appropriate velocity
        float bulletSpeed = 2.5f; // Slightly faster for better challenge
        Vector2 bulletVelocity = direction * bulletSpeed;

        // Spawn bullet from triangle tip (front point)
        Vector2 spawnPos = position + direction * (radius * 1.5f);
        EnemyProjectile bullet(spawnPos, bulletVelocity);

        // Add to projectile list
        projectiles.push_back(bullet);
        return true;
    }

    return false;
}

void Target::UpdateProjectiles(BSplineTrack* track) {
    // Update each projectile
    for (auto& proj : projectiles) {
        if (proj.active) {
            proj.Update();

            // Check wall collision if track is provided
            if (track) {
                proj.CheckCollisionWithTrack(track);
            }
        }
    }

    // Remove inactive projectiles
    projectiles.erase(
        std::remove_if(
            projectiles.begin(),
            projectiles.end(),
            [](const EnemyProjectile& p) { return !p.active; }
        ),
        projectiles.end()
    );
}

bool EnemyProjectile::CheckCollisionWithTrack(BSplineTrack* track) {
    if (!active || !track) return false;

    // Get closest points on both track boundaries
    ClosestPointInfo cpiLeft = track->findClosestPointOnCurve(position, CurveSide::Left);
    ClosestPointInfo cpiRight = track->findClosestPointOnCurve(position, CurveSide::Right);

    // Check collision with left boundary
    if (cpiLeft.isValid) {
        Vector2 vec_proj_to_cl_point = position - cpiLeft.point;
        float projection = vec_proj_to_cl_point.x * cpiLeft.normal.x + vec_proj_to_cl_point.y * cpiLeft.normal.y;

        // If projection is positive, projectile is outside the left boundary
        if (projection > 0.0f && projection < radius) {
            active = false;
            return true;
        }
    }

    // Check collision with right boundary
    if (cpiRight.isValid) {
        Vector2 vec_proj_to_cr_point = position - cpiRight.point;
        float projection = vec_proj_to_cr_point.x * cpiRight.normal.x + vec_proj_to_cr_point.y * cpiRight.normal.y;

        // If projection is negative, projectile is outside the right boundary
        if (projection < 0.0f && std::abs(projection) < radius) {
            active = false;
            return true;
        }
    }

    return false;
}
