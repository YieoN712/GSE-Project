#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "Dependencies/glew.h"
#include "UiCanvas.h"
#include <map>

void UiPanel(float x, float y, float width, float height, float r, float g, float b, float alpha)
{
    glColor4f(r, g, b, alpha);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

struct UiCanvas::Implementation
{
    HDC dc = nullptr;
    HFONT font = nullptr;
    HGDIOBJ previous = nullptr;
    int size = 0;
    std::map<wchar_t, GLuint> glyphs;

    ~Implementation()
    {
        Clear();
    }

    void Clear()
    {
        for (auto glyph : glyphs)
        {
            glDeleteLists(glyph.second, 1);
        }

        glyphs.clear();

        if (dc && previous)
        {
            SelectObject(dc, previous);
        }

        if (font)
        {
            DeleteObject(font);
        }

        font = nullptr;
        previous = nullptr;
    }

    void Resize(int pixels)
    {
        if (size == pixels)
        {
            return;
        }

        Clear();
        size = pixels;
        dc = wglGetCurrentDC();
        font = CreateFontW(-pixels,
                           0,
                           0,
                           0,
                           FW_MEDIUM,
                           FALSE,
                           FALSE,
                           FALSE,
                           DEFAULT_CHARSET,
                           OUT_DEFAULT_PRECIS,
                           CLIP_DEFAULT_PRECIS,
                           ANTIALIASED_QUALITY,
                           DEFAULT_PITCH,
                           L"Malgun Gothic");

        if (dc && font)
        {
            previous = SelectObject(dc, font);
        }
    }

    void Text(
        float x, float y, const std::wstring& text, float r = .93f, float g = .94f, float b = .87f)
    {
        glColor3f(r, g, b);
        glRasterPos2f(x, y);

        for (wchar_t character : text)
        {
            auto found = glyphs.find(character);

            if (found == glyphs.end())
            {
                GLuint list = glGenLists(1);

                if (!list || !wglUseFontBitmapsW(dc, character, 1, list))
                {
                    if (list)
                    {
                        glDeleteLists(list, 1);
                    }
                    continue;
                }

                found = glyphs.emplace(character, list).first;
            }

            glCallList(found->second);
        }
    }
};

UiCanvas::UiCanvas() : implementation(new Implementation)
{
}

UiCanvas::~UiCanvas() = default;

void UiCanvas::Resize(int pixels)
{
    implementation->Resize(pixels);
}

void UiCanvas::Begin(float width, float height)
{
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void UiCanvas::Text(float x, float y, const std::wstring& text, float r, float g, float b)
{
    implementation->Text(x, y, text, r, g, b);
}

void UiCanvas::WorldText(float x, float y, float z, const std::wstring& text)
{
    glColor3f(.93f, .94f, .87f);
    glRasterPos3f(x, y, z);
    for (wchar_t character : text)
    {
        auto& font = *implementation;
        auto found = font.glyphs.find(character);
        if (found == font.glyphs.end())
        {
            GLuint list = glGenLists(1);
            if (!list || !wglUseFontBitmapsW(font.dc, character, 1, list))
            {
                if (list)
                {
                    glDeleteLists(list, 1);
                }
                continue;
            }
            found = font.glyphs.emplace(character, list).first;
        }
        glCallList(found->second);
    }
}
