/**
 * ExplosionManager.h
 * Sistema de partículas para gerenciar efeitos visuais de explosão.
 * Controla a criação, atualização e renderização de partículas
 * para criar efeitos realistas de explosão após colisões.
 */

#ifndef __EXPLOSION_MANAGER_H__
#define __EXPLOSION_MANAGER_H__

#include "Vector2.h"
#include "gl_canvas2d.h"
#include <vector>
#include <cstdlib>
#include <cmath>

// uma única partícula em uma explosão
struct ExplosionParticle {
    Vector2 position;
    Vector2 velocity;
    float size;
    float lifetime;
    float maxLifetime;
    float r, g, b;  // valores de cor RGB
    bool active;
    
    ExplosionParticle() : position(), velocity(), size(0), 
                          lifetime(0), maxLifetime(0), 
                          r(0), g(0), b(0), active(false) {}
    
    void Update() {
        if (!active) return;
        
        position = position + velocity;
        lifetime--;
        
        // desacelera gradualmente
        velocity = velocity * 0.95f;
        
        // desativa quando o tempo de vida acabar
        if (lifetime <= 0) {
            active = false;
        }
    }
    
    void Render() {
        if (!active) return;
        
        // calcula alpha (desaparece conforme o tempo de vida diminui)
        float alpha = lifetime / maxLifetime;
        
        // renderiza a partícula com cor e alpha
        CV::color(r, g*alpha, b*alpha, alpha);
        CV::circleFill(position.x, position.y, size*alpha, 10);
        
        // adiciona efeito de brilho
        CV::color(r, g, b, alpha * 0.5f);
        CV::circleFill(position.x, position.y, size*1.5f*alpha, 8);
    }
};

// gerencia múltiplas explosões
class ExplosionManager {
private:
    std::vector<ExplosionParticle> particles;
    
public:
    ExplosionManager() {}
    
    void CreateExplosion(const Vector2& position, const Vector2& direction, int particleCount = 20) {
        // calcula direção normalizada e vetor perpendicular
        Vector2 normalizedDir = direction.normalized();
        Vector2 perpDir(-normalizedDir.y, normalizedDir.x);
        
        // reutiliza partículas inativas ou adiciona novas se necessário
        for (int i = 0; i < particleCount; i++) {
            ExplosionParticle particle;
            
            // encontra partícula inativa ou adiciona uma nova
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
            
            // configura propriedades da partícula
            particle.position = position;
            
            // calcula velocidade aleatória baseada na direção
            float speed = 2.0f + (rand() % 300) / 100.0f;  // 2.0 a 5.0
            
            // espalha partículas em forma de cone na direção do movimento
            float angle = ((rand() % 60) - 30) * M_PI / 180.0f;  // -30 a 30 graus
            float cos_ang = cos(angle);
            float sin_ang = sin(angle);
            Vector2 spreadDir;
            spreadDir.x = normalizedDir.x * cos_ang - normalizedDir.y * sin_ang;
            spreadDir.y = normalizedDir.x * sin_ang + normalizedDir.y * cos_ang;
            
            // adiciona alguma aleatoriedade à dispersão
            float sidewaysSpeed = ((rand() % 200) - 100) / 100.0f;  // -1.0 a 1.0
            particle.velocity = spreadDir * speed + perpDir * sidewaysSpeed;
            
            // tamanho aleatório (3-8 pixels)
            particle.size = 3.0f + (rand() % 50) / 10.0f;
            
            // define tempo de vida (20-40 frames)
            particle.maxLifetime = 20.0f + (rand() % 200) / 10.0f;
            particle.lifetime = particle.maxLifetime;
            
            // escolhe cor - amarelo, laranja ou vermelho
            int colorType = rand() % 3;
            switch (colorType) {
                case 0: // amarelo
                    particle.r = 1.0f;
                    particle.g = 0.9f + (rand() % 10) / 100.0f; // 0.9-1.0
                    particle.b = 0.0f;
                    break;
                case 1: // laranja
                    particle.r = 1.0f;
                    particle.g = 0.5f + (rand() % 30) / 100.0f; // 0.5-0.8
                    particle.b = 0.0f;
                    break;
                case 2: // vermelho
                    particle.r = 1.0f;
                    particle.g = 0.0f + (rand() % 30) / 100.0f; // 0.0-0.3
                    particle.b = 0.0f;
                    break;
            }
            
            // ativa partícula
            particle.active = true;
            
            // atualiza a partícula no vetor
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
