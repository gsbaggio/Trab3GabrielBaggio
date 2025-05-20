#ifndef __EXPLOSION_MANAGER_H__
#define __EXPLOSION_MANAGER_H__

#include "Vector2.h"
#include "gl_canvas2d.h"
#include <vector>
#include <cstdlib>
#include <cmath>

// A single particle in an explosion
struct ExplosionParticle {
    Vector2 position;
    Vector2 velocity;
    float size;
    float lifetime;
    float maxLifetime;
    float r, g, b;  // RGB color values
    bool active;
    
    ExplosionParticle() : position(), velocity(), size(0), 
                          lifetime(0), maxLifetime(0), 
                          r(0), g(0), b(0), active(false) {}
    
    void Update() {
        if (!active) return;
        
        position = position + velocity;
        lifetime--;
        
        // Gradually slow down
        velocity = velocity * 0.95f;
        
        // Deactivate when lifetime is up
        if (lifetime <= 0) {
            active = false;
        }
    }
    
    void Render() {
        if (!active) return;
        
        // Calculate alpha (fade out as lifetime decreases)
        float alpha = lifetime / maxLifetime;
        
        // Render particle with color and alpha
        CV::color(r, g*alpha, b*alpha, alpha);
        CV::circleFill(position.x, position.y, size*alpha, 10);
        
        // Add glow effect
        CV::color(r, g, b, alpha * 0.5f);
        CV::circleFill(position.x, position.y, size*1.5f*alpha, 8);
    }
};

// Manages multiple explosions
class ExplosionManager {
private:
    std::vector<ExplosionParticle> particles;
    
public:
    ExplosionManager() {}
    
    void CreateExplosion(const Vector2& position, const Vector2& direction, int particleCount = 20) {
        // Calculate normalized direction and perpendicular vector
        Vector2 normalizedDir = direction.normalized();
        Vector2 perpDir(-normalizedDir.y, normalizedDir.x);
        
        // Reuse inactive particles or add new ones if needed
        for (int i = 0; i < particleCount; i++) {
            ExplosionParticle particle;
            
            // Find inactive particle or add new one
            bool foundInactive = false;
            for (size_t j = 0; j < particles.size(); j++) {
                if (!particles[j].active) {
                    particle = particles[j];
                    foundInactive = true;
                    break;
                }
            }
            
            if (!foundInactive) {
                particles.push_back(particle);
                particle = particles.back();
            }
            
            // Set particle properties
            particle.position = position;
            
            // Calculate random velocity based on direction
            float speed = 2.0f + (rand() % 300) / 100.0f;  // 2.0 to 5.0
            
            // Spread particles in a cone shape in the direction of travel
            float angle = ((rand() % 60) - 30) * M_PI / 180.0f;  // -30 to 30 degrees
            float cos_ang = cos(angle);
            float sin_ang = sin(angle);
            Vector2 spreadDir;
            spreadDir.x = normalizedDir.x * cos_ang - normalizedDir.y * sin_ang;
            spreadDir.y = normalizedDir.x * sin_ang + normalizedDir.y * cos_ang;
            
            // Add some randomness to the spread
            float sidewaysSpeed = ((rand() % 200) - 100) / 100.0f;  // -1.0 to 1.0
            particle.velocity = spreadDir * speed + perpDir * sidewaysSpeed;
            
            // Randomize size (3-8 pixels)
            particle.size = 3.0f + (rand() % 50) / 10.0f;
            
            // Set lifetime (20-40 frames)
            particle.maxLifetime = 20.0f + (rand() % 200) / 10.0f;
            particle.lifetime = particle.maxLifetime;
            
            // Choose color - yellow, orange, or red
            int colorType = rand() % 3;
            switch (colorType) {
                case 0: // Yellow
                    particle.r = 1.0f;
                    particle.g = 0.9f + (rand() % 10) / 100.0f; // 0.9-1.0
                    particle.b = 0.0f;
                    break;
                case 1: // Orange
                    particle.r = 1.0f;
                    particle.g = 0.5f + (rand() % 30) / 100.0f; // 0.5-0.8
                    particle.b = 0.0f;
                    break;
                case 2: // Red
                    particle.r = 1.0f;
                    particle.g = 0.0f + (rand() % 30) / 100.0f; // 0.0-0.3
                    particle.b = 0.0f;
                    break;
            }
            
            // Activate particle
            particle.active = true;
            
            // Update the particle in the vector
            if (foundInactive) {
                for (size_t j = 0; j < particles.size(); j++) {
                    if (!particles[j].active) {
                        particles[j] = particle;
                        break;
                    }
                }
            } else {
                particles[particles.size() - 1] = particle;
            }
        }
    }
    
    void Update() {
        for (auto& particle : particles) {
            particle.Update();
        }
    }
    
    void Render() {
        for (auto& particle : particles) {
            particle.Render();
        }
    }
    
    void Clear() {
        particles.clear();
    }
};

#endif 
