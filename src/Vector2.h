#ifndef __VECTOR_2_H__
#define __VECTOR_2_H__

#include <cmath> // Include for sqrt, etc.
#include <cstdio> // Include for printf, if you keep it in normalize


class Vector2
{
public:
   float x, y;

   Vector2()
   {
      x = y = 0;
   }

   Vector2(float _x, float _y)
   {
       x = _x;
       y = _y;
   }

   void set(float _x, float _y)
   {
       x = _x;
       y = _y;
   }

   // Calculates the squared distance to another vector
   float distSq(const Vector2& other) const
   {
       float dx = x - other.x;
       float dy = y - other.y;
       return dx * dx + dy * dy;
   }

   // Calculates the length (magnitude) of the vector
   float length() const
   {
       return std::sqrt(x * x + y * y);
   }

   // Calculates the squared length (squared magnitude) of the vector
   float lengthSq() const
   {
       return x * x + y * y;
   }

   // Returns a new normalized version of this vector
   Vector2 normalized() const
   {
       float l = length();
       if (l == 0.0f) // Or use a small epsilon
       {
           // Depending on use case, could return (0,0), (1,0) or throw error
           // printf("\n\nNormalized::Divisao por zero");
           return Vector2(0.0f, 0.0f); // Return zero vector if length is zero
       }
       return Vector2(x / l, y / l);
   }

   // Normalizes this vector in-place
   void normalize()
   {
       float norm = (float)sqrt(x*x + y*y);

       if(norm==0.0)
       {
          printf("\n\nNormalize::Divisao por zero");
          x = 1;
          y = 1;
          return;
       }
       x /= norm;
       y /= norm;
   }

   Vector2 operator - (const Vector2& v)
   {
        Vector2 aux( x - v.x, y - v.y);
        return( aux );
   }

   Vector2 operator + (const Vector2& v)
   {
       Vector2 aux( x + v.x, y + v.y);
       return( aux );
   }

   // Scalar multiplication
   Vector2 operator * (float scalar) const
   {
       return Vector2(x * scalar, y * scalar);
   }

   // Optional: Friend function for scalar * vector
   friend Vector2 operator * (float scalar, const Vector2& vec)
   {
       return Vector2(vec.x * scalar, vec.y * scalar);
   }

   //Adicionem os demais overloads de operadores aqui.


};

#endif
