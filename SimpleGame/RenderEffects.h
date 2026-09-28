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
    void SetTime(float seconds);
    void ObjectTransform(const float* matrix, float r = 1, float g = 1, float b = 1);
    void Unlit();
    void Composite();

    bool ShadowPass() const
    {
        return shadowPass;
    }

private:
    struct ObjectUniforms
    {
        GLint matrix = -1;
        GLint normal = -1;
        GLint tint = -1;
        GLint material = -1;
    };

    ObjectUniforms sceneObject;
    ObjectUniforms shadowObject;
    GLuint sceneProgram = 0, postProgram = 0, shadowFbo = 0, shadowTexture = 0;
    GLuint shadowProgram = 0;
    float sceneTime = 0;
    GLuint sceneFbo = 0, sceneTexture = 0, depthBuffer = 0;
    int w = 0, h = 0, materialLocation = -1;
    bool enabled = false, shadowPass = false, sceneActive = false;
    float lightMatrix[16]{};
    void Resize(int width, int height);
    void ReleaseScene();
};
