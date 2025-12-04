#ifndef WORLD_HPP
#define WORLD_HPP

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <unordered_map>
#include <array>

// World constants
const int WORLD_HEIGHT = 64;
const int CHUNK_SIZE = 16;

// Optimized block types with integer IDs
enum BlockType : unsigned char {
    BLOCK_AIR = 0,
    BLOCK_GRASS,
    BLOCK_DIRT,
    BLOCK_STONE,
    BLOCK_WOOD,
    BLOCK_LEAVES,
    BLOCK_WATER,
    BLOCK_SAND,
    BLOCK_COUNT
};

// Optimized block data - just a byte
struct Block {
    unsigned char type;

    Block(unsigned char t = BLOCK_AIR) : type(t) {}

    Color GetColor() const {
        static const std::array<Color, BLOCK_COUNT> colors = { {
            BLANK,                     // AIR
            GREEN,                     // GRASS
            BROWN,                     // DIRT
            GRAY,                      // STONE
            Color{139, 69, 19, 255},   // WOOD
            Color{34, 139, 34, 200},   // LEAVES
            Color{0, 105, 148, 150},   // WATER
            Color{194, 178, 128, 255}  // SAND
        } };
        return colors[type];
    }

    bool IsTransparent() const {
        return type == BLOCK_AIR || type == BLOCK_LEAVES || type == BLOCK_WATER;
    }

    bool IsSolid() const {
        return type != BLOCK_AIR && type != BLOCK_WATER;
    }
};

// Chunk-based system for optimization
struct Chunk {
    int x, z;
    std::vector<Block> blocks;
    bool dirty;
    Mesh mesh;
    Model model;
    bool initialized;

    Chunk(int chunkX, int chunkZ);
    ~Chunk();

    Block GetBlock(int x, int y, int z) const;
    void SetBlock(int x, int y, int z, Block block);

    void GenerateMesh();
    void Draw();
};

// Optimized World
class OptimizedWorld {
private:
    static const int CHUNK_COUNT_X = 4;  // 4x4 chunks = 64x64 world
    static const int CHUNK_COUNT_Z = 4;

    std::vector<std::vector<Chunk*>> chunks;
    int seed;

    // Visible block cache for faster drawing
    std::vector<std::tuple<Vector3, Color, bool>> visibleBlocks;
    bool cacheDirty;

public:
    OptimizedWorld(int worldSeed = 1337);
    ~OptimizedWorld();

    void Update(Vector3 playerPos);
    void Draw();

    Block GetBlock(Vector3 worldPos) const;
    void SetBlock(Vector3 worldPos, Block block);
    void PlaceBlock(Vector3 position, int blockType);
    void BreakBlock(Vector3 position);

    bool IsBlockAt(Vector3 position) const;
    int GetWorldSize() const { return CHUNK_COUNT_X * CHUNK_SIZE; }

private:
    void GenerateTerrain();
    void UpdateVisibleBlocks(Vector3 playerPos);
    void AddTree(int worldX, int worldY, int worldZ);
    float GetNoise(float x, float z) const;

    // Helper to get chunk from world position
    Chunk* GetChunk(int worldX, int worldZ) const;
    std::pair<int, int> WorldToChunkPos(int worldX, int worldZ) const;
    std::tuple<int, int, int> WorldToLocalPos(int worldX, int worldY, int worldZ) const;
};

#endif