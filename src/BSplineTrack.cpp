#include "BSplineTrack.h"
#include <vector>
#include <cmath>       
#include <algorithm>   
#include <cstdio>     
#include <string>      
#include <cfloat>      

BSplineTrack::BSplineTrack(bool isLoop)
    : degree(3), selectedPointIndex(-1), loop(isLoop), activeEditingCurve(CurveSide::Left), selectedCurve(CurveSide::None) {
    if (loop) { // desenha curvas iniciais
        // parte interna
        controlPointsLeft.push_back(Vector2(370, 300));  
        controlPointsLeft.push_back(Vector2(670, 300));
        controlPointsLeft.push_back(Vector2(970, 480)); 
        controlPointsLeft.push_back(Vector2(260, 530)); 

        // parte externa
        controlPointsRight.push_back(Vector2(80, 80));   
        controlPointsRight.push_back(Vector2(1200, 80));  
        controlPointsRight.push_back(Vector2(1200, 640)); 
        controlPointsRight.push_back(Vector2(80, 640)); 
    }
}

void BSplineTrack::switchActiveEditingCurve() {
    activeEditingCurve = (activeEditingCurve == CurveSide::Left) ? CurveSide::Right : CurveSide::Left;
    deselectControlPoint(); 
}

// adiciona um ponto de controle na curva selecionada
void BSplineTrack::addControlPoint(const Vector2& p, int index) {
    std::vector<Vector2>& points = (activeEditingCurve == CurveSide::Left) ? controlPointsLeft : controlPointsRight;
    if (points.size() >= MAX_CONTROL_POINTS) return;

    if (index < 0 || index > (int)points.size()) {
        index = points.size(); 
    }

    if (index == (int)points.size()) {
        points.push_back(p);
    } else {
        points.insert(points.begin() + index, p);
    }
}

// remove o ultimo ponto de controle da curva selecionada
bool BSplineTrack::removeControlPoint(int index) {
    std::vector<Vector2>& points = (activeEditingCurve == CurveSide::Left) ? controlPointsLeft : controlPointsRight;

    if (points.size() <= MIN_CONTROL_POINTS_PER_CURVE) return false;

    int removalIdx = -1;

    if (index != -1) { 
        if (index >= 0 && index < (int)points.size()) {
            removalIdx = index;
        } else {
            return false; 
        }
    } else { 
        if (selectedPointIndex != -1 && selectedCurve == activeEditingCurve) {
            removalIdx = selectedPointIndex;
        } else if (!points.empty()) {
            removalIdx = points.size() - 1;
        } else {
            return false; 
        }
    }

    if (removalIdx >= 0 && removalIdx < (int)points.size()) {
        points.erase(points.begin() + removalIdx);

        if (selectedCurve == activeEditingCurve) {
            if (selectedPointIndex == removalIdx) {
                deselectControlPoint();
            } else if (selectedPointIndex > removalIdx && selectedPointIndex > 0) { 
                selectedPointIndex--; 
            }
        }
        return true;
    }
    return false;
}

// seleciona o ponto de controle que o mouse clicou
bool BSplineTrack::selectControlPoint(float mx, float my) {
    deselectControlPoint(); 

    for (size_t i = 0; i < controlPointsLeft.size(); ++i) {
        if (controlPointsLeft[i].distSq(Vector2(mx, my)) < CONTROL_POINT_SELECT_RADIUS_SQ) {
            selectedPointIndex = i;
            selectedCurve = CurveSide::Left;
            return true;
        }
    }
    for (size_t i = 0; i < controlPointsRight.size(); ++i) {
        if (controlPointsRight[i].distSq(Vector2(mx, my)) < CONTROL_POINT_SELECT_RADIUS_SQ) {
            selectedPointIndex = i;
            selectedCurve = CurveSide::Right;
            return true;
        }
    }
    return false;
}

// move o ponto de controle selecionado
void BSplineTrack::moveSelectedControlPoint(float mx, float my) {
    if (selectedPointIndex == -1 || selectedCurve == CurveSide::None) return;

    if (selectedCurve == CurveSide::Left && selectedPointIndex >= 0 && selectedPointIndex < (int)controlPointsLeft.size()) {
        controlPointsLeft[selectedPointIndex].set(mx, my);
    } else if (selectedCurve == CurveSide::Right && selectedPointIndex >=0 && selectedPointIndex < (int)controlPointsRight.size()) {
        controlPointsRight[selectedPointIndex].set(mx, my);
    }
}

// deseleciona o ponto de controle
void BSplineTrack::deselectControlPoint() {
    selectedPointIndex = -1;
    selectedCurve = CurveSide::None;
}

// calcula o ponto de controle na curva B-Spline
Vector2 BSplineTrack::calculateBSplinePoint(float t, const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3) const {
    float t2 = t * t;
    float t3 = t2 * t;
    float b0 = (1 - t) * (1 - t) * (1 - t) / 6.0f;
    float b1 = (3 * t3 - 6 * t2 + 4) / 6.0f;
    float b2 = (-3 * t3 + 3 * t2 + 3 * t + 1) / 6.0f;
    float b3 = t3 / 6.0f;
    return p0 * b0 + p1 * b1 + p2 * b2 + p3 * b3;
}

// calcula o vetor tangente na curva B-Spline
Vector2 BSplineTrack::calculateBSplineTangent(float t, const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3) const {
    float t2 = t * t;
    float b0_prime = -0.5f * (1 - t) * (1 - t);
    float b1_prime = (1.5f * t2 - 2.0f * t);
    float b2_prime = (-1.5f * t2 + t + 0.5f);
    float b3_prime = (0.5f * t2);
    return p0 * b0_prime + p1 * b1_prime + p2 * b2_prime + p3 * b3_prime;
}

// utiliza o t (0 a 1) para calcular o ponto na curva B-Spline
Vector2 BSplineTrack::getPointOnCurveInternal(float t_global, const std::vector<Vector2>& points_list) const {
    if (points_list.size() < MIN_CONTROL_POINTS_PER_CURVE) {
        return points_list.empty() ? Vector2(0,0) : points_list.front(); 
    }

    int num_control_points = points_list.size();
    int num_segments = loop ? num_control_points : num_control_points - degree;
    if (num_segments <= 0) return points_list.front(); 

    float t = t_global * num_segments;
    int segment_idx = static_cast<int>(floor(t));
    segment_idx = std::max(0, std::min(segment_idx, num_segments - 1)); // clamp para uma faixa válida
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
    // Fill the track surface with a solid color first (new code)
    if (controlPointsLeft.size() >= MIN_CONTROL_POINTS_PER_CURVE && 
        controlPointsRight.size() >= MIN_CONTROL_POINTS_PER_CURVE) {
        
        // Road/track surface color - changed to light gray (former background color)
        CV::color(0.5f, 0.5f, 0.5f);
        
        const int fill_steps = 100; // More segments for smoother fill
        
        // Draw filled triangles between the curves to create a solid surface
        for (int i = 0; i < fill_steps; ++i) {
            float t1 = static_cast<float>(i) / fill_steps;
            float t2 = static_cast<float>(i+1) / fill_steps;
            
            Vector2 left1 = getPointOnCurve(t1, CurveSide::Left);
            Vector2 right1 = getPointOnCurve(t1, CurveSide::Right);
            Vector2 left2 = getPointOnCurve(t2, CurveSide::Left);
            Vector2 right2 = getPointOnCurve(t2, CurveSide::Right);
            
            // Draw two triangles to form a quad between the curves
            // Triangle 1: left1, right1, left2
            float vx1[3] = {left1.x, right1.x, left2.x};
            float vy1[3] = {left1.y, right1.y, left2.y};
            CV::triangleFill(vx1, vy1);
            
            // Triangle 2: left2, right1, right2
            float vx2[3] = {left2.x, right1.x, right2.x};
            float vy2[3] = {left2.y, right1.y, right2.y};
            CV::triangleFill(vx2, vy2);
        }
        
        // Add a yellow dotted line in the center of the track
        CV::color(1.0f, 1.0f, 0.0f); // Bright yellow
        
        const int dash_length = 2;  // Reduced from 10 to 5 (shorter dashes)
        const int space_length = 2; // Reduced from 10 to 5 (more frequent dashes)
        
        // Calculate and draw the centerline dashes
        for (int i = 0; i < fill_steps; i += (dash_length + space_length)) {
            // For each dash
            for (int j = i; j < i + dash_length && j < fill_steps; j++) {
                float t1 = static_cast<float>(j) / fill_steps;
                float t2 = static_cast<float>(j + 1) / fill_steps;
                
                // Get points on both curves
                Vector2 left1 = getPointOnCurve(t1, CurveSide::Left);
                Vector2 right1 = getPointOnCurve(t1, CurveSide::Right);
                Vector2 left2 = getPointOnCurve(t2, CurveSide::Left);
                Vector2 right2 = getPointOnCurve(t2, CurveSide::Right);
                
                // Calculate center points
                Vector2 center1((left1.x + right1.x) * 0.5f, (left1.y + right1.y) * 0.5f);
                Vector2 center2((left2.x + right2.x) * 0.5f, (left2.y + right2.y) * 0.5f);
                
                // Draw line segment for this part of the dash
                CV::line(center1.x, center1.y, center2.x, center2.y);
            }
        }
    }
    
    // Render left curve boundary (e.g., green boundary)
    renderCurve(controlPointsLeft, 0.1f, 0.1f, 0.4f); // Darker green for the line itself
    
    // Render right curve boundary (e.g., red boundary)
    renderCurve(controlPointsRight, 0.1f, 0.1f, 0.4f); // Darker red for the line itself

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
        char editorHelpTextLine3[200];
        char editorHelpTextLine4[200];
        sprintf(editorHelpTextLine1, "Modo de edicao | Curva selecionada: %s", 
                activeCurveStr.c_str(), selectedInfoStr.c_str());
        sprintf(editorHelpTextLine2, "'A' = Add (adiciona ponto de controle para a curva selecionada)");
        sprintf(editorHelpTextLine3, "'D' = Delete (deleta um ponto de controle da curva)");
        sprintf(editorHelpTextLine4, "'S' = Switch (troca entre pontos das curvas Left e Right)");
        CV::text(10, 20, editorHelpTextLine1);
        CV::text(10, 40, editorHelpTextLine2);
        CV::text(10, 60, editorHelpTextLine3);
        CV::text(10, 80, editorHelpTextLine4);
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
