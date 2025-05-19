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
#include "PowerUp.h" // New include for power-ups

void motion(int x, int y);
void resetTankToTrackStart(Tanque* tanque, BSplineTrack* track); // Forward declaration

//largura e altura inicial da tela . Alteram com o redimensionamento de tela.
int screenWidth = 1280, screenHeight = 720;

Tanque *g_tanque = NULL;
BSplineTrack *g_track = NULL;

bool g_editorMode = false;
bool g_mousePressed = false;

int mouseX, mouseY; //variaveis globais do mouse para poder exibir dentro da render().

// flags para guardar rotacao do tanque
bool keyA_down = false;
bool keyD_down = false;

// Constants for targets
const int NUM_TARGETS = 5;

// Global variables for game state
std::vector<Target> g_targets;
int g_playerScore = 0;
int g_gameLevel = 1;  // Current game level
int g_destroyedTargets = 0;  // Counter for destroyed targets in current level

// Global variables for power-ups
PowerUp g_powerUp;
PowerUpType g_storedPowerUp = PowerUpType::None;

// Function declarations
void SpawnPowerUp(BSplineTrack* track); // Add this forward declaration

// Modified function to avoid respawning near the tank or previous position
Vector2 GenerateRandomTargetPosition(BSplineTrack* track, const Vector2& avoidPosition = Vector2(0,0), bool checkAvoidance = false) {
    const int MAX_ATTEMPTS = 100;
    const float MIN_SAFE_DISTANCE_SQ = 150.0f * 150.0f; // Min 150 pixels away from tank/previous position

    Vector2 position;
    int attempts = 0;

    do {
        attempts++;
        if (attempts > MAX_ATTEMPTS) {
            // If we've tried too many times without success, return the last generated position
            printf("Warning: Could not find ideal target position after %d attempts\n", MAX_ATTEMPTS);
            return position;
        }

        // Generate a random parameter value along the track (0.0 to 1.0)
        float t = static_cast<float>(rand()) / RAND_MAX;

        // Get points on both left and right curves at this parameter
        Vector2 leftPoint = track->getPointOnCurve(t, CurveSide::Left);
        Vector2 rightPoint = track->getPointOnCurve(t, CurveSide::Right);

        // Calculate a point between the curves with a random interpolation
        float interpFactor = 0.2f + 0.6f * static_cast<float>(rand()) / RAND_MAX; // 0.2 to 0.8
        position = leftPoint + (rightPoint - leftPoint) * interpFactor;

        // Check distance to tank or previous position (if needed)
        if (checkAvoidance) {
            float distSq = position.distSq(avoidPosition);
            if (distSq < MIN_SAFE_DISTANCE_SQ) {
                continue; // Too close, try a different position
            }
        }

        // Check distance to other active targets
        bool tooCloseToOtherTargets = false;
        for (const auto& target : g_targets) {
            if (target.active) {
                float distSq = position.distSq(target.position);
                if (distSq < 50.0f * 50.0f) { // Minimum distance of 50 pixels
                    tooCloseToOtherTargets = true;
                    break;
                }
            }
        }

        if (!tooCloseToOtherTargets) {
            return position; // Found a good position
        }

    } while (attempts <= MAX_ATTEMPTS);

    // Fallback position if we can't find a good spot
    return Vector2(0, 0);
}

// Modified function to initialize or reset all targets with level-scaled health
void InitializeTargets(BSplineTrack* track) {
    g_targets.clear();
    g_destroyedTargets = 0;  // Reset the destroyed targets counter

    // Create NUM_TARGETS targets
    for (int i = 0; i < NUM_TARGETS; i++) {
        // Find a good position for the target
        Vector2 position = GenerateRandomTargetPosition(track, g_tanque->position, true);

        // Determine target type based on game level and position in array
        TargetType targetType = TargetType::Basic; // Default is basic

        // Higher levels have more varied enemies
        float rand_val = (float)rand() / RAND_MAX;

        if (g_gameLevel >= 3 && i == NUM_TARGETS - 1) {
            // From level 3, last enemy is always a Star
            targetType = TargetType::Star;
        }
        else if (g_gameLevel >= 2 && i == NUM_TARGETS - 2) {
            // From level 2, second-to-last enemy is always a Shooter
            targetType = TargetType::Shooter;
        }
        else if (g_gameLevel >= 4 && rand_val < 0.3f) {
            // From level 4, 30% chance for any other enemy to be a Star
            targetType = TargetType::Star;
        }
        else if (g_gameLevel >= 2 && rand_val < 0.4f) {
            // From level 2, 40% chance for remaining enemies to be Shooters
            targetType = TargetType::Shooter;
        }

        // Create the target with health scaled to current level
        Target target(position, targetType);
        target.active = true;

        // Scale health with game level: base health (2) + additional health based on level
        target.maxHealth = 2 + (g_gameLevel - 1);

        // Shooter types get +1 health
        if (target.type == TargetType::Shooter) {
            target.maxHealth += 1;
        }
        // Star types get +2 health and custom movement properties
        else if (target.type == TargetType::Star) {
            target.maxHealth += 2;

            // Stars move more slowly but still scale with level
            target.moveSpeed = 0.5f + (g_gameLevel * 0.1f);  // Much slower base speed

            // Stars detect from further away at higher levels
            target.detectionRadius = 150.0f + (g_gameLevel * 25.0f);

            // Rotation speed increases slightly with level
            target.rotationSpeed = 0.05f + (g_gameLevel - 1) * 0.01f;
        }

        target.health = target.maxHealth;

        g_targets.push_back(target);
    }

    // Count special targets for log message
    int shooterCount = 0;
    int starCount = 0;
    for (const auto& target : g_targets) {
        if (target.type == TargetType::Shooter) shooterCount++;
        else if (target.type == TargetType::Star) starCount++;
    }

    printf("Level %d started with %d targets (%d shooters, %d stars)\n",
           g_gameLevel, NUM_TARGETS, shooterCount, starCount);

    // Always spawn a power-up when initializing targets if none exists
    SpawnPowerUp(track);
}

// Function to spawn a random power-up
void SpawnPowerUp(BSplineTrack* track) {
    // Only spawn if no active power-up exists and no stored power-up
    if (g_powerUp.active || g_storedPowerUp != PowerUpType::None) return;

    if (!track || track->controlPointsLeft.size() < 4 || track->controlPointsRight.size() < 4) {
        printf("Warning: Cannot spawn power-up, track is not properly initialized\n");
        return;
    }

    // Generate a random position on the track
    Vector2 position = GenerateRandomTargetPosition(track, g_tanque->position, true);

    // Choose a random power-up type
    int randType = rand() % 3 + 1; // 1-3
    PowerUpType type = static_cast<PowerUpType>(randType);

    // Create and activate the power-up
    g_powerUp = PowerUp(position, type);
    printf("PowerUp spawned: %s at position (%.1f, %.1f)\n",
           PowerUp::GetTypeName(type), position.x, position.y);
}

// Function to reset the game state
void ResetGameState(Tanque* tanque, BSplineTrack* track) {
    g_playerScore = 0;
    g_gameLevel = 1;  // Reset level to 1
    g_destroyedTargets = 0;  // Reset destroyed targets counter

    // Reset tank health
    if (tanque) {
        tanque->health = tanque->maxHealth;
        tanque->isInvulnerable = false;
        tanque->invulnerabilityTimer = 0;
    }

    InitializeTargets(track);

    // Reset power-ups
    g_powerUp.active = false;
    g_storedPowerUp = PowerUpType::None;

    // Spawn a new power-up
    SpawnPowerUp(track);
}

// Implement UsePowerUp function
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

            // Update score and destroyed targets count for each destroyed target
            g_playerScore += targetsDestroyed * 100;
            g_destroyedTargets += targetsDestroyed;

            // Check if all targets have been destroyed
            if (g_destroyedTargets >= NUM_TARGETS) {
                // Level complete - advance to next level
                g_gameLevel++;
                InitializeTargets(g_track);
            }
            break;
        }

        default:
            break;
    }

    // Clear the stored power-up after use
    g_storedPowerUp = PowerUpType::None;
}

//funcao chamada continuamente. Deve-se controlar o que desenhar por meio de variaveis globais
//Todos os comandos para desenho na canvas devem ser chamados dentro da render().
//Deve-se manter essa funo com poucas linhas de codigo.
void render()
{
   CV::clear(0.25f, 0.25f, 0.3f); // Changed to dark asphalt gray (former road color)

   if(!g_editorMode){
    CV::translate(-g_tanque->position.x + screenWidth/2, -g_tanque->position.y + screenHeight/2);
   }

   if (g_track) {
       g_track->Render(g_editorMode);
   }

   // Update and render power-up - MOVED HERE for proper rendering order
   if (!g_editorMode) {
       g_powerUp.Update();
       g_powerUp.Render();
   }

   // Render targets
   if (!g_editorMode) {
       for (auto& target : g_targets) {
           target.Render();
       }
   }


   // Update and Render Tank only if not in editor mode
   if (!g_editorMode && g_tanque) {
       g_tanque->Update(static_cast<float>(mouseX + g_tanque->position.x - screenWidth/2),
                       static_cast<float>(mouseY + g_tanque->position.y - screenHeight/2),
                       keyA_down, keyD_down, g_track);

       // Update all targets
       for (auto& target : g_targets) {
           target.Update(g_tanque->position, g_track);
       }

       // Check if tank collects power-up
       if (g_powerUp.active && g_powerUp.CheckCollection(g_tanque->position, g_tanque->baseWidth/2.0f)) {
           // Store the power-up type and deactivate the power-up
           g_storedPowerUp = g_powerUp.type;
           g_powerUp.active = false;

           printf("PowerUp collected: %s\n", PowerUp::GetTypeName(g_storedPowerUp));
       }

       // Check for shielded tank when taking damage
       if (!g_tanque->isInvulnerable) {
           // Check for star targets hitting the tank
           for (size_t i = 0; i < g_targets.size(); i++) {
               Target& target = g_targets[i];
               if (target.active && target.type == TargetType::Star) {
                   float dx = target.position.x - g_tanque->position.x;
                   float dy = target.position.y - g_tanque->position.y;
                   float distSq = dx*dx + dy*dy;
                   float combinedRadius = g_tanque->baseWidth/2.0f + target.radius;

                   if (distSq < combinedRadius * combinedRadius) {
                       // Check if shield is active
                       if (g_tanque->hasShield) {
                           // Shield blocks damage
                           g_tanque->hasShield = false; // Consume the shield
                           // Make tank temporarily invulnerable and flash
                           g_tanque->isInvulnerable = true;
                           g_tanque->isShieldInvulnerable = true; // Set flag for shield invulnerability
                           g_tanque->invulnerabilityTimer = g_tanque->INVULNERABILITY_FRAMES;
                           printf("Shield blocked damage from star enemy!\n");
                           
                           // Kill the star target (always for shield)
                           target.active = false;
                       } else {
                           // No shield, apply normal damage
                           int damage = g_tanque->maxHealth / 2;
                           g_tanque->health -= damage;
                           if (g_tanque->health < 0) g_tanque->health = 0;
                           
                           // Make tank temporarily invulnerable
                           g_tanque->isInvulnerable = true;
                           g_tanque->isShieldInvulnerable = false; // Regular damage
                           g_tanque->invulnerabilityTimer = g_tanque->INVULNERABILITY_FRAMES;
                           
                           printf("Tank hit by star enemy! Damage: %d, Health: %d/%d\n", 
                                damage, g_tanque->health, g_tanque->maxHealth);
                           
                           // Kill the star target
                           target.active = false;
                       }
                       
                       // Increase score and destroyed targets count
                       g_playerScore += 150;  // More points for star enemies
                       g_destroyedTargets++;

                       // Check if all targets have been destroyed
                       if (g_destroyedTargets >= NUM_TARGETS) {
                           // Level complete - advance to next level
                           g_gameLevel++;
                           InitializeTargets(g_track);
                       }
                   }
               }
           }
       }
       
       // Check for enemy projectiles hitting the tank
       if (!g_tanque->isInvulnerable) {
           for (auto& target : g_targets) {
               if (target.active && target.type == TargetType::Shooter) {
                   for (auto& proj : target.projectiles) {
                       if (proj.active) {
                           // Simple circular collision with tank center
                           float dx = proj.position.x - g_tanque->position.x;
                           float dy = proj.position.y - g_tanque->position.y;
                           float distSq = dx*dx + dy*dy;
                           float combinedRadius = g_tanque->baseWidth/2.0f + proj.radius;

                           if (distSq < combinedRadius * combinedRadius) {
                               // Tank hit by enemy projectile
                               proj.active = false;

                               // Check if shield is active
                               if (g_tanque->hasShield) {
                                   // Shield blocks damage
                                   g_tanque->hasShield = false; // Consume the shield
                                   // Make tank temporarily invulnerable and flash
                                   g_tanque->isInvulnerable = true;
                                   g_tanque->isShieldInvulnerable = true; // Set flag for shield invulnerability
                                   g_tanque->invulnerabilityTimer = g_tanque->INVULNERABILITY_FRAMES;
                                   printf("Shield blocked damage from enemy projectile!\n");
                               } else {
                                   // Apply damage to tank
                                   int damage = g_tanque->maxHealth / 8; // 12.5% damage per hit
                                   g_tanque->health -= damage;
                                   if (g_tanque->health < 0) g_tanque->health = 0;
                                   
                                   // Make tank temporarily invulnerable
                                   g_tanque->isInvulnerable = true;
                                   g_tanque->isShieldInvulnerable = false; // Regular damage
                                   g_tanque->invulnerabilityTimer = g_tanque->INVULNERABILITY_FRAMES;
                                   
                                   printf("Tank hit by enemy projectile! Health: %d/%d\n", 
                                          g_tanque->health, g_tanque->maxHealth);
                               }
                           }
                       }
                   }
               }
           }
       }

       // Check for tank collision with targets - now catching the return value
       int destroyedTargetIndex = g_tanque->CheckTargetCollisions(g_targets);
       if (destroyedTargetIndex >= 0) {
           // Target was destroyed by tank collision - increase score
           g_playerScore += 100;
           g_destroyedTargets++;

           // Check if all targets have been destroyed
           if (g_destroyedTargets >= NUM_TARGETS) {
               // Level complete - advance to next level
               g_gameLevel++;
               InitializeTargets(g_track);  // Spawn new wave with increased health
           }
       }

       // Check for projectile-target collisions
       int hitTargetIndex = -1;
       int hitProjectileIndex = -1;
       if (g_tanque->CheckAllProjectilesAgainstTargets(g_targets, hitTargetIndex, hitProjectileIndex)) {
           // Apply damage to target when hit by projectile
           if (hitTargetIndex >= 0 && hitTargetIndex < static_cast<int>(g_targets.size())) {
               // Apply damage to the target
               g_targets[hitTargetIndex].TakeDamage(1);

               // If target was destroyed (health reached 0)
               if (!g_targets[hitTargetIndex].active) {
                   g_playerScore += 100;
                   g_destroyedTargets++;

                   // Check if all targets have been destroyed
                   if (g_destroyedTargets >= NUM_TARGETS) {
                       // Level complete - advance to next level
                       g_gameLevel++;
                       InitializeTargets(g_track);  // Spawn new wave with increased health
                   }
               }
           }

           if (hitProjectileIndex >= 0 && hitProjectileIndex < static_cast<int>(g_tanque->projectiles.size())) {
               g_tanque->projectiles[hitProjectileIndex].active = false;
           }
       }

       // Check for game over condition
       if (g_tanque->health <= 0) {
           CV::color(1.0f, 0.0f, 0.0f);
           char gameOverText[100];
           sprintf(gameOverText, "GAME OVER! Final Score: %d - Press 'E' to restart", g_playerScore);

           // Need to undo the translation for game over text to show on screen
           CV::translate(g_tanque->position.x - screenWidth/2, g_tanque->position.y - screenHeight/2);
           CV::text(screenWidth/2 - 180, screenHeight/2, gameOverText);
           CV::translate(-g_tanque->position.x + screenWidth/2, -g_tanque->position.y + screenHeight/2);
       }

       g_tanque->Render();
   } else if (g_editorMode && g_tanque) {
       // In editor mode, show the tank at its potential spawn position and orientation.
       // This position is derived from the start of the track.
       if (g_track && g_track->controlPointsLeft.size() >= g_track->MIN_CONTROL_POINTS_PER_CURVE && g_track->controlPointsRight.size() >= g_track->MIN_CONTROL_POINTS_PER_CURVE) {
           Vector2 pL = g_track->getPointOnCurve(0.0f, CurveSide::Left);
           Vector2 pR = g_track->getPointOnCurve(0.0f, CurveSide::Right);
           g_tanque->position = (pL + pR) * 0.5f; // Center of track start

           Vector2 tangentL = g_track->getTangentOnCurve(0.0f, CurveSide::Left);
           Vector2 tangentR = g_track->getTangentOnCurve(0.0f, CurveSide::Right);
           Vector2 avgTangent = (tangentL + tangentR) * 0.5f;

           if (avgTangent.lengthSq() > 0.001f) { // Check if tangent is reasonably valid
               avgTangent.normalize();
               g_tanque->baseAngle = atan2(avgTangent.y, avgTangent.x);
           } else {
               g_tanque->baseAngle = 0.0f; // Default orientation if tangent is not clear
           }
           g_tanque->topAngle = g_tanque->baseAngle; // Align turret with base
           g_tanque->forwardVector.set(cos(g_tanque->baseAngle), sin(g_tanque->baseAngle));
       }
       g_tanque->Render(); // Render the tank statically
   }


   // Draw player score at top-left (BEFORE translate to keep it fixed on screen)
   char scoreText[100]; // Increased buffer size to accommodate level information
   char powerText[100];
   if(!g_editorMode){
    CV::translate(0, 0);
    sprintf(scoreText, "Score: %d | Level: %d | Targets: %d/%d",
            g_playerScore, g_gameLevel, g_destroyedTargets, NUM_TARGETS);
    CV::color(1.0f, 1.0f, 1.0f);
    CV::text(10, 40, scoreText);

    // Display stored power-up info
    sprintf(powerText, "PowerUp: %s", PowerUp::GetTypeName(g_storedPowerUp));
    CV::text(10, 60, powerText);

    // Display shield status if active
    if (g_tanque && g_tanque->hasShield) {
        CV::color(0.3f, 0.3f, 1.0f);
        CV::text(10, 80, "Shield Active");
    }

    CV::text(10, 20, "Modo de Jogo | A/D = Girar | 'E' = Editor | 'M1' = Tiro | 'M2' = Poder");
   }

   Sleep(10); // This introduces a fixed delay
   glutPostRedisplay(); // Request a redraw for the next frame
}

//funcao chamada toda vez que uma tecla for pressionada.
void keyboard(int key)
{
   printf("\nTecla: %d" , key);

   switch(key)
   {
      case 27: // ESC
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
              // Reset game state when exiting editor
              if(g_track) g_track->deselectControlPoint();
              // Reset tank to track start when exiting editor mode
              resetTankToTrackStart(g_tanque, g_track);
              // Reset score, tank health, and targets
              ResetGameState(g_tanque, g_track);
          } else {
              keyA_down = false;
              keyD_down = false;
          }
          printf("Editor mode: %s\n", g_editorMode ? "ON" : "OFF");
          break;

      case 's': // Switch active curve for editing
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
   printf("\nLiberou: %d" , key);
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
   mouseX = x; //guarda as coordenadas do mouse para exibir dentro da render()
   mouseY = y;



   if (g_editorMode && g_track) {
       if (button == 0) { // Left mouse button
           if (state == 0) { // Pressed
               g_mousePressed = true;

           } else { // Released
               g_mousePressed = false;

           }
       }
   }
   else if (!g_editorMode && g_tanque) {
       if (button == 0 && state == 0) { // Left mouse button pressed
           // Fire projectile using the tank's current turret angle (no teleporting)
           bool fired = g_tanque->FireProjectile();
           if (fired) {
               printf("Tank fired projectile!\n");
           }
       }
       else if (button == 2 && state == 0) { // Right mouse button pressed
            // Use stored power-up
            UsePowerUp(g_tanque, g_targets);
        }
   }

   if (g_editorMode && g_track && g_mousePressed && g_track->selectedPointIndex != -1) {
       motion(static_cast<int>(x), static_cast<int>(y)); // Call motion to update position
   }

}

// Function for mouse motion when a button is pressed (dragging)
void motion(int x, int y)
{
    mouseX = x;
    mouseY = y;

    if (g_editorMode && g_track && g_mousePressed && g_track->selectedPointIndex != -1) {
        g_track->moveSelectedControlPoint(static_cast<float>(x), static_cast<float>(y));
    }
    //glutPostRedisplay(); // Request redraw
}

// Function for mouse motion when no buttons are pressed (passive)
void passiveMotion(int x, int y)
{
    mouseX = x;
    mouseY = y;
    //glutPostRedisplay(); // Request redraw if needed for hover effects etc.
}

// New function to reset tank position and orientation based on track
void resetTankToTrackStart(Tanque* tanque, BSplineTrack* track) {
    if (!tanque || !track) {
        printf("Error: Tank or Track is null in resetTankToTrackStart.\\\\n");
        return;
    }

    Vector2 start_point_left, start_point_right;
    bool position_calculated = false;

    // Try to get points on the actual spline curves at t=0
    if (track->controlPointsLeft.size() >= track->MIN_CONTROL_POINTS_PER_CURVE &&
        track->controlPointsRight.size() >= track->MIN_CONTROL_POINTS_PER_CURVE) {

        start_point_left = track->getPointOnCurve(0.0f, CurveSide::Left);
        start_point_right = track->getPointOnCurve(0.0f, CurveSide::Right);

        tanque->position.set((start_point_left.x + start_point_right.x) / 2.0f,
                             (start_point_left.y + start_point_right.y) / 2.0f);
        position_calculated = true;
        printf("Tank positioned at midpoint of initial B-Spline curve points.\\\\n");

    } else {
        // Fallback to using the first control points if splines are not yet defined
        printf("Warning: Not enough control points for full B-Spline definition at track start (L:%zu, R:%zu, MinPerCurve:%d).\\\\n",
               track->controlPointsLeft.size(), track->controlPointsRight.size(), track->MIN_CONTROL_POINTS_PER_CURVE);
        printf("Attempting to position tank at midpoint of L0/R0 control points as fallback.\\\\n");

        if (!track->controlPointsLeft.empty() && !track->controlPointsRight.empty()) {
            start_point_left = track->controlPointsLeft[0];
            start_point_right = track->controlPointsRight[0];
            tanque->position.set((start_point_left.x + start_point_right.x) / 2.0f,
                                 (start_point_left.y + start_point_right.y) / 2.0f);
            position_calculated = true;
        } else {
            printf("Error: Fallback L0/R0 control points are also missing. Tank cannot be positioned.\\\\n");
            return;
        }
    }

    // Orientation calculation
    if (track->controlPointsLeft.size() >= track->MIN_CONTROL_POINTS_PER_CURVE &&
        track->controlPointsRight.size() >= track->MIN_CONTROL_POINTS_PER_CURVE) {

        Vector2 tangent_left = track->getTangentOnCurve(0.0f, CurveSide::Left);
        Vector2 tangent_right = track->getTangentOnCurve(0.0f, CurveSide::Right);

        Vector2 final_tangent;
        bool tangent_calculated = false;

        Vector2 norm_tangent_left = tangent_left.normalized();
        Vector2 norm_tangent_right = tangent_right.normalized();

        if (norm_tangent_left.lengthSq() > 0.0001f && norm_tangent_right.lengthSq() > 0.0001f) {
            final_tangent = (norm_tangent_left + norm_tangent_right).normalized();
            if (final_tangent.lengthSq() > 0.0001f) {
                tangent_calculated = true;
            }
        } else if (norm_tangent_left.lengthSq() > 0.0001f) {
            final_tangent = norm_tangent_left;
            tangent_calculated = true;
        } else if (norm_tangent_right.lengthSq() > 0.0001f) {
            final_tangent = norm_tangent_right;
            tangent_calculated = true;
        }

        if (tangent_calculated) {
            tanque->baseAngle = atan2(final_tangent.y, final_tangent.x);
        } else {
            tanque->baseAngle = 0.0f;
            printf("Warning: Could not determine track orientation from tangents for tank. Defaulting angle.\\\\n");
        }
    } else {
        tanque->baseAngle = 0.0f;
        printf("Warning: Not enough control points for tangent calculation in resetTankToTrackStart. Defaulting angle.\\\\n");
    }

    tanque->forwardVector.set(cos(tanque->baseAngle), sin(tanque->baseAngle));

    if (position_calculated) {
        printf("Tank (re)positioned to (%.2f, %.2f), angle: %.2f rad\\\\n",
               tanque->position.x, tanque->position.y, tanque->baseAngle);
    } else {
        // This case should ideally be caught by the return earlier if L0/R0 are also missing.
        printf("Error: Tank position could not be calculated in resetTankToTrackStart.\\\\n");
    }
}


int main(void)
{
   srand(static_cast<unsigned int>(time(NULL))); // Initialize random seed

   g_tanque = new Tanque(screenWidth / 4.0f, screenHeight / 2.0f, 0.7f, 0.02f);
   g_track = new BSplineTrack(true);

   // Reset tank to the start of the track after creation
   resetTankToTrackStart(g_tanque, g_track);

   // Initialize targets
   InitializeTargets(g_track);

   // Make sure a power-up spawns at start
   if (!g_powerUp.active) {
       SpawnPowerUp(g_track);
   }

   CV::init(&screenWidth, &screenHeight, "Tanque B-Spline - Editor: E, Switch: S, Add/Remove: +/-");
   glutMotionFunc(motion); // Register mouse drag callback
   glutPassiveMotionFunc(passiveMotion); // Register mouse move callback
   CV::run();
}
