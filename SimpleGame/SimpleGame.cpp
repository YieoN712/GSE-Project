/*
Copyright 2022 Lee Taek Hee (Tech University of Korea)

This program is free software: you can redistribute it and/or modify
it under the terms of the What The Hell License. Do it plz.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.
*/

#include "stdafx.h"
#include <iostream>
#include "Dependencies\glew.h"
#include "Dependencies\freeglut.h"

#include "Tutorial.h"
#include "LevelOne.h"
#include "LevelOneView.h"
#include <memory>
#include <algorithm>
#include <chrono>
#include <string>
#include <limits>
#include <stdexcept>

namespace
{
    std::unique_ptr<Tutorial> tutorial;
    std::unique_ptr<TutorialView> tutorialView;
    std::unique_ptr<LevelOne> level;
    std::unique_ptr<LevelOneView> levelView;
    int windowWidth = 1280, windowHeight = 800;
    int lastTime = 0;
    bool visible = true;
} // namespace

void RenderScene(void)
{
    if (levelView)
    {
        levelView->Draw(*level, windowWidth, windowHeight);
    }
    else if (tutorialView)
    {
        tutorialView->Draw(*tutorial, windowWidth, windowHeight);
    }
    glutSwapBuffers();
}

void Tick(int)
{
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (std::max)((now - lastTime) / 1000.f, 0.f);
    lastTime = now;
    // Losing focus must not leave a held WASD key stuck on return.
    bool focused = GetForegroundWindow() == WindowFromDC(wglGetCurrentDC());
    if (level)
    {
        if (visible && focused)
        {
            level->Update(dt);
        }
        else
        {
            level->ClearKeys();
        }
    }
    else if (tutorial)
    {
        if (visible && focused)
        {
            tutorial->Update(dt);
        }
        else
        {
            tutorial->ClearKeys();
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16, Tick, 0);
}

void Resize(int width, int height)
{
    windowWidth = (std::max)(width, 1);
    windowHeight = (std::max)(height, 1);
}

void KeyInput(unsigned char key, int x, int y)
{
    if (level)
    {
        level->KeyDown(key);
    }
    else if (tutorial)
    {
        tutorial->KeyDown(key);
    }
}

void KeyReleased(unsigned char key, int, int)
{
    if (level)
    {
        level->KeyUp(key);
    }
    else if (tutorial)
    {
        tutorial->KeyUp(key);
    }
}

void Visibility(int state)
{
    visible = state == GLUT_VISIBLE;
    if (level)
    {
        level->ClearKeys();
    }
    if (tutorial)
    {
        tutorial->ClearKeys();
    }
}

void Close()
{
    levelView.reset();
    tutorialView.reset();
}

int main(int argc, char** argv)
{
    bool tutorialMode = false;
    std::uint32_t seed =
        static_cast<std::uint32_t>(std::chrono::system_clock::now().time_since_epoch().count());
    int outputArgument = 1;

    for (int i = 1; i < argc; ++i)
    {
        std::string argument = argv[i];

        if (argument == "--tutorial")
        {
            tutorialMode = true;
        }
        else if (argument.find("--seed=") == 0)
        {
            try
            {
                size_t consumed = 0;
                std::string value = argument.substr(7);
                unsigned long long parsed = std::stoull(value, &consumed);

                if (value.empty() || value[0] == '-' || consumed != value.size() ||
                    parsed > (std::numeric_limits<std::uint32_t>::max)())
                {
                    throw std::invalid_argument("seed");
                }

                seed = static_cast<std::uint32_t>(parsed);
            }
            catch (const std::exception&)
            {
                std::cerr << "Expected --seed=<unsigned 32-bit integer>.\n";
                return 1;
            }
        }
        else
        {
            argv[outputArgument++] = argv[i];
        }
    }

    argc = outputArgument;
    argv[argc] = nullptr;

    // Initialize GL things
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(60, 60);
    glutInitWindowSize(windowWidth, windowHeight);
    // The tutorial uses compatibility geometry and native Unicode bitmap text.
    glutInitContextVersion(2, 1);
    glutCreateWindow(tutorialMode ? "GSE - Haeeon / Tutorial"
                                  : "GSE - Level 1 / Echoes on the Way Home");

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "GLEW initialization failed.\n";
        return 1;
    }
    try
    {
        if (tutorialMode)
        {
            tutorial.reset(new Tutorial());
            tutorialView.reset(new TutorialView());
        }
        else
        {
            level.reset(new LevelOne(seed));
            levelView.reset(new LevelOneView());
            std::cout << "Level 1 seed: " << seed
                      << ", reachable ground tiles: " << level->reachableTiles << "\n";
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "Level initialization failed: " << error.what() << "\n";
        Close();
        return 1;
    }
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutIgnoreKeyRepeat(1);

    glutDisplayFunc(RenderScene);
    glutKeyboardFunc(KeyInput);
    glutKeyboardUpFunc(KeyReleased);
    glutReshapeFunc(Resize);
    glutVisibilityFunc(Visibility);
    glutCloseFunc(Close);
    lastTime = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, Tick, 0);

    glutMainLoop();

    Close();

    return 0;
}
