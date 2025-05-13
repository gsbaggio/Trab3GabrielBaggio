#include "BSplineTrack.h"
#include <stdexcept> // For std::runtime_error

// Constructor
BSplineTrack::BSplineTrack(float width, bool isLoop) 
    : trackWidth(width), degree(3), selectedPointIndex(-1), loop(isLoop) {
    // Add some default control points to form a basic loop if empty
    // These should be within typical screen coordinates (e.g., 1280x720)
    if (controlPoints.empty() && loop) {
        controlPoints.push_back(Vector2(200, 200));
        controlPoints.push_back(Vector2(1080, 200));
        controlPoints.push_back(Vector2(1080, 520));
        controlPoints.push_back(Vector2(200, 520));
    }
}

// Add a control point
void BSplineTrack::addControlPoint(const Vector2& p, int index) {
    if (controlPoints.size() >= MAX_CONTROL_POINTS) return;
    if (index < -1 || index > (int)controlPoints.size()) index = -1; //sanitize index

    if (index == -1 || index == (int)controlPoints.size()) {
        controlPoints.push_back(p);
    } else {
        controlPoints.insert(controlPoints.begin() + index, p);
    }
}

// Remove a control point
bool BSplineTrack::removeControlPoint(int index) {
    if (controlPoints.size() <= MIN_CONTROL_POINTS) return false;

    if (index != -1 && (index < 0 || index >= (int)controlPoints.size())) {
        return false; // Invalid index
    }

    int removalIdx = -1;
    if (index != -1) {
        removalIdx = index;
    } else if (selectedPointIndex != -1) {
        removalIdx = selectedPointIndex;
    } else if (!controlPoints.empty()){
        removalIdx = controlPoints.size() - 1; // Remove last if no specific index and none selected
    }

    if (removalIdx != -1) {
        controlPoints.erase(controlPoints.begin() + removalIdx);
        if (selectedPointIndex == removalIdx) {
            deselectControlPoint();
        } else if (selectedPointIndex > removalIdx && selectedPointIndex > 0) {
            selectedPointIndex--; // Adjust selection if it was after the removed point
        }
        return true;
    }
    return false;
}

// Select a control point if mouse click is near one
bool BSplineTrack::selectControlPoint(float mx, float my) {
    for (size_t i = 0; i < controlPoints.size(); ++i) {
        if (controlPoints[i].distSq(Vector2(mx, my)) < CONTROL_POINT_SELECT_RADIUS_SQ) {
            selectedPointIndex = i;
            return true;
        }
    }
    selectedPointIndex = -1;
    return false;
}

// Move the selected control point
void BSplineTrack::moveSelectedControlPoint(float mx, float my) {
    if (selectedPointIndex >= 0 && selectedPointIndex < (int)controlPoints.size()) {
        controlPoints[selectedPointIndex].set(mx, my);
    }
}

// Deselect any control point
void BSplineTrack::deselectControlPoint() {
    selectedPointIndex = -1;
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

    float b0_prime = -3 * (1 - t) * (1 - t) / 6.0f; // -0.5 * (1-t)^2
    float b1_prime = (9 * t2 - 12 * t) / 6.0f;     // (1.5 * t^2 - 2*t)
    float b2_prime = (-9 * t2 + 6 * t + 3) / 6.0f; // (-1.5 * t^2 + t + 0.5)
    float b3_prime = 3 * t2 / 6.0f;                // 0.5 * t^2

    return p0 * b0_prime + p1 * b1_prime + p2 * b2_prime + p3 * b3_prime;
}


// Get a point on the curve for a global t (0 to 1)
Vector2 BSplineTrack::getPointOnCurve(float t_global) const {
    if (controlPoints.size() < MIN_CONTROL_POINTS) {
        return Vector2(0,0); // Or throw error, or return a default
    }

    int num_control_points = controlPoints.size();
    int num_segments = loop ? num_control_points : num_control_points - degree;
    if (num_segments <= 0) return Vector2(0,0);

    float t = t_global * num_segments;
    int segment_idx = static_cast<int>(floor(t));
    segment_idx = std::max(0, std::min(segment_idx, num_segments - 1)); // Clamp to valid range
    float t_local = t - segment_idx;

    Vector2 p0, p1, p2, p3;
    if (loop) {
        p0 = controlPoints[segment_idx % num_control_points];
        p1 = controlPoints[(segment_idx + 1) % num_control_points];
        p2 = controlPoints[(segment_idx + 2) % num_control_points];
        p3 = controlPoints[(segment_idx + 3) % num_control_points];
    } else {
        // For open B-spline, ensure indices are valid
        // This example focuses on loop, open needs more careful indexing for ends
        // For simplicity, we assume loop or enough points for non-loop to behave like loop segments
        p0 = controlPoints[segment_idx];
        p1 = controlPoints[segment_idx + 1];
        p2 = controlPoints[segment_idx + 2];
        p3 = controlPoints[segment_idx + 3];
    }
    return calculateBSplinePoint(t_local, p0, p1, p2, p3);
}

// Get tangent on the curve for a global t (0 to 1)
Vector2 BSplineTrack::getTangentOnCurve(float t_global) const {
     if (controlPoints.size() < MIN_CONTROL_POINTS) {
        return Vector2(1,0); // Default tangent
    }
    int num_control_points = controlPoints.size();
    int num_segments = loop ? num_control_points : num_control_points - degree;
    if (num_segments <= 0) return Vector2(1,0);

    float t = t_global * num_segments;
    int segment_idx = static_cast<int>(floor(t));
    segment_idx = std::max(0, std::min(segment_idx, num_segments - 1));
    float t_local = t - segment_idx;

    Vector2 p0, p1, p2, p3;
    if (loop) {
        p0 = controlPoints[segment_idx % num_control_points];
        p1 = controlPoints[(segment_idx + 1) % num_control_points];
        p2 = controlPoints[(segment_idx + 2) % num_control_points];
        p3 = controlPoints[(segment_idx + 3) % num_control_points];
    } else {
        p0 = controlPoints[segment_idx];
        p1 = controlPoints[segment_idx + 1];
        p2 = controlPoints[segment_idx + 2];
        p3 = controlPoints[segment_idx + 3];
    }
    return calculateBSplineTangent(t_local, p0, p1, p2, p3);
}

// Render the track
void BSplineTrack::Render(bool editorMode) {
    if (controlPoints.size() < MIN_CONTROL_POINTS) return;

    int num_render_segments = loop ? controlPoints.size() : controlPoints.size() - degree;
    if (num_render_segments <= 0) return;

    const int steps_per_segment = 20; // Number of small lines to draw per B-Spline segment

    Vector2 last_left_pt, last_right_pt, last_center_pt;

    for (int i = 0; i < num_render_segments; ++i) {
        Vector2 cp0, cp1, cp2, cp3;
        if (loop) {
            cp0 = controlPoints[i % controlPoints.size()];
            cp1 = controlPoints[(i + 1) % controlPoints.size()];
            cp2 = controlPoints[(i + 2) % controlPoints.size()];
            cp3 = controlPoints[(i + 3) % controlPoints.size()];
        } else {
            // This simplified version for open loop might not look good at ends
            // A full open B-spline requires knot vector manipulation for C2 at ends
            if (i + degree >= controlPoints.size()) continue;
            cp0 = controlPoints[i];
            cp1 = controlPoints[i+1];
            cp2 = controlPoints[i+2];
            cp3 = controlPoints[i+3];
        }

        for (int j = 0; j <= steps_per_segment; ++j) {
            float t_local = static_cast<float>(j) / steps_per_segment;
            Vector2 current_center_pt = calculateBSplinePoint(t_local, cp0, cp1, cp2, cp3);
            Vector2 tangent = calculateBSplineTangent(t_local, cp0, cp1, cp2, cp3);
            
            if (tangent.length() < 0.001f) { // Avoid division by zero if tangent is zero
                if (j > 0) tangent = (current_center_pt - last_center_pt).normalized();
                else tangent = Vector2(1,0); // Default if first point and no tangent
            } else {
                tangent.normalize();
            }
            
            Vector2 normal(-tangent.y, tangent.x); // Perpendicular to the tangent

            Vector2 current_left_pt = current_center_pt + normal * (trackWidth / 2.0f);
            Vector2 current_right_pt = current_center_pt - normal * (trackWidth / 2.0f);

            if (j > 0 || i > 0) { // For all but the very first point of the entire track
                CV::color(0.3f, 0.3f, 0.3f); // Track color (dark grey)
                CV::line(last_left_pt.x, last_left_pt.y, current_left_pt.x, current_left_pt.y);
                CV::line(last_right_pt.x, last_right_pt.y, current_right_pt.x, current_right_pt.y);
                // Optional: draw centerline for debugging
                // CV::color(1,1,0); CV::line(last_center_pt.x, last_center_pt.y, current_center_pt.x, current_center_pt.y);
            }
            last_left_pt = current_left_pt;
            last_right_pt = current_right_pt;
            last_center_pt = current_center_pt;
        }
    }
    
    // If loop and enough points, close the final segment to the first
    if (loop && num_render_segments > 0 && steps_per_segment > 0) {
        // Calculate the very first point of the track again to close the loop
        Vector2 first_cp0 = controlPoints[0];
        Vector2 first_cp1 = controlPoints[1 % controlPoints.size()];
        Vector2 first_cp2 = controlPoints[2 % controlPoints.size()];
        Vector2 first_cp3 = controlPoints[3 % controlPoints.size()];
        Vector2 first_center_pt = calculateBSplinePoint(0.0f, first_cp0, first_cp1, first_cp2, first_cp3);
        Vector2 first_tangent = calculateBSplineTangent(0.0f, first_cp0, first_cp1, first_cp2, first_cp3).normalized();
        if (first_tangent.length() < 0.001f) first_tangent = Vector2(1,0);
        Vector2 first_normal(-first_tangent.y, first_tangent.x);
        Vector2 first_left_pt = first_center_pt + first_normal * (trackWidth / 2.0f);
        Vector2 first_right_pt = first_center_pt - first_normal * (trackWidth / 2.0f);

        CV::color(0.3f, 0.3f, 0.3f);
        CV::line(last_left_pt.x, last_left_pt.y, first_left_pt.x, first_left_pt.y);
        CV::line(last_right_pt.x, last_right_pt.y, first_right_pt.x, first_right_pt.y);
        // CV::line(last_center_pt.x, last_center_pt.y, first_center_pt.x, first_center_pt.y); // Close centerline
    }

    // Draw control points if in editor mode
    if (editorMode) {
        for (size_t i = 0; i < controlPoints.size(); ++i) {
            if (i == selectedPointIndex) {
                CV::color(1, 0, 0); // Selected point color (red)
            } else {
                CV::color(0, 1, 0); // Default control point color (green)
            }
            CV::circleFill(controlPoints[i].x, controlPoints[i].y, CONTROL_POINT_DRAW_RADIUS, 10);

            // Draw the control point number
            char pointNumberStr[4]; // Buffer for string conversion, max 999 points
            sprintf(pointNumberStr, "%zu", i);
            CV::color(1, 1, 1); // White color for text
            // Offset the text slightly from the control point circle
            CV::text(controlPoints[i].x + CONTROL_POINT_DRAW_RADIUS + 2, controlPoints[i].y + CONTROL_POINT_DRAW_RADIUS + 2, pointNumberStr);
        }
        CV::color(1,1,1);
        CV::text(10, 30, "EDITOR MODE: Click to select/drag points. +/- to add/remove.");
    }
}
