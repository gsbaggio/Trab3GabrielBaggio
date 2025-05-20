#include "Target.h"
#include "BSplineTrack.h"
#include <cmath>
#include <algorithm> 

Target::Target()
    : active(false), radius(12.0f), health(2), maxHealth(2), type(TargetType::Basic),
      aimAngle(0.0f), shootingRadius(250.0f), firingCooldown(0), firingCooldownReset(120),
      detectionRadius(200.0f), moveSpeed(0.8f), isChasing(false), rotationAngle(0.0f), rotationSpeed(0.05f) {}

Target::Target(const Vector2& pos, TargetType targetType)
    : position(pos), active(true), radius(12.0f), health(2), maxHealth(2), type(targetType),
      aimAngle(0.0f), shootingRadius(200.0f), firingCooldown(0), firingCooldownReset(90),
      detectionRadius(200.0f), moveSpeed(0.8f), isChasing(false), rotationAngle(0.0f), rotationSpeed(0.05f) {}

void Target::Update(const Vector2& tankPosition, BSplineTrack* track) {
    if (!active) return;
    
    // atualiza comportamento baseado no tipo de alvo
    if (type == TargetType::Shooter) {
        // calcula ângulo de mira para o tanque
        float dx = tankPosition.x - position.x;
        float dy = tankPosition.y - position.y;
        aimAngle = atan2(dy, dx);

        // calcula distância até o tanque
        float distSq = Vector2(dx, dy).lengthSq();

        // diminui tempo de recarga
        if (firingCooldown > 0) {
            firingCooldown--;
        }

        // dispara no tanque se estiver no alcance e recarga concluída
        if (distSq <= shootingRadius * shootingRadius && firingCooldown <= 0) {
            if (FireAtTarget(tankPosition)) {
                firingCooldown = firingCooldownReset;
            }
        }

        // atualiza projéteis existentes
        UpdateProjectiles(track);
    }
    else if (type == TargetType::Star) {
        // sempre atualiza o ângulo de rotação para efeito giratório
        rotationAngle += rotationSpeed;
        if (rotationAngle > 2 * M_PI) {
            rotationAngle -= 2 * M_PI;
        }
        
        // calcula distância até o tanque
        float dx = tankPosition.x - position.x;
        float dy = tankPosition.y - position.y;
        float distSq = dx*dx + dy*dy;
        
        // se estiver dentro do raio de detecção, começa a perseguir
        if (distSq <= detectionRadius * detectionRadius) {
            isChasing = true;
            
            // calcula direção para o tanque
            float dist = sqrt(distSq);
            
            // só move se não estiver bem em cima do tanque
            if (dist > 0.1f) {
                // normaliza e escala pela velocidade
                float dirX = dx / dist;
                float dirY = dy / dist;
                
                // move em direção ao tanque (mais devagar que antes)
                position.x += dirX * moveSpeed;
                position.y += dirY * moveSpeed;
            }
        }
    }
}

void Target::Render() {
    if (!active) return;
    
    if (type == TargetType::Basic) {
        RenderBasicTarget();
    } else if (type == TargetType::Shooter) {
        RenderShooterTarget();
        
        // renderiza projéteis
        for (auto& proj : projectiles) {
            proj.Render();
        }
    } else if (type == TargetType::Star) {
        RenderStarTarget();
    }
    
    // desenha barra de vida para todos os tipos de alvo se health < maxHealth
    if (health < maxHealth) {
        float healthRatio = static_cast<float>(health) / maxHealth;
        float barWidth = radius * 2.0f;
        float barHeight = 4.0f;
        float fillWidth = barWidth * healthRatio;

        // fundo da barra de vida
        CV::color(0.3f, 0.3f, 0.3f);
        CV::rectFill(position.x - radius, position.y - radius - 10,
                     position.x - radius + barWidth, position.y - radius - 10 + barHeight);

        // preenchimento da barra de vida
        CV::color(1.0f - healthRatio, healthRatio, 0.0f); // vermelho para verde
        CV::rectFill(position.x - radius, position.y - radius - 10,
                     position.x - radius + fillWidth, position.y - radius - 10 + barHeight);
    }
}

void Target::RenderBasicTarget() {
    // renderização do alvo circular original
    float healthRatio = static_cast<float>(health) / maxHealth;
    CV::color(0.8f * healthRatio, 0.3f * healthRatio, 0.0f);
    CV::circleFill(position.x, position.y, radius, 15);
    CV::color(0.4f * healthRatio, 0.15f * healthRatio, 0.0f);
    CV::circle(position.x, position.y, radius, 15);

    // adiciona uma marcação "X" simples
    CV::color(0.2f, 0.2f, 0.2f);
    CV::line(position.x - radius/1.5f, position.y - radius/1.5f,
            position.x + radius/1.5f, position.y + radius/1.5f);
    CV::line(position.x + radius/1.5f, position.y - radius/1.5f,
            position.x - radius/1.5f, position.y + radius/1.5f);
}

void Target::RenderShooterTarget() {
    // alvo triangular que mira no tanque
    float size = radius * 1.5f; // um pouco maior que o alvo básico

    // calcula vértices do triângulo
    float cosA = cos(aimAngle);
    float sinA = sin(aimAngle);

    // ponto 1: ponto frontal (direcionado ao tanque)
    float x1 = position.x + cosA * size;
    float y1 = position.y + sinA * size;

    // pontos 2 e 3: cantos traseiros (perpendiculares à direção de mira)
    float x2 = position.x - cosA * size * 0.5f + sinA * size * 0.7f;
    float y2 = position.y - sinA * size * 0.5f - cosA * size * 0.7f;
    float x3 = position.x - cosA * size * 0.5f - sinA * size * 0.7f;
    float y3 = position.y - sinA * size * 0.5f + cosA * size * 0.7f;

    // desenha o triângulo
    float vx[3] = {x1, x2, x3};
    float vy[3] = {y1, y2, y3};

    float healthRatio = static_cast<float>(health) / maxHealth;
    CV::color(1.0f * healthRatio, 0.9f * healthRatio, 0.0f); // amarelo
    CV::polygonFill(vx, vy, 3);

    CV::color(0.7f * healthRatio, 0.6f * healthRatio, 0.0f); // borda
    CV::line(x1, y1, x2, y2);
    CV::line(x2, y2, x3, y3);
    CV::line(x3, y3, x1, y1);

    // desenha indicador de alcance de tiro quando em recarga
    if (firingCooldown > 0) {
        float cooldownRatio = static_cast<float>(firingCooldown) / firingCooldownReset;
        CV::color(1.0f, 0.0f, 0.0f, 0.3f); // vermelho com transparência
        CV::circle(position.x, position.y, shootingRadius * cooldownRatio * 0.1f, 30);
    }
}

// renderiza uma forma de estrela com geometria adequada
void Target::RenderStarTarget() {
    // parâmetros da estrela
    float outerRadius = radius * 1.5f;
    float innerRadius = radius * 0.6f;
    const int numPoints = 5;  // estrela de 5 pontas
    
    // precisamos de 10 pontos no total (5 pontos externos e 5 internos)
    Vector2 points[numPoints * 2];
    float angle = rotationAngle - M_PI/2; 
    
    // desenha pontos em ordem horária, alternando entre pontos externos e internos
    for (int i = 0; i < numPoints * 2; i++) {
        float r = (i % 2 == 0) ? outerRadius : innerRadius;
        
        points[i].x = position.x + r * cos(angle);
        points[i].y = position.y + r * sin(angle);
        
        angle += M_PI / numPoints;
    }

    
    // cor cinza com tom de saúde
    float healthRatio = static_cast<float>(health) / maxHealth;
    float shade = 0.5f + 0.3f * (1.0f - healthRatio); // mais escuro quando danificado
    
    // desenha contorno
    CV::color(shade * 0.7f, shade * 0.2f, shade * 0.2f); // contorno cinza mais escuro
    for (int i = 0; i < numPoints * 2; i++) {
        int next = (i + 1) % (numPoints * 2);
        CV::line(points[i].x, points[i].y, points[next].x, points[next].y);
    }
    
    // desenha indicador quando está perseguindo
    if (isChasing) {
        CV::color(1.0f, 0.2f, 0.2f); // indicador vermelho
        CV::circleFill(position.x, position.y, radius * 0.3f, 8);
    }
}

bool Target::CheckCollision(const Vector2& point) {
    if (!active) return false;

    float dx = point.x - position.x;
    float dy = point.y - position.y;
    float distanceSquared = dx*dx + dy*dy;

    return distanceSquared <= radius * radius;
}

bool Target::CheckCollisionWithTank(const Vector2& tankPos, float tankWidth, float tankHeight, float tankAngle) {
    if (!active) return false;

    // converte limites do tanque para coordenadas do mundo
    float halfW = tankWidth / 2.0f;
    float halfH = tankHeight / 2.0f;
    float cosB = cos(tankAngle);
    float sinB = sin(tankAngle);

    Vector2 localCorners[4] = {
        Vector2(-halfW, -halfH), Vector2(halfW, -halfH),
        Vector2(halfW, halfH), Vector2(-halfW, halfH)
    };

    Vector2 worldCorners[4];
    for (int i = 0; i < 4; ++i) {
        worldCorners[i].x = localCorners[i].x * cosB - localCorners[i].y * sinB + tankPos.x;
        worldCorners[i].y = localCorners[i].x * sinB + localCorners[i].y * cosB + tankPos.y;
    }

    // verifica se qualquer canto do tanque está dentro do círculo do alvo
    for (int i = 0; i < 4; ++i) {
        if (CheckCollision(worldCorners[i])) return true;
    }
    
    // adiciona um ponto extra no centro frontal do tanque para colisao
    Vector2 frontCenterPoint;
    Vector2 backCenterPoint;
    backCenterPoint.x = tankPos.x - halfW * cosB;
    backCenterPoint.y = tankPos.y - halfW * sinB;
    frontCenterPoint.x = tankPos.x + halfW * cosB;
    frontCenterPoint.y = tankPos.y + halfW * sinB;
    
    if (CheckCollision(frontCenterPoint)) return true;
    if (CheckCollision(backCenterPoint)) return true;

    // por simplicidade, estamos mantendo a mesma verificação de colisão para todos os tipos de alvo
    return false;
}

void Target::TakeDamage(int amount) {
    health -= amount;
    if (health <= 0) {
        health = 0;
        active = false;
    }
}

bool Target::FireAtTarget(const Vector2& targetPos) {
    if (!active || type != TargetType::Shooter) return false;

    // calcula direção do tiro
    Vector2 direction(targetPos.x - position.x, targetPos.y - position.y);
    if (direction.lengthSq() > 0.001f) {
        direction.normalize();

        // cria projétil com velocidade apropriada
        float bulletSpeed = 2.5f; // um pouco mais rápido para melhor desafio
        Vector2 bulletVelocity = direction * bulletSpeed;

        // gera projétil a partir da ponta do triângulo (ponto frontal)
        Vector2 spawnPos = position + direction * (radius * 1.5f);
        EnemyProjectile bullet(spawnPos, bulletVelocity);

        // adiciona à lista de projéteis
        projectiles.push_back(bullet);
        return true;
    }

    return false;
}

void Target::UpdateProjectiles(BSplineTrack* track) {
    // atualiza cada projétil
    for (auto& proj : projectiles) {
        if (proj.active) {
            proj.Update();

            // verifica colisão com parede se o track for fornecido
            if (track) {
                proj.CheckCollisionWithTrack(track);
            }
        }
    }

    // remove projéteis inativos
    projectiles.erase(
        std::remove_if(
            projectiles.begin(),
            projectiles.end(),
            [](const EnemyProjectile& p) { return !p.active; }
        ),
        projectiles.end()
    );
}

bool EnemyProjectile::CheckCollisionWithTrack(BSplineTrack* track) {
    if (!active || !track) return false;

    // obtém pontos mais próximos em ambos os limites da pista
    ClosestPointInfo cpiLeft = track->findClosestPointOnCurve(position, CurveSide::Left);
    ClosestPointInfo cpiRight = track->findClosestPointOnCurve(position, CurveSide::Right);

    // verifica colisão com limite esquerdo
    if (cpiLeft.isValid) {
        Vector2 vec_proj_to_cl_point = position - cpiLeft.point;
        float projection = vec_proj_to_cl_point.x * cpiLeft.normal.x + vec_proj_to_cl_point.y * cpiLeft.normal.y;

        // se a projeção for positiva, o projétil está fora do limite esquerdo
        if (projection > 0.0f && projection < radius) {
            active = false;
            return true;
        }
    }

    // verifica colisão com limite direito
    if (cpiRight.isValid) {
        Vector2 vec_proj_to_cr_point = position - cpiRight.point;
        float projection = vec_proj_to_cr_point.x * cpiRight.normal.x + vec_proj_to_cr_point.y * cpiRight.normal.y;

        // se a projeção for negativa, o projétil está fora do limite direito
        if (projection < 0.0f && std::abs(projection) < radius) {
            active = false;
            return true;
        }
    }

    return false;
}
