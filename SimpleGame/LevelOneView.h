#pragma once

#include <memory>

class LevelOne;

class LevelOneView
{
public:
    LevelOneView();
    ~LevelOneView();
    void Draw(const LevelOne& game, int width, int height);

private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation;
};
