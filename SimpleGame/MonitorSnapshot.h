#pragma once

#include <cstdint>
#include <string>

class LevelOne;
class Tutorial;

// Called only on the simulation thread. The transport receives an immutable JSON copy.
std::string MakeMonitorSnapshot(const LevelOne* level,
                                const Tutorial* tutorial,
                                bool foreground,
                                std::uint64_t sequence);
