#pragma once
#include "Dependencies/glew.h"

// Compatibility geometry is retained, with an optional shader/FBO quality path.
class RenderEffects
{
public:
    RenderEffects();
    ~RenderEffects();
    bool BeginShadow(float x, float z);
    void BeginScene(int width, int height);
    void Material(int material);
    void Unlit();
    void Composite();

    bool ShadowPass() const
    {
        return shadowPass;
    }

private:
    GLuint sceneProgram = 0, postProgram = 0, shadowFbo = 0, shadowTexture = 0;
    GLuint sceneFbo = 0, sceneTexture = 0, depthBuffer = 0;
    int w = 0, h = 0, materialLocation = -1;
    bool enabled = false, shadowPass = false, sceneActive = false;
    float lightMatrix[16]{};
    void Resize(int width, int height);
    void ReleaseScene();
};
