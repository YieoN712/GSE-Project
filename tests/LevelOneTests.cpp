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
                Check(level.InOutskirts(enemy.home), "Monsters must spawn outside town.");
            }

            Check(level.residents.size() == 8, "Every map must contain eight neighbors.");
            for (const auto& resident : level.residents)
            {
                Check(level.InTown(resident.position) && !level.Blocked(resident.position),
                      "Residents must start on safe neighborhood ground.");
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

    void Neighborhood()
    {
        LevelOne level(7);
        level.Player().position = level.residents[0].position;
        level.KeyDown('e');
        level.KeyUp('e');
        Check(level.neighborhood.neighborsMet == 1 && level.neighborhood.Reputation() == 1 &&
                  level.neighborhood.heardRumors[0],
              "First greetings must earn trust and a rumor.");
        level.KeyDown('e');
        level.KeyUp('e');
        Check(level.neighborhood.Reputation() == 1, "Repeated greetings must not farm trust.");
        level.kills = 3;
        level.KeyDown('e');
        level.KeyUp('e');
        Check(level.neighborhood.outskirtsReports == 1 && level.neighborhood.Reputation() == 3 &&
                  level.neighborhood.heardRumors[2],
              "New reports must affect trust and rumors.");
        level.KeyDown('e');
        level.KeyUp('e');
        Check(level.neighborhood.Reputation() == 3, "Reports must not award trust twice.");
        level.kills = 1000;
        level.KeyDown('e');
        level.KeyUp('e');
        Check(level.neighborhood.outskirtsReports == 6, "Reports must have a prototype cap.");
        level.KeyDown('\t');
        const float before = level.time;
        level.Update(.05f);
        Check(level.neighborhoodOpen && level.time == before, "Records must pause simulation.");
        level.KeyUp('\t');
        level.KeyDown(27);
        level.KeyUp(27);
        Check(!level.neighborhoodOpen && !level.paused, "Escape must close records first.");
        level.Reset(7);
        Check(level.neighborhood.Reputation() == 0 && level.residents.size() == 8,
              "Reset must clear social state without duplicating residents.");
        for (int step = 0; step < 1800; ++step)
        {
            level.Update(.05f);
            for (const auto& resident : level.residents)
            {
                Check(level.InTown(resident.position) && !level.Blocked(resident.position),
                      "Residents must stay on traversable town ground.");
            }
            for (const auto& enemy : level.enemies)
            {
                Check(level.InOutskirts(enemy.position), "Monsters must not wander into town.");
            }
        }
        level.Player().position = level.enemies[0].home;
        level.stats.health = 100000;
        for (int step = 0; step < 400; ++step)
        {
            level.Update(.05f);
            for (const auto& enemy : level.enemies)
            {
                Check(level.InOutskirts(enemy.position), "Chasing must respect the boundary.");
            }
        }
    }

    void CombatAndItems()
    {
        LevelOne level(7);
        level.enemies.clear();
        level.drops.clear();
        LevelEnemy enemy;
        enemy.position = enemy.home = {level.Player().position.x, level.Player().position.z + 1};
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
                  level.stats.totalExperience == experience &&
                  !level.Blocked(level.Player().position),
              "Recovery must preserve growth and return to safe ground.");
    }
} // namespace

int main()
{
    try
    {
        Growth();
        Maps();
        Neighborhood();
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
