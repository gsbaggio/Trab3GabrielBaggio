/**
 * Target.h
 * Define as classes relacionadas aos alvos/inimigos do jogo.
 * Inclui diferentes tipos de alvos (básico, atirador e estrela) 
 * e implementa projéteis inimigos com seus comportamentos específicos.
 */

#ifndef __TARGET_H__
#define __TARGET_H__

#include "Vector2.h"
#include "gl_canvas2d.h"
#include <cmath>
#include <algorithm> 
#include <vector>   

// declaração antecipada para detecção de colisão
class BSplineTrack;

// define uma classe para projéteis inimigos
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
        CV::color(1.0f, 0.5f, 0.0f); // laranja brilhante para melhor visibilidade
        CV::circleFill(position.x, position.y, radius, 8);
        CV::color(1.0f, 0.2f, 0.0f); // contorno vermelho
        CV::circle(position.x, position.y, radius, 8);
    }
    
    bool CheckCollisionWithTrack(BSplineTrack* track);
};

// define tipos de alvos
enum class TargetType {
    Basic = 0,    // alvo circular estacionário original
    Shooter = 1,  // atirador triangular que dispara contra o tanque
    Star = 2      // estrela que persegue o tanque quando está ao alcance
};

class Target {
public:
    Vector2 position;
    bool active;
    float radius;
    int health;        // pontos de vida
    int maxHealth;     // vida máxima
    TargetType type;   // tipo de alvo
    
    // propriedades específicas do atirador
    float aimAngle;                // ângulo em que o atirador está mirando
    float shootingRadius;          // alcance dentro do qual o atirador dispara
    int firingCooldown;            // temporizador de recarga atual
    int firingCooldownReset;       // tempo entre disparos
    std::vector<EnemyProjectile> projectiles; // projéteis inimigos
    
    // propriedades específicas da estrela
    float detectionRadius;    // alcance dentro do qual a estrela começa a perseguir
    float moveSpeed;          // velocidade de movimento da estrela
    bool isChasing;           // se a estrela está perseguindo o tanque
    float rotationAngle;      // ângulo de rotação atual da estrela
    float rotationSpeed;      // velocidade de rotação da estrela
    
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
