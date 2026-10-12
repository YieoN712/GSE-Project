#pragma once
#include "SceneGraph.h"
#include <array>
#include <initializer_list>
#include <string>
#include <vector>

using GroundPoint = WorldPoint;

struct Resident : Actor
{
    Resident(const wchar_t* title = L"",
             const wchar_t* text = L"",
             GroundPoint start = {},
             GroundPoint point = {},
             float red = 1,
             float green = 1,
             float blue = 1)
        : name(title), greeting(text), home(start), r(red), g(green), b(blue)
    {
        position = point;
        Actor::name = "Resident";
    }

    const wchar_t* name;
    const wchar_t* greeting;
    GroundPoint home;
    float r, g, b;
    float stride = 0, walkSpeed = 0;
    int slot = -1;
};

using Footstep = DustActor;

struct Building : Actor
{
    Building(float x = 0,
             float z = 0,
             float w = 1,
             float d = 1,
             float h = 1,
             float red = 1,
             float green = 1,
             float blue = 1,
             const wchar_t* title = L"")
        : width(w), depth(d), buildingHeight(h), r(red), g(green), b(blue), sign(title)
    {
        position = {x, z};
        name = "Building";
    }

    float width, depth, buildingHeight;
    float r, g, b;
    const wchar_t* sign;
};

class TutorialProp : public MeshActor
{
public:
    int minimumStage = 0;
    int maximumStage = 100;
};

// Simulation is independent of the graphics context. All progression commits when dialogue closes.
class Tutorial
{
public:
    Tutorial();
    void Reset();
    PlayerActor* FindPlayer();
    const PlayerActor* FindPlayer() const;
    PlayerActor& Player();
    const PlayerActor& Player() const;
    const Resident* FindResident(int slot) const;
    void Update(float dt);
    void KeyDown(unsigned char key);
    void KeyUp(unsigned char key);
    void ClearKeys();
    int Target() const;
    GroundPoint ObjectivePosition() const;
    const wchar_t* Objective() const;
    const wchar_t* TargetName(int target) const;
    bool Blocked(GroundPoint p) const;

    SceneGraph scene;
    GroundPoint camera;
    ActorList<Resident> residents;
    ActorList<Building> buildings;
    ActorList<Footstep> footsteps;
    float stepDistance = 0;
    static constexpr float HalfWidth = 44, HalfDepth = 32;
    int stage = 0; // greet, deliver, photograph, inspect bench, speak to neighbor, complete
    float elapsed = 0, animation = 0, stalled = 0, shadowTime = -1;
    bool paused = false, journal = false, completion = false;
    std::vector<std::wstring> dialogue;
    size_t line = 0;
    int speaker = -1;
    static const GroundPoint board, bench;

private:
    ActorId playerId = 0;
    std::array<bool, 256> keys{};
    int pendingStage = -1;
    void BuildScenery();
    void UpdateMarkers();
    void Interact();
    void Say(int who, std::initializer_list<const wchar_t*> lines, int next = -1);
};

class RenderEffects;
class ModelCache;
class TutorialUi;

// Compatibility geometry with a shader material / shadow / postprocessing quality path.
class TutorialView
{
public:
    TutorialView();
    ~TutorialView();
    void Draw(const Tutorial& game, int width, int height);

private:
    std::unique_ptr<TutorialUi> ui;
    std::unique_ptr<ModelCache> models;
    std::unique_ptr<RenderEffects> effects;
};
