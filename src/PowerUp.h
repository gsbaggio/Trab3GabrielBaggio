#ifndef __POWER_UP_H__
#define __POWER_UP_H__

#include "Vector2.h"
#include "gl_canvas2d.h"
#include <cmath>
#include <vector> 

// Forward declarations
class BSplineTrack;
class Tanque;
class Target;

// Types of power-ups
enum class PowerUpType {
    None = 0,
    Health = 1,     // Green '+' - Heals 50% health
    Shield = 2,     // Blue triangle - Blocks next damage
    Laser = 3       // Blue 'X' - Destroys enemies in line
};

class PowerUp {
public:
    Vector2 position;
    bool active;
    PowerUpType type;
    float radius;
    float animationAngle;
    
    // Laser effect properties
    static bool laserActive;
    static int laserDuration;
    static Vector2 laserStart;
    static Vector2 laserEnd;
    static const int LASER_MAX_DURATION = 45; // Laser lasts for 45 frames (3/4 second at 60fps)
    
    PowerUp();
    PowerUp(const Vector2& pos, PowerUpType powerUpType);
    
    void Update();
    void Render();
    
    // Check if tank collects this power-up
    bool CheckCollection(const Vector2& tankPos, float tankRadius);
    
    // Get name of power-up type for UI
    static const char* GetTypeName(PowerUpType type);
    
    // Apply power-up effects
    static void ApplyHealthEffect(Tanque* tank);
    static bool ApplyShieldEffect(Tanque* tank); // Returns if shield was applied
    static int ApplyLaserEffect(Tanque* tank, std::vector<Target>& targets); // Fix: Match signature with implementation
    
    // Update and render the laser effect
    static void UpdateLaserEffect();
    static void RenderLaserEffect();
};

#endif
