#include "Tanque.h"
#include "gl_canvas2d.h"
#include <cmath> // For M_PI, cos, sin, atan2
#include "BSplineTrack.h" // Added this include

// Define M_PI if not available (common on Windows with MSVC)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Tanque::Tanque(float x, float y, float initialSpeed, float initialRotationRate) {
    position.set(x, y);
    baseAngle = 0.0f;
    topAngle = 0.0f;
    speed = initialSpeed;
    rotationRate = initialRotationRate;
    turretRotationSpeed = 0.05f; // Assuming this is now per-frame if deltaTime was globally removed
    baseWidth = 60.0f;
    baseHeight = 40.0f;
    turretRadius = 15.0f;
    cannonLength = 40.0f;
    cannonWidth = 6.0f;

    forwardVector.set(cos(baseAngle), sin(baseAngle));

    // Initialize collision members
    lastSafePosition = position;
    isColliding = false;
    collisionTimer = 0;
}

void Tanque::Update(float mouseX, float mouseY, bool rotateLeft, bool rotateRight, BSplineTrack* track) {
    if (isColliding) {
        // Rebound movement
        position.x -= forwardVector.x * speed * REBOUND_SPEED_FACTOR;
        position.y -= forwardVector.y * speed * REBOUND_SPEED_FACTOR;
        collisionTimer--;
        if (collisionTimer <= 0) {
            isColliding = false;
            printf("Rebound finished.\\\\n");
        }

        // Turret can still aim during rebound
        float dx_turret = mouseX - position.x;
        float dy_turret = mouseY - position.y;
        float targetAngle_turret = atan2(dy_turret, dx_turret);
        float currentTopAngle_turret = fmod(topAngle + M_PI, 2 * M_PI) - M_PI;
        if (currentTopAngle_turret < -M_PI) currentTopAngle_turret += 2 * M_PI;
        float angleDifference_turret = targetAngle_turret - currentTopAngle_turret;
        if (angleDifference_turret > M_PI) angleDifference_turret -= 2 * M_PI;
        else if (angleDifference_turret < -M_PI) angleDifference_turret += 2 * M_PI;
        
        float maxRotationThisFrame_turret = turretRotationSpeed; // Assuming per-frame

        if (std::abs(angleDifference_turret) <= maxRotationThisFrame_turret) {
            topAngle = targetAngle_turret;
        } else {
            topAngle += (angleDifference_turret > 0 ? maxRotationThisFrame_turret : -maxRotationThisFrame_turret);
        }
        topAngle = fmod(topAngle, 2 * M_PI);
        if (topAngle < 0) topAngle += 2 * M_PI;
        
        return; // Skip normal movement and new collision checks
    }

    // Not currently colliding and rebounding: Normal update logic
    lastSafePosition = position;

    // Update base rotation
    if (rotateLeft) {
        baseAngle -= rotationRate;
    }
    if (rotateRight) {
        baseAngle += rotationRate;
    }

    // Keep baseAngle within 0 to 2*PI
    if (baseAngle > 2 * M_PI) baseAngle -= 2 * M_PI;
    if (baseAngle < 0) baseAngle += 2 * M_PI;

    // Update forward vector based on baseAngle
    forwardVector.set(cos(baseAngle), sin(baseAngle));

    // Tentative new position (no deltaTime)
    Vector2 tentativePosition = position;
    tentativePosition.x += forwardVector.x * speed;
    tentativePosition.y += forwardVector.y * speed;

    // Store current position, set to tentative for collision check
    Vector2 currentActualPosition = position;
    position = tentativePosition;

    CheckCollisionAndRespond(track); // This method might revert position to lastSafePosition and set isColliding

    // If a collision occurred, position was reset by CheckCollisionAndRespond.
    // The rebound will start in the next Update call.
    // If no collision, position remains the new tentativePosition.

    // Update top (turret) angle to point towards the mouse
    float dx = mouseX - position.x; // Use the (potentially reverted) position
    float dy = mouseY - position.y;
    float targetAngle = atan2(dy, dx);

    float currentTopAngle = fmod(topAngle + M_PI, 2 * M_PI) - M_PI;
    if (currentTopAngle < -M_PI) currentTopAngle += 2 * M_PI; // Ensure it's in [-PI, PI]
    float angleDifference = targetAngle - currentTopAngle;

    // Normalize the angle difference to the shortest path (-PI to PI)
    if (angleDifference > M_PI) {
        angleDifference -= 2 * M_PI;
    } else if (angleDifference < -M_PI) {
        angleDifference += 2 * M_PI;
    }

    float maxRotationThisFrame = turretRotationSpeed; // Assuming turretRotationSpeed is now per-frame

    if (std::abs(angleDifference) <= maxRotationThisFrame) {
        topAngle = targetAngle;
    } else {
        topAngle += (angleDifference > 0 ? maxRotationThisFrame : -maxRotationThisFrame);
    }

    // Normalize topAngle to be within 0 to 2*PI (or -PI to PI, consistently)
    topAngle = fmod(topAngle, 2 * M_PI);
    if (topAngle < 0) {
        topAngle += 2 * M_PI;
    }
}

void Tanque::Render() {
    CV::color(0.2f, 0.5f, 0.2f); // Dark green for base

    // Render Base (Rectangle)
    // We need to draw the rectangle rotated around its center (position)
    // 1. Translate to the tank's position
    // 2. Rotate by baseAngle
    // 3. Draw the rectangle centered at (0,0) in local coordinates
    // 4. Undo rotation and translation (or use CV::pushMatrix/popMatrix if available)

    // For simplicity, we'll calculate the 4 corners of the rotated rectangle manually.
    // This is equivalent to applying a rotation matrix to the local corner coordinates.
    float halfW = baseWidth / 2.0f;
    float halfH = baseHeight / 2.0f;

    // Local corners of the base rectangle (before rotation)
    Vector2 p1_local(-halfW, -halfH);
    Vector2 p2_local( halfW, -halfH);
    Vector2 p3_local( halfW,  halfH);
    Vector2 p4_local(-halfW,  halfH);

    // Rotated corners
    float cosB = cos(baseAngle);
    float sinB = sin(baseAngle);

    Vector2 p1_world(p1_local.x * cosB - p1_local.y * sinB + position.x, p1_local.x * sinB + p1_local.y * cosB + position.y);
    Vector2 p2_world(p2_local.x * cosB - p2_local.y * sinB + position.x, p2_local.x * sinB + p2_local.y * cosB + position.y);
    Vector2 p3_world(p3_local.x * cosB - p3_local.y * sinB + position.x, p3_local.x * sinB + p3_local.y * cosB + position.y);
    Vector2 p4_world(p4_local.x * cosB - p4_local.y * sinB + position.x, p4_local.x * sinB + p4_local.y * cosB + position.y);

    // Draw the base as a polygon (or multiple lines/triangles if rectFill takes only axis-aligned)
    // Assuming CV::rectFill might not support rotated rectangles directly, we draw lines.
    // If CV::polygon or similar exists, that would be better.
    // For now, let's use lines to form the rectangle.
    CV::line(p1_world.x, p1_world.y, p2_world.x, p2_world.y);
    CV::line(p2_world.x, p2_world.y, p3_world.x, p3_world.y);
    CV::line(p3_world.x, p3_world.y, p4_world.x, p4_world.y);
    CV::line(p4_world.x, p4_world.y, p1_world.x, p1_world.y);

    // Draw a line indicating the front of the base
    CV::color(1,1,1); // White line for direction
    float frontMarkerLength = baseWidth * 0.6f;
    CV::line(position.x, position.y, position.x + forwardVector.x * frontMarkerLength, position.y + forwardVector.y * frontMarkerLength);


    // Render Top (Turret - Circle and Cannon - Rectangle)
    CV::color(0.1f, 0.3f, 0.1f); // Darker green for turret
    CV::circleFill(position.x, position.y, turretRadius, 20); // Draw turret base

    // Cannon
    // Calculate cannon end points based on topAngle
    float cannonStartX = position.x + cos(topAngle) * (turretRadius * 0.5f); // Start a bit from center
    float cannonStartY = position.y + sin(topAngle) * (turretRadius * 0.5f);
    float cannonEndX = position.x + cos(topAngle) * cannonLength;
    float cannonEndY = position.y + sin(topAngle) * cannonLength;

    // To draw the cannon as a rectangle, we need its 4 corners.
    // The cannon points along topAngle. Its width is cannonWidth.
    float halfCW = cannonWidth / 2.0f;
    float cosT = cos(topAngle);
    float sinT = sin(topAngle);
    float cosT_perp = cos(topAngle + M_PI / 2.0f); // Perpendicular to cannon direction
    float sinT_perp = sin(topAngle + M_PI / 2.0f); // Perpendicular to cannon direction

    // Points relative to tank's position, then add tank's position
    // Start of cannon (closer to turret center)
    Vector2 c1_local(0, -halfCW); // Local to cannon's orientation
    Vector2 c2_local(0,  halfCW);
    // End of cannon
    Vector2 c3_local(cannonLength, -halfCW);
    Vector2 c4_local(cannonLength,  halfCW);

    // Rotate these local points by topAngle and add to tank's position
    Vector2 c1(position.x + (c1_local.x * cosT - c1_local.y * sinT), position.y + (c1_local.x * sinT + c1_local.y * cosT));
    Vector2 c2(position.x + (c2_local.x * cosT - c2_local.y * sinT), position.y + (c2_local.x * sinT + c2_local.y * cosT));
    Vector2 c3(position.x + (c3_local.x * cosT - c3_local.y * sinT), position.y + (c3_local.x * sinT + c3_local.y * cosT));
    Vector2 c4(position.x + (c4_local.x * cosT - c4_local.y * sinT), position.y + (c4_local.x * sinT + c4_local.y * cosT));

    // Draw cannon using lines (assuming no rotated rectFill)
    CV::line(c1.x, c1.y, c3.x, c3.y); // One side
    CV::line(c2.x, c2.y, c4.x, c4.y); // Other side
    CV::line(c3.x, c3.y, c4.x, c4.y); // Tip
    // CV::line(c1.x, c1.y, c2.x, c2.y); // Base (optional, might be covered by turret)

    // Draw the movement vector line
    CV::color(1, 0, 0); // Red for movement vector
    CV::line(position.x, position.y, position.x + forwardVector.x * 50, position.y + forwardVector.y * 50); // Length 50 for visibility
}

// New method for collision detection and response
void Tanque::CheckCollisionAndRespond(BSplineTrack* track) {
    if (!track) {
        this->isColliding = false;
        return;
    }

    Vector2 currentTankPosition = this->position; // Current tentative position of the tank's center

    // Calculate tank's world corners based on currentTankPosition and baseAngle
    float halfW = this->baseWidth / 2.0f;
    float halfH = this->baseHeight / 2.0f;
    float cosB = cos(this->baseAngle);
    float sinB = sin(this->baseAngle);

    Vector2 local_corners[4] = {
        Vector2(-halfW, -halfH), Vector2(halfW, -halfH),
        Vector2(halfW,  halfH), Vector2(-halfW,  halfH)
    };
    Vector2 world_corners[4];
    for (int i = 0; i < 4; ++i) {
        world_corners[i].x = local_corners[i].x * cosB - local_corners[i].y * sinB + currentTankPosition.x;
        world_corners[i].y = local_corners[i].x * sinB + local_corners[i].y * cosB + currentTankPosition.y;
    }

    ClosestPointInfo cpiLeft = track->findClosestPointOnCurve(currentTankPosition, CurveSide::Left);
    ClosestPointInfo cpiRight = track->findClosestPointOnCurve(currentTankPosition, CurveSide::Right);

    bool collisionThisFrame = false;
    // Store projections for debugging if needed, ensure they are initialized.
    // float max_proj_left_debug = -FLT_MAX;
    // float max_proj_right_debug = -FLT_MAX;

    // Check collision with Left Curve
    if (cpiLeft.isValid) {
        float max_projection_left = -FLT_MAX;
        for (int i = 0; i < 4; ++i) {
            Vector2 vec_corner_to_cl_point = world_corners[i] - cpiLeft.point;
            float projection = vec_corner_to_cl_point.x * cpiLeft.normal.x + vec_corner_to_cl_point.y * cpiLeft.normal.y;
            if (projection > max_projection_left) {
                max_projection_left = projection;
            }
        }
        // Assuming cpiLeft.normal points "outward" from the track (to the left of the LeftCurve).
        // If any corner's projection is positive, it's on the "outside".
        if (max_projection_left > 0.0f) {
            collisionThisFrame = true;
        }
    }

    // Check collision with Right Curve
    if (!collisionThisFrame && cpiRight.isValid) {
        // cpiRight.normal is the "left normal" of the RightCurve.
        // A collision occurs if any tank corner is to the "right" of the RightCurve.
        // This means the projection of (corner - closest_point_on_curve) onto normal_R should be negative.
        // We look for the most negative projection (min_projection_right).
        float min_projection_right = FLT_MAX; 
        for (int i = 0; i < 4; ++i) {
            Vector2 vec_corner_to_cr_point = world_corners[i] - cpiRight.point;
            float projection = vec_corner_to_cr_point.x * cpiRight.normal.x + vec_corner_to_cr_point.y * cpiRight.normal.y;
            if (projection < min_projection_right) {
                min_projection_right = projection;
            }
        }
        // If the smallest projection (most "to the right" relative to N_R) is negative,
        // it means a part of the tank is to the right of the RightCurve (outside the track).
        if (min_projection_right < 0.0f) { 
            collisionThisFrame = true;
        }
    }

    if (collisionThisFrame) {
        this->isColliding = true;
        this->collisionTimer = COLLISION_REBOUND_FRAMES;
        this->position = this->lastSafePosition; // Revert to last known safe position
        // Optional: printf for debugging
        // printf("Collision! Left_Proj: %.2f, Right_Proj: %.2f Reverting to (%.2f, %.2f)\\\\n", max_proj_left_debug, max_proj_right_debug, this->position.x, this->position.y);
    } else {
        // No new collision detected by this check.
        // If isColliding was true due to an ongoing rebound, Update() handles the timer.
        // If timer runs out, Update() sets isColliding = false.
    }
}
