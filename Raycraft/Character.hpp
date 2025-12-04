#ifndef CHARACTER_HPP
#define CHARACTER_HPP

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>

// Forward declaration
class OptimizedWorld;

class Character {
private:
    // Character properties
    Vector3 position;
    Vector3 velocity;
    Vector3 size;
    float speed;
    float jumpForce;
    float gravity;
    bool isGrounded;
    bool isFlying;

    // Camera
    Camera3D camera;
    Vector2 lastMousePosition;
    float cameraPitch;
    float cameraYaw;
    float mouseSensitivity;

    // Movement states
    bool isWalking;
    bool isRunning;
    bool isJumping;
    float bobTimer;

    // Inventory/Interaction
    int selectedBlockType;
    float reachDistance;
    float breakProgress;
    float breakTimer;
    bool isBreaking;
    Vector3 breakingBlock;

    // World reference
    OptimizedWorld* world;

public:
    Character(OptimizedWorld* worldRef, Vector3 startPos = { 0, 10, 0 });
    ~Character() = default;

    void Update();
    void Draw();
    void DrawHUD();

    Camera3D& GetCamera() { return camera; }
    Vector3 GetPosition() const { return position; }
    Vector3 GetCameraPosition() const { return camera.position; }
    Vector3 GetForwardVector();
    Vector3 GetTargetPosition(float distance = 5.0f);

    int GetSelectedBlock() const { return selectedBlockType; }
    void SetSelectedBlock(int type) { selectedBlockType = type; }

    bool IsGrounded() const { return isGrounded; }
    bool IsRunning() const { return isRunning; }
    bool IsFlying() const { return isFlying; }
    void ToggleFlying() { isFlying = !isFlying; }

    // Raycasting for block interaction
    bool RaycastBlock(Vector3& hitPos, Vector3& normal, Vector3& blockPos, float maxDistance = 10.0f);

    // Collision methods
    bool CheckCollision(const Vector3& newPos) const;
    Vector3 ResolveCollision(const Vector3& newPos) const;

private:
    void HandleInput();
    void UpdateCamera();
    void UpdatePhysics();
    void UpdateCameraBobbing();

    // Collision helper
    bool BoxCollision(const Vector3& box1Min, const Vector3& box1Max,
        const Vector3& box2Min, const Vector3& box2Max) const;
};

#endif