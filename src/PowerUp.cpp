#include "PowerUp.h"
#include "Tanque.h"
#include "Target.h"
#include <cmath>

// inicializa membros estáticos
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
    
    // anima rotação
    animationAngle += 0.02f;
    if (animationAngle > 2 * M_PI) {
        animationAngle -= 2 * M_PI;
    }
}

void PowerUp::Render() {
    if (!active) return;
    
    // desenha círculo verde ao redor de todos os power-ups
    CV::color(0.0f, 0.8f, 0.0f);
    CV::circle(position.x, position.y, radius * 1.2f, 20);
    
    // desenha power-up baseado no tipo
    switch (type) {
        case PowerUpType::Health: {
            // desenha o '+' verde para saúde
            CV::color(0.0f, 0.9f, 0.0f);
            float plusSize = radius * 0.8f;
            float thickness = radius * 0.3f;
            
            // linha horizontal
            CV::rectFill(position.x - plusSize, position.y - thickness/2, 
                         position.x + plusSize, position.y + thickness/2);
            
            // linha vertical
            CV::rectFill(position.x - thickness/2, position.y - plusSize,
                         position.x + thickness/2, position.y + plusSize);
            break;
        }
        case PowerUpType::Shield: {
            // desenha escudo triangular azul escuro
            CV::color(0.0f, 0.0f, 0.8f);
            float triangleSize = radius * 1.0f;
            float x1 = position.x;
            float y1 = position.y - triangleSize;
            float x2 = position.x - triangleSize * 0.866f;
            float y2 = position.y + triangleSize * 0.5f;
            float x3 = position.x + triangleSize * 0.866f;
            float y3 = position.y + triangleSize * 0.5f;
            
            // rotaciona triângulo para animação
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
            
            // adiciona um contorno azul mais claro
            CV::color(0.3f, 0.3f, 1.0f);
            CV::line(rx1, ry1, rx2, ry2);
            CV::line(rx2, ry2, rx3, ry3);
            CV::line(rx3, ry3, rx1, ry1);
            break;
        }
        case PowerUpType::Laser: {
            // desenha 'X' azul para o laser
            CV::color(0.2f, 0.2f, 1.0f);
            float xSize = radius * 0.8f;
            float thickness = radius * 0.25f;
            
            // primeira diagonal
            float angle1 = M_PI/4 + animationAngle;
            float x1 = position.x + cos(angle1) * xSize;
            float y1 = position.y + sin(angle1) * xSize;
            float x2 = position.x + cos(angle1 + M_PI) * xSize;
            float y2 = position.y + sin(angle1 + M_PI) * xSize;
            
            // segunda diagonal
            float angle2 = 3*M_PI/4 + animationAngle;
            float x3 = position.x + cos(angle2) * xSize;
            float y3 = position.y + sin(angle2) * xSize;
            float x4 = position.x + cos(angle2 + M_PI) * xSize;
            float y4 = position.y + sin(angle2 + M_PI) * xSize;
            
            // correção: remove parâmetro de espessura das chamadas de linha
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
        case PowerUpType::Health: return "Recuperar Vida";
        case PowerUpType::Shield: return "Escudo";
        case PowerUpType::Laser: return "Raio Laser";
        default: return "Nenhum";
    }
}

void PowerUp::ApplyHealthEffect(Tanque* tank) {
    if (!tank) return;
    
    // recupera 50% da saúde máxima
    int healAmount = tank->maxHealth / 2;
    tank->health = std::min(tank->health + healAmount, tank->maxHealth);
}

bool PowerUp::ApplyShieldEffect(Tanque* tank) {
    if (!tank) return false;
    
    // aplica efeito de escudo ao tanque
    tank->hasShield = true;
    return true;
}

int PowerUp::ApplyLaserEffect(Tanque* tank, std::vector<Target>& targets) {
    if (!tank) return 0;
    
    // calcula a direção do laser baseado no ângulo da torre do tanque
    Vector2 laserDir(cos(tank->topAngle), sin(tank->topAngle));
    Vector2 laserStart = tank->GetCannonTipPosition();
    int targetsDestroyed = 0;
    
    // define o alcance do laser (muito longe)
    const float LASER_RANGE = 2000.0f; 
    Vector2 laserEnd = laserStart + laserDir * LASER_RANGE;
    
    // configura as propriedades do efeito de laser estático para renderização em múltiplos frames
    PowerUp::laserActive = true;
    PowerUp::laserDuration = LASER_MAX_DURATION;
    PowerUp::laserStart = laserStart;
    PowerUp::laserEnd = laserEnd;
    
    // verifica cada alvo para colisão com o feixe de laser
    for (auto& target : targets) {
        if (!target.active) continue;
        
        // teste simples de intersecção círculo-linha
        Vector2 targetToLaserStart = laserStart - target.position;
        
        // calcula projeção de targetToLaserStart em laserDir
        float t = -(targetToLaserStart.x * laserDir.x + targetToLaserStart.y * laserDir.y);
        
        // se a projeção for negativa, o laser começa após o alvo
        if (t < 0) continue;
        
        // se a projeção for maior que LASER_RANGE, o alvo está além do alcance do laser
        if (t > LASER_RANGE) continue;
        
        // calcula o ponto mais próximo na linha do laser ao centro do alvo
        Vector2 closestPoint = laserStart + laserDir * t;
        
        // verifica se o ponto mais próximo está dentro do raio do alvo
        if ((closestPoint - target.position).lengthSq() <= target.radius * target.radius) {
            // alvo é atingido pelo laser - morte instantânea
            target.active = false;
            targetsDestroyed++;
            
            // cria uma explosão na posição do alvo quando atingido pelo laser
            if (tank) {  // corrigido: apenas verifica se o tanque é válido
                Vector2 explosionDir = (target.position - laserStart).normalized();
                tank->explosions.CreateExplosion(target.position, explosionDir, 30);
            }
        }
    }
    
    return targetsDestroyed;
}

void PowerUp::UpdateLaserEffect() {
    // decrementa a duração se o laser estiver ativo
    if (laserActive) {
        laserDuration--;
        if (laserDuration <= 0) {
            laserActive = false;
        }
    }
}

void PowerUp::RenderLaserEffect() {
    if (!laserActive) return;
    
    // calcula alpha para efeito de fade-out
    float alpha = static_cast<float>(laserDuration) / LASER_MAX_DURATION;
    
    // desenha o feixe de laser com espessura crescente para melhor visibilidade
    float thickness = 6.0f * alpha; // mais grosso no início, mais fino conforme desaparece
    
    // desenha o feixe de laser principal
    CV::color(0.2f, 0.4f, 1.0f, alpha);
    CV::line(laserStart.x, laserStart.y, laserEnd.x, laserEnd.y);
    
    // desenha feixes paralelos adicionais para espessura
    Vector2 dir = (laserEnd - laserStart).normalized();
    Vector2 perpDir(-dir.y, dir.x); // direção perpendicular
    
    for (float offset = 1.0f; offset <= thickness; offset += 1.0f) {
        // desenha linhas paralelas em ambos os lados
        Vector2 offset1 = perpDir * offset;
        Vector2 offset2 = perpDir * -offset;
        
        float alphaLine = alpha * (1.0f - offset/thickness); // desaparece nas bordas
        
        CV::color(0.3f, 0.5f, 1.0f, alphaLine * 0.7f);
        CV::line(laserStart.x + offset1.x, laserStart.y + offset1.y, 
                 laserEnd.x + offset1.x, laserEnd.y + offset1.y);
        CV::line(laserStart.x + offset2.x, laserStart.y + offset2.y, 
                 laserEnd.x + offset2.x, laserEnd.y + offset2.y);
    }
    
    // adiciona um efeito brilhante no ponto inicial
    CV::color(0.4f, 0.6f, 1.0f, alpha * 0.8f);
    CV::circleFill(laserStart.x, laserStart.y, 10.0f * alpha, 16);
}
