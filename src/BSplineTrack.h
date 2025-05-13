#ifndef BSPLINETRACK_H
#define BSPLINETRACK_H

#include <vector>
#include "Vector2.h" // Assuming Vector2.h provides x, y, basic ops, distSq, length, normalized
#include "gl_canvas2d.h" // For CV::line, CV::circleFill etc.
#include <cmath>      // For M_PI, fmod, floor, abs
#include <algorithm>  // For std::min/max
#include <cstdio>    // For sprintf in Render
#include <string> // For std::string
#include <cfloat>  // Required for FLT_MAX

// Define M_PI if not available (common on Windows with MSVC)
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
    float t_global;     // Global parameter t (0-1) along the entire curve
    float distance;     // Distance from query point to closest point on curve (renamed from distanceSquared)
    Vector2 normal;     // Normal of the curve at the closest point
    int segmentIndex;   // Index of the segment where the closest point lies (added)
    bool isValid;       // Is the information valid? (added)

    // Default constructor to initialize members
    ClosestPointInfo() : point(), t_global(0.0f), distance(FLT_MAX), normal(), segmentIndex(-1), isValid(false) {}
};

class BSplineTrack {
public:
    std::vector<Vector2> controlPointsLeft;
    std::vector<Vector2> controlPointsRight;
    
    // Reordered to match typical initializer list order and help with -Wreorder warnings
    int degree;
    int selectedPointIndex;
    bool loop; // Moved up
    CurveSide activeEditingCurve;
    CurveSide selectedCurve;

    // Constants
    const int MIN_CONTROL_POINTS = 4; 
    const int MIN_CONTROL_POINTS_PER_CURVE = 4; 
    const int MAX_CONTROL_POINTS = 20;
    const float CONTROL_POINT_DRAW_RADIUS = 8.0f;
    const float CONTROL_POINT_SELECT_RADIUS_SQ = 100.0f; // (10px)^2


    BSplineTrack(bool isLoop = true); // Removed trackWidth

    void addControlPoint(const Vector2& p, int index = -1); // Adds to activeEditingCurve, added index
    bool removeControlPoint(int index = -1); // Removes from activeEditingCurve, added index
    
    bool selectControlPoint(float mx, float my); // Selects a point from either curve
    void moveSelectedControlPoint(float mx, float my);
    void deselectControlPoint();
    void switchActiveEditingCurve();

    void Render(bool editorMode);

    // Public getters for curve points if needed by other systems (e.g., tank collision)
    Vector2 getPointOnCurve(float t_global, CurveSide side) const;
    Vector2 getTangentOnCurve(float t_global, CurveSide side) const; // Not normalized

    ClosestPointInfo findClosestPointOnCurve(const Vector2& queryPoint, CurveSide side) const;

private:
    // Helper for B-Spline calculation (cubic)
    Vector2 calculateBSplinePoint(float t, const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3) const;
    Vector2 calculateBSplineTangent(float t, const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3) const;
    
    // Renamed from renderSingleCurve to renderCurve to match definition and usage in .cpp
    void renderCurve(const std::vector<Vector2>& points, float r_color, float g_color, float b_color) const;
    // Internal helper to get points/tangents from a specific list
    Vector2 getPointOnCurveInternal(float t_global, const std::vector<Vector2>& points_list) const;
    Vector2 getTangentOnCurveInternal(float t_global, const std::vector<Vector2>& points_list) const;
};

#endif // BSPLINETRACK_H
