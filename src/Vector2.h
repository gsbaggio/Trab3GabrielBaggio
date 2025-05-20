#ifndef __VECTOR_2_H__
#define __VECTOR_2_H__

#include <cmath> 
#include <cstdio> 


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

   float distSq(const Vector2& other) const
   {
       float dx = x - other.x;
       float dy = y - other.y;
       return dx * dx + dy * dy;
   }

   float length() const
   {
       return std::sqrt(x * x + y * y);
   }

   float lengthSq() const
   {
       return x * x + y * y;
   }

   Vector2 normalized() const
   {
       float l = length();
       if (l == 0.0f)
       {
           return Vector2(0.0f, 0.0f); 
       }
       return Vector2(x / l, y / l);
   }

   void normalize()
   {
       float norm = (float)sqrt(x*x + y*y);

       if(norm==0.0)
       {
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

   Vector2 operator * (float scalar) const
   {
       return Vector2(x * scalar, y * scalar);
   }

   friend Vector2 operator * (float scalar, const Vector2& vec)
   {
       return Vector2(vec.x * scalar, vec.y * scalar);
   }
};

#endif
