#include "LevelOne.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <stdexcept>

namespace
{
    float Distance(LevelPoint a, LevelPoint b)
    {
        return std::hypot(a.x - b.x, a.z - b.z);
    }

    unsigned char Lower(unsigned char key)
    {
        return key >= 'A' && key <= 'Z' ? key + ('a' - 'A') : key;
    }

    float Clamp(float value, float low, float high)
    {
        return (std::max)(low, (std::min)(value, high));
    }
} // namespace

int PlayerStats::MaximumHealth() const
{
    return 100 + (level - 1) * 16 + vitality * 12;
}

int PlayerStats::Attack() const
{
    return 14 + (level - 1) * 4 + strength * 2 + (weapon ? 3 : 0);
}

int PlayerStats::Defense() const
{
    return 2 + (level - 1) + resilience;
}

int PlayerStats::NextExperience() const
{
    return 40 + (level - 1) * 20;
}

bool PlayerStats::AddExperience(int amount)
{
    if (amount <= 0 || level >= 20)
    {
        return false;
    }

    experience += amount;
    totalExperience += amount;
    bool leveled = false;

    while (level < 20 && experience >= NextExperience())
    {
        experience -= NextExperience();
        ++level;
        ++points;
        health = (std::min)(MaximumHealth(), health + 16);
        leveled = true;
    }

    if (level == 20)
    {
        experience = 0;
    }

    return leveled;
}

bool PlayerStats::SpendPoint(int choice)
{
    if (points <= 0 || choice < 1 || choice > 3)
    {
        return false;
    }

    --points;

    if (choice == 1)
    {
        ++strength;
    }
    else if (choice == 2)
    {
        ++vitality;
        health += 12;
    }
    else
    {
        ++resilience;
    }

    return true;
}

LevelOne::LevelOne(std::uint32_t seed)
    : buildings(scene), trees(scene), enemies(scene), residents(scene), drops(scene), dust(scene)
{
    Reset(seed);
}

const std::array<const wchar_t*, 3> NeighborhoodState::Rumors = {
    L"해 질 무렵, 외곽 길에서 발소리가 하나 더 들린대요.",
    L"비어 있는 우편함에 누가 매일 새 이름표를 붙인대요.",
    L"귀갓길을 살펴 주는 이웃이 생겨 조금 안심된대요."};

int NeighborhoodState::Reputation() const
{
    return neighborsMet + outskirtsReports * 2;
}

const wchar_t* NeighborhoodState::ReputationLabel() const
{
    return Reputation() >= 12  ? L"믿고 인사하는 이웃"
           : Reputation() >= 4 ? L"낯익은 얼굴"
                               : L"아직 낯선 이웃";
}

const wchar_t* NeighborhoodState::Atmosphere() const
{
    return outskirtsReports > 0 ? L"귀갓길 이야기를 나누며 조금 안심하는 동네"
                                : L"밝은 일상 속에 외곽을 걱정하는 동네";
}

bool LevelOne::InTown(LevelPoint point) const
{
    return Distance(point, spawn) <= TownRadius;
}

bool LevelOne::InOutskirts(LevelPoint point) const
{
    return Distance(point, spawn) >= OutskirtsRadius;
}

LevelPoint LevelOne::Center(int column, int row) const
{
    return {(column - Columns / 2 + .5f) * TileSize, (row - Rows / 2 + .5f) * TileSize};
}

int LevelOne::Cell(LevelPoint position) const
{
    int x = int(std::floor(position.x / TileSize + Columns / 2));
    int z = int(std::floor(position.z / TileSize + Rows / 2));

    if (x < 0 || x >= Columns || z < 0 || z >= Rows)
    {
        return -1;
    }

    return z * Columns + x;
}

bool LevelOne::Walkable(int column, int row) const
{
    if (column < 0 || column >= Columns || row < 0 || row >= Rows)
    {
        return false;
    }

    LevelTile tile = tiles[row * Columns + column];
    return tile == LevelTile::Grass || tile == LevelTile::Road;
}

bool LevelOne::AllGroundConnected() const
{
    std::array<bool, Columns * Rows> visited{};
    std::queue<int> pending;
    const int start = Cell(spawn);

    if (start < 0 || !Walkable(start % Columns, start / Columns))
    {
        return false;
    }

    visited[start] = true;
    pending.push(start);

    while (!pending.empty())
    {
        int current = pending.front();
        pending.pop();
        int x = current % Columns;
        int z = current / Columns;

        for (auto delta :
             {LevelPoint{1, 0}, LevelPoint{-1, 0}, LevelPoint{0, 1}, LevelPoint{0, -1}})
        {
            int nx = x + int(delta.x);
            int nz = z + int(delta.z);

            if (!Walkable(nx, nz))
            {
                continue;
            }

            int next = nz * Columns + nx;

            if (!visited[next])
            {
                visited[next] = true;
                pending.push(next);
            }
        }
    }

    for (int i = 0; i < Columns * Rows; ++i)
    {
        if (Walkable(i % Columns, i / Columns) && !visited[i])
        {
            return false;
        }
    }

    return true;
}

void LevelOne::Generate()
{
    tiles.fill(LevelTile::Grass);
    buildings.clear();
    trees.clear();
    spawn = Center(Columns / 2, Rows / 2);

    // A connected cross plus two ring roads is reserved before obstacles are placed.
    for (int z = 0; z < Rows; ++z)
    {
        for (int x = 0; x < Columns; ++x)
        {
            if (std::abs(x - Columns / 2) <= 1 || std::abs(z - Rows / 2) <= 1 || x == 5 ||
                x == Columns - 6 || z == 5 || z == Rows - 6)
            {
                tiles[z * Columns + x] = LevelTile::Road;
            }
        }
    }

    // Four neighborhood landmarks surround the central cross; random plots fill the rest.
    for (int index = 0; index < 4; ++index)
    {
        int x = index % 2 == 0 ? 16 : 25;
        int z = index < 2 ? 11 : 19;
        for (int row = z; row < z + 2; ++row)
        {
            for (int col = x; col < x + 3; ++col)
            {
                tiles[row * Columns + col] = LevelTile::Building;
            }
        }
        LevelPoint corner = Center(x, z);
        buildings.push_back({{corner.x + 2, corner.z + 1}, 3, 2, BuildingStyle(index)});
    }

    for (int attempt = 0; attempt < 180 && buildings.size() < 20; ++attempt)
    {
        int width = 2 + int(random() % 3);
        int depth = 2 + int(random() % 2);
        int x = 2 + int(random() % (Columns - width - 4));
        int z = 2 + int(random() % (Rows - depth - 4));
        bool free = true;

        // A one-tile margin preserves a physically traversable sidewalk, not a point-sized gap.
        for (int row = z - 1; row <= z + depth; ++row)
        {
            for (int col = x - 1; col <= x + width; ++col)
            {
                if (tiles[row * Columns + col] != LevelTile::Grass ||
                    Distance(Center(col, row), spawn) < 7)
                {
                    free = false;
                }
            }
        }

        if (!free)
        {
            continue;
        }

        bool pond = attempt % 5 == 0;

        for (int row = z; row < z + depth; ++row)
        {
            for (int col = x; col < x + width; ++col)
            {
                tiles[row * Columns + col] = pond ? LevelTile::Water : LevelTile::Building;
            }
        }

        if (!AllGroundConnected())
        {
            for (int row = z; row < z + depth; ++row)
            {
                for (int col = x; col < x + width; ++col)
                {
                    tiles[row * Columns + col] = LevelTile::Grass;
                }
            }

            continue;
        }

        if (!pond)
        {
            LevelPoint corner = Center(x, z);
            buildings.push_back({{corner.x + (width - 1), corner.z + (depth - 1)},
                                 width,
                                 depth,
                                 BuildingStyle(buildings.size() % 4)});
        }
    }

    if (!AllGroundConnected())
    {
        throw std::runtime_error("Level generation connectivity invariant failed.");
    }

    reachableTiles = 0;
    std::vector<int> enemyCells;

    for (int i = 0; i < Columns * Rows; ++i)
    {
        if (!Walkable(i % Columns, i / Columns))
        {
            continue;
        }

        ++reachableTiles;
        LevelPoint point = Center(i % Columns, i / Columns);

        if (Distance(point, spawn) >= OutskirtsRadius + 2)
        {
            enemyCells.push_back(i);
        }

        // Decorative trees have no collider; they cannot invalidate the navigation proof.
        if (tiles[i] == LevelTile::Grass && random() % 24 == 0 && Distance(point, spawn) > 6)
        {
            trees.push_back(point);
        }
    }

    std::shuffle(enemyCells.begin(), enemyCells.end(), random);
    enemies.clear();

    for (int cell : enemyCells)
    {
        LevelPoint point = Center(cell % Columns, cell / Columns);
        bool spaced = true;

        for (const auto& enemy : enemies)
        {
            if (Distance(point, enemy.home) < 5)
            {
                spaced = false;
            }
        }

        if (spaced)
        {
            LevelEnemy enemy;
            enemy.name = "Echo";
            enemy.position = enemy.home = point;
            enemy.kind = int(enemies.size() % 3);
            enemy.maximumHealth = enemy.health = 34 + enemy.kind * 12;
            enemies.push_back(enemy);
        }

        if (enemies.size() == 18)
        {
            break;
        }
    }
}

void LevelOne::Reset(std::uint32_t seed)
{
    if (!FindPlayer())
    {
        playerId = scene.Spawn<PlayerActor>(0).Id();
    }
    scene.Reparent(Player().Id(), 0);
    mapSeed = seed;
    random.seed(seed);
    ActorList<MeshActor>(scene).clear();
    ActorList<MarkerActor>(scene).clear();
    stats = PlayerStats{};
    neighborhood = NeighborhoodState{};
    residents.clear();
    neighborhoodOpen = false;
    conversationActor = 0;
    conversationTime = 0;
    conversation.clear();
    drops.clear();
    feedback.clear();
    dust.clear();
    potions = 3;
    fragments = kills = 0;
    time = Player().heading = Player().stride = Player().speed = attackTime = invulnerable = 0;
    attackCooldown = pathTimer = stepDistance = 0;
    attackResolved = true;
    paused = statsOpen = accomplished = false;
    ClearKeys();
    Generate();
    BuildResidents();
    BuildScenery();
    Player().name = "Player";
    Player().active = Player().visible = true;
    Player().scale = {};
    Player().height = 0;
    scene.Reparent(Player().Id(), 0);
    Player().position = camera = spawn;
    drops.push_back({{spawn.x + 2, spawn.z}, DropKind::Potion});
    RebuildPaths();
    Notify(L"레벨 1 · 먼저 이웃에게 인사해 보세요. 잔향은 동네 너머 외곽에 있습니다. [E] 대화 "
           L"[Tab] 동네");
}

bool LevelOne::Blocked(LevelPoint point) const
{
    constexpr float radius = .32f;
    int left = int(std::floor((point.x - radius) / TileSize + Columns / 2));
    int right = int(std::floor((point.x + radius) / TileSize + Columns / 2));
    int top = int(std::floor((point.z - radius) / TileSize + Rows / 2));
    int bottom = int(std::floor((point.z + radius) / TileSize + Rows / 2));

    for (int z = top; z <= bottom; ++z)
    {
        for (int x = left; x <= right; ++x)
        {
            if (Walkable(x, z))
            {
                continue;
            }

            LevelPoint center = Center(x, z);
            float nx = Clamp(point.x, center.x - 1, center.x + 1);
            float nz = Clamp(point.z, center.z - 1, center.z + 1);

            if (std::hypot(point.x - nx, point.z - nz) < radius)
            {
                return true;
            }
        }
    }

    return false;
}

void LevelOne::Move(LevelPoint& position, float dx, float dz) const
{
    if (!Blocked({position.x + dx, position.z}))
    {
        position.x += dx;
    }

    if (!Blocked({position.x, position.z + dz}))
    {
        position.z += dz;
    }
}

bool LevelOne::Visible(LevelPoint from, LevelPoint to) const
{
    int steps = (std::max)(1, int(std::ceil(Distance(from, to) / .2f)));

    for (int i = 1; i <= steps; ++i)
    {
        float t = float(i) / steps;

        if (Blocked({from.x + (to.x - from.x) * t, from.z + (to.z - from.z) * t}))
        {
            return false;
        }
    }

    return true;
}

void LevelOne::RebuildPaths()
{
    distances.fill(-1);
    int start = Cell(Player().position);
    if (start < 0)
    {
        return;
    }

    std::queue<int> queue;
    distances[start] = 0;
    queue.push(start);

    while (!queue.empty())
    {
        int current = queue.front();
        queue.pop();

        for (auto delta :
             {LevelPoint{1, 0}, LevelPoint{-1, 0}, LevelPoint{0, 1}, LevelPoint{0, -1}})
        {
            int x = current % Columns + int(delta.x);
            int z = current / Columns + int(delta.z);

            if (!Walkable(x, z) || !InOutskirts(Center(x, z)))
            {
                continue;
            }

            int next = z * Columns + x;

            if (distances[next] == -1)
            {
                distances[next] = distances[current] + 1;
                queue.push(next);
            }
        }
    }
}

void LevelOne::Notify(const std::wstring& message)
{
    toast = message;
    toastTime = 5;
}

void LevelOne::GainExperience(int amount)
{
    bool leveled = stats.AddExperience(amount);
    feedback.push_back({Player().position, L"+" + std::to_wstring(amount) + L" EXP", 1.4f, true});

    if (leveled)
    {
        Notify(L"레벨 업! HP +16 / 공격 +4 / 방어 +1 / 배분 포인트 +1  ·  C로 확인");
    }
}

void LevelOne::Kill(LevelEnemy& enemy)
{
    enemy.state = EnemyState::Dead;
    enemy.timer = 12;
    ++kills;
    GainExperience(20 + enemy.kind * 5);
    DropKind kind =
        kills == 3 ? DropKind::Weapon : (kills % 2 == 0 ? DropKind::Potion : DropKind::Memory);

    // Bound accumulated uncollected loot during indefinite farming.
    if (drops.size() >= 96)
    {
        drops.erase_at(0);
    }

    drops.push_back({enemy.position, kind});
}

void LevelOne::ResolveAttack()
{
    for (auto& enemy : enemies)
    {
        if (!scene.IsActive(enemy.Id()))
        {
            continue;
        }
        float distance = Distance(Player().position, enemy.position);
        float dot = (enemy.position.x - Player().position.x) * std::sin(Player().heading) +
                    (enemy.position.z - Player().position.z) * std::cos(Player().heading);

        if (enemy.state == EnemyState::Dead || distance > 2.3f ||
            (distance > .5f && dot / distance < .10f) ||
            !Visible(Player().position, enemy.position))
        {
            continue;
        }

        int damage = (std::max)(1, stats.Attack() - enemy.kind);
        enemy.health = (std::max)(0, enemy.health - damage);
        enemy.hitFlash = .2f;
        feedback.push_back({enemy.position, std::to_wstring(damage), .8f, false});

        if (enemy.health == 0)
        {
            Kill(enemy);
        }
    }
}

void LevelOne::UpdateEnemies(float dt)
{
    for (auto& enemy : enemies)
    {
        if (!scene.IsActive(enemy.Id()))
        {
            continue;
        }
        enemy.hitFlash = (std::max)(0.f, enemy.hitFlash - dt);
        enemy.timer -= dt;
        float distance = Distance(Player().position, enemy.position);

        if (enemy.state == EnemyState::Dead)
        {
            if (enemy.timer <= 0 && Distance(Player().position, enemy.home) > 5)
            {
                enemy.position = enemy.home;
                enemy.health = enemy.maximumHealth;
                enemy.state = EnemyState::Roaming;
            }

            continue;
        }

        if (enemy.state == EnemyState::Windup)
        {
            if (enemy.timer <= 0)
            {
                if (scene.IsActive(Player().Id()) && distance < 1.65f && invulnerable <= 0 &&
                    InOutskirts(Player().position) && Visible(enemy.position, Player().position))
                {
                    int damage = (std::max)(1, 12 + enemy.kind * 4 - stats.Defense());
                    stats.health = (std::max)(0, stats.health - damage);
                    invulnerable = .65f;
                    feedback.push_back(
                        {Player().position, L"-" + std::to_wstring(damage), 1, false});
                }

                enemy.state = EnemyState::Recovery;
                enemy.timer = 1.0f;
            }

            continue;
        }

        if (enemy.state == EnemyState::Recovery && enemy.timer > 0)
        {
            continue;
        }

        bool aggro =
            scene.IsActive(Player().Id()) && distance < 10 && InOutskirts(Player().position);
        enemy.state = aggro ? EnemyState::Chasing : EnemyState::Roaming;

        if (aggro && distance < 1.3f && Visible(enemy.position, Player().position))
        {
            enemy.heading = std::atan2(Player().position.x - enemy.position.x,
                                       Player().position.z - enemy.position.z);
            enemy.state = EnemyState::Windup;
            enemy.timer = .65f;
            continue;
        }

        LevelPoint target = {enemy.home.x + std::sin(time * .25f + enemy.kind),
                             enemy.home.z + std::cos(time * .21f + enemy.kind)};

        if (aggro)
        {
            if (Visible(enemy.position, Player().position))
            {
                target = Player().position;
            }
            else
            {
                int cell = Cell(enemy.position);
                int best = cell;

                for (auto delta :
                     {LevelPoint{1, 0}, LevelPoint{-1, 0}, LevelPoint{0, 1}, LevelPoint{0, -1}})
                {
                    int x = cell % Columns + int(delta.x);
                    int z = cell / Columns + int(delta.z);
                    int next = z * Columns + x;

                    if (Walkable(x, z) && InOutskirts(Center(x, z)) && distances[next] >= 0 &&
                        distances[next] < distances[best])
                    {
                        best = next;
                    }
                }

                target = Center(best % Columns, best / Columns);

                // Recenter before turning a blocked corner.
                if (!Visible(enemy.position, target))
                {
                    target = Center(cell % Columns, cell / Columns);
                }
            }
        }

        float length = Distance(enemy.position, target);

        if (length > .05f)
        {
            float movement = (std::min)(length, dt * (aggro ? 1.9f : .55f));
            LevelPoint previous = enemy.position;
            Move(enemy.position,
                 (target.x - previous.x) / length * movement,
                 (target.z - previous.z) / length * movement);
            if (!InOutskirts(enemy.position))
            {
                enemy.position = previous;
            }
            enemy.stride += Distance(previous, enemy.position) * 6;
            enemy.heading = std::atan2(target.x - previous.x, target.z - previous.z);
        }
    }
}

void LevelOne::BuildResidents()
{
    const wchar_t* titles[] = {L"빵집 사장",
                               L"꽃집 이웃",
                               L"우편 배달원",
                               L"산책하는 어르신",
                               L"학생",
                               L"편의점 직원",
                               L"청소하는 이웃",
                               L"강아지를 기다리는 주민"};
    const wchar_t* roles[] = {L"가게 돌보기",
                              L"꽃 돌보기",
                              L"우편 배달",
                              L"산책",
                              L"등굣길",
                              L"쉬는 시간",
                              L"동네 청소",
                              L"이웃 기다리기"};
    const LevelPoint offsets[] = {
        {-4, 0}, {4, 0}, {0, -8}, {0, 8}, {-12, 0}, {12, 0}, {0, -12}, {0, 12}};
    for (int index = 0; index < 8; ++index)
    {
        auto& resident = scene.Spawn<LevelResident>(0);
        resident.name = "Neighborhood resident";
        resident.title = titles[index];
        resident.role = roles[index];
        resident.kind = index;
        resident.home =
            resident.position = {spawn.x + offsets[index].x, spawn.z + offsets[index].z};
        resident.rest = 1.f + index * .7f;
    }
}

void LevelOne::PlanResidentWalk(LevelResident& resident)
{
    resident.route.clear();
    resident.waypoint = 0;
    std::vector<int> destinations;
    for (int cell = 0; cell < Columns * Rows; ++cell)
    {
        if (tiles[cell] == LevelTile::Road &&
            Distance(Center(cell % Columns, cell / Columns), spawn) < TownRadius - 1)
        {
            destinations.push_back(cell);
        }
    }
    const int start = Cell(resident.position);
    if (start < 0 || destinations.empty())
    {
        return;
    }
    const int target = destinations[random() % destinations.size()];
    std::array<int, Columns * Rows> previous;
    previous.fill(-1);
    std::queue<int> pending;
    previous[start] = start;
    pending.push(start);
    while (!pending.empty() && previous[target] < 0)
    {
        int current = pending.front();
        pending.pop();
        for (auto delta :
             {LevelPoint{1, 0}, LevelPoint{-1, 0}, LevelPoint{0, 1}, LevelPoint{0, -1}})
        {
            int x = current % Columns + int(delta.x);
            int z = current / Columns + int(delta.z);
            if (!Walkable(x, z) || Distance(Center(x, z), spawn) >= TownRadius - 1)
            {
                continue;
            }
            int next = z * Columns + x;
            if (previous[next] < 0)
            {
                previous[next] = current;
                pending.push(next);
            }
        }
    }
    if (previous[target] < 0)
    {
        return;
    }
    for (int current = target; current != start; current = previous[current])
    {
        resident.route.push_back(Center(current % Columns, current / Columns));
    }
    std::reverse(resident.route.begin(), resident.route.end());
}

void LevelOne::UpdateResidents(float dt)
{
    for (auto& resident : residents)
    {
        resident.speed = 0;
        if (!scene.IsActive(resident.Id()) ||
            (conversationTime > 0 && conversationActor == resident.Id()))
        {
            continue;
        }
        resident.rest -= dt;
        if (resident.rest > 0)
        {
            continue;
        }
        if (resident.waypoint >= resident.route.size())
        {
            PlanResidentWalk(resident);
            resident.rest = 1.5f + float(random() % 40) * .1f;
            continue;
        }
        const LevelPoint target = resident.route[resident.waypoint];
        const LevelPoint before = resident.position;
        const float distance = Distance(before, target);
        if (distance < .04f)
        {
            resident.position = target;
            ++resident.waypoint;
            continue;
        }
        const float movement = (std::min)(distance, dt * (.65f + resident.kind * .035f));
        Move(resident.position,
             (target.x - before.x) / distance * movement,
             (target.z - before.z) / distance * movement);
        if (!InTown(resident.position))
        {
            resident.position = before;
        }
        const float moved = Distance(before, resident.position);
        resident.speed = dt > 0 ? moved / dt : 0;
        resident.stride += moved * 6;
        resident.heading = std::atan2(target.x - before.x, target.z - before.z);
        if (moved < .0001f)
        {
            resident.route.clear();
        }
    }
}

const LevelResident* LevelOne::NearestResident() const
{
    if (!FindPlayer())
    {
        return nullptr;
    }
    const LevelResident* nearest = nullptr;
    float range = 1.8f;
    for (const auto& resident : residents)
    {
        const float distance = Distance(Player().position, resident.position);
        if (scene.IsVisible(resident.Id()) && distance < range &&
            Visible(Player().position, resident.position))
        {
            nearest = &resident;
            range = distance;
        }
    }
    return nearest;
}

void LevelOne::TalkToResident()
{
    const auto* nearest = NearestResident();
    auto* resident = nearest ? dynamic_cast<LevelResident*>(scene.Find(nearest->Id())) : nullptr;
    if (!resident)
    {
        return;
    }
    conversationActor = resident->Id();
    conversationTime = 6;
    resident->heading = std::atan2(Player().position.x - resident->position.x,
                                   Player().position.z - resident->position.z);
    conversation = {resident->title + L" · " + resident->role};
    if (!resident->greeted)
    {
        resident->greeted = true;
        ++neighborhood.neighborsMet;
        conversation.push_back(L"얼굴을 기억해 둘게요. 돌아오는 길에도 인사해요. [동네 신뢰 +1]");
    }
    else
    {
        conversation.push_back(neighborhood.Reputation() >= 4
                                   ? L"또 만났네요! 오늘도 무사히 돌아와요."
                                   : L"안녕하세요. 오늘 동네는 조용하네요.");
    }
    const int reports = (std::min)(6, kills / 3);
    int rumor =
        neighborhood.outskirtsReports > 0 && resident->kind % 3 == 2 ? 2 : resident->kind % 2;
    if (resident->kind == 0 && reports > neighborhood.outskirtsReports)
    {
        const int gain = (reports - neighborhood.outskirtsReports) * 2;
        neighborhood.outskirtsReports = reports;
        rumor = 2;
        conversation[1] =
            L"귀갓길 소식을 전해 줘서 고마워요. [동네 신뢰 +" + std::to_wstring(gain) + L"]";
    }
    neighborhood.heardRumors[rumor] = true;
    conversation.push_back(NeighborhoodState::Rumors[rumor]);
    Notify(L"[Tab] 동네 기록에서 들은 소문과 평판을 확인할 수 있습니다.");
}

int LevelOne::NearestDrop() const
{
    if (!FindPlayer())
    {
        return -1;
    }
    int best = -1;
    float range = 1.8f;

    int index = 0;
    for (const auto& drop : drops)
    {
        int current = index++;
        if (!scene.IsActive(drop.Id()))
        {
            continue;
        }
        float distance = Distance(Player().position, drop.position);

        if (distance < range && Visible(Player().position, drop.position))
        {
            best = current;
            range = distance;
        }
    }

    return best;
}

void LevelOne::PickUp()
{
    int index = NearestDrop();

    if (index < 0)
    {
        return;
    }

    DropKind kind = drops[index].kind;
    drops.erase_at(index);

    if (kind == DropKind::Potion)
    {
        ++potions;
        Notify(L"회복 음료 +1  ·  Q로 최대 HP의 40% 회복");
    }
    else if (kind == DropKind::Weapon)
    {
        stats.weapon = true;
        Notify(L"강화 손전등 획득 · 자동 장착 · 공격력 +3");
    }
    else
    {
        ++fragments;
        GainExperience(6);
        Notify(L"기억 조각 +1  ·  경험치 +6");
    }
}

void LevelOne::Update(float dt)
{
    if (!FindPlayer())
    {
        return;
    }
    if (paused || statsOpen || neighborhoodOpen || stats.health <= 0)
    {
        return;
    }

    dt = Clamp(dt, 0, .05f);
    time += dt;
    conversationTime = (std::max)(0.f, conversationTime - dt);
    toastTime = (std::max)(0.f, toastTime - dt);
    invulnerable = (std::max)(0.f, invulnerable - dt);
    attackCooldown = (std::max)(0.f, attackCooldown - dt);
    attackTime = (std::max)(0.f, attackTime - dt);
    pathTimer -= dt;

    LevelPoint previous = Player().position;
    bool playerActive = scene.IsActive(Player().Id());
    float x = playerActive ? float(keys['d']) - float(keys['a']) : 0;
    float z = playerActive ? float(keys['w']) - float(keys['s']) : 0;
    float length = std::hypot(x, z);

    if (length > 0)
    {
        float dx = (x - z) * .70710678f / length;
        float dz = (-x - z) * .70710678f / length;
        float movement = dt * (attackTime > 0 ? 2.1f : 3.7f);
        Move(Player().position, dx * movement, dz * movement);

        if (attackTime <= 0)
        {
            Player().heading = std::atan2(dx, dz);
        }
    }

    float moved = Distance(previous, Player().position);
    Player().speed = dt > 0 ? moved / dt : 0;
    Player().stride += moved * 6;
    stepDistance += moved;

    if (stepDistance >= .55f)
    {
        stepDistance = 0;
        dust.push_back({Player().position, 0});
    }

    if (pathTimer <= 0)
    {
        RebuildPaths();
        pathTimer = .25f;
    }

    if (playerActive && (keys[' '] || keys['j']) && attackCooldown <= 0)
    {
        attackCooldown = .55f;
        attackTime = .36f;
        attackResolved = false;
    }

    if (playerActive && !attackResolved && attackTime <= .23f)
    {
        ResolveAttack();
        attackResolved = true;
    }

    UpdateEnemies(dt);
    UpdateResidents(dt);

    for (auto& item : feedback)
    {
        item.life -= dt;
    }

    scene.Update(dt);
    if (!FindPlayer())
    {
        return;
    }

    feedback.erase(std::remove_if(feedback.begin(),
                                  feedback.end(),
                                  [](const LevelFeedback& f)
                                  {
                                      return f.life <= 0;
                                  }),
                   feedback.end());
    dust.RemoveIf(
        [](const LevelDust& item)
        {
            return item.age > .8f;
        });
    camera.x += (Clamp(Player().position.x, -37, 37) - camera.x) * (1 - std::exp(-5 * dt));
    camera.z += (Clamp(Player().position.z, -26, 26) - camera.z) * (1 - std::exp(-5 * dt));

    if (!accomplished && stats.level >= 5 && stats.strength + stats.vitality + stats.resilience > 0)
    {
        accomplished = true;
        Notify(L"첫 레벨 목표 달성! 캐릭터 Lv.5 + 스탯 배분 완료 · 계속 사냥할 수 있습니다.");
    }
}

void LevelOne::ClearKeys()
{
    keys.fill(false);
}

void LevelOne::KeyDown(unsigned char raw)
{
    unsigned char key = Lower(raw);

    if (keys[key])
    {
        return;
    }

    keys[key] = true;

    if (!FindPlayer())
    {
        if (key == 'r')
        {
            Reset(mapSeed);
        }
        return;
    }

    if (key == 27)
    {
        if (neighborhoodOpen)
        {
            neighborhoodOpen = false;
        }
        else if (statsOpen)
        {
            statsOpen = false;
        }
        else
        {
            paused = !paused;
        }

        ClearKeys();
        return;
    }

    if (stats.health <= 0)
    {
        if (key == 'r')
        {
            Player().position = camera = spawn;
            stats.health = stats.MaximumHealth();
            potions = (std::max)(potions, 1);
            invulnerable = 3;
            attackTime = attackCooldown = 0;
            attackResolved = true;
            paused = statsOpen = neighborhoodOpen = false;
            ClearKeys();
            RebuildPaths();
            Notify(L"쉼터에서 회복했습니다. 경험치와 아이템은 유지됩니다.");
        }

        return;
    }

    if (paused)
    {
        if (key == 'n')
        {
            Reset(mapSeed + 1);
        }

        return;
    }

    if (key == '\t')
    {
        neighborhoodOpen = !neighborhoodOpen;
        statsOpen = false;
        ClearKeys();
        return;
    }

    if (key == 'c')
    {
        statsOpen = !statsOpen;
        neighborhoodOpen = false;
        ClearKeys();
        return;
    }

    if (neighborhoodOpen)
    {
        return;
    }

    if (statsOpen)
    {
        if (key >= '1' && key <= '3' && stats.SpendPoint(key - '0'))
        {
            Notify(L"스탯을 배분했습니다. 변경된 능력치가 즉시 적용됩니다.");
        }

        return;
    }

    if (!scene.IsActive(Player().Id()))
    {
        return;
    }

    if (key == 'e')
    {
        if (NearestResident())
        {
            TalkToResident();
        }
        else
        {
            PickUp();
        }
    }
    else if (key == 'q' && potions > 0 && stats.health < stats.MaximumHealth())
    {
        --potions;
        stats.health =
            (std::min)(stats.MaximumHealth(), stats.health + stats.MaximumHealth() * 40 / 100);
        Notify(L"회복 음료를 사용했습니다.");
    }
}

void LevelOne::KeyUp(unsigned char key)
{
    keys[Lower(key)] = false;
}

void LevelOne::BuildScenery()
{
    auto mesh = [&](ModelKind model,
                    float x,
                    float y,
                    float z,
                    float sx,
                    float sy,
                    float sz,
                    float r,
                    float g,
                    float b,
                    int material)
    {
        auto& actor = scene.Spawn<MeshActor>(0);
        actor.name = "Scenery";
        actor.model = model;
        actor.position = {x, z};
        actor.height = y;
        actor.scale = {sx, sy, sz};
        actor.r = r;
        actor.g = g;
        actor.b = b;
        actor.material = material;
        return actor.Id();
    };
    for (int z = 0; z < Rows; ++z)
    {
        for (int x = 0; x < Columns;)
        {
            LevelTile tile = tiles[z * Columns + x];
            int end = x + 1;
            while (end < Columns && tiles[z * Columns + end] == tile)
            {
                ++end;
            }
            LevelPoint point = Center(x, z);
            if (tile == LevelTile::Water)
            {
                for (int col = x; col < end; ++col)
                {
                    LevelPoint water = Center(col, z);
                    mesh(ModelKind::Water, water.x, -.05f, water.z, 2, 1, 2, 1, 1, 1, -1);
                }
            }
            else
            {
                bool road = tile == LevelTile::Road;
                mesh(ModelKind::Box,
                     point.x + end - x - 1,
                     -.10f,
                     point.z,
                     (end - x) * 2.f,
                     .10f,
                     2,
                     road ? .54f : .53f,
                     road ? .56f : .67f,
                     road ? .55f : .40f,
                     road ? 3 : 1);
            }
            x = end;
        }
    }
    for (auto& tree : trees)
    {
        auto& leaf = scene.Spawn<MeshActor>(tree.Id());
        leaf.name = "Windblown leaf";
        leaf.model = ModelKind::Leaf;
        leaf.height = 3;
    }
    mesh(ModelKind::Box, spawn.x, .01f, spawn.z, 3.6f, .06f, 3.6f, .82f, .81f, .70f, 3);
    // Children use local coordinates, so moving the post also moves its lamp.
    ActorId lamp =
        mesh(ModelKind::Box, spawn.x - 2, 0, spawn.z, .14f, 2.8f, .14f, .32f, .39f, .40f, 5);
    auto& light = scene.Spawn<MeshActor>(lamp);
    light.name = "Lamp globe";
    light.model = ModelKind::Sphere;
    light.height = 2.85f / 2.8f;
    light.scale = {.22f / .14f, .20f / 2.8f, .22f / .14f};
    light.r = .99f;
    light.g = .87f;
    light.b = .49f;
    auto& safe = scene.Spawn<MarkerActor>(0, MarkerKind::Spawn);
    safe.position = spawn;
    scene.Spawn<MarkerActor>(Player().Id(), MarkerKind::Player);
    scene.Spawn<MarkerActor>(Player().Id(), MarkerKind::Attack);
    for (const auto& enemy : enemies)
    {
        scene.Spawn<MarkerActor>(enemy.Id(), MarkerKind::Enemy);
    }
    scene.FlushDestroyed();
}

PlayerActor* LevelOne::FindPlayer()
{
    return dynamic_cast<PlayerActor*>(scene.Find(playerId));
}

const PlayerActor* LevelOne::FindPlayer() const
{
    return dynamic_cast<const PlayerActor*>(scene.Find(playerId));
}

PlayerActor& LevelOne::Player()
{
    auto* actor = FindPlayer();
    if (!actor)
    {
        throw std::logic_error("Level player has been removed.");
    }
    return *actor;
}

const PlayerActor& LevelOne::Player() const
{
    const auto* actor = FindPlayer();
    if (!actor)
    {
        throw std::logic_error("Level player has been removed.");
    }
    return *actor;
}
