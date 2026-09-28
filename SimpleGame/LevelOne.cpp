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
{
    Reset(seed);
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

        if (Distance(point, spawn) > 11)
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
    mapSeed = seed;
    random.seed(seed);
    stats = PlayerStats{};
    drops.clear();
    feedback.clear();
    dust.clear();
    potions = 3;
    fragments = kills = 0;
    time = heading = stride = speed = attackTime = invulnerable = 0;
    attackCooldown = pathTimer = stepDistance = 0;
    attackResolved = true;
    paused = statsOpen = accomplished = false;
    ClearKeys();
    Generate();
    player = camera = spawn;
    drops.push_back({{spawn.x + 2, spawn.z}, DropKind::Potion});
    RebuildPaths();
    Notify(L"레벨 1 · 귀갓길의 잔향 | 잔향을 처치하고 캐릭터 Lv.5와 스탯 배분을 달성하세요.");
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
    int start = Cell(player);
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

            if (!Walkable(x, z))
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
    feedback.push_back({player, L"+" + std::to_wstring(amount) + L" EXP", 1.4f, true});

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
        drops.erase(drops.begin());
    }

    drops.push_back({enemy.position, kind});
}

void LevelOne::ResolveAttack()
{
    for (auto& enemy : enemies)
    {
        float distance = Distance(player, enemy.position);
        float dot = (enemy.position.x - player.x) * std::sin(heading) +
                    (enemy.position.z - player.z) * std::cos(heading);

        if (enemy.state == EnemyState::Dead || distance > 2.3f ||
            (distance > .5f && dot / distance < .10f) || !Visible(player, enemy.position))
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
        enemy.hitFlash = (std::max)(0.f, enemy.hitFlash - dt);
        enemy.timer -= dt;
        float distance = Distance(player, enemy.position);

        if (enemy.state == EnemyState::Dead)
        {
            if (enemy.timer <= 0 && Distance(player, enemy.home) > 5)
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
                if (distance < 1.65f && invulnerable <= 0 && Distance(player, spawn) > 5 &&
                    Visible(enemy.position, player))
                {
                    int damage = (std::max)(1, 12 + enemy.kind * 4 - stats.Defense());
                    stats.health = (std::max)(0, stats.health - damage);
                    invulnerable = .65f;
                    feedback.push_back({player, L"-" + std::to_wstring(damage), 1, false});
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

        bool aggro = distance < 10 && Distance(player, spawn) > 5;
        enemy.state = aggro ? EnemyState::Chasing : EnemyState::Roaming;

        if (aggro && distance < 1.3f && Visible(enemy.position, player))
        {
            enemy.heading = std::atan2(player.x - enemy.position.x, player.z - enemy.position.z);
            enemy.state = EnemyState::Windup;
            enemy.timer = .65f;
            continue;
        }

        LevelPoint target = {enemy.home.x + std::sin(time * .25f + enemy.kind),
                             enemy.home.z + std::cos(time * .21f + enemy.kind)};

        if (aggro)
        {
            if (Visible(enemy.position, player))
            {
                target = player;
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

                    if (Walkable(x, z) && distances[next] >= 0 && distances[next] < distances[best])
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
            enemy.stride += Distance(previous, enemy.position) * 6;
            enemy.heading = std::atan2(target.x - previous.x, target.z - previous.z);
        }
    }
}

int LevelOne::NearestDrop() const
{
    int best = -1;
    float range = 1.8f;

    for (size_t i = 0; i < drops.size(); ++i)
    {
        float distance = Distance(player, drops[i].position);

        if (distance < range && Visible(player, drops[i].position))
        {
            best = int(i);
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
    drops.erase(drops.begin() + index);

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
    if (paused || statsOpen || stats.health <= 0)
    {
        return;
    }

    dt = Clamp(dt, 0, .05f);
    time += dt;
    toastTime = (std::max)(0.f, toastTime - dt);
    invulnerable = (std::max)(0.f, invulnerable - dt);
    attackCooldown = (std::max)(0.f, attackCooldown - dt);
    attackTime = (std::max)(0.f, attackTime - dt);
    pathTimer -= dt;

    LevelPoint previous = player;
    float x = float(keys['d']) - float(keys['a']);
    float z = float(keys['w']) - float(keys['s']);
    float length = std::hypot(x, z);

    if (length > 0)
    {
        float dx = (x - z) * .70710678f / length;
        float dz = (-x - z) * .70710678f / length;
        float movement = dt * (attackTime > 0 ? 2.1f : 3.7f);
        Move(player, dx * movement, dz * movement);

        if (attackTime <= 0)
        {
            heading = std::atan2(dx, dz);
        }
    }

    float moved = Distance(previous, player);
    speed = dt > 0 ? moved / dt : 0;
    stride += moved * 6;
    stepDistance += moved;

    if (stepDistance >= .55f)
    {
        stepDistance = 0;
        dust.push_back({player, 0});
    }

    if (pathTimer <= 0)
    {
        RebuildPaths();
        pathTimer = .25f;
    }

    if ((keys[' '] || keys['j']) && attackCooldown <= 0)
    {
        attackCooldown = .55f;
        attackTime = .36f;
        attackResolved = false;
    }

    if (!attackResolved && attackTime <= .23f)
    {
        ResolveAttack();
        attackResolved = true;
    }

    UpdateEnemies(dt);

    for (auto& item : feedback)
    {
        item.life -= dt;
    }

    for (auto& item : dust)
    {
        item.age += dt;
    }

    feedback.erase(std::remove_if(feedback.begin(),
                                  feedback.end(),
                                  [](const LevelFeedback& f)
                                  {
                                      return f.life <= 0;
                                  }),
                   feedback.end());
    dust.erase(std::remove_if(dust.begin(),
                              dust.end(),
                              [](const LevelDust& d)
                              {
                                  return d.age > .8f;
                              }),
               dust.end());
    camera.x += (Clamp(player.x, -37, 37) - camera.x) * (1 - std::exp(-5 * dt));
    camera.z += (Clamp(player.z, -26, 26) - camera.z) * (1 - std::exp(-5 * dt));

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

    if (key == 27)
    {
        if (statsOpen)
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
            player = camera = spawn;
            stats.health = stats.MaximumHealth();
            potions = (std::max)(potions, 1);
            invulnerable = 3;
            attackTime = attackCooldown = 0;
            attackResolved = true;
            paused = statsOpen = false;
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

    if (key == 'c')
    {
        statsOpen = !statsOpen;
        ClearKeys();
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

    if (key == 'e')
    {
        PickUp();
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
