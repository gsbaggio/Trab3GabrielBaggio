/**
 * Projectile.cpp
 * Implementa a funcionalidade dos projéteis disparados pelo tanque.
 * Controla o movimento, detecção de colisão com a pista e criação
 * de efeitos de explosão quando ocorrem colisões.
 */

#include "Projectile.h"
#include "ExplosionManager.h"
#include <cmath>

Projectile::Projectile()
    : position(0, 0), previousPosition(0, 0), velocity(0, 0), 
      active(false), lifetime(300), collisionRadius(12.0f) { // aumentado de 8.0f para 12.0f
}

Projectile::Projectile(const Vector2& pos, const Vector2& vel, float radius) // aumentado padrão de 4.0f para 8.0f
    : position(pos), previousPosition(pos), velocity(vel), 
      active(true), lifetime(300), collisionRadius(radius) {
}

void Projectile::Update() {
    if (!active) return;
    
    previousPosition = position; // armazena posição atual antes de atualizar
    position = position + velocity;
    
    if (lifetime > 0) {
        lifetime--;
        if (lifetime <= 0) active = false; // adicionado active = false;
    }
}

void Projectile::Render() {
    if (!active) return;
    CV::color(1.0f, 0.7f, 0.0f); // laranja-amarelo para projéteis
    CV::circleFill(position.x, position.y, 8.0f, 10); // aumentado de 4.0f para 8.0f para corresponder à colisão maior
}

bool Projectile::CheckCollisionWithTrack(BSplineTrack* track, ExplosionManager* explosions) {
    if (!active || !track) return false;
    
    // obtém pontos mais próximos em ambos os limites da pista para a posição atual
    ClosestPointInfo cpiLeftCurrent = track->findClosestPointOnCurve(position, CurveSide::Left);
    ClosestPointInfo cpiRightCurrent = track->findClosestPointOnCurve(position, CurveSide::Right);
    
    // verifica colisão com posição atual e limites
    if (cpiLeftCurrent.isValid) {
        Vector2 vec_proj_to_cl_point = position - cpiLeftCurrent.point;
        float projection = vec_proj_to_cl_point.x * cpiLeftCurrent.normal.x + vec_proj_to_cl_point.y * cpiLeftCurrent.normal.y;
            
        // se a projeção for positiva, o projétil está fora do limite esquerdo.
        // se a projeção for menor que o raio de colisão, está colidindo com o limite
        if (projection > 0.0f && projection < collisionRadius) {
            active = false;
            // cria explosão no ponto de colisão
            if (explosions) {
                CreateExplosionOnCollision(explosions);
            }
            return true;
        }
    }
    
    if (cpiRightCurrent.isValid) { // usou cpiRightCurrent
        Vector2 vec_proj_to_cr_point = position - cpiRightCurrent.point; // usou cpiRightCurrent
        float projection = vec_proj_to_cr_point.x * cpiRightCurrent.normal.x + vec_proj_to_cr_point.y * cpiRightCurrent.normal.y; // usou cpiRightCurrent
            
        // se a projeção for negativa, o projétil está fora do limite direito.
        // se o valor absoluto da projeção for menor que o raio de colisão, está colidindo
        if (projection < 0.0f && std::abs(projection) < collisionRadius) { // adicionado bloco de código ausente e std::abs
            active = false;
            // cria explosão no ponto de colisão
            if (explosions) {
                CreateExplosionOnCollision(explosions);
            }
            return true;
        }
    }
    
    // se movendo rápido, também verifica "túnel" através dos limites amostrando pontos ao longo do caminho de movimento
    float movementLength = (position - previousPosition).length();
    if (movementLength > collisionRadius) { // removido * 1.5f
        const int numSamples = 5; // amostra alguns pontos ao longo do caminho de movimento
        
        for (int i = 1; i < numSamples; i++) {
            float t = static_cast<float>(i) / numSamples;
            Vector2 samplePos = previousPosition + (position - previousPosition) * t;
            
            // verifica ponto de amostra contra ambos os limites
            ClosestPointInfo cpiLeftSample = track->findClosestPointOnCurve(samplePos, CurveSide::Left);
            if (cpiLeftSample.isValid && cpiLeftSample.distance < collisionRadius) {
                active = false;
                return true;
            }
            
            ClosestPointInfo cpiRightSample = track->findClosestPointOnCurve(samplePos, CurveSide::Right);
            if (cpiRightSample.isValid && cpiRightSample.distance < collisionRadius) {
                active = false;
                return true;
            }
        }
    }
    
    return false;
}

void Projectile::CreateExplosionOnCollision(ExplosionManager* explosions) {
    if (!explosions) return;
    
    // cria explosão na posição atual usando velocidade como direção
    explosions->CreateExplosion(position, velocity, 30); // 30 partículas para efeito rico
}
