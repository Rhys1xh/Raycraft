#include "Character.hpp"
#include "World.hpp"
#include <cmath>
#include <iostream>
#include <algorithm>

Character::Character(OptimizedWorld* worldRef, Vector3 startPos)
    : position(startPos),
    velocity({ 0, 0, 0 }),
    size({ 0.6f, 1.8f, 0.6f }),
    speed(4.0f),
    jumpForce(8.0f),
    gravity(20.0f),
    isGrounded(false),
    isFlying(false),
    cameraPitch(0.0f),
    cameraYaw(-90.0f),
    mouseSensitivity(0.1f),
    isWalking(false),
    isRunning(false),
    isJumping(false),
    bobTimer(0.0f),
    selectedBlockType(1),
    reachDistance(6.0f),
    breakProgress(0.0f),
    breakTimer(0.0f),
    isBreaking(false),
    world(worldRef) {

    // Initialize camera
    camera.position = position;
    camera.position.y += 1.6f; // Eye height
    camera.target = position;
    camera.target.z += 1.0f; // Look forward
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 70.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    lastMousePosition = GetMousePosition();
    HideCursor();
}

void Character::Update() {
    HandleInput();
    UpdateCamera();
    UpdatePhysics();
    UpdateCameraBobbing();

    // Update camera position (account for bobbing)
    Vector3 cameraPos = position;
    cameraPos.y += 1.6f;

    // Add bobbing effect when walking
    if (isWalking && isGrounded && !isFlying) {
        cameraPos.y += sinf(bobTimer * 10.0f) * 0.05f;
    }

    camera.position = cameraPos;
    camera.target = Vector3Add(camera.position, GetForwardVector());

    // Update breaking progress
    if (isBreaking) {
        breakTimer += GetFrameTime();
        breakProgress = breakTimer / 1.0f; // 1 second to break a block

        if (breakTimer >= 1.0f) {
            world->BreakBlock(breakingBlock);
            isBreaking = false;
            breakProgress = 0.0f;
            breakTimer = 0.0f;
        }
    }
}

void Character::HandleInput() {
    // Mouse input
    Vector2 mouseDelta = Vector2Subtract(GetMousePosition(), lastMousePosition);
    lastMousePosition = GetMousePosition();

    cameraYaw += mouseDelta.x * mouseSensitivity;
    cameraPitch -= mouseDelta.y * mouseSensitivity;

    // Clamp pitch to prevent flipping
    if (cameraPitch > 89.0f) cameraPitch = 89.0f;
    if (cameraPitch < -89.0f) cameraPitch = -89.0f;

    // Movement
    Vector3 moveDirection = { 0, 0, 0 };
    isWalking = false;

    Vector3 forward = GetForwardVector();
    forward.y = 0;
    Vector3 forwardNormalized = Vector3Normalize(forward);

    Vector3 right = Vector3CrossProduct(forwardNormalized, Vector3{ 0.0f, 1.0f, 0.0f });
    right = Vector3Normalize(right);

    if (IsKeyDown(KEY_W)) {
        moveDirection = Vector3Add(moveDirection, forwardNormalized);
        isWalking = true;
    }
    if (IsKeyDown(KEY_S)) {
        moveDirection = Vector3Subtract(moveDirection, forwardNormalized);
        isWalking = true;
    }
    if (IsKeyDown(KEY_A)) {
        moveDirection = Vector3Subtract(moveDirection, right);
        isWalking = true;
    }
    if (IsKeyDown(KEY_D)) {
        moveDirection = Vector3Add(moveDirection, right);
        isWalking = true;
    }

    // Sprint
    isRunning = IsKeyDown(KEY_LEFT_SHIFT);
    float currentSpeed = isRunning ? speed * 1.8f : speed;

    // Flying movement (up/down)
    if (isFlying) {
        if (IsKeyDown(KEY_SPACE)) {
            moveDirection.y += 1.0f;
            isWalking = true;
        }
        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            moveDirection.y -= 1.0f;
            isWalking = true;
        }
    }

    // Normalize and apply movement
    if (Vector3Length(moveDirection) > 0) {
        moveDirection = Vector3Normalize(moveDirection);
        velocity.x = moveDirection.x * currentSpeed;
        velocity.z = moveDirection.z * currentSpeed;
        if (isFlying) {
            velocity.y = moveDirection.y * currentSpeed;
        }
    }
    else if (!isFlying) {
        velocity.x *= 0.9f;
        velocity.z *= 0.9f;
    }

    // Jump (only if not flying)
    if (!isFlying && IsKeyPressed(KEY_SPACE) && isGrounded) {
        velocity.y = jumpForce;
        isJumping = true;
    }

    // Toggle flying mode
    if (IsKeyPressed(KEY_F)) {
        ToggleFlying();
        if (isFlying) {
            velocity.y = 0;
        }
    }

    // Block selection
    if (IsKeyPressed(KEY_ONE)) selectedBlockType = 1;
    if (IsKeyPressed(KEY_TWO)) selectedBlockType = 2;
    if (IsKeyPressed(KEY_THREE)) selectedBlockType = 3;
    if (IsKeyPressed(KEY_FOUR)) selectedBlockType = 4;
    if (IsKeyPressed(KEY_FIVE)) selectedBlockType = 5;

    // Block interaction
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        Vector3 hitPos, normal, blockPos;
        if (RaycastBlock(hitPos, normal, blockPos, reachDistance)) {
            if (!isBreaking || !Vector3Equals(breakingBlock, blockPos)) {
                isBreaking = true;
                breakingBlock = blockPos;
                breakTimer = 0.0f;
            }
        }
        else {
            isBreaking = false;
            breakProgress = 0.0f;
        }
    }
    else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        if (isBreaking && breakProgress >= 1.0f) {
            world->BreakBlock(breakingBlock);
        }
        isBreaking = false;
        breakProgress = 0.0f;
        breakTimer = 0.0f;
    }

    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        Vector3 hitPos, normal, blockPos;
        if (RaycastBlock(hitPos, normal, blockPos, reachDistance)) {
            // Calculate placement position (adjacent to hit face)
            Vector3 placePos = {
                blockPos.x + normal.x,
                blockPos.y + normal.y,
                blockPos.z + normal.z
            };

            // Check if placement position is valid (not inside player)
            Vector3 playerPos = GetPosition();
            float distance = Vector3Distance(placePos, playerPos);
            if (distance > 1.5f) {
                world->PlaceBlock(placePos, selectedBlockType);
            }
        }
    }
}

void Character::UpdateCamera() {
    if (cameraYaw > 360.0f) cameraYaw -= 360.0f;
    if (cameraYaw < -360.0f) cameraYaw += 360.0f;
}

void Character::UpdatePhysics() {
    if (!isFlying) {
        velocity.y -= gravity * GetFrameTime();
    }

    Vector3 newPos = position;
    newPos.x += velocity.x * GetFrameTime();
    newPos.z += velocity.z * GetFrameTime();
    newPos.y += velocity.y * GetFrameTime();

    if (CheckCollision(newPos)) {
        Vector3 resolvedPos = ResolveCollision(newPos);
        position = resolvedPos;

        Vector3 belowPos = position;
        belowPos.y -= 0.1f;
        if (CheckCollision(belowPos)) {
            isGrounded = true;
            velocity.y = 0;
            isJumping = false;
        }
        else {
            isGrounded = false;
        }

        if (fabs(resolvedPos.x - newPos.x) > 0.01f) velocity.x = 0;
        if (fabs(resolvedPos.z - newPos.z) > 0.01f) velocity.z = 0;
    }
    else {
        position = newPos;
        isGrounded = false;
    }

    if (isGrounded && !isFlying) {
        velocity.x *= 0.8f;
        velocity.z *= 0.8f;
    }

    if (position.y < -10.0f) {
        position.y = 20.0f;
        velocity = Vector3{ 0, 0, 0 };
    }
}

void Character::UpdateCameraBobbing() {
    if (isWalking && (isGrounded || isFlying)) {
        bobTimer += GetFrameTime() * (isRunning ? 1.5f : 1.0f);
        if (bobTimer > 2 * PI) {
            bobTimer -= 2 * PI;
        }
    }
    else {
        bobTimer = 0.0f;
    }
}

Vector3 Character::GetForwardVector() {
    Vector3 direction;
    float yawRad = cameraYaw * DEG2RAD;
    float pitchRad = cameraPitch * DEG2RAD;

    direction.x = cosf(yawRad) * cosf(pitchRad);
    direction.y = sinf(pitchRad);
    direction.z = sinf(yawRad) * cosf(pitchRad);

    return Vector3Normalize(direction);
}

Vector3 Character::GetTargetPosition(float distance) {
    Vector3 forward = GetForwardVector();
    return Vector3Add(camera.position, Vector3Scale(forward, distance));
}

bool Character::RaycastBlock(Vector3& hitPos, Vector3& normal, Vector3& blockPos, float maxDistance) {
    Vector3 rayStart = camera.position;
    Vector3 rayDir = GetForwardVector();
    Vector3 rayEnd = Vector3Add(rayStart, Vector3Scale(rayDir, maxDistance));

    // Use raylib's built-in raycast for better performance
    Ray ray = { rayStart, rayDir };
    RayCollision collision = GetRayCollisionBox(ray, BoundingBox{ {0, 0, 0}, {(float)world->GetWorldSize(), (float)world->GetWorldSize(), (float)world->GetWorldSize()} });

    if (collision.hit) {
        // Step through the ray in small increments
        float stepSize = 0.1f;
        Vector3 currentPos = rayStart;
        float traveled = 0.0f;

        while (traveled < maxDistance) {
            currentPos = Vector3Add(rayStart, Vector3Scale(rayDir, traveled));

            // Check block at current position
            Vector3 checkPos = {
                floorf(currentPos.x),
                floorf(currentPos.y),
                floorf(currentPos.z)
            };

            if (world->IsBlockAt(checkPos)) {
                hitPos = currentPos;
                blockPos = checkPos;

                // Determine which face was hit
                Vector3 blockCenter = Vector3Add(checkPos, { 0.5f, 0.5f, 0.5f });
                Vector3 diff = Vector3Subtract(currentPos, blockCenter);

                // Find which face has the largest absolute difference
                float absX = fabsf(diff.x);
                float absY = fabsf(diff.y);
                float absZ = fabsf(diff.z);

                if (absX > absY && absX > absZ) {
                    normal = { diff.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f };
                }
                else if (absY > absX && absY > absZ) {
                    normal = { 0.0f, diff.y > 0 ? 1.0f : -1.0f, 0.0f };
                }
                else {
                    normal = { 0.0f, 0.0f, diff.z > 0 ? 1.0f : -1.0f };
                }

                return true;
            }

            traveled += stepSize;
        }
    }

    return false;
}

void Character::Draw() {
    if (IsKeyDown(KEY_F5)) { // Third person
        Vector3 modelPos = position;
        modelPos.y += size.y / 2.0f;

        Color playerColor = isFlying ? PURPLE : BLUE;
        DrawCube(modelPos, size.x, size.y, size.z, playerColor);
        DrawCubeWires(modelPos, size.x, size.y, size.z, DARKBLUE);
    }
}

void Character::DrawHUD() {
    // Draw breaking progress
    if (isBreaking && breakProgress > 0.0f) {
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        int barWidth = 100;
        int barHeight = 10;
        int barX = screenWidth / 2 - barWidth / 2;
        int barY = screenHeight / 2 + 50;

        DrawRectangle(barX, barY, barWidth, barHeight, GRAY);
        DrawRectangle(barX, barY, (int)(barWidth * breakProgress), barHeight, RED);
        DrawRectangleLines(barX, barY, barWidth, barHeight, WHITE);
    }
}

bool Character::BoxCollision(const Vector3& box1Min, const Vector3& box1Max,
    const Vector3& box2Min, const Vector3& box2Max) const {
    return (box1Min.x < box2Max.x && box1Max.x > box2Min.x &&
        box1Min.y < box2Max.y && box1Max.y > box2Min.y &&
        box1Min.z < box2Max.z && box1Max.z > box2Min.z);
}

bool Character::CheckCollision(const Vector3& newPos) const {
    if (!world) return false;

    Vector3 halfSize = Vector3Scale(size, 0.5f);
    Vector3 newBoxMin = { newPos.x - halfSize.x, newPos.y, newPos.z - halfSize.z };
    Vector3 newBoxMax = { newPos.x + halfSize.x, newPos.y + size.y, newPos.z + halfSize.z };

    // Check 9 key points in the bounding box (corners + center)
    std::vector<Vector3> checkPoints = {
        newBoxMin,
        {newBoxMax.x, newBoxMin.y, newBoxMin.z},
        {newBoxMin.x, newBoxMin.y, newBoxMax.z},
        {newBoxMax.x, newBoxMin.y, newBoxMax.z},
        {newBoxMin.x, newBoxMax.y, newBoxMin.z},
        {newBoxMax.x, newBoxMax.y, newBoxMin.z},
        {newBoxMin.x, newBoxMax.y, newBoxMax.z},
        newBoxMax,
        {newPos.x, newPos.y + size.y / 2, newPos.z}
    };

    for (const auto& point : checkPoints) {
        if (world->IsBlockAt(point)) {
            return true;
        }
    }

    return false;
}

Vector3 Character::ResolveCollision(const Vector3& newPos) const {
    Vector3 resolvedPos = position;

    // Try X
    Vector3 testPosX = resolvedPos;
    testPosX.x = newPos.x;
    if (!CheckCollision(testPosX)) {
        resolvedPos.x = newPos.x;
    }

    // Try Z
    Vector3 testPosZ = resolvedPos;
    testPosZ.z = newPos.z;
    if (!CheckCollision(testPosZ)) {
        resolvedPos.z = newPos.z;
    }

    // Try Y
    Vector3 testPosY = resolvedPos;
    testPosY.y = newPos.y;
    if (!CheckCollision(testPosY)) {
        resolvedPos.y = newPos.y;
    }
    else if (newPos.y < resolvedPos.y) {
        // Landing on ground
        resolvedPos.y = floorf(newPos.y) + 1.01f;
    }

    return resolvedPos;
}