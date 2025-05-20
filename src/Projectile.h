/**
 * Projectile.h
 * Define a classe para os projéteis disparados pelo tanque.
 * Gerencia o movimento, renderização e detecção de colisão dos projéteis
 * com a pista e com os alvos.
 */

#ifndef __PROJECTILE_H__
#define __PROJECTILE_H__

#include "Vector2.h"
#include "gl_canvas2d.h"
#include "BSplineTrack.h"
#include <cmath>

// declaração antecipada
class ExplosionManager;

class Projectile {
public:
    Vector2 position;
    Vector2 previousPosition; // armazena posição anterior para detecção contínua de colisão
    Vector2 velocity;
    bool active;
    int lifetime; // opcional: limita a vida útil do projétil
    float collisionRadius; // valor explícito do raio de colisão
    
    Projectile();

    Projectile(const Vector2& pos, const Vector2& vel, float radius = 4.0f);
    
    void Update();
    
    void Render();
    
    // atualizado para aceitar ExplosionManager para criar explosões
    bool CheckCollisionWithTrack(BSplineTrack* track, ExplosionManager* explosions = nullptr);
    
    // auxiliar para criar explosão
    void CreateExplosionOnCollision(ExplosionManager* explosions);
};

#endif
