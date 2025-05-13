#ifndef __BOLA_H__
#define __BOLA_H__

#include "gl_canvas2d.h"

class Bola{
  float altura, vel;
  bool caindo;
  float posicao;
  float r, g, b; //cor da bola
  float direcao;

public:
  Bola()
  {
     caindo  = true;
     vel     = 0.001;
     altura  = 410;
     posicao = 200;
     r = 1;
     g = 0;
     b = 0;

     direcao = 0;
  }

  void anima()
  {
      posicao += direcao;

      r = altura/400.0;
      CV::clear(0,0,0);
      CV::color(r, 1-r, b);
      if( altura > 10 && caindo == true ) //caindo
      {
         vel += 0.001;
         altura -= vel;
      }
      else if( altura < 10 && caindo == true ) //bate no chao
      {
         vel = -vel;
         altura -= vel;
         caindo = false;
      }
      if( altura < 400 && caindo == false ) //subindo
      {
         vel += 0.001;
         altura-= vel;
      }
      else if( vel < 0 && caindo == false )
      {
         vel    = 0.001;
         caindo = true;
      }
      CV::circleFill(posicao, (int)altura, 30, 15);
  }

  void move(int delta) //direita ou esquerda.
  {
      if( delta > 0 )
      {
          direcao = 0.04;
      }


      if( delta < 0 )
      {
          direcao = -0.04;
      }
  }
};

#endif
