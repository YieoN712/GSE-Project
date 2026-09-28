#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

struct LevelPoint
{
    float x = 0;
    float z = 0;
};

enum class LevelTile : unsigned char
{
    Grass,
    Road,
    Building,
    Water
};

enum class BuildingStyle
{
    Shop,
    Home,
    Apartments,
    Utility
};

struct LevelBuilding
{
    LevelPoint position;
    int columns = 0;
    int rows = 0;
    BuildingStyle style = BuildingStyle::Home;
};

struct PlayerStats
{
    int level = 1;
    int experience = 0;
    int totalExperience = 0;
    int health = 100;
    int points = 0;
    int strength = 0;
    int vitality = 0;
    int resilience = 0;
    bool weapon = false;

    int MaximumHealth() const;
    int Attack() const;
    int Defense() const;
    int NextExperience() const;
    bool AddExperience(int amount);
    bool SpendPoint(int choice);
};

enum class EnemyState
{
    Roaming,
    Chasing,
    Windup,
    Recovery,
    Dead
};

struct LevelEnemy
{
    LevelPoint position;
    LevelPoint home;
    float heading = 0;
    float stride = 0;
    float timer = 0;
    float hitFlash = 0;
    int health = 40;
    int maximumHealth = 40;
    int kind = 0;
    EnemyState state = EnemyState::Roaming;
};

enum class DropKind
{
    Potion,
    Memory,
    Weapon
};

struct LevelDrop
{
    LevelPoint position;
    DropKind kind = DropKind::Memory;
};

struct LevelFeedback
{
    LevelPoint position;
    std::wstring text;
    float life = 1;
    bool positive = false;
};

struct LevelDust
{
    LevelPoint position;
    float age = 0;
};

// No OpenGL or Windows dependency: map topology, combat and progression live here.
class LevelOne
{
public:
    static constexpr int Columns = 44;
    static constexpr int Rows = 32;
    static constexpr float TileSize = 2;

    explicit LevelOne(std::uint32_t seed);
    void Reset(std::uint32_t seed);
    void Update(float dt);
    void KeyDown(unsigned char key);
    void KeyUp(unsigned char key);
    void ClearKeys();
    bool Walkable(int column, int row) const;
    bool Blocked(LevelPoint point) const;
    bool Visible(LevelPoint from, LevelPoint to) const;
    bool AllGroundConnected() const;
    LevelPoint Center(int column, int row) const;
    int Cell(LevelPoint position) const;
    int NearestDrop() const;

    std::array<LevelTile, Columns * Rows> tiles{};
    std::vector<LevelBuilding> buildings;
    std::vector<LevelPoint> trees;
    std::vector<LevelEnemy> enemies;
    std::vector<LevelDrop> drops;
    std::vector<LevelFeedback> feedback;
    std::vector<LevelDust> dust;
    PlayerStats stats;
    LevelPoint player;
    LevelPoint camera;
    LevelPoint spawn;
    std::uint32_t mapSeed = 0;
    float time = 0;
    float heading = 0;
    float stride = 0;
    float speed = 0;
    float attackTime = 0;
    float invulnerable = 0;
    float toastTime = 0;
    int potions = 3;
    int fragments = 0;
    int kills = 0;
    int reachableTiles = 0;
    bool paused = false;
    bool statsOpen = false;
    bool accomplished = false;
    std::wstring toast;

private:
    std::array<bool, 256> keys{};
    std::array<int, Columns * Rows> distances{};
    std::mt19937 random;
    float attackCooldown = 0;
    float pathTimer = 0;
    float stepDistance = 0;
    bool attackResolved = true;
    void Generate();
    void RebuildPaths();
    void Move(LevelPoint& position, float dx, float dz) const;
    void UpdateEnemies(float dt);
    void ResolveAttack();
    void Kill(LevelEnemy& enemy);
    void PickUp();
    void GainExperience(int amount);
    void Notify(const std::wstring& message);
};
