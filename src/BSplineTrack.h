#ifndef BSPLINETRACK_H
#define BSPLINETRACK_H

#include <vector>
#include "Vector2.h" // Assuming Vector2.h provides x, y, basic ops, distSq, length, normalized
#include "gl_canvas2d.h" // For CV::line, CV::circleFill etc.
#include <cmath>      // For M_PI, fmod, floor, abs
#include <algorithm>  // For std::min/max

// Define M_PI if not available (common on Windows with MSVC)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class BSplineTrack {
public:
    std::vector<Vector2> controlPoints;
    float trackWidth;
    int degree; // Typically 3 for cubic B-Splines

    const int MIN_CONTROL_POINTS = 4; // Minimum for a cubic B-Spline loop
    const int MAX_CONTROL_POINTS = 20;
    const float CONTROL_POINT_DRAW_RADIUS = 8.0f;
    const float CONTROL_POINT_SELECT_RADIUS_SQ = 100.0f; // (10px)^2

    int selectedPointIndex;
    bool loop; // True if the track should form a closed loop

    BSplineTrack(float width, bool isLoop = true);

    void addControlPoint(const Vector2& p, int index = -1);
    bool removeControlPoint(int index = -1); // Removes selected or last if index is -1
    
    bool selectControlPoint(float mx, float my);
    void moveSelectedControlPoint(float mx, float my);
    void deselectControlPoint();

    void Render(bool editorMode);

    Vector2 getPointOnCurve(float t_global) const; // t_global from 0 to 1 for the entire curve length
    Vector2 getTangentOnCurve(float t_global) const; // Not normalized

private:
    // Helper for B-Spline calculation (cubic)
    Vector2 calculateBSplinePoint(float t, const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3) const;
    Vector2 calculateBSplineTangent(float t, const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3) const;
};

#endif // BSPLINETRACK_H
