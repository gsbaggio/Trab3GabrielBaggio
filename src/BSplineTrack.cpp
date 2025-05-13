#include "BSplineTrack.h"
#include <vector>
#include <cmath>       // For floor
#include <algorithm>   // For std::max, std::min
#include <cstdio>      // For sprintf
#include <string>      // For std::string, std::to_string in Render text
#include <cfloat>      // For FLT_MAX

// Constructor
BSplineTrack::BSplineTrack(bool isLoop)
    : degree(3), selectedPointIndex(-1), loop(isLoop), activeEditingCurve(CurveSide::Left), selectedCurve(CurveSide::None) {
    // Add some default control points to form a basic loop
    if (loop) {
        // Initial left curve (e.g., inner part of a rectangular track, clockwise)
        controlPointsLeft.push_back(Vector2(200, 200));  
        controlPointsLeft.push_back(Vector2(1080, 200));
        controlPointsLeft.push_back(Vector2(1080, 420)); 
        controlPointsLeft.push_back(Vector2(200, 420)); 

        // Initial right curve (e.g., outer part of a rectangular track, clockwise)
        controlPointsRight.push_back(Vector2(150, 150));   
        controlPointsRight.push_back(Vector2(1130, 150));  
        controlPointsRight.push_back(Vector2(1130, 470)); 
        controlPointsRight.push_back(Vector2(150, 470)); 
    }
}

void BSplineTrack::switchActiveEditingCurve() {
    activeEditingCurve = (activeEditingCurve == CurveSide::Left) ? CurveSide::Right : CurveSide::Left;
    deselectControlPoint(); // Deselect to avoid confusion when switching active curve
    printf("Active editing curve switched to: %s\n", (activeEditingCurve == CurveSide::Left ? "LEFT" : "RIGHT"));
}

// Add a control point to the active curve
void BSplineTrack::addControlPoint(const Vector2& p, int index /*= -1*/) {
    std::vector<Vector2>& points = (activeEditingCurve == CurveSide::Left) ? controlPointsLeft : controlPointsRight;
    if (points.size() >= MAX_CONTROL_POINTS) return;

    // Sanitize index: if out of bounds or -1, prepare to add to end.
    if (index < 0 || index > (int)points.size()) {
        index = points.size(); 
    }

    if (index == (int)points.size()) {
        points.push_back(p);
    } else {
        points.insert(points.begin() + index, p);
    }
}

// Remove a control point
bool BSplineTrack::removeControlPoint(int index /*= -1*/) {
    std::vector<Vector2>& points = (activeEditingCurve == CurveSide::Left) ? controlPointsLeft : controlPointsRight;

    if (points.size() <= MIN_CONTROL_POINTS_PER_CURVE) return false;

    int removalIdx = -1;

    if (index != -1) { // Specific index provided for the active curve
        if (index >= 0 && index < (int)points.size()) {
            removalIdx = index;
        } else {
            return false; // Invalid explicit index for active curve
        }
    } else { // No specific index, use selection on active curve or last point of active curve
        if (selectedPointIndex != -1 && selectedCurve == activeEditingCurve) {
            // A point is selected, and it's on the currently active curve
            removalIdx = selectedPointIndex;
        } else if (!points.empty()) {
            // No selection on active curve, or no selection at all; remove last point of active curve
            removalIdx = points.size() - 1;
        } else {
            return false; // No points in active curve to remove
        }
    }

    if (removalIdx >= 0 && removalIdx < (int)points.size()) {
        points.erase(points.begin() + removalIdx);

        // Adjust selection if the removed point was the selected one on the active curve
        if (selectedCurve == activeEditingCurve) {
            if (selectedPointIndex == removalIdx) {
                deselectControlPoint();
            } else if (selectedPointIndex > removalIdx && selectedPointIndex > 0) { // Check selectedPointIndex > 0 before decrementing
                selectedPointIndex--; // Adjust index if selection was after the removed point
            }
        }
        return true;
    }
    return false;
}

// Select a control point if mouse click is near one
bool BSplineTrack::selectControlPoint(float mx, float my) {
    deselectControlPoint(); // Clear previous selection first

    // Check left curve
    for (size_t i = 0; i < controlPointsLeft.size(); ++i) {
        if (controlPointsLeft[i].distSq(Vector2(mx, my)) < CONTROL_POINT_SELECT_RADIUS_SQ) {
            selectedPointIndex = i;
            selectedCurve = CurveSide::Left;
            return true;
        }
    }
    // Check right curve
    for (size_t i = 0; i < controlPointsRight.size(); ++i) {
        if (controlPointsRight[i].distSq(Vector2(mx, my)) < CONTROL_POINT_SELECT_RADIUS_SQ) {
            selectedPointIndex = i;
            selectedCurve = CurveSide::Right;
            return true;
        }
    }
    return false;
}

// Move the selected control point
void BSplineTrack::moveSelectedControlPoint(float mx, float my) {
    if (selectedPointIndex == -1 || selectedCurve == CurveSide::None) return;

    if (selectedCurve == CurveSide::Left && selectedPointIndex >= 0 && selectedPointIndex < (int)controlPointsLeft.size()) {
        controlPointsLeft[selectedPointIndex].set(mx, my);
    } else if (selectedCurve == CurveSide::Right && selectedPointIndex >=0 && selectedPointIndex < (int)controlPointsRight.size()) {
        controlPointsRight[selectedPointIndex].set(mx, my);
    }
}

// Deselect any control point
void BSplineTrack::deselectControlPoint() {
    selectedPointIndex = -1;
    selectedCurve = CurveSide::None;
}

// Calculate a point on a cubic B-Spline segment
Vector2 BSplineTrack::calculateBSplinePoint(float t, const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3) const {
    float t2 = t * t;
    float t3 = t2 * t;
    float b0 = (1 - t) * (1 - t) * (1 - t) / 6.0f;
    float b1 = (3 * t3 - 6 * t2 + 4) / 6.0f;
    float b2 = (-3 * t3 + 3 * t2 + 3 * t + 1) / 6.0f;
    float b3 = t3 / 6.0f;
    return p0 * b0 + p1 * b1 + p2 * b2 + p3 * b3;
}

// Calculate tangent on a cubic B-Spline segment (not normalized)
Vector2 BSplineTrack::calculateBSplineTangent(float t, const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3) const {
    float t2 = t * t;
    // Derivatives of basis functions
    float b0_prime = -0.5f * (1 - t) * (1 - t);
    float b1_prime = (1.5f * t2 - 2.0f * t);
    float b2_prime = (-1.5f * t2 + t + 0.5f);
    float b3_prime = (0.5f * t2);
    return p0 * b0_prime + p1 * b1_prime + p2 * b2_prime + p3 * b3_prime;
}

// Internal helper to get a point on a specific list of control points for a global t (0 to 1)
Vector2 BSplineTrack::getPointOnCurveInternal(float t_global, const std::vector<Vector2>& points_list) const {
    if (points_list.size() < MIN_CONTROL_POINTS_PER_CURVE) {
        return points_list.empty() ? Vector2(0,0) : points_list.front(); // Fallback
    }

    int num_control_points = points_list.size();
    int num_segments = loop ? num_control_points : num_control_points - degree;
    if (num_segments <= 0) return points_list.front(); // Fallback

    float t = t_global * num_segments;
    int segment_idx = static_cast<int>(floor(t));
    segment_idx = std::max(0, std::min(segment_idx, num_segments - 1)); // Clamp to valid range
    float t_local = t - segment_idx;

    Vector2 p0, p1, p2, p3;
    if (loop) {
        p0 = points_list[segment_idx % num_control_points];
        p1 = points_list[(segment_idx + 1) % num_control_points];
        p2 = points_list[(segment_idx + 2) % num_control_points];
        p3 = points_list[(segment_idx + 3) % num_control_points];
    } else {
        p0 = points_list[segment_idx];
        p1 = points_list[segment_idx + 1];
        p2 = points_list[segment_idx + 2];
        p3 = points_list[segment_idx + 3];
    }
    return calculateBSplinePoint(t_local, p0, p1, p2, p3);
}

// Internal helper to get tangent on a specific list of control points for a global t (0 to 1)
Vector2 BSplineTrack::getTangentOnCurveInternal(float t_global, const std::vector<Vector2>& points_list) const {
    if (points_list.size() < MIN_CONTROL_POINTS_PER_CURVE) {
        return Vector2(1,0); // Default tangent
    }
    int num_control_points = points_list.size();
    int num_segments = loop ? num_control_points : num_control_points - degree;
    if (num_segments <= 0) return Vector2(1,0);

    float t = t_global * num_segments;
    int segment_idx = static_cast<int>(floor(t));
    segment_idx = std::max(0, std::min(segment_idx, num_segments - 1));
    float t_local = t - segment_idx;

    Vector2 p0, p1, p2, p3;
    if (loop) {
        p0 = points_list[segment_idx % num_control_points];
        p1 = points_list[(segment_idx + 1) % num_control_points];
        p2 = points_list[(segment_idx + 2) % num_control_points];
        p3 = points_list[(segment_idx + 3) % num_control_points];
    } else {
        p0 = points_list[segment_idx];
        p1 = points_list[segment_idx + 1];
        p2 = points_list[segment_idx + 2];
        p3 = points_list[segment_idx + 3];
    }
    return calculateBSplineTangent(t_local, p0, p1, p2, p3);
}


// Public interface to get point on a curve
Vector2 BSplineTrack::getPointOnCurve(float t_global, CurveSide side) const {
    if (side == CurveSide::Left) {
        return getPointOnCurveInternal(t_global, controlPointsLeft);
    } else if (side == CurveSide::Right) {
        return getPointOnCurveInternal(t_global, controlPointsRight);
    }
    return Vector2(0,0); // Should not happen if side is valid
}

// Public interface to get tangent on a curve
Vector2 BSplineTrack::getTangentOnCurve(float t_global, CurveSide side) const {
     if (side == CurveSide::Left) {
        return getTangentOnCurveInternal(t_global, controlPointsLeft);
    } else if (side == CurveSide::Right) {
        return getTangentOnCurveInternal(t_global, controlPointsRight);
    }
    return Vector2(1,0); // Should not happen
}

// Helper to render a single B-Spline curve
void BSplineTrack::renderCurve(const std::vector<Vector2>& points, float r, float g, float b) const {
    if (points.size() < MIN_CONTROL_POINTS_PER_CURVE) return;

    int num_control_points = points.size();
    int num_render_segments = loop ? num_control_points : num_control_points - degree;
    if (num_render_segments <= 0) return;

    const int steps_per_segment = 20; // Density of line segments for drawing the curve
    Vector2 last_pt;

    for (int i = 0; i < num_render_segments; ++i) { // Iterate through each B-Spline segment
        Vector2 cp0, cp1, cp2, cp3;
        if (loop) {
            cp0 = points[i % num_control_points];
            cp1 = points[(i + 1) % num_control_points];
            cp2 = points[(i + 2) % num_control_points];
            cp3 = points[(i + 3) % num_control_points];
        } else {
            cp0 = points[i]; cp1 = points[i+1]; cp2 = points[i+2]; cp3 = points[i+3];
        }

        for (int j = 0; j <= steps_per_segment; ++j) { // Iterate t_local from 0 to 1 for this segment
            float t_local_in_segment = static_cast<float>(j) / steps_per_segment;
            Vector2 current_pt = calculateBSplinePoint(t_local_in_segment, cp0, cp1, cp2, cp3);

            if (j > 0 || i > 0) { // Draw line if not the very first point calculated
                CV::color(r, g, b);
                CV::line(last_pt.x, last_pt.y, current_pt.x, current_pt.y);
            }
            last_pt = current_pt;
        }
    }
    
    if (loop && points.size() >= MIN_CONTROL_POINTS_PER_CURVE) {
         Vector2 first_point_of_curve = getPointOnCurveInternal(0.0f, points);
         CV::color(r, g, b);
         CV::line(last_pt.x, last_pt.y, first_point_of_curve.x, first_point_of_curve.y);
    }
}

// Render the track
void BSplineTrack::Render(bool editorMode) {
    // Render left curve (e.g., green boundary)
    renderCurve(controlPointsLeft, 0.1f, 0.4f, 0.1f); // Darker green for the line itself
    // Render right curve (e.g., red boundary)
    renderCurve(controlPointsRight, 0.4f, 0.1f, 0.1f); // Darker red for the line itself

    // Draw "fill" lines between the two curves to represent the track surface
    if (controlPointsLeft.size() >= MIN_CONTROL_POINTS_PER_CURVE && controlPointsRight.size() >= MIN_CONTROL_POINTS_PER_CURVE) {
        CV::color(0.3f, 0.3f, 0.3f); // Color for track surface lines (grey)
        const int fill_steps = 50; // Number of connecting lines
        for (int i = 0; i <= fill_steps; ++i) {
            float t_global = static_cast<float>(i) / fill_steps;
            Vector2 pt_left = getPointOnCurve(t_global, CurveSide::Left);
            Vector2 pt_right = getPointOnCurve(t_global, CurveSide::Right);
            CV::line(pt_left.x, pt_left.y, pt_right.x, pt_right.y);
        }
    }

    // Draw control points if in editor mode
    if (editorMode) {
        char pointLabel[10];

        // Draw Left Control Points
        for (size_t i = 0; i < controlPointsLeft.size(); ++i) {
            bool isSelected = (selectedPointIndex == static_cast<int>(i) && selectedCurve == CurveSide::Left);
            bool isActiveEditing = (activeEditingCurve == CurveSide::Left);

            if (isSelected) CV::color(1.0f, 0.65f, 0.0f); // Orange for selected
            else if (isActiveEditing) CV::color(0.0f, 1.0f, 0.0f); // Bright Green for active editing curve
            else CV::color(0.0f, 0.5f, 0.0f); // Darker Green for inactive
            
            CV::circleFill(controlPointsLeft[i].x, controlPointsLeft[i].y, CONTROL_POINT_DRAW_RADIUS, 10);
            sprintf(pointLabel, "L%zu", i);
            CV::color(1,1,1); // White text
            CV::text(controlPointsLeft[i].x + CONTROL_POINT_DRAW_RADIUS + 3, controlPointsLeft[i].y - CONTROL_POINT_DRAW_RADIUS - 12, pointLabel);
        }

        // Draw Right Control Points
        for (size_t i = 0; i < controlPointsRight.size(); ++i) {
            bool isSelected = (selectedPointIndex == static_cast<int>(i) && selectedCurve == CurveSide::Right);
            bool isActiveEditing = (activeEditingCurve == CurveSide::Right);

            if (isSelected) CV::color(1.0f, 0.65f, 0.0f); // Orange for selected
            else if (isActiveEditing) CV::color(1.0f, 0.0f, 0.0f); // Bright Red for active editing curve
            else CV::color(0.5f, 0.0f, 0.0f); // Darker Red for inactive

            CV::circleFill(controlPointsRight[i].x, controlPointsRight[i].y, CONTROL_POINT_DRAW_RADIUS, 10);
            sprintf(pointLabel, "R%zu", i);
            CV::color(1,1,1); // White text
            CV::text(controlPointsRight[i].x + CONTROL_POINT_DRAW_RADIUS + 3, controlPointsRight[i].y - CONTROL_POINT_DRAW_RADIUS - 12, pointLabel);
        }
        
        // Editor mode help text
        CV::color(1,1,1);
        std::string activeCurveStr = (activeEditingCurve == CurveSide::Left) ? "LEFT (Green)" : "RIGHT (Red)";
        std::string selectedInfoStr = "None";
        if (selectedCurve != CurveSide::None && selectedPointIndex != -1) {
            selectedInfoStr = (selectedCurve == CurveSide::Left ? "L" : "R") + std::to_string(selectedPointIndex);
        }
        
        char editorHelpTextLine1[200];
        char editorHelpTextLine2[200];
        sprintf(editorHelpTextLine1, "EDITOR MODE | Active: %s (S to switch) | Selected: %s", 
                activeCurveStr.c_str(), selectedInfoStr.c_str());
        sprintf(editorHelpTextLine2, "Click to select/drag. +/- to add/remove on active curve.");
        CV::text(10, 20, editorHelpTextLine1);
        CV::text(10, 40, editorHelpTextLine2);
    }
}

// Find the closest point on the specified curve to a query point
ClosestPointInfo BSplineTrack::findClosestPointOnCurve(const Vector2& queryPoint, CurveSide side) const {
    ClosestPointInfo closestInfo;

    const std::vector<Vector2>* points_list_ptr = nullptr;
    if (side == CurveSide::Left) {
        points_list_ptr = &controlPointsLeft;
    } else if (side == CurveSide::Right) {
        points_list_ptr = &controlPointsRight;
    } else {
        return closestInfo; 
    }

    const std::vector<Vector2>& points_list = *points_list_ptr;

    if (points_list.size() < static_cast<size_t>(MIN_CONTROL_POINTS_PER_CURVE)) {
        if (!points_list.empty()) {
            for (size_t i = 0; i < points_list.size(); ++i) {
                float distSq = queryPoint.distSq(points_list[i]);
                if (distSq < closestInfo.distance * closestInfo.distance) { 
                    closestInfo.distance = std::sqrt(distSq);
                    closestInfo.point = points_list[i];
                    closestInfo.t_global = -1.0f; 
                    closestInfo.segmentIndex = -1; 
                    closestInfo.isValid = true; 
                }
            }
        }
        return closestInfo; 
    }

    const int NUM_SAMPLES_FOR_CLOSEST_POINT = 200; 

    int num_control_points = static_cast<int>(points_list.size());
    int num_segments = loop ? num_control_points : num_control_points - degree;
    if (num_segments <= 0) { 
        if (!points_list.empty()) { 
             closestInfo.point = points_list.front();
             closestInfo.distance = std::sqrt(queryPoint.distSq(points_list.front()));
             closestInfo.t_global = 0.0f;
             closestInfo.segmentIndex = 0;
             closestInfo.isValid = true;
        }
        return closestInfo;
    }

    for (int i = 0; i <= NUM_SAMPLES_FOR_CLOSEST_POINT; ++i) {
        float t_global_sample = static_cast<float>(i) / NUM_SAMPLES_FOR_CLOSEST_POINT;
        Vector2 current_curve_point = getPointOnCurveInternal(t_global_sample, points_list);
        float dist_sq_current = queryPoint.distSq(current_curve_point);

        if (dist_sq_current < closestInfo.distance * closestInfo.distance) {
            closestInfo.distance = std::sqrt(dist_sq_current);
            closestInfo.point = current_curve_point;
            closestInfo.t_global = t_global_sample;
            closestInfo.isValid = true;

            float t_scaled_for_segment = t_global_sample * num_segments;
            int segment_idx;
            if (t_global_sample >= 1.0f) { 
                 segment_idx = num_segments - 1; 
            } else {
                 segment_idx = static_cast<int>(floor(t_scaled_for_segment));
            }
            closestInfo.segmentIndex = std::max(0, std::min(segment_idx, num_segments - 1));
        }
    }
    
    if (closestInfo.isValid) {
        Vector2 tangent = getTangentOnCurveInternal(closestInfo.t_global, points_list);
        if (tangent.lengthSq() > 1e-6) { // Avoid normalizing zero vector
            tangent.normalize();
            closestInfo.normal = Vector2(-tangent.y, tangent.x); 
        } else {
            // Cannot determine normal if tangent is zero, could set to a default or mark as less reliable
            closestInfo.normal = Vector2(0,0); // Or some other indicator
        }
    }
    return closestInfo;
}
