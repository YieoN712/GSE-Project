#pragma once
#include <array>
#include <initializer_list>
#include <string>
#include <vector>

struct GroundPoint
{
    float x, z;
};

struct Resident
{
    const wchar_t* name;
    const wchar_t* greeting;
    GroundPoint home, position;
    float r, g, b;
    float heading = 0, stride = 0, walkSpeed = 0;
};

struct Footstep
{
    GroundPoint position;
    float age;
};

struct Building
{
    float x, z, width, depth, height;
    float r, g, b;
    const wchar_t* sign;
};

// Simulation is independent of the graphics context. All progression commits when dialogue closes.
class Tutorial
{
public:
    Tutorial();
    void Reset();
    void Update(float dt);
    void KeyDown(unsigned char key);
    void KeyUp(unsigned char key);
    void ClearKeys();
    int Target() const;
    GroundPoint ObjectivePosition() const;
    const wchar_t* Objective() const;
    const wchar_t* TargetName(int target) const;
    bool Blocked(GroundPoint p) const;

    GroundPoint player, camera;
    std::array<Resident, 8> residents;
    std::vector<Building> buildings;
    std::vector<Footstep> footsteps;
    float playerHeading = 0, playerStride = 0, playerSpeed = 0, stepDistance = 0;
    static constexpr float HalfWidth = 44, HalfDepth = 32;
    int stage = 0; // greet, deliver, photograph, inspect bench, speak to neighbor, complete
    float elapsed = 0, animation = 0, stalled = 0, shadowTime = -1;
    bool paused = false, journal = false, completion = false;
    std::vector<std::wstring> dialogue;
    size_t line = 0;
    int speaker = -1;
    static const GroundPoint board, bench;

private:
    std::array<bool, 256> keys{};
    int pendingStage = -1;
    void Interact();
    void Say(int who, std::initializer_list<const wchar_t*> lines, int next = -1);
};

class RenderEffects;

// Compatibility geometry with a shader material / shadow / postprocessing quality path.
class TutorialView
{
public:
    TutorialView();
    ~TutorialView();
    void Draw(const Tutorial& game, int width, int height);

private:
    struct Font;
    Font* font;
    RenderEffects* effects;
};
