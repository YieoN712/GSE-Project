#pragma once

#include <memory>
#include <string>

// Screen-space UI is intentionally not an Actor and is never inserted into SceneGraph.
void UiPanel(
    float x, float y, float width, float height, float r, float g, float b, float alpha = 1);

class UiCanvas
{
public:
    UiCanvas();
    ~UiCanvas();
    void Resize(int pixels);
    void Begin(float width, float height);
    void Text(
        float x, float y, const std::wstring& text, float r = .93f, float g = .94f, float b = .87f);
    void WorldText(float x, float y, float z, const std::wstring& text);

private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation;
};
