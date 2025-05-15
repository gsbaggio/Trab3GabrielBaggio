/*********************************************************************
// Canvas para desenho, criada sobre a API OpenGL. Nao eh necessario conhecimentos de OpenGL para usar.
//  Autor: Cesar Tadeu Pozzer
//         02/2025
//
//  Pode ser utilizada para fazer desenhos, animacoes, e jogos simples.
//  Tem tratamento de mouse e teclado
//  Estude o OpenGL antes de tentar compreender o arquivo gl_canvas.cpp
//
//  Versao 2.1
//
//  Instru��es:
//	  Para alterar a animacao, digite numeros entre 1 e 3
// *********************************************************************/

#include <GL/glut.h>
#include <GL/freeglut_ext.h> //callback da wheel do mouse.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "gl_canvas2d.h"

#include "Relogio.h"
#include "Tanque.h" 
#include "BSplineTrack.h"
#include "Target.h" // Add include for Target class
#include "Projectile.h" // Add include for Projectile class

void motion(int x, int y);
void resetTankToTrackStart(Tanque* tanque, BSplineTrack* track); // Forward declaration

//largura e altura inicial da tela . Alteram com o redimensionamento de tela.
int screenWidth = 1280, screenHeight = 720;

// Old demo objects - can be removed or commented out if Tanque is the primary focus
// Bola    *b = NULL;
// Relogio *r = NULL;
// Botao   *bt = NULL; //se a aplicacao tiver varios botoes, sugiro implementar um manager de botoes.
// int opcao  = 50;//variavel global para selecao do que sera exibido na canvas.

Tanque *g_tanque = NULL; // Global pointer for the tank
BSplineTrack *g_track = NULL; // Global pointer for the track

bool g_editorMode = false;
bool g_mousePressed = false;

int mouseX, mouseY; //variaveis globais do mouse para poder exibir dentro da render().

// Flags for tank rotation
bool keyA_down = false;
bool keyD_down = false;

// Constants for targets
const int NUM_TARGETS = 5;

// Global variables for game state
std::vector<Target> g_targets;
int g_playerScore = 0;

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

// Function to spawn a target at a random position
void SpawnTarget(BSplineTrack* track) {
    // Find an inactive target slot or the first active one if all are active
    int targetIndex = -1;
    for (size_t i = 0; i < g_targets.size(); i++) {
        if (!g_targets[i].active) {
            targetIndex = i;
            break;
        }
    }
    
    // If no inactive slot found and we have fewer than NUM_TARGETS, add a new one
    bool isNewTarget = false;
    if (targetIndex == -1 && g_targets.size() < NUM_TARGETS) {
        g_targets.emplace_back();
        targetIndex = g_targets.size() - 1;
        isNewTarget = true;
    }
    
    // If we found a slot to use or added a new one
    if (targetIndex >= 0 && targetIndex < static_cast<int>(g_targets.size())) {
        // Store previous position for existing targets (those being respawned)
        Vector2 previousPosition = g_targets[targetIndex].position;
        Vector2 position;
        
        if (isNewTarget) {
            // For completely new targets, just avoid the tank
            position = GenerateRandomTargetPosition(track, g_tanque->position, true);
        } else {
            // For respawning targets, avoid BOTH the tank AND the previous position
            position = GenerateRandomTargetPosition(track, g_tanque->position, true);
            
            // If we successfully found a position away from the tank,
            // make sure it's also away from previous position
            if (position.x != 0.0f || position.y != 0.0f) {
                if (position.distSq(previousPosition) < 150.0f * 150.0f) {
                    // If still too close to previous position, try again
                    Vector2 betterPosition = GenerateRandomTargetPosition(track, previousPosition, true);
                    if (betterPosition.x != 0.0f || betterPosition.y != 0.0f) {
                        position = betterPosition;
                    }
                }
            }
        }
        
        // Set the target's new position and activate it
        g_targets[targetIndex].position = position;
        g_targets[targetIndex].active = true;
        g_targets[targetIndex].health = g_targets[targetIndex].maxHealth; // Reset health when respawning
    }
}

// Function to initialize or reset all targets
void InitializeTargets(BSplineTrack* track) {
    g_targets.clear();
    
    // Create NUM_TARGETS targets
    for (int i = 0; i < NUM_TARGETS; i++) {
        SpawnTarget(track);
    }
}

// Function to reset the game state
void ResetGameState(Tanque* tanque, BSplineTrack* track) {
    g_playerScore = 0;
    
    // Reset tank health
    if (tanque) {
        tanque->health = tanque->maxHealth;
        tanque->isInvulnerable = false;
        tanque->invulnerabilityTimer = 0;
    }
    
    InitializeTargets(track);
    
    // Any additional reset logic goes here
}

// --- Comment out or remove old drawing functions if no longer needed ---
/*
void DesenhaSenoide()
{
   float x=0, y;
   CV::color(1);
   CV::translate(20, 200); //desenha o objeto a partir da coordenada (200, 200)
   for(float i=0; i < 68; i+=0.001)
   {
      y = sin(i)*50;
      CV::point(x, y);
      x+=0.01;
   }
   CV::translate(0, 0);
}

void DesenhaLinhaDegrade()
{
   Vector2 p;
   for(float i=0; i<350; i++)
   {
	  CV::color(i/200, i/200, i/200);
	  p.set(i+100, 240);
	  CV::point(p);
   }

   //desenha paleta de cores predefinidas na Canvas2D.
   for(int idx = 0; idx < 14; idx++)
   {
	  CV::color(idx);
      CV::translate(20 + idx*30, 100);
	  CV::rectFill(Vector2(0,0), Vector2(30, 30));
   }
   CV::translate(0, 0);
}
*/

void DrawMouseScreenCoords()
{
    char str[100];
    sprintf(str, "Mouse: (%d,%d)", mouseX, mouseY);
    CV::text(10,300, str);
    sprintf(str, "Screen: (%d,%d)", screenWidth, screenHeight);
    CV::text(10,320, str);
}

//funcao chamada continuamente. Deve-se controlar o que desenhar por meio de variaveis globais
//Todos os comandos para desenho na canvas devem ser chamados dentro da render().
//Deve-se manter essa funo com poucas linhas de codigo.
void render()
{
   CV::clear(0.5, 0.5, 0.5); // Clear screen with a background color
   
   // Draw player score at top-left (BEFORE translate to keep it fixed on screen)
   char scoreText[50];
   sprintf(scoreText, "Score: %d", g_playerScore); // Removed health percentage display
   CV::color(1.0f, 1.0f, 1.0f);
   CV::text(10, screenHeight - 40, scoreText);
   
   CV::text(10, screenHeight - 20, "Jogo de Tanque - Use A/D para girar, Mouse para mirar. 'E' para Editor.");

   if(!g_editorMode){
    CV::translate(-g_tanque->position.x + screenWidth/2, -g_tanque->position.y + screenHeight/2);
   }

   if (g_track) {
       g_track->Render(g_editorMode);
   }
   
   // Render targets
   if (!g_editorMode) {
       for (auto& target : g_targets) {
           target.Render();
       }
   }

   DrawMouseScreenCoords();

   // Update and Render Tank only if not in editor mode
   if (!g_editorMode && g_tanque) {
       g_tanque->Update(static_cast<float>(mouseX + g_tanque->position.x - screenWidth/2), 
                       static_cast<float>(mouseY + g_tanque->position.y - screenHeight/2), 
                       keyA_down, keyD_down, g_track);
       
       // Check for tank collision with targets - now catching the return value
       int destroyedTargetIndex = g_tanque->CheckTargetCollisions(g_targets);
       if (destroyedTargetIndex >= 0) {
           // Target was destroyed by tank collision - increase score and respawn
           g_playerScore++;
           
           // Spawn a new target to replace the destroyed one
           SpawnTarget(g_track);
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
                   g_playerScore++;
                   
                   // Spawn a new target to replace the destroyed one
                   SpawnTarget(g_track);
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
        // Optionally render tank statically in editor mode or hide it
        // g_tanque->Render(); // Render without update
   }

   // --- Remove or comment out old demo logic ---
   /*
   bt->Render();
   DesenhaLinhaDegrade();

   if( opcao == 49 ) //'1' -> relogio
   {
      r->anima();
   }
   if( opcao == '2' ) //50 -> bola
   {
      b->anima();
   }
   if( opcao == 51 ) //'3' -> senoide
   {
       DesenhaSenoide();
   }
   */

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
          break;
      case 'd':
      case 'D':
          if (!g_editorMode) keyD_down = true;
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

      case '+': // Add control point in editor mode
      case '=': // GLUT often uses '=' for '+' without shift
          if (g_editorMode && g_track) {
              g_track->addControlPoint(Vector2(mouseX, mouseY));
          }
          break;
      case '-': // Remove control point in editor mode
          if (g_editorMode && g_track) {
              // removeControlPoint now works on the active curve's selection or its last point
              g_track->removeControlPoint();
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

   // printf("\nmouse button: %d state: %d wheel: %d dir: %d x: %d y: %d", button, state, wheel, direction,  x, y);

   if (g_editorMode && g_track) {
       if (button == 0) { // Left mouse button
           if (state == 0) { // Pressed
               g_mousePressed = true;
               if (!g_track->selectControlPoint(static_cast<float>(x), static_cast<float>(y))) {
                   // If click was not on a point, deselect any selected point
                   // g_track->deselectControlPoint(); // Or keep selected for adding new points relative to it
               }
           } else { // Released
               g_mousePressed = false;
               // Optional: deselect point on release if not dragging, or keep selected
               // g_track->deselectControlPoint();
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

   CV::init(&screenWidth, &screenHeight, "Tanque B-Spline - Editor: E, Switch: S, Add/Remove: +/-");
   glutMotionFunc(motion); // Register mouse drag callback
   glutPassiveMotionFunc(passiveMotion); // Register mouse move callback
   CV::run();
}