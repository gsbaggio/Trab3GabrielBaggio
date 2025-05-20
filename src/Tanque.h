/**
 * Tanque.h
 * Define a classe principal do jogador (tanque).
 * Gerencia movimento, rotação, disparo de projéteis, 
 * detecção de colisão e sistemas de vida/escudo.
 */

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
    float baseAngle; // radianos, 0 é ao longo do eixo X positivo
    float topAngle;  // radianos, 0 é ao longo do eixo X positivo, relativo ao mundo
    float speed;
    float rotationRate; // radianos por quadro para rotação da base
    float turretRotationSpeed; // radianos por quadro para rotação da torre

    Vector2 forwardVector; // direção para onde a base está apontando

    // dimensões
    float baseWidth;
    float baseHeight;
    float turretRadius;
    float cannonLength;
    float cannonWidth;

    // membros relacionados à colisão
    Vector2 lastSafePosition;
    bool isColliding;
    int collisionTimer; // contagem regressiva de quadros para recuo

    // constantes de colisão
    static const int COLLISION_REBOUND_FRAMES = 90; 
    static constexpr float REBOUND_SPEED_FACTOR = 0.3f; 

    // membros relacionados a projéteis
    int firingCooldown;
    int firingCooldownReset;
    float projectileSpeed;
    std::vector<Projectile> projectiles;

    // membros relacionados à saúde
    int health;
    int maxHealth;
    bool isInvulnerable;
    int invulnerabilityTimer;
    static const int INVULNERABILITY_FRAMES = 60; 
    bool isShieldInvulnerable; 

    // adiciona propriedade de escudo
    bool hasShield;

    // adiciona gerenciador de explosão
    ExplosionManager explosions;

    Tanque(float x, float y, float initialSpeed = 1.0f, float initialRotationRate = 0.03f);

    void Update(float mouseX, float mouseY, bool rotateLeft, bool rotateRight, BSplineTrack* track);
    void Render();
    
    // métodos relacionados a projéteis
    bool FireProjectile();
    Vector2 GetCannonTipPosition() const;
    void UpdateProjectiles(BSplineTrack* track);

    // verifica se um projétil atinge algum alvo e retorna o índice do alvo atingido ou -1
    int CheckProjectileTargetCollision(const Projectile& proj, std::vector<Target>& targets);
    
    // verifica todos os projéteis contra todos os alvos
    bool CheckAllProjectilesAgainstTargets(std::vector<Target>& targets, int& hitTargetIndex, int& hitProjectileIndex);

    // altera o tipo de retorno de void para int
    int CheckTargetCollisions(std::vector<Target>& targets);

private:
    void CheckCollisionAndRespond(BSplineTrack* track);
};

#endif
