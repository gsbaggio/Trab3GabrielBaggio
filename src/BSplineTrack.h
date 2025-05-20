#ifndef BSPLINETRACK_H
#define BSPLINETRACK_H

#include <vector>
#include "Vector2.h" 
#include "gl_canvas2d.h" 
#include <cmath>     
#include <algorithm>  
#include <cstdio>    
#include <string>
#include <cfloat>  

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

enum class CurveSide {
    None = -1,
    Left = 0,
    Right = 1
};

struct ClosestPointInfo {
    Vector2 point;
    float t_global;     // parametro t
    float distance;     // distância do ponto de consulta ao ponto mais próximo na curva (renomeado de distanceSquared)
    Vector2 normal;     // normal da curva no ponto mais próximo
    int segmentIndex;   // indice do segmento onde o ponto mais próximo está
    bool isValid;     

    ClosestPointInfo() : point(), t_global(0.0f), distance(FLT_MAX), normal(), segmentIndex(-1), isValid(false) {}
};

class BSplineTrack {
public:
    std::vector<Vector2> controlPointsLeft;
    std::vector<Vector2> controlPointsRight;
    
    int degree;
    int selectedPointIndex;
    bool loop; 
    CurveSide activeEditingCurve;
    CurveSide selectedCurve;

    // constantes
    const int MIN_CONTROL_POINTS_PER_CURVE = 4; 
    const int MAX_CONTROL_POINTS = 20;
    const float CONTROL_POINT_DRAW_RADIUS = 8.0f;
    const float CONTROL_POINT_SELECT_RADIUS_SQ = 100.0f; // distancia pra clicar num ponto de controle


    BSplineTrack(bool isLoop = true);

    void addControlPoint(const Vector2& p, int index = -1);
    bool removeControlPoint(int index = -1);

    bool selectControlPoint(float mx, float my);
    void moveSelectedControlPoint(float mx, float my);
    void deselectControlPoint();
    void switchActiveEditingCurve();

    void Render(bool editorMode);

    Vector2 getPointOnCurve(float t_global, CurveSide side) const;
    Vector2 getTangentOnCurve(float t_global, CurveSide side) const; 

    ClosestPointInfo findClosestPointOnCurve(const Vector2& queryPoint, CurveSide side) const;

private:
    Vector2 calculateBSplinePoint(float t, const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3) const;
    Vector2 calculateBSplineTangent(float t, const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3) const;
    
    void renderCurve(const std::vector<Vector2>& points, float r_color, float g_color, float b_color) const;

    Vector2 getPointOnCurveInternal(float t_global, const std::vector<Vector2>& points_list) const;
    Vector2 getTangentOnCurveInternal(float t_global, const std::vector<Vector2>& points_list) const;
};

#endif 
