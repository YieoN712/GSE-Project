#pragma once
#include "UiCanvas.h"
class LevelOne;

class LevelOneUi
{
public:
    void Draw(const LevelOne& game, int width, int height, bool loaded, bool saved);

private:
    UiCanvas font;
    void Hud(const LevelOne& game, float width, float height, bool loaded, bool saved);
};
