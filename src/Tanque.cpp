#include "Tanque.h"
#include "gl_canvas2d.h"
#include <cmath> 
#include <algorithm> 
#include "BSplineTrack.h" 

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Tanque::Tanque(float x, float y, float initialSpeed, float initialRotationRate) {
    position.set(x, y);
    baseAngle = 0.0f;
    topAngle = 0.0f;
    speed = initialSpeed;
    rotationRate = initialRotationRate;
    turretRotationSpeed = 0.05f; // assumindo que isto é por quadro se deltaTime foi globalmente removido
    baseWidth = 60.0f;
    baseHeight = 40.0f;
    turretRadius = 15.0f;
    cannonLength = 40.0f;
    cannonWidth = 6.0f;

    forwardVector.set(cos(baseAngle), sin(baseAngle));

    // inicializa membros de colisão
    lastSafePosition = position;
    isColliding = false;
    collisionTimer = 0;

    // inicializa membros relacionados a projéteis
    firingCooldown = 0;
    firingCooldownReset = 45; 
    projectileSpeed = 3.5f;  

    // inicializa membros relacionados à saúde
    health = 100;
    maxHealth = 100;
    isInvulnerable = false;
    invulnerabilityTimer = 0;
    isShieldInvulnerable = false;

    // inicializa escudo
    hasShield = false;
}

void Tanque::Update(float mouseX, float mouseY, bool rotateLeft, bool rotateRight, BSplineTrack* track) {
    // diminui o tempo de recarga de tiro se ativo
    if (firingCooldown > 0) {
        firingCooldown--;
    }

    // atualiza projéteis
    UpdateProjectiles(track);

    // atualiza explosões
    explosions.Update();

    if (isColliding) {
        // movimento de recuo
        position.x -= forwardVector.x * speed * REBOUND_SPEED_FACTOR;
        position.y -= forwardVector.y * speed * REBOUND_SPEED_FACTOR;
        collisionTimer--;
        if (collisionTimer <= 0) {
            isColliding = false;
        }

        // a torre ainda pode mirar durante o recuo
        float dx_turret = mouseX - position.x;
        float dy_turret = mouseY - position.y;
        float targetAngle_turret = atan2(dy_turret, dx_turret);
        float currentTopAngle_turret = fmod(topAngle + M_PI, 2 * M_PI) - M_PI;
        if (currentTopAngle_turret < -M_PI) currentTopAngle_turret += 2 * M_PI;
        float angleDifference_turret = targetAngle_turret - currentTopAngle_turret;
        if (angleDifference_turret > M_PI) angleDifference_turret -= 2 * M_PI;
        else if (angleDifference_turret < -M_PI) angleDifference_turret += 2 * M_PI;

        float maxRotationThisFrame_turret = turretRotationSpeed; // assumindo por quadro

        if (std::abs(angleDifference_turret) <= maxRotationThisFrame_turret) {
            topAngle = targetAngle_turret;
        } else {
            topAngle += (angleDifference_turret > 0 ? maxRotationThisFrame_turret : -maxRotationThisFrame_turret);
        }
        topAngle = fmod(topAngle, 2 * M_PI);
        if (topAngle < 0) topAngle += 2 * M_PI;

        return; // pula movimento normal e novas verificações de colisão
    }

    // não está colidindo e recuando: lógica de atualização normal
    lastSafePosition = position;

    // atualiza rotação da base
    if (rotateLeft) {
        baseAngle -= rotationRate;
    }
    if (rotateRight) {
        baseAngle += rotationRate;
    }

    // mantém baseAngle entre 0 e 2*PI
    if (baseAngle > 2 * M_PI) baseAngle -= 2 * M_PI;
    if (baseAngle < 0) baseAngle += 2 * M_PI;

    // atualiza vetor de direção baseado em baseAngle
    forwardVector.set(cos(baseAngle), sin(baseAngle));

    // posição tentativa (sem deltaTime)
    Vector2 tentativePosition = position;
    tentativePosition.x += forwardVector.x * speed;
    tentativePosition.y += forwardVector.y * speed;

    // armazena posição atual, ajusta para tentativa para verificação de colisão
    Vector2 currentActualPosition = position;
    position = tentativePosition;

    CheckCollisionAndRespond(track); 


    // atualiza o ângulo superior (torre) para apontar para o mouse
    float dx = mouseX - position.x; // usa a posição (potencialmente revertida)
    float dy = mouseY - position.y;
    float targetAngle = atan2(dy, dx);

    float currentTopAngle = fmod(topAngle + M_PI, 2 * M_PI) - M_PI;
    if (currentTopAngle < -M_PI) currentTopAngle += 2 * M_PI; // garante que esteja em [-PI, PI]
    float angleDifference = targetAngle - currentTopAngle;

    // normaliza a diferença de ângulo para o caminho mais curto (-PI a PI)
    if (angleDifference > M_PI) {
        angleDifference -= 2 * M_PI;
    } else if (angleDifference < -M_PI) {
        angleDifference += 2 * M_PI;
    }

    float maxRotationThisFrame = turretRotationSpeed; // assumindo que turretRotationSpeed é agora por quadro

    if (std::abs(angleDifference) <= maxRotationThisFrame) {
        topAngle = targetAngle;
    } else {
        topAngle += (angleDifference > 0 ? maxRotationThisFrame : -maxRotationThisFrame);
    }

    // normaliza topAngle para estar entre 0 e 2*PI (ou -PI a PI, consistentemente)
    topAngle = fmod(topAngle, 2 * M_PI);
    if (topAngle < 0) {
        topAngle += 2 * M_PI;
    }
}

void Tanque::Render() {
    // renderiza projéteis primeiro (para que o tanque apareça acima deles)
    for (auto& proj : projectiles) {
        proj.Render();
    }

    // renderiza explosões antes do tanque
    explosions.Render();

    // desenha barra de vida abaixo do tanque (alterado de acima)
    float healthBarWidth = baseWidth * 1.2f;  // torna-a ligeiramente mais larga que o tanque
    float healthBarHeight = 5.0f;
    float healthBarY = position.y + baseHeight / 2.0f + 5.0f; // posiciona abaixo do tanque em vez de acima
    float healthPercent = static_cast<float>(health) / maxHealth;

    // fundo da barra de vida (vermelho)
    CV::color(1.0f, 0.2f, 0.2f);
    CV::rectFill(position.x - healthBarWidth / 2.0f, healthBarY,
                position.x + healthBarWidth / 2.0f, healthBarY + healthBarHeight);

    // preenchimento da barra de vida (verde)
    CV::color(0.2f, 0.8f, 0.2f);
    CV::rectFill(position.x - healthBarWidth / 2.0f, healthBarY,
                position.x - healthBarWidth / 2.0f + healthBarWidth * healthPercent, healthBarY + healthBarHeight);

    // usa um efeito de flash quando o tanque está invulnerável (atingido)
    if (isInvulnerable) {
        if ((invulnerabilityTimer / 5) % 2 == 0) {
            if (isShieldInvulnerable) {
                CV::color(0.3f, 0.3f, 1.0f); // flash azul para escudo
            } else {
                CV::color(1.0f, 0.2f, 0.2f); // flash vermelho para dano
            }
        } else {
            CV::color(0.2f, 0.5f, 0.2f); // verde normal
        }
    } else {
        CV::color(0.2f, 0.5f, 0.2f); // verde normal
    }

    // renderiza base (retângulo)
    float halfW = baseWidth / 2.0f;
    float halfH = baseHeight / 2.0f;

    // cantos locais do retângulo base (antes da rotação)
    Vector2 p1_local(-halfW, -halfH);
    Vector2 p2_local( halfW, -halfH);
    Vector2 p3_local( halfW,  halfH);
    Vector2 p4_local(-halfW,  halfH);

    // cantos rotacionados
    float cosB = cos(baseAngle);
    float sinB = sin(baseAngle);

    Vector2 p1_world(p1_local.x * cosB - p1_local.y * sinB + position.x, p1_local.x * sinB + p1_local.y * cosB + position.y);
    Vector2 p2_world(p2_local.x * cosB - p2_local.y * sinB + position.x, p2_local.x * sinB + p2_local.y * cosB + position.y);
    Vector2 p3_world(p3_local.x * cosB - p3_local.y * sinB + position.x, p3_local.x * sinB + p3_local.y * cosB + position.y);
    Vector2 p4_world(p4_local.x * cosB - p4_local.y * sinB + position.x, p4_local.x * sinB + p4_local.y * cosB + position.y);

    // desenha a base como um polígono preenchido em vez de linhas
    float vx_base[4] = {p1_world.x, p2_world.x, p3_world.x, p4_world.x};
    float vy_base[4] = {p1_world.y, p2_world.y, p3_world.y, p4_world.y};
    CV::polygonFill(vx_base, vy_base, 4);

    // desenha visualização de escudo se o tanque tiver um escudo
    CV::color(0.0f, 0.0f, 1.0f); // cor azul para escudo
    if (hasShield) {
        for(int side = 0; side < 4; side++){
            CV::line(vx_base[side], vy_base[side], vx_base[(side + 1) % 4], vy_base[(side + 1) % 4]);
        }
    }

    // renderiza topo (torre - círculo e canhão - retângulo)
    CV::color(0.1f, 0.3f, 0.1f); // verde mais escuro para torre
    CV::circleFill(position.x, position.y, turretRadius, 20); // desenha base da torre

    // cálculos do canhão
    float halfCW = cannonWidth / 2.0f;
    float cosT = cos(topAngle);
    float sinT = sin(topAngle);

    // pontos relativos à posição do tanque
    Vector2 c1_local(0, -halfCW);
    Vector2 c2_local(0,  halfCW);
    Vector2 c3_local(cannonLength, -halfCW);
    Vector2 c4_local(cannonLength,  halfCW);

    // rotaciona estes pontos locais pelo topAngle e adiciona à posição do tanque
    Vector2 c1(position.x + (c1_local.x * cosT - c1_local.y * sinT), position.y + (c1_local.x * sinT + c1_local.y * cosT));
    Vector2 c2(position.x + (c2_local.x * cosT - c2_local.y * sinT), position.y + (c2_local.x * sinT + c2_local.y * cosT));
    Vector2 c3(position.x + (c3_local.x * cosT - c3_local.y * sinT), position.y + (c3_local.x * sinT + c3_local.y * cosT));
    Vector2 c4(position.x + (c4_local.x * cosT - c4_local.y * sinT), position.y + (c4_local.x * sinT + c4_local.y * cosT));

    // desenha o canhão como um polígono preenchido em vez de linhas
    float vx_cannon[4] = {c1.x, c2.x, c4.x, c3.x}; // a ordem importa para polígonos convexos
    float vy_cannon[4] = {c1.y, c2.y, c4.y, c3.y};
    CV::polygonFill(vx_cannon, vy_cannon, 4);


    // desenha indicador de recarga se estiver recarregando
    if (firingCooldown > 0) {
        float cooldownFraction = (float)firingCooldown / firingCooldownReset;
        float barLength = 25.0f * cooldownFraction;
        CV::color(1.0f, 0.3f, 0.3f); // barra vermelha de recarga
        CV::rectFill(position.x - 12.5f, position.y - turretRadius - 10, position.x - 12.5f + barLength, position.y - turretRadius - 5);
    }
}

// novo método para disparar projéteis
bool Tanque::FireProjectile() {
    if (firingCooldown > 0) {
        return false; // ainda não pode disparar
    }

    // obtém a posição da ponta do canhão
    Vector2 cannonTip = GetCannonTipPosition();

    // cria vetor de velocidade baseado na direção do canhão
    Vector2 projectileVelocity(cos(topAngle) * projectileSpeed, sin(topAngle) * projectileSpeed);

    // cria e adiciona o projétil
    Projectile newProjectile(cannonTip, projectileVelocity);
    projectiles.push_back(newProjectile);

    // reinicia recarga
    firingCooldown = firingCooldownReset;

    return true;
}

// auxiliar para calcular a posição da ponta do canhão
Vector2 Tanque::GetCannonTipPosition() const {
    return Vector2(
        position.x + cos(topAngle) * cannonLength,
        position.y + sin(topAngle) * cannonLength
    );
}

// novo método para atualizar projéteis
void Tanque::UpdateProjectiles(BSplineTrack* track) {
    // atualiza cada projétil e verifica colisões
    for (auto& proj : projectiles) {
        if (proj.active) {
            proj.Update();
            // passa o gerenciador de explosões para efeitos de colisão
            if (proj.CheckCollisionWithTrack(track, &explosions)) {
                // projétil agora está inativo devido à colisão
            }
        }
    }

    // remove projéteis inativos (usando idioma erase-remove)
    projectiles.erase(
        std::remove_if(
            projectiles.begin(),
            projectiles.end(),
            [](const Projectile& p) { return !p.active; }
        ),
        projectiles.end()
    );
}

void Tanque::CheckCollisionAndRespond(BSplineTrack* track) {
    if (!track) {
        this->isColliding = false;
        return;
    }

    // pula dano de saúde se o tanque já estiver invulnerável
    bool canTakeDamage = !isInvulnerable;

    Vector2 currentTankPosition = this->position; // posição tentativa atual do centro do tanque

    // calcula os cantos mundiais do tanque baseado em currentTankPosition e baseAngle
    float halfW = this->baseWidth / 2.0f;
    float halfH = this->baseHeight / 2.0f;
    float cosB = cos(this->baseAngle);
    float sinB = sin(this->baseAngle);

    Vector2 local_corners[4] = {
        Vector2(-halfW, -halfH), Vector2(halfW, -halfH),
        Vector2(halfW,  halfH), Vector2(-halfW,  halfH)
    };
    Vector2 world_corners[4];
    for (int i = 0; i < 4; ++i) {
        world_corners[i].x = local_corners[i].x * cosB - local_corners[i].y * sinB + currentTankPosition.x;
        world_corners[i].y = local_corners[i].x * sinB + local_corners[i].y * cosB + currentTankPosition.y;
    }

    ClosestPointInfo cpiLeft = track->findClosestPointOnCurve(currentTankPosition, CurveSide::Left);
    ClosestPointInfo cpiRight = track->findClosestPointOnCurve(currentTankPosition, CurveSide::Right);

    bool collisionThisFrame = false;

    // verifica colisão com curva esquerda
    if (cpiLeft.isValid) {
        float max_projection_left = -FLT_MAX;
        for (int i = 0; i < 4; ++i) {
            Vector2 vec_corner_to_cl_point = world_corners[i] - cpiLeft.point;
            float projection = vec_corner_to_cl_point.x * cpiLeft.normal.x + vec_corner_to_cl_point.y * cpiLeft.normal.y;
            if (projection > max_projection_left) {
                max_projection_left = projection;
            }
        }
        if (max_projection_left > 0.0f) {
            collisionThisFrame = true;
        }
    }

    // verifica colisão com curva direita
    if (!collisionThisFrame && cpiRight.isValid) {
        float min_projection_right = FLT_MAX;
        for (int i = 0; i < 4; ++i) {
            Vector2 vec_corner_to_cr_point = world_corners[i] - cpiRight.point;
            float projection = vec_corner_to_cr_point.x * cpiRight.normal.x + vec_corner_to_cr_point.y * cpiRight.normal.y;
            if (projection < min_projection_right) {
                min_projection_right = projection;
            }
        }
        if (min_projection_right < 0.0f) {
            collisionThisFrame = true;
        }
    }

    if (collisionThisFrame) {
        this->isColliding = true;
        this->collisionTimer = COLLISION_REBOUND_FRAMES;
        this->position = this->lastSafePosition; // reverte para última posição segura conhecida

        // apenas aplica dano se o tanque ainda não estiver invulnerável
        if (canTakeDamage) {
            // verifica se temos um escudo para bloquear o dano
            if (hasShield) {
                hasShield = false; // consome o escudo
                // ainda faz o tanque temporariamente invulnerável e pisca
                isInvulnerable = true;
                isShieldInvulnerable = true; // define flag para invulnerabilidade de escudo
                invulnerabilityTimer = INVULNERABILITY_FRAMES;
            } else {
                // tanque sofre dano - 25% da saúde máxima (igual à colisão com alvo)
                int damageTaken = maxHealth / 4;  // 25% da saúde máxima
                health -= damageTaken;
                if (health < 0) health = 0;

                // torna o tanque temporariamente invulnerável e pisca (igual às colisões com alvo)
                isInvulnerable = true;
                isShieldInvulnerable = false; // dano regular
                invulnerabilityTimer = INVULNERABILITY_FRAMES;
            }
        }
    }
}

// alterado o tipo de retorno de void para int para corresponder à declaração no header
int Tanque::CheckTargetCollisions(std::vector<Target>& targets) {
    // pula se o tanque estiver atualmente invulnerável
    if (isInvulnerable) {
        invulnerabilityTimer--;
        if (invulnerabilityTimer <= 0) {
            isInvulnerable = false;
            isShieldInvulnerable = false; // também reinicia a flag de invulnerabilidade de escudo
        }
        return -1;
    }

    // verifica cada alvo para colisão com o tanque
    for (size_t i = 0; i < targets.size(); i++) {
        // pula alvos do tipo Star já que são tratados separadamente no loop principal de renderização
        if (targets[i].type == TargetType::Star) {
            continue;
        }

        if (targets[i].active && targets[i].CheckCollisionWithTank(position, baseWidth, baseHeight, baseAngle)) {
            // verifica se temos um escudo para bloquear o dano
            if (hasShield) {
                hasShield = false; // consome o escudo
                // ainda torna o tanque temporariamente invulnerável e pisca
                isInvulnerable = true;
                isShieldInvulnerable = true; // define flag para invulnerabilidade de escudo
                invulnerabilityTimer = INVULNERABILITY_FRAMES;

                // o alvo sofre dano e deve ser destruído ao colidir com um escudo
                targets[i].TakeDamage(targets[i].health); // mata o alvo aplicando sua saúde total como dano

                // verifica se o alvo foi destruído por esta colisão
                if (!targets[i].active) {
                    return i; // retorna o índice do alvo destruído
                }
            } else {
                // aplica dano normal
                int damageTaken = maxHealth / 4;  // 25% da saúde máxima
                health -= damageTaken;
                if (health < 0) health = 0;

                // torna o tanque temporariamente invulnerável para evitar múltiplos golpes rápidos
                isInvulnerable = true;
                isShieldInvulnerable = false; // dano regular
                invulnerabilityTimer = INVULNERABILITY_FRAMES;

                // o alvo também sofre dano
                targets[i].TakeDamage(1);

                // verifica se o alvo foi destruído por esta colisão
                if (!targets[i].active) {
                    return i; // retorna o índice do alvo destruído
                }
            }

            // se acertamos algum alvo, podemos quebrar o loop já que agora estamos invulneráveis
            break;
        }
    }

    return -1; // retorna -1 se nenhum alvo foi destruído
}

// adiciona as implementações ausentes de Tanque.h
int Tanque::CheckProjectileTargetCollision(const Projectile& proj, std::vector<Target>& targets) {
    if (!proj.active) return -1;

    for (size_t i = 0; i < targets.size(); i++) {
        if (targets[i].active && targets[i].CheckCollision(proj.position)) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

bool Tanque::CheckAllProjectilesAgainstTargets(std::vector<Target>& targets, int& hitTargetIndex, int& hitProjectileIndex) {
    for (size_t i = 0; i < projectiles.size(); i++) {
        int targetIdx = CheckProjectileTargetCollision(projectiles[i], targets);
        if (targetIdx >= 0) {
            hitTargetIndex = targetIdx;
            hitProjectileIndex = static_cast<int>(i);

            // cria explosão ao acertar um alvo
            projectiles[i].CreateExplosionOnCollision(&explosions);

            return true;
        }
    }

    hitTargetIndex = -1;
    hitProjectileIndex = -1;
    return false;
}
