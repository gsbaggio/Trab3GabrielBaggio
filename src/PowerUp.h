/**
 * PowerUp.h
 * Define os diferentes tipos de power-ups que o jogador pode coletar.
 * Inclui funcionalidades para cura, escudo protetor e laser especial
 * que podem ser usados para auxiliar o jogador durante o jogo.
 */

#ifndef __POWER_UP_H__
#define __POWER_UP_H__

#include "Vector2.h"
#include "gl_canvas2d.h"
#include <cmath>
#include <vector> 

// declarações antecipadas
class BSplineTrack;
class Tanque;
class Target;

// tipos de power-ups
enum class PowerUpType {
    None = 0,
    Health = 1,     // "+" verde - cura 50% de vida
    Shield = 2,     // triângulo azul - bloqueia o próximo dano
    Laser = 3       // "X" azul - destrói inimigos em linha
};

class PowerUp {
public:
    Vector2 position;
    bool active;
    PowerUpType type;
    float radius;
    float animationAngle;
    
    // propriedades do efeito laser
    static bool laserActive;
    static int laserDuration;
    static Vector2 laserStart;
    static Vector2 laserEnd;
    static const int LASER_MAX_DURATION = 45; // laser dura 45 quadros (3/4 segundo a 60fps)
    
    PowerUp();
    PowerUp(const Vector2& pos, PowerUpType powerUpType);
    
    void Update();
    void Render();
    
    // verifica se o tanque coleta este power-up
    bool CheckCollection(const Vector2& tankPos, float tankRadius);
    
    // obtém nome do tipo de power-up para a UI
    static const char* GetTypeName(PowerUpType type);
    
    // aplica efeitos de power-up
    static void ApplyHealthEffect(Tanque* tank);
    static bool ApplyShieldEffect(Tanque* tank); // retorna se o escudo foi aplicado
    static int ApplyLaserEffect(Tanque* tank, std::vector<Target>& targets); // correção: corresponder assinatura com implementação
    
    // atualiza e renderiza o efeito do laser
    static void UpdateLaserEffect();
    static void RenderLaserEffect();
};

#endif
