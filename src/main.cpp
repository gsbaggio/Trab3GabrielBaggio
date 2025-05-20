/**
 * main.cpp
 * Arquivo principal que coordena todos os componentes do jogo.
 * Gerencia inicialização, loop principal, entrada do usuário,
 * e interação entre tanque, alvos, power-ups e pista.
 */

#include <GL/glut.h>
#include <GL/freeglut_ext.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "gl_canvas2d.h"

#include "Tanque.h"
#include "BSplineTrack.h"
#include "Target.h"
#include "Projectile.h"
#include "PowerUp.h" 

//largura e altura inicial da tela . Alteram com o redimensionamento de tela.
int screenWidth = 1280, screenHeight = 720;

Tanque *g_tanque = NULL;
BSplineTrack *g_track = NULL;

bool g_editorMode = false;
bool g_mousePressed = false;

int mouseX, mouseY; //variaveis globais do mouse para utilizar em qualquer lugar

// flags para guardar rotacao do tanque
bool keyA_down = false;
bool keyD_down = false;

// numero de alvos por nivel
const int NUM_TARGETS = 5;

// variaveis globais para o jogo
std::vector<Target> g_targets;
int g_playerScore = 0;
int g_gameLevel = 1;  
int g_destroyedTargets = 0; 
PowerUp g_powerUp;
PowerUpType g_storedPowerUp = PowerUpType::None;

// gera uma posicao aleatoria no track, usada para gerar alvos e power ups
Vector2 GenerateRandomPosTrack(BSplineTrack* track, const Vector2& avoidPosition = Vector2(0,0), bool checkAvoidance = false) {
    const int MAX_ATTEMPTS = 1000; // maximo de tentativas para achar posicao
    const float MIN_SAFE_DISTANCE_SQ = 150.0f * 150.0f; // pra nao spawn muito perto do tanque

    Vector2 position;
    int attempts = 0;

    do {
        attempts++;
        if (attempts > MAX_ATTEMPTS) {
            return position; // nao achou pos
        }

        float t = static_cast<float>(rand()) / RAND_MAX;

        Vector2 leftPoint = track->getPointOnCurve(t, CurveSide::Left);
        Vector2 rightPoint = track->getPointOnCurve(t, CurveSide::Right);

        // interpolacao entre os dois pontos
        float interpFactor = 0.2f + 0.6f * static_cast<float>(rand()) / RAND_MAX; 
        position = leftPoint + (rightPoint - leftPoint) * interpFactor;

        // checa se esta perto do tanque
        if (checkAvoidance) {
            float distSq = position.distSq(avoidPosition);
            if (distSq < MIN_SAFE_DISTANCE_SQ) {
                continue; 
            }
        }

        // checa distancia de outros alvos
        bool tooCloseToOtherTargets = false;
        for (const auto& target : g_targets) {
            if (target.active) {
                float distSq = position.distSq(target.position);
                if (distSq < 50.0f * 50.0f) { 
                    tooCloseToOtherTargets = true;
                    break;
                }
            }
        }

        if (!tooCloseToOtherTargets) {
            return position; 
        }

    } while (attempts <= MAX_ATTEMPTS);

    // fallback caso nao ache posicao
    return Vector2(0, 0);
}

// spawna o tanque no inicio do track
void resetTankToTrackStart(Tanque* tanque, BSplineTrack* track) {
    if (!tanque || !track) {
        return;
    }

    Vector2 start_point_left, start_point_right;


    // calcula a posicao inicial do tanque no track
    start_point_left = track->getPointOnCurve(0.0f, CurveSide::Left);
    start_point_right = track->getPointOnCurve(0.0f, CurveSide::Right);

    tanque->position.set((start_point_left.x + start_point_right.x) / 2.0f,(start_point_left.y + start_point_right.y) / 2.0f);


    // calcula a tangente do tanque no track (direcao que ele spawna)
    Vector2 tangent_left = track->getTangentOnCurve(0.0f, CurveSide::Left);
    Vector2 tangent_right = track->getTangentOnCurve(0.0f, CurveSide::Right);

    Vector2 final_tangent;

    Vector2 norm_tangent_left = tangent_left.normalized();
    Vector2 norm_tangent_right = tangent_right.normalized();

    if (norm_tangent_left.lengthSq() > 0.0001f && norm_tangent_right.lengthSq() > 0.0001f) {
        final_tangent = (norm_tangent_left + norm_tangent_right).normalized();
    } else if (norm_tangent_left.lengthSq() > 0.0001f) {
        final_tangent = norm_tangent_left;
    } else if (norm_tangent_right.lengthSq() > 0.0001f) {
        final_tangent = norm_tangent_right;
    }
    tanque->baseAngle = atan2(final_tangent.y, final_tangent.x);


    tanque->forwardVector.set(cos(tanque->baseAngle), sin(tanque->baseAngle));
}

// spawna um power-up no track
void SpawnPowerUp(BSplineTrack* track) {
    // se ja ativo ou ja existe, nao spawn
    if (g_powerUp.active || g_storedPowerUp != PowerUpType::None) return;

    // gera uma posicao aleatoria no track
    Vector2 position = GenerateRandomPosTrack(track, g_tanque->position, true);

    // escolhe um tipo de power-up aleatorio
    int randType = rand() % 3 + 1; // 1-3
    PowerUpType type = static_cast<PowerUpType>(randType);

    // cria e ativa o power-up
    g_powerUp = PowerUp(position, type);
}

// inicializa os alvos do jogo
void InitializeTargets(BSplineTrack* track) {
    g_targets.clear();
    g_destroyedTargets = 0; 

    // cria os alvos
    for (int i = 0; i < NUM_TARGETS; i++) {
        // encontra uma boa posição para o alvo
        Vector2 position = GenerateRandomPosTrack(track, g_tanque->position, true);

        
        TargetType targetType = TargetType::Basic; 

        // aleatoriamente escolhe o tipo de alvo
        float rand_val = (float)rand() / RAND_MAX;

        // dependendo do nivel, escolhe o tipo de alvo de forma diferente
        if (g_gameLevel >= 3 && i == NUM_TARGETS - 1) {
            targetType = TargetType::Star;
        }
        else if (g_gameLevel >= 2 && i == NUM_TARGETS - 2) {
            targetType = TargetType::Shooter;
        }
        else if (g_gameLevel >= 4 && rand_val < 0.3f) {
            targetType = TargetType::Star;
        }
        else if (g_gameLevel >= 2 && rand_val < 0.4f) {
            targetType = TargetType::Shooter;
        }

        // cria o alvo de fato
        Target target(position, targetType);
        target.active = true;

        // vida aumenta com o nivel
        target.maxHealth = 2 + (g_gameLevel - 1);

        // shooter tem +1 de vida
        if (target.type == TargetType::Shooter) {
            target.maxHealth += 1;
        }
        // estrela tem +2 de vida
        else if (target.type == TargetType::Star) {
            target.maxHealth += 2;

            // estrela move mais rapido com o nivel
            target.moveSpeed = 0.5f + (g_gameLevel * 0.1f);

            // estrelas detectam de mais longe em niveis mais altos
            target.detectionRadius = 150.0f + (g_gameLevel * 25.0f);

            // a velocidade de rotação aumenta ligeiramente com o nível
            target.rotationSpeed = 0.05f + (g_gameLevel - 1) * 0.01f;
        }

        target.health = target.maxHealth;

        g_targets.push_back(target);
    }

    SpawnPowerUp(track);
}

// reseta tudo
void ResetGameState(Tanque* tanque, BSplineTrack* track) {
    g_playerScore = 0;
    g_gameLevel = 1;  
    g_destroyedTargets = 0;  

    if (tanque) {
        tanque->health = tanque->maxHealth;
        tanque->isInvulnerable = false;
        tanque->invulnerabilityTimer = 0;
    }

    InitializeTargets(track);

    g_powerUp.active = false;
    g_storedPowerUp = PowerUpType::None;

    SpawnPowerUp(track);
}

// uso do power up
void UsePowerUp(Tanque* tank, std::vector<Target>& targets) {
    if (!tank || g_storedPowerUp == PowerUpType::None) return;

    switch (g_storedPowerUp) {
        case PowerUpType::Health:
            PowerUp::ApplyHealthEffect(tank);
            break;

        case PowerUpType::Shield:
            PowerUp::ApplyShieldEffect(tank);
            break;

        case PowerUpType::Laser: {
            int targetsDestroyed = PowerUp::ApplyLaserEffect(tank, targets);

            
            g_playerScore += targetsDestroyed * 100;
            g_destroyedTargets += targetsDestroyed;

            
            if (g_destroyedTargets >= NUM_TARGETS) {
                
                g_gameLevel++;
                InitializeTargets(g_track);
            }
            break;
        }

        default:
            break;
    }

    // remove o power-up depois de usar
    g_storedPowerUp = PowerUpType::None;
}

//funcao chamada continuamente. Deve-se controlar o que desenhar por meio de variaveis globais
//Todos os comandos para desenho na canvas devem ser chamados dentro da render().
//Deve-se manter essa funo com poucas linhas de codigo
void render()
{
    CV::clear(0.25f, 0.25f, 0.3f);


    // pra seguir o tanque a tela
    if(!g_editorMode){
        CV::translate(-g_tanque->position.x + screenWidth/2, -g_tanque->position.y + screenHeight/2);
    }

    // renderiza o track
    if (g_track) {
        g_track->Render(g_editorMode);
    }

   // renderiza power ups
    if (!g_editorMode) {
        g_powerUp.Update();
        g_powerUp.Render();


        PowerUp::UpdateLaserEffect();
    }

   // renderiza os inimigos
    if (!g_editorMode) {
        for (auto& target : g_targets) {
            target.Render();
        }
    }

    // renderiza o efeito do laser (deve ser renderizado após os inimigos, mas antes do tanque)
    if (!g_editorMode) {
        PowerUp::RenderLaserEffect();
    }

    // renderiza o tanque fora do editor
    if (!g_editorMode && g_tanque) {
        g_tanque->Update(static_cast<float>(mouseX + g_tanque->position.x - screenWidth/2),
                        static_cast<float>(mouseY + g_tanque->position.y - screenHeight/2),
                        keyA_down, keyD_down, g_track);

        // att os targets
        for (auto& target : g_targets) {
            target.Update(g_tanque->position, g_track);
        }

        // checa se ele pegou o power-up
        if (g_powerUp.active && g_powerUp.CheckCollection(g_tanque->position, g_tanque->baseWidth/2.0f)) {
            // armazena o tipo de power-up e desativa ele do chao
            g_storedPowerUp = g_powerUp.type;
            g_powerUp.active = false;

        }

        // checa dano
        if (!g_tanque->isInvulnerable) {
            // a estrela deve dar muito dano, checa colisao com ela
            for (size_t i = 0; i < g_targets.size(); i++) {
                Target& target = g_targets[i];
                if (target.active && target.type == TargetType::Star) {
                    float dx = target.position.x - g_tanque->position.x;
                    float dy = target.position.y - g_tanque->position.y;
                    float distSq = dx*dx + dy*dy;
                    float combinedRadius = g_tanque->baseWidth/2.0f + target.radius;

                    if (distSq < combinedRadius * combinedRadius) {
                        // checa escudo
                        if (g_tanque->hasShield) {
                            g_tanque->hasShield = false; 
                            g_tanque->isInvulnerable = true;
                            g_tanque->isShieldInvulnerable = true; 
                            g_tanque->invulnerabilityTimer = g_tanque->INVULNERABILITY_FRAMES;
                            

                            // a estrela sempre morre ao bater no tanque
                            target.active = false;
                        } else {
                            // aplica o dano normal da estrela
                            int damage = g_tanque->maxHealth / 2;
                            g_tanque->health -= damage;
                            if (g_tanque->health < 0) g_tanque->health = 0;

                           
                            g_tanque->isInvulnerable = true;
                            g_tanque->isShieldInvulnerable = false; 
                            g_tanque->invulnerabilityTimer = g_tanque->INVULNERABILITY_FRAMES;

                            // a estrela sempre morre ao bater no tanque
                            target.active = false;
                        }

                        // a estrela morre, entao ganha pontos e aumenta o contador
                        g_playerScore += 100;  
                        g_destroyedTargets++;

                        // checa se todos morreram e passa o level
                        if (g_destroyedTargets >= NUM_TARGETS) {
                            g_gameLevel++;
                            InitializeTargets(g_track);
                        }
                    }
                }
            }
        }

        // checa projeteis no tanque
        if (!g_tanque->isInvulnerable) {
            for (auto& target : g_targets) {
                if (target.active && target.type == TargetType::Shooter) {
                    for (auto& proj : target.projectiles) { // passa por todos os projeteis dos shooters
                        if (proj.active) {
                            // checa se o projeteis colide com o tanque
                            float dx = proj.position.x - g_tanque->position.x;
                            float dy = proj.position.y - g_tanque->position.y;
                            float distSq = dx*dx + dy*dy;
                            float combinedRadius = g_tanque->baseWidth/2.0f + proj.radius;  

                            if (distSq < combinedRadius * combinedRadius) {
                               
                                proj.active = false;    

                                // checa escudo
                                if (g_tanque->hasShield) {
                                    
                                    g_tanque->hasShield = false; 
                                   
                                    g_tanque->isInvulnerable = true;
                                    g_tanque->isShieldInvulnerable = true; 
                                    g_tanque->invulnerabilityTimer = g_tanque->INVULNERABILITY_FRAMES;                               
                                } else {
                                    // sem escudo, aplica dano
                                    int damage = g_tanque->maxHealth / 8; 
                                    g_tanque->health -= damage;
                                    if (g_tanque->health < 0) g_tanque->health = 0;

                                   
                                    g_tanque->isInvulnerable = true;
                                    g_tanque->isShieldInvulnerable = false; 
                                    g_tanque->invulnerabilityTimer = g_tanque->INVULNERABILITY_FRAMES;
                                }
                            }
                        }
                    }
                }
            }
        }

        // checa colisao do tanque com os alvos normais
        int destroyedTargetIndex = g_tanque->CheckTargetCollisions(g_targets);
        if (destroyedTargetIndex >= 0) {
            // alvo e destruido com colisao
            g_playerScore += 100;
            g_destroyedTargets++;   

            // checa se todos morreram e passa o level
            if (g_destroyedTargets >= NUM_TARGETS) {
               
                g_gameLevel++;
                InitializeTargets(g_track); 
            }
        }

        // checa projeteis do tanque contra os alvos
        int hitTargetIndex = -1;
        int hitProjectileIndex = -1;
        if (g_tanque->CheckAllProjectilesAgainstTargets(g_targets, hitTargetIndex, hitProjectileIndex)) {
            // aplica o dano ao alvo
            if (hitTargetIndex >= 0 && hitTargetIndex < static_cast<int>(g_targets.size())) {
                // pega a posicao do alvo e a o vetor do projetil pra explosao
                Vector2 hitPosition = g_targets[hitTargetIndex].position;
                Vector2 hitVelocity = Vector2(0, 0);

                if (hitProjectileIndex >= 0 && hitProjectileIndex < static_cast<int>(g_tanque->projectiles.size())) {
                    hitVelocity = g_tanque->projectiles[hitProjectileIndex].velocity;
                    // cria a explosao na posicao do alvo
                    g_tanque->explosions.CreateExplosion(hitPosition, hitVelocity, 25);
                }

                // aplica dano ao alvo
                g_targets[hitTargetIndex].TakeDamage(1);    

                // se o alvo morreu, da ponto e aumenta o contador
                if (!g_targets[hitTargetIndex].active) {
                    g_playerScore += 100;
                    g_destroyedTargets++;

                    // checa se todos morreram e passa o level
                    if (g_destroyedTargets >= NUM_TARGETS) {
                       
                        g_gameLevel++;
                        InitializeTargets(g_track);  
                    }
                }
            }

            if (hitProjectileIndex >= 0 && hitProjectileIndex < static_cast<int>(g_tanque->projectiles.size())) {
                g_tanque->projectiles[hitProjectileIndex].active = false;
            }
        }

        g_tanque->Render();
    } else if (g_editorMode && g_tanque) {
        // no editar, coloca so a posicao inicial que o tanque spawnaria
        if (g_track) {
            Vector2 pL = g_track->getPointOnCurve(0.0f, CurveSide::Left);
            Vector2 pR = g_track->getPointOnCurve(0.0f, CurveSide::Right);
            g_tanque->position = (pL + pR) * 0.5f;  

            Vector2 tangentL = g_track->getTangentOnCurve(0.0f, CurveSide::Left);
            Vector2 tangentR = g_track->getTangentOnCurve(0.0f, CurveSide::Right);
            Vector2 avgTangent = (tangentL + tangentR) * 0.5f;

            if (avgTangent.lengthSq() > 0.001f) { 
                avgTangent.normalize();
                g_tanque->baseAngle = atan2(avgTangent.y, avgTangent.x);
            } else {
                g_tanque->baseAngle = 0.0f; 
            }
            g_tanque->topAngle = g_tanque->baseAngle; 
            g_tanque->forwardVector.set(cos(g_tanque->baseAngle), sin(g_tanque->baseAngle));
        }
        g_tanque->Render();
    }

    // textos na tela
    char scoreText[100]; 
    char powerText[100];
    if(!g_editorMode){
        CV::translate(0, 0);
        sprintf(scoreText, "Score: %d | Level: %d | Targets: %d/%d", g_playerScore, g_gameLevel, g_destroyedTargets, NUM_TARGETS);
        CV::color(1.0f, 1.0f, 1.0f);
        CV::text(10, 40, scoreText);

        sprintf(powerText, "PowerUp: %s", PowerUp::GetTypeName(g_storedPowerUp));
        CV::text(10, 60, powerText);

        CV::text(10, 20, "Modo de Jogo | A/D = Girar | 'E' = Editor | 'M1' = Tiro | 'M2' = Poder");

        // checa game over
        if (g_tanque->health <= 0) {
            CV::color(1.0f, 0.0f, 0.0f);
            char gameOverText[100];
            sprintf(gameOverText, "GAME OVER! Final Score: %d - Pressione 'E' para reiniciar!", g_playerScore);
            CV::text(screenWidth/2 - 180, screenHeight/2, gameOverText);
        }
   }

   Sleep(10); 
}

//funcao chamada toda vez que uma tecla for pressionada.
void keyboard(int key)
{

    switch(key)
    {
        case 27: // esc sai do jogo
    	    exit(0);
    	break;

        case 'a':
        case 'A':
            if (!g_editorMode) keyA_down = true;
            else if(g_track){
                g_track->addControlPoint(Vector2(mouseX, mouseY));
            }
        break;
        case 'd':
        case 'D':
            if (!g_editorMode) keyD_down = true;
            else if (g_track){
                g_track->removeControlPoint();
            }
        break;
        case 'e':
        case 'E':
            g_editorMode = !g_editorMode;
            if (!g_editorMode) {
              
                if(g_track){
                    g_track->deselectControlPoint();
                }
              
                resetTankToTrackStart(g_tanque, g_track);
              
                ResetGameState(g_tanque, g_track);
            } else {
                keyA_down = false;
                keyD_down = false;
            }
        break;

        case 's': 
        case 'S':
            if (g_editorMode && g_track) {
                g_track->switchActiveEditingCurve();
            }
        break;



   }
}

//funcao chamada toda vez que uma tecla for liberada
void keyboardUp(int key)
{
    switch(key)
    {
        case 'a':
        case 'A':
            keyA_down = false;
            break;
        case 'd':
        case 'D':
            keyD_down = false;
            break;
    }
}

//funcao para tratamento de mouse: cliques, movimentos e arrastos
void mouse(int button, int state, int wheel, int direction, int x, int y)
{
    mouseX = x; //guarda as coordenadas do mouse para utilizar em outras funcs
    mouseY = y;

    if (g_editorMode && g_track) { // verifica os arratos dos pontos de controle
        if (button == 0) {
            if (state == 0) { 
                g_mousePressed = true;
                if (!g_track->selectControlPoint(static_cast<float>(x), static_cast<float>(y))) {
                    
                }
            } else { 
                g_mousePressed = false;
            }
        }
    }
    else if (!g_editorMode && g_tanque) {
        if (button == 0 && state == 0) { // TIRO
            bool fired = g_tanque->FireProjectile();
        }
        else if (button == 2 && state == 0) { // PODER
            UsePowerUp(g_tanque, g_targets);
        }
    }

    if (g_editorMode && g_track && g_mousePressed && g_track->selectedPointIndex != -1) { // arrasto do ponto de controle
        g_track->moveSelectedControlPoint(static_cast<float>(x), static_cast<float>(y));
    }

}

int main(void)
{
    srand(static_cast<unsigned int>(time(NULL))); // randoms

    g_tanque = new Tanque(screenWidth / 4.0f, screenHeight / 2.0f, 0.7f, 0.02f);
    g_track = new BSplineTrack(true);

    // inicializa o tanque, os targets e um power-up
    resetTankToTrackStart(g_tanque, g_track);

    InitializeTargets(g_track);

    if (!g_powerUp.active) {
        SpawnPowerUp(g_track);
    }

    CV::init(&screenWidth, &screenHeight, "Gabriel 'Theft' Baggio VI - Tanque Edition");
    CV::run();
}
