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

#include "gl_canvas2d.h"

#include "Relogio.h"
#include "Tanque.h" // Include the new Tanque header
#include "BSplineTrack.h" // Include the BSplineTrack header

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
   // Calculate deltaTime for frame-rate independent movement
    // static float lastTime = 0.0f;
    // float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f; // Time in seconds
    // float deltaTime = currentTime - lastTime;
    // lastTime = currentTime;

    // // Avoid issues if deltaTime is zero (e.g., first frame or very fast calls)
    // if (deltaTime <= 0.0f || deltaTime > 0.2f) { // also cap deltaTime to avoid large jumps
    //     deltaTime = 1.0f / 60.0f; // Assume 60 FPS if deltaTime is invalid or too large
    // }

   CV::clear(0.5, 0.5, 0.5); // Clear screen with a background color
   CV::text(10, screenHeight - 20, "Jogo de Tanque - Use A/D para girar, Mouse para mirar. 'E' para Editor.");

   if(!g_editorMode){
    CV::translate(-g_tanque->position.x + screenWidth/2, -g_tanque->position.y + screenHeight/2);
   }

   if (g_track) {
       g_track->Render(g_editorMode);
   }

   DrawMouseScreenCoords();


   // Update and Render Tank only if not in editor mode
   if (!g_editorMode && g_tanque) {
       g_tanque->Update(static_cast<float>(mouseX + g_tanque->position.x - screenWidth/2), static_cast<float>(mouseY + g_tanque->position.y - screenHeight/2), keyA_down, keyD_down, g_track);
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
              // Reset/reinitialize game state if needed when exiting editor
              if(g_track) g_track->deselectControlPoint();
              // Reset tank to track start when exiting editor mode
              resetTankToTrackStart(g_tanque, g_track);
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
   g_tanque = new Tanque(screenWidth / 4.0f, screenHeight / 2.0f, 0.7f, 0.02f); // Adjusted rotation rate from 0.05f to 0.1f
   g_track = new BSplineTrack(true); // Track is a loop, trackWidth removed

   // Reset tank to the start of the track after creation
   resetTankToTrackStart(g_tanque, g_track);

   CV::init(&screenWidth, &screenHeight, "Tanque B-Spline - Editor: E, Switch: S, Add/Remove: +/-");
   glutMotionFunc(motion); // Register mouse drag callback
   glutPassiveMotionFunc(passiveMotion); // Register mouse move callback
   CV::run();
}
