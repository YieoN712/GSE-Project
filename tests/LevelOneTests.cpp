#include "LevelOne.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
    void Check(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    void Growth()
    {
        PlayerStats stats;
        Check(!stats.SpendPoint(1), "Cannot spend a point before leveling.");
        Check(stats.AddExperience(280), "Bulk XP must level up.");
        Check(stats.level == 5 && stats.experience == 0 && stats.points == 4,
              "XP overflow must be carried through all four levels.");
        Check(stats.MaximumHealth() == 164 && stats.Attack() == 30 && stats.Defense() == 6,
              "Automatic level bonuses must follow the documented formulas.");
        Check(stats.SpendPoint(2) && stats.health == 176 && stats.MaximumHealth() == 176,
              "Vitality must immediately raise maximum and current health.");
        Check(stats.SpendPoint(1) && stats.Attack() == 32, "Strength must change attack.");
        Check(stats.SpendPoint(3) && stats.Defense() == 7, "Resilience must change defense.");
        Check(!stats.SpendPoint(4) && stats.points == 1,
              "Invalid choices must not consume points.");
        stats.AddExperience(100000);
        Check(stats.level == 20 && stats.experience == 0, "Character level must remain capped.");
    }

    void Maps()
    {
        for (unsigned seed = 0; seed < 100; ++seed)
        {
            LevelOne level(seed);
            Check(level.AllGroundConnected(), "Every walkable tile must connect to spawn.");
            Check(!level.Blocked(level.spawn), "Spawn must have character-radius clearance.");
            Check(!level.enemies.empty(), "A generated level must contain huntable enemies.");

            for (const auto& enemy : level.enemies)
            {
                Check(!level.Blocked(enemy.home), "An enemy spawn must be traversable.");
            }

            for (int z = 0; z < LevelOne::Rows; ++z)
            {
                for (int x = 0; x < LevelOne::Columns; ++x)
                {
                    if (!level.Walkable(x, z))
                    {
                        continue;
                    }

                    Check(!level.Blocked(level.Center(x, z)),
                          "A walkable cell center must fit the actor.");

                    if (level.Walkable(x + 1, z))
                    {
                        Check(level.Visible(level.Center(x, z), level.Center(x + 1, z)),
                              "Adjacent walkable cells must be physically connected.");
                    }
                    if (level.Walkable(x, z + 1))
                    {
                        Check(level.Visible(level.Center(x, z), level.Center(x, z + 1)),
                              "Adjacent walkable cells must be physically connected.");
                    }
                }
            }

            auto first = level.tiles;
            level.Reset(seed);
            Check(first == level.tiles, "A seed must reproduce its own map.");
        }
    }

    void CombatAndItems()
    {
        LevelOne level(7);
        level.enemies.clear();
        level.drops.clear();
        LevelEnemy enemy;
        enemy.position = enemy.home = {level.player.x, level.player.z + 1};
        enemy.health = 1;
        level.enemies.push_back(enemy);
        level.KeyDown(' ');

        for (int i = 0; i < 8; ++i)
        {
            level.Update(.05f);
        }

        level.KeyUp(' ');
        Check(level.kills == 1 && level.stats.totalExperience == 20,
              "One death must award XP exactly once.");
        Check(level.enemies[0].state == EnemyState::Dead && level.drops.size() == 1,
              "A death must transition state and create one drop.");
        level.KeyDown('e');
        level.KeyDown('e');
        level.KeyUp('e');
        Check(level.fragments == 1 && level.drops.empty() && level.stats.totalExperience == 26,
              "Repeated pickup input must not duplicate item rewards.");

        level.stats.health = 50;
        level.KeyDown('q');
        level.KeyDown('q');
        level.KeyUp('q');
        Check(level.stats.health == 90 && level.potions == 2,
              "One keypress must consume one potion.");

        level.stats.AddExperience(280);
        level.KeyDown('c');
        level.KeyUp('c');
        float before = level.time;
        level.Update(.05f);
        Check(level.time == before, "Stats screen must pause combat.");
        level.KeyDown('1');
        level.KeyUp('1');
        Check(level.stats.strength == 1, "Stats input must spend the earned point.");
        level.KeyDown('c');
        level.KeyUp('c');
        level.Update(.05f);
        Check(level.accomplished, "Level 5 and a spent point must satisfy the first-level goal.");

        level.stats.health = 0;
        int experience = level.stats.totalExperience;
        level.KeyDown('r');
        level.KeyUp('r');
        Check(level.stats.health == level.stats.MaximumHealth() &&
                  level.stats.totalExperience == experience && !level.Blocked(level.player),
              "Recovery must preserve growth and return to safe ground.");
    }
} // namespace

int main()
{
    try
    {
        Growth();
        Maps();
        CombatAndItems();
        std::cout << "LevelOne regression checks passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << "\n";
        return 1;
    }
}
