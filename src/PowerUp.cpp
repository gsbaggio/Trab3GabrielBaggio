#include "PowerUp.h"
#include "Tanque.h"
#include "Target.h"
#include <cmath>

// Initialize static members
bool PowerUp::laserActive = false;
int PowerUp::laserDuration = 0;
Vector2 PowerUp::laserStart = Vector2(0, 0);
Vector2 PowerUp::laserEnd = Vector2(0, 0);

PowerUp::PowerUp() 
    : position(0, 0), active(false), type(PowerUpType::None), radius(15.0f), animationAngle(0.0f) {}

PowerUp::PowerUp(const Vector2& pos, PowerUpType powerUpType)
    : position(pos), active(true), type(powerUpType), radius(15.0f), animationAngle(0.0f) {}

void PowerUp::Update() {
    if (!active) return;
    
    // Animate rotation
    animationAngle += 0.02f;
    if (animationAngle > 2 * M_PI) {
        animationAngle -= 2 * M_PI;
    }
}

void PowerUp::Render() {
    if (!active) return;
    
    // Draw green circle around all power-ups
    CV::color(0.0f, 0.8f, 0.0f);
    CV::circle(position.x, position.y, radius * 1.2f, 20);
    
    // Draw power-up based on type
    switch (type) {
        case PowerUpType::Health: {
            // Draw green '+' for health
            CV::color(0.0f, 0.9f, 0.0f);
            float plusSize = radius * 0.8f;
            float thickness = radius * 0.3f;
            
            // Horizontal line
            CV::rectFill(position.x - plusSize, position.y - thickness/2, 
                         position.x + plusSize, position.y + thickness/2);
            
            // Vertical line
            CV::rectFill(position.x - thickness/2, position.y - plusSize,
                         position.x + thickness/2, position.y + plusSize);
            break;
        }
        case PowerUpType::Shield: {
            // Draw dark blue triangle shield
            CV::color(0.0f, 0.0f, 0.8f);
            float triangleSize = radius * 1.0f;
            float x1 = position.x;
            float y1 = position.y - triangleSize;
            float x2 = position.x - triangleSize * 0.866f;
            float y2 = position.y + triangleSize * 0.5f;
            float x3 = position.x + triangleSize * 0.866f;
            float y3 = position.y + triangleSize * 0.5f;
            
            // Rotate triangle for animation
            float cosA = cos(animationAngle);
            float sinA = sin(animationAngle);
            
            float rx1 = position.x + (x1 - position.x) * cosA - (y1 - position.y) * sinA;
            float ry1 = position.y + (x1 - position.x) * sinA + (y1 - position.y) * cosA;
            float rx2 = position.x + (x2 - position.x) * cosA - (y2 - position.y) * sinA;
            float ry2 = position.y + (x2 - position.x) * sinA + (y2 - position.y) * cosA;
            float rx3 = position.x + (x3 - position.x) * cosA - (y3 - position.y) * sinA;
            float ry3 = position.y + (x3 - position.x) * sinA + (y3 - position.y) * cosA;
            
            float vx[3] = {rx1, rx2, rx3};
            float vy[3] = {ry1, ry2, ry3};
            CV::polygonFill(vx, vy, 3);
            
            // Add a lighter blue outline
            CV::color(0.3f, 0.3f, 1.0f);
            CV::line(rx1, ry1, rx2, ry2);
            CV::line(rx2, ry2, rx3, ry3);
            CV::line(rx3, ry3, rx1, ry1);
            break;
        }
        case PowerUpType::Laser: {
            // Draw blue 'X' for laser
            CV::color(0.2f, 0.2f, 1.0f);
            float xSize = radius * 0.8f;
            float thickness = radius * 0.25f;
            
            // First diagonal
            float angle1 = M_PI/4 + animationAngle;
            float x1 = position.x + cos(angle1) * xSize;
            float y1 = position.y + sin(angle1) * xSize;
            float x2 = position.x + cos(angle1 + M_PI) * xSize;
            float y2 = position.y + sin(angle1 + M_PI) * xSize;
            
            // Second diagonal
            float angle2 = 3*M_PI/4 + animationAngle;
            float x3 = position.x + cos(angle2) * xSize;
            float y3 = position.y + sin(angle2) * xSize;
            float x4 = position.x + cos(angle2 + M_PI) * xSize;
            float y4 = position.y + sin(angle2 + M_PI) * xSize;
            
            // Fix: Remove thickness parameter from line calls
            CV::line(x1, y1, x2, y2);
            CV::line(x3, y3, x4, y4);
            break;
        }
        default:
            break;
    }
}

bool PowerUp::CheckCollection(const Vector2& tankPos, float tankRadius) {
    if (!active) return false;
    
    float dx = tankPos.x - position.x;
    float dy = tankPos.y - position.y;
    float distSq = dx*dx + dy*dy;
    float combinedRadius = radius + tankRadius;
    
    return distSq < combinedRadius * combinedRadius;
}

const char* PowerUp::GetTypeName(PowerUpType type) {
    switch (type) {
        case PowerUpType::Health: return "Health (+)";
        case PowerUpType::Shield: return "Shield (Triangle)";
        case PowerUpType::Laser: return "Laser Beam (X)";
        default: return "None";
    }
}

void PowerUp::ApplyHealthEffect(Tanque* tank) {
    if (!tank) return;
    
    // Heal 50% of max health
    int healAmount = tank->maxHealth / 2;
    tank->health = std::min(tank->health + healAmount, tank->maxHealth);
    
    // Visual effect or sound could be added here
    printf("PowerUp: Health restored! Current health: %d/%d\n", tank->health, tank->maxHealth);
}

bool PowerUp::ApplyShieldEffect(Tanque* tank) {
    if (!tank) return false;
    
    // Apply shield effect to tank
    tank->hasShield = true;
    printf("PowerUp: Shield activated! Next damage will be blocked\n");
    return true;
}

int PowerUp::ApplyLaserEffect(Tanque* tank, std::vector<Target>& targets) {
    if (!tank) return 0;
    
    // Calculate laser direction based on tank's turret angle
    Vector2 laserDir(cos(tank->topAngle), sin(tank->topAngle));
    Vector2 laserStart = tank->GetCannonTipPosition();
    int targetsDestroyed = 0;
    
    // Define laser range (very far)
    const float LASER_RANGE = 2000.0f; 
    Vector2 laserEnd = laserStart + laserDir * LASER_RANGE;
    
    // Set the static laser effect properties for rendering over multiple frames
    PowerUp::laserActive = true;
    PowerUp::laserDuration = LASER_MAX_DURATION;
    PowerUp::laserStart = laserStart;
    PowerUp::laserEnd = laserEnd;
    
    // Check each target for collision with the laser beam
    for (auto& target : targets) {
        if (!target.active) continue;
        
        // Simple circle-line intersection test
        Vector2 targetToLaserStart = laserStart - target.position;
        
        // Calculate projection of targetToLaserStart onto laserDir
        float t = -(targetToLaserStart.x * laserDir.x + targetToLaserStart.y * laserDir.y);
        
        // If projection is negative, laser starts after the target
        if (t < 0) continue;
        
        // If projection is greater than LASER_RANGE, target is beyond laser range
        if (t > LASER_RANGE) continue;
        
        // Calculate closest point on laser line to target center
        Vector2 closestPoint = laserStart + laserDir * t;
        
        // Check if closest point is within target radius
        if ((closestPoint - target.position).lengthSq() <= target.radius * target.radius) {
            // Target is hit by laser - instant kill
            target.active = false;
            targetsDestroyed++;
            
            // Create an explosion at target position when hit by laser
            if (tank) {  // Fixed: Just check if tank is valid
                Vector2 explosionDir = (target.position - laserStart).normalized();
                tank->explosions.CreateExplosion(target.position, explosionDir, 30);
            }
        }
    }
    
    printf("PowerUp: Laser fired! Destroyed %d targets\n", targetsDestroyed);
    return targetsDestroyed;
}

void PowerUp::UpdateLaserEffect() {
    // Decrement the duration if the laser is active
    if (laserActive) {
        laserDuration--;
        if (laserDuration <= 0) {
            laserActive = false;
        }
    }
}

void PowerUp::RenderLaserEffect() {
    if (!laserActive) return;
    
    // Calculate alpha for fade-out effect
    float alpha = static_cast<float>(laserDuration) / LASER_MAX_DURATION;
    
    // Draw the laser beam with increasing thickness for better visibility
    float thickness = 6.0f * alpha; // Thicker at start, thinner as it fades
    
    // Draw the main laser beam
    CV::color(0.2f, 0.4f, 1.0f, alpha);
    CV::line(laserStart.x, laserStart.y, laserEnd.x, laserEnd.y);
    
    // Draw additional parallel beams for thickness
    Vector2 dir = (laserEnd - laserStart).normalized();
    Vector2 perpDir(-dir.y, dir.x); // Perpendicular direction
    
    for (float offset = 1.0f; offset <= thickness; offset += 1.0f) {
        // Draw parallel lines on both sides
        Vector2 offset1 = perpDir * offset;
        Vector2 offset2 = perpDir * -offset;
        
        float alphaLine = alpha * (1.0f - offset/thickness); // Fade out at edges
        
        CV::color(0.3f, 0.5f, 1.0f, alphaLine * 0.7f);
        CV::line(laserStart.x + offset1.x, laserStart.y + offset1.y, 
                 laserEnd.x + offset1.x, laserEnd.y + offset1.y);
        CV::line(laserStart.x + offset2.x, laserStart.y + offset2.y, 
                 laserEnd.x + offset2.x, laserEnd.y + offset2.y);
    }
    
    // Add a glowing effect at the start point
    CV::color(0.4f, 0.6f, 1.0f, alpha * 0.8f);
    CV::circleFill(laserStart.x, laserStart.y, 10.0f * alpha, 16);
}
