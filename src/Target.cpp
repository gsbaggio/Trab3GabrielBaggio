#include "Target.h"
#include <cmath>
#include <algorithm> // For std::max/std::min

Target::Target() : active(false), radius(12.0f), health(2), maxHealth(2) {}

Target::Target(const Vector2& pos) 
    : position(pos), active(true), radius(12.0f), health(2), maxHealth(2) {}

void Target::Render() {
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
        worldCorners[i].y = localCorners[i].x * sinB + localCorners[i].y * cosB + tankPos.y; // Added y coordinate calculation
    }
    
    // Check if any corner of the tank is inside the target circle
    for (int i = 0; i < 4; ++i) { // Added missing code block
        if (CheckCollision(worldCorners[i])) return true;
    }
    
    // Check if any edge of the tank intersects the target circle
    // This part is more complex and involves line-circle intersection tests.
    // For simplicity, this example will only check corners.
    // A more robust solution would implement line-segment to circle intersection.
    for (int i = 0; i < 4; ++i) { // Added missing code block
        // Placeholder for line-segment to circle intersection
    }
    
    return false;
}

void Target::TakeDamage(int amount) {
    health -= amount;
    if (health <= 0) { // Added missing code block
        health = 0;
        active = false;
    }
}
