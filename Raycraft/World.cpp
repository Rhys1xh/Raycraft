#include "World.hpp"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iostream>

// ==================== CHUNK IMPLEMENTATION ====================

Chunk::Chunk(int chunkX, int chunkZ)
    : x(chunkX), z(chunkZ), dirty(true), initialized(false) {
    blocks.resize(CHUNK_SIZE * WORLD_HEIGHT * CHUNK_SIZE, Block(BLOCK_AIR));
}

Chunk::~Chunk() {
    if (initialized) {
        UnloadModel(model);
        UnloadMesh(mesh);
    }
}

Block Chunk::GetBlock(int x, int y, int z) const {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= WORLD_HEIGHT || z < 0 || z >= CHUNK_SIZE) {
        return Block(BLOCK_AIR);
    }
    return blocks[(y * CHUNK_SIZE + z) * CHUNK_SIZE + x];
}

void Chunk::SetBlock(int x, int y, int z, Block block) {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= WORLD_HEIGHT || z < 0 || z >= CHUNK_SIZE) {
        return;
    }
    blocks[(y * CHUNK_SIZE + z) * CHUNK_SIZE + x] = block;
    dirty = true;
}

// Simplified mesh generation - we'll use immediate mode for now
void Chunk::GenerateMesh() {
    // Empty for now - we'll draw cubes directly
    dirty = false;
}

void Chunk::Draw() {
    // We don't use this for now - drawing is handled by OptimizedWorld
}

// ==================== WORLD IMPLEMENTATION ====================

OptimizedWorld::OptimizedWorld(int worldSeed) : seed(worldSeed), cacheDirty(true) {
    srand(seed);

    // Initialize chunks
    chunks.resize(CHUNK_COUNT_X, std::vector<Chunk*>(CHUNK_COUNT_Z, nullptr));
    for (int x = 0; x < CHUNK_COUNT_X; x++) {
        for (int z = 0; z < CHUNK_COUNT_Z; z++) {
            chunks[x][z] = new Chunk(x, z);
        }
    }

    GenerateTerrain();
}

OptimizedWorld::~OptimizedWorld() {
    for (auto& row : chunks) {
        for (auto chunk : row) {
            delete chunk;
        }
    }
}

void OptimizedWorld::Update(Vector3 playerPos) {
    // Update which blocks are visible based on player position
    UpdateVisibleBlocks(playerPos);

    // Update dirty chunks
    for (auto& row : chunks) {
        for (auto chunk : row) {
            if (chunk->dirty) {
                chunk->GenerateMesh();
            }
        }
    }
}

void OptimizedWorld::Draw() {
    // Draw all visible blocks from cache
    for (const auto& block : visibleBlocks) {
        Vector3 pos = std::get<0>(block);
        Color color = std::get<1>(block);
        bool isWater = std::get<2>(block);

        if (isWater) {
            DrawCube(pos, 1.0f, 0.9f, 1.0f, color);
            DrawCubeWires(pos, 1.0f, 0.9f, 1.0f, ColorAlpha(BLUE, 0.3f));
        }
        else {
            DrawCube(pos, 1.0f, 1.0f, 1.0f, color);
            DrawCubeWires(pos, 1.0f, 1.0f, 1.0f, ColorAlpha(BLACK, 0.1f));
        }
    }
}

Block OptimizedWorld::GetBlock(Vector3 worldPos) const {
    int x = (int)floorf(worldPos.x);
    int y = (int)floorf(worldPos.y);
    int z = (int)floorf(worldPos.z);

    if (y < 0 || y >= WORLD_HEIGHT) return Block(BLOCK_AIR);

    auto chunk = GetChunk(x, z);
    if (!chunk) return Block(BLOCK_AIR);

    auto [localX, localY, localZ] = WorldToLocalPos(x, y, z);
    return chunk->GetBlock(localX, localY, localZ);
}

void OptimizedWorld::SetBlock(Vector3 worldPos, Block block) {
    int x = (int)floorf(worldPos.x);
    int y = (int)floorf(worldPos.y);
    int z = (int)floorf(worldPos.z);

    if (y < 0 || y >= WORLD_HEIGHT) return;

    auto chunk = GetChunk(x, z);
    if (!chunk) return;

    auto [localX, localY, localZ] = WorldToLocalPos(x, y, z);
    chunk->SetBlock(localX, localY, localZ, block);
    cacheDirty = true;
}

void OptimizedWorld::PlaceBlock(Vector3 position, int blockType) {
    Vector3 blockPos = {
        floorf(position.x),
        floorf(position.y),
        floorf(position.z)
    };
    SetBlock(blockPos, Block((unsigned char)blockType));
}

void OptimizedWorld::BreakBlock(Vector3 position) {
    Vector3 blockPos = {
        floorf(position.x),
        floorf(position.y),
        floorf(position.z)
    };
    SetBlock(blockPos, Block(BLOCK_AIR));
}

bool OptimizedWorld::IsBlockAt(Vector3 position) const {
    return GetBlock(position).IsSolid();
}

void OptimizedWorld::GenerateTerrain() {
    const int worldSize = GetWorldSize();

    for (int x = 0; x < worldSize; x++) {
        for (int z = 0; z < worldSize; z++) {
            float noise = GetNoise(x * 0.05f, z * 0.05f);
            int height = 20 + (int)(noise * 15.0f);

            for (int y = 0; y < WORLD_HEIGHT; y++) {
                Vector3 pos = { (float)x, (float)y, (float)z };

                if (y > height) {
                    SetBlock(pos, Block(BLOCK_AIR));
                }
                else if (y == height) {
                    if (height < 22) {
                        SetBlock(pos, Block(BLOCK_SAND));
                    }
                    else if (height > 30) {
                        SetBlock(pos, Block(BLOCK_STONE));
                    }
                    else {
                        SetBlock(pos, Block(BLOCK_GRASS));
                    }
                }
                else if (y > height - 4) {
                    SetBlock(pos, Block(BLOCK_DIRT));
                }
                else {
                    SetBlock(pos, Block(BLOCK_STONE));
                }
            }

            // Add trees
            if (height >= 22 && height <= 30) {
                if ((rand() % 100) < 8) {
                    AddTree(x, height + 1, z);
                }
            }

            // Add water
            for (int y = 0; y < 16; y++) {
                Vector3 pos = { (float)x, (float)y, (float)z };
                if (y < height && GetBlock(pos).type == BLOCK_AIR) {
                    SetBlock(pos, Block(BLOCK_WATER));
                }
            }
        }
    }

    cacheDirty = true;
}

void OptimizedWorld::UpdateVisibleBlocks(Vector3 playerPos) {
    if (!cacheDirty) return;

    visibleBlocks.clear();
    const int worldSize = GetWorldSize();
    const int renderDistance = 24;

    // Calculate visible area around player
    int minX = std::max(0, (int)playerPos.x - renderDistance);
    int maxX = std::min(worldSize - 1, (int)playerPos.x + renderDistance);
    int minZ = std::max(0, (int)playerPos.z - renderDistance);
    int maxZ = std::min(worldSize - 1, (int)playerPos.z + renderDistance);

    // Frustum culling (simplified - just distance based)
    for (int x = minX; x <= maxX; x++) {
        for (int z = minZ; z <= maxZ; z++) {
            for (int y = 0; y < WORLD_HEIGHT; y++) {
                Vector3 pos = { (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f };
                Block block = GetBlock({ (float)x, (float)y, (float)z });

                if (block.type == BLOCK_AIR) continue;

                // Simple visibility check: check if any neighbor is air
                bool visible =
                    GetBlock({ (float)x + 1, (float)y, (float)z }).IsTransparent() ||
                    GetBlock({ (float)x - 1, (float)y, (float)z }).IsTransparent() ||
                    GetBlock({ (float)x, (float)y + 1, (float)z }).IsTransparent() ||
                    GetBlock({ (float)x, (float)y - 1, (float)z }).IsTransparent() ||
                    GetBlock({ (float)x, (float)y, (float)z + 1 }).IsTransparent() ||
                    GetBlock({ (float)x, (float)y, (float)z - 1 }).IsTransparent();

                if (visible) {
                    visibleBlocks.push_back({ pos, block.GetColor(), block.type == BLOCK_WATER });
                }
            }
        }
    }

    cacheDirty = false;
}

float OptimizedWorld::GetNoise(float x, float z) const {
    // Fast noise function using sine waves
    float noise = 0;
    noise += sinf(x * 0.1f) * cosf(z * 0.1f) * 0.5f;
    noise += sinf(x * 0.3f + 1.0f) * cosf(z * 0.3f + 1.0f) * 0.25f;
    noise += sinf(x * 0.9f + 2.0f) * cosf(z * 0.9f + 2.0f) * 0.125f;
    return (noise + 1.0f) * 0.5f;
}

void OptimizedWorld::AddTree(int worldX, int worldY, int worldZ) {
    int trunkHeight = 3 + (rand() % 3);

    // Trunk
    for (int i = 0; i < trunkHeight; i++) {
        SetBlock({ (float)worldX, (float)(worldY + i), (float)worldZ }, Block(BLOCK_WOOD));
    }

    // Leaves
    int leavesStart = worldY + trunkHeight - 1;
    for (int dy = 0; dy < 3; dy++) {
        int radius = (dy == 0) ? 2 : (dy == 1) ? 3 : 2;
        for (int dx = -radius; dx <= radius; dx++) {
            for (int dz = -radius; dz <= radius; dz++) {
                // Fixed: Explicit cast to float
                float fdx = static_cast<float>(dx);
                float fdy = static_cast<float>(dy);
                float fdz = static_cast<float>(dz);
                float distance = sqrtf(fdx * fdx + fdz * fdz + fdy * fdy);

                if (distance <= static_cast<float>(radius)) {
                    SetBlock({ (float)(worldX + dx), (float)(leavesStart + dy), (float)(worldZ + dz) },
                        Block(BLOCK_LEAVES));
                }
            }
        }
    }
}

Chunk* OptimizedWorld::GetChunk(int worldX, int worldZ) const {
    auto [chunkX, chunkZ] = WorldToChunkPos(worldX, worldZ);
    if (chunkX < 0 || chunkX >= CHUNK_COUNT_X || chunkZ < 0 || chunkZ >= CHUNK_COUNT_Z) {
        return nullptr;
    }
    return chunks[chunkX][chunkZ];
}

std::pair<int, int> OptimizedWorld::WorldToChunkPos(int worldX, int worldZ) const {
    return {
        worldX / CHUNK_SIZE,
        worldZ / CHUNK_SIZE
    };
}

std::tuple<int, int, int> OptimizedWorld::WorldToLocalPos(int worldX, int worldY, int worldZ) const {
    return {
        worldX % CHUNK_SIZE,
        worldY,
        worldZ % CHUNK_SIZE
    };
}