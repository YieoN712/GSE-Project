#pragma once
#include "UiCanvas.h"
class Tutorial;

class TutorialUi
{
public:
    void Prepare(int width, int height);
    void DrawWorldLabels();
    void Draw(const Tutorial& game, int width, int height);

private:
    UiCanvas canvas;
};
