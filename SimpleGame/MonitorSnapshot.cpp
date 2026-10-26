#include "MonitorSnapshot.h"
#include "LevelOne.h"
#include "Tutorial.h"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>

namespace
{
    std::string Quote(const std::string& value)
    {
        std::ostringstream out;
        out << '"';
        for (unsigned char c : value)
        {
            if (c == '"' || c == '\\')
            {
                out << '\\' << c;
            }
            else if (c < 32)
            {
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(c);
            }
            else
            {
                out << c;
            }
        }
        out << '"';
        return out.str();
    }

    std::string Quote(const wchar_t* value)
    {
        std::ostringstream out;
        out << '"' << std::hex << std::setfill('0');
        for (const wchar_t* current = value ? value : L""; *current; ++current)
        {
            const auto code = static_cast<std::uint32_t>(*current);
            if (code <= 0xffff)
            {
                out << "\\u" << std::setw(4) << code;
            }
            else if (code <= 0x10ffff)
            {
                out << "\\u" << std::setw(4) << (0xd800 + ((code - 0x10000) >> 10)) << "\\u"
                    << std::setw(4) << (0xdc00 + ((code - 0x10000) & 0x3ff));
            }
            else
            {
                out << "\\ufffd";
            }
        }
        out << '"';
        return out.str();
    }

    float Finite(float value)
    {
        return std::isfinite(value) ? value : 0;
    }

    void ActorFields(std::ostream& out, const SceneGraph& scene, const Actor& actor)
    {
        const auto world = scene.WorldMatrix(actor.Id());
        // IDs are strings to preserve uint64 precision in JavaScript.
        out << "\"id\":" << Quote(std::to_string(actor.Id()))
            << ",\"townId\":\"haeeon\",\"active\":" << scene.IsActive(actor.Id())
            << ",\"visible\":" << scene.IsVisible(actor.Id())
            << ",\"position\":{\"x\":" << Finite(world[12]) << ",\"y\":" << Finite(world[13])
            << ",\"z\":" << Finite(world[14]) << '}';
    }

    const char* State(EnemyState state)
    {
        switch (state)
        {
        case EnemyState::Roaming:
            return "roaming";
        case EnemyState::Chasing:
            return "chasing";
        case EnemyState::Windup:
            return "windup";
        case EnemyState::Recovery:
            return "recovery";
        case EnemyState::Dead:
            return "dead";
        }
        return "unknown";
    }
} // namespace

std::string MakeMonitorSnapshot(const LevelOne* level,
                                const Tutorial* tutorial,
                                bool foreground,
                                std::uint64_t sequence)
{
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::boolalpha << std::setprecision(6);
    const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();
    const bool paused =
        level ? level->paused || level->statsOpen || level->neighborhoodOpen ||
                    level->stats.health <= 0 || !level->FindPlayer()
              : tutorial && (tutorial->paused || tutorial->completion || !tutorial->FindPlayer());
    out << "{\"schemaVersion\":1,\"sequence\":" << sequence << ",\"timestamp\":" << timestamp
        << ",\"scene\":"
        << Quote(level      ? "level-one"
                 : tutorial ? "tutorial"
                            : "none")
        << ",\"foreground\":" << foreground << ",\"paused\":" << paused << ",\"simulationSeconds\":"
        << Finite(level      ? level->time
                  : tutorial ? tutorial->elapsed
                             : 0)
        << ",\"npcs\":[";

    int npcCount = 0;
    int npcPresent = 0;
    if (level)
    {
        for (const auto& npc : level->residents)
        {
            if (npcCount++)
            {
                out << ',';
            }
            const bool present = level->scene.IsVisible(npc.Id());
            npcPresent += present ? 1 : 0;
            const bool talking =
                level->conversationTime > 0 && level->conversationActor == npc.Id();
            out << '{';
            ActorFields(out, level->scene, npc);
            out << ",\"name\":" << Quote(npc.title.c_str())
                << ",\"role\":" << Quote(npc.role.c_str()) << ",\"present\":" << present
                << ",\"greeted\":" << npc.greeted << ",\"state\":"
                << Quote(!level->scene.IsActive(npc.Id()) ? "inactive"
                         : !present                       ? "absent"
                         : talking                        ? "talking"
                         : npc.speed > .01f               ? "walking"
                                                          : "idle")
                << '}';
        }
    }
    if (tutorial)
    {
        for (const auto& npc : tutorial->residents)
        {
            if (npcCount++)
            {
                out << ',';
            }
            const bool present =
                !(npc.slot == 1 && tutorial->stage >= 3) && tutorial->scene.IsVisible(npc.Id());
            npcPresent += present ? 1 : 0;
            out << '{';
            ActorFields(out, tutorial->scene, npc);
            out << ",\"name\":" << Quote(npc.name) << ",\"present\":" << present << ",\"state\":"
                << Quote(!tutorial->scene.IsActive(npc.Id())                            ? "inactive"
                         : !present                                                     ? "absent"
                         : tutorial->speaker == npc.slot && !tutorial->dialogue.empty() ? "talking"
                         : npc.walkSpeed > 0.01f                                        ? "walking"
                                                                                        : "idle")
                << '}';
        }
    }
    out << "],\"mobs\":[";
    int mobCount = 0;
    int mobAlive = 0;
    if (level)
    {
        for (const auto& mob : level->enemies)
        {
            if (mobCount++)
            {
                out << ',';
            }
            mobAlive += mob.state != EnemyState::Dead ? 1 : 0;
            out << '{';
            ActorFields(out, level->scene, mob);
            out << ",\"name\":" << Quote("Echo " + std::to_string(mob.kind + 1))
                << ",\"state\":" << Quote(State(mob.state)) << ",\"health\":" << mob.health
                << ",\"maximumHealth\":" << mob.maximumHealth << ",\"timer\":" << Finite(mob.timer)
                << '}';
        }
    }
    out << "],\"counts\":{\"npcs\":" << npcCount << ",\"presentNpcs\":" << npcPresent
        << ",\"mobs\":" << mobCount << ",\"aliveMobs\":" << mobAlive << '}'
        << ",\"towns\":[{\"id\":\"haeeon\",\"name\":" << Quote(L"해온")
        << ",\"district\":" << Quote(level ? L"생활 구역과 외곽 / 레벨 1" : L"작은 동네 / 튜토리얼")
        << ",\"scope\":\"loaded-scene\",\"population\":" << npcPresent << ",\"atmosphere\":";
    if (level)
    {
        const auto& social = level->neighborhood;
        out << Quote(social.Atmosphere()) << ",\"rumors\":[";
        for (size_t index = 0; index < NeighborhoodState::Rumors.size(); ++index)
        {
            if (index == 2 && social.outskirtsReports == 0)
            {
                break;
            }
            if (index > 0)
            {
                out << ',';
            }
            out << Quote(NeighborhoodState::Rumors[index]);
        }
        out << "],\"tendencies\":{\"neighborsMet\":" << social.neighborsMet
            << ",\"outskirtsReports\":" << social.outskirtsReports
            << "},\"reputation\":{\"trust\":" << social.Reputation()
            << ",\"label\":" << Quote(social.ReputationLabel()) << "},\"heardRumors\":[";
        for (size_t index = 0; index < social.heardRumors.size(); ++index)
        {
            if (index > 0)
            {
                out << ',';
            }
            out << social.heardRumors[index];
        }
        out << ']';
    }
    else
    {
        out << "null,\"rumors\":null,\"tendencies\":null,\"reputation\":null";
    }
    out << "}],\"player\":";
    const SceneGraph* scene = level ? &level->scene : tutorial ? &tutorial->scene : nullptr;
    const Actor* player = level ? level->FindPlayer() : tutorial ? tutorial->FindPlayer() : nullptr;
    if (scene && player)
    {
        out << '{';
        ActorFields(out, *scene, *player);
        if (level)
        {
            out << ",\"level\":" << level->stats.level << ",\"health\":" << level->stats.health
                << ",\"experience\":" << level->stats.totalExperience
                << ",\"kills\":" << level->kills;
        }
        out << '}';
    }
    else
    {
        out << "null";
    }
    out << '}';
    return out.str();
}
