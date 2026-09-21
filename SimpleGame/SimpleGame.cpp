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
#include <memory>
#include <algorithm>

namespace
{
    Tutorial game;
    std::unique_ptr<TutorialView> view;
    int windowWidth = 1280, windowHeight = 800;
    int lastTime = 0;
    bool visible = true;
} // namespace

void RenderScene(void)
{
    if (view)
        view->Draw(game, windowWidth, windowHeight);
    glutSwapBuffers();
}

void Tick(int)
{
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (std::max)((now - lastTime) / 1000.f, 0.f);
    lastTime = now;
    // Losing focus must not leave a held WASD key stuck on return.
    bool focused = GetForegroundWindow() == WindowFromDC(wglGetCurrentDC());
    if (visible && focused)
        game.Update(dt);
    else
        game.ClearKeys();
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
    game.KeyDown(key);
}

void KeyReleased(unsigned char key, int, int)
{
    game.KeyUp(key);
}

void Visibility(int state)
{
    visible = state == GLUT_VISIBLE;
    game.ClearKeys();
}

void Close()
{
    view.reset();
}

int main(int argc, char** argv)
{
    // Initialize GL things
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(60, 60);
    glutInitWindowSize(windowWidth, windowHeight);
    // The tutorial uses compatibility geometry and native Unicode bitmap text.
    glutInitContextVersion(2, 1);
    glutCreateWindow("GSE - Haeeon Neighborhood / Tutorial");

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "GLEW initialization failed.\n";
        return 1;
    }
    view.reset(new TutorialView());
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

    view.reset();

    return 0;
}
