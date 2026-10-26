#pragma once

#include "SceneGraph.h"

#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

using LevelPoint = WorldPoint;

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

struct LevelBuilding : Actor
{
    LevelBuilding(LevelPoint point = {},
                  int width = 0,
                  int depth = 0,
                  BuildingStyle type = BuildingStyle::Home)
        : columns(width), rows(depth), style(type)
    {
        position = point;
        name = "Building";
    }

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

struct LevelEnemy : Actor
{
    LevelPoint home;
    float stride = 0;
    float timer = 0;
    float hitFlash = 0;
    int health = 40;
    int maximumHealth = 40;
    int kind = 0;
    EnemyState state = EnemyState::Roaming;
};

struct LevelResident : Actor
{
    std::wstring title;
    std::wstring role;
    LevelPoint home;
    std::vector<LevelPoint> route;
    size_t waypoint = 0;
    float stride = 0;
    float speed = 0;
    float rest = 0;
    int kind = 0;
    bool greeted = false;
};

struct NeighborhoodState
{
    int neighborsMet = 0;
    int outskirtsReports = 0;
    std::array<bool, 3> heardRumors{};
    int Reputation() const;
    const wchar_t* ReputationLabel() const;
    const wchar_t* Atmosphere() const;
    static const std::array<const wchar_t*, 3> Rumors;
};

enum class DropKind
{
    Potion,
    Memory,
    Weapon
};

struct LevelDrop : Actor
{
    LevelDrop(LevelPoint point = {}, DropKind type = DropKind::Memory) : kind(type)
    {
        position = point;
        name = "Drop";
    }

    DropKind kind = DropKind::Memory;
};

struct LevelFeedback
{
    LevelPoint position;
    std::wstring text;
    float life = 1;
    bool positive = false;
};

using LevelDust = DustActor;

struct LevelTree : MeshActor
{
    LevelTree(LevelPoint point = {})
    {
        position = point;
        model = ModelKind::Tree;
        name = "Tree";
    }
};

// No OpenGL or Windows dependency: map topology, combat and progression live here.
class LevelOne
{
public:
    static constexpr int Columns = 44;
    static constexpr int Rows = 32;
    static constexpr float TileSize = 2;
    static constexpr float TownRadius = 18;
    static constexpr float OutskirtsRadius = 24;

    explicit LevelOne(std::uint32_t seed);
    void Reset(std::uint32_t seed);
    PlayerActor* FindPlayer();
    const PlayerActor* FindPlayer() const;
    PlayerActor& Player();
    const PlayerActor& Player() const;
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
    const LevelResident* NearestResident() const;
    bool InTown(LevelPoint point) const;
    bool InOutskirts(LevelPoint point) const;

    SceneGraph scene;
    std::array<LevelTile, Columns * Rows> tiles{};
    ActorList<LevelBuilding> buildings;
    ActorList<LevelTree> trees;
    ActorList<LevelEnemy> enemies;
    ActorList<LevelResident> residents;
    ActorList<LevelDrop> drops;
    std::vector<LevelFeedback> feedback;
    ActorList<LevelDust> dust;
    PlayerStats stats;
    NeighborhoodState neighborhood;
    ActorId conversationActor = 0;
    float conversationTime = 0;
    std::vector<std::wstring> conversation;
    bool neighborhoodOpen = false;
    LevelPoint camera;
    LevelPoint spawn;
    std::uint32_t mapSeed = 0;
    float time = 0;
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
    ActorId playerId = 0;
    std::array<bool, 256> keys{};
    std::array<int, Columns * Rows> distances{};
    std::mt19937 random;
    float attackCooldown = 0;
    float pathTimer = 0;
    float stepDistance = 0;
    bool attackResolved = true;
    void Generate();
    void BuildScenery();
    void RebuildPaths();
    void Move(LevelPoint& position, float dx, float dz) const;
    void UpdateEnemies(float dt);
    void BuildResidents();
    void UpdateResidents(float dt);
    void PlanResidentWalk(LevelResident& resident);
    void TalkToResident();
    void ResolveAttack();
    void Kill(LevelEnemy& enemy);
    void PickUp();
    void GainExperience(int amount);
    void Notify(const std::wstring& message);
};
