#include "stdafx.h"
#include "Tutorial.h"
#include "RenderEffects.h"
#include "Dependencies/glew.h"
#include "Dependencies/freeglut.h"
#include <algorithm>
#include <cmath>
#include <map>

namespace
{
    float Clamp(float v, float lo, float hi)
    {
        return (std::max)(lo, (std::min)(v, hi));
    }

    float Distance(GroundPoint a, GroundPoint b)
    {
        return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z));
    }

    unsigned char Lower(unsigned char c)
    {
        return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
    }
} // namespace

const GroundPoint Tutorial::board = {7, -11};
const GroundPoint Tutorial::bench = {8, 3};

Tutorial::Tutorial()
{
    Reset();
}

void Tutorial::Reset()
{
    player = {-14, 9};
    camera = player;

    stage = 0;
    elapsed = animation = stalled = 0;
    shadowTime = -1;
    paused = journal = completion = false;
    footsteps.clear();
    playerHeading = playerStride = playerSpeed = stepDistance = 0;

    dialogue.clear();
    line = 0;
    speaker = pendingStage = -1;
    ClearKeys();

    residents = {
        {{L"빵집 사장님",
          L"오늘도 좋은 하루 보내. 빵 냄새가 골목 끝까지 나지?",
          {-12, 3},
          {-12, 3},
          .88f,
          .56f,
          .31f},
         {L"어르신", L"바람이 참 좋구나. 잠깐 쉬었다 가렴.", {8, 3}, {8, 3}, .56f, .49f, .72f},
         {L"청소하는 이웃",
          L"좋은 아침! 깨끗해진 길을 걸으면 기분도 좋아지지.",
          {-15, 8},
          {-15, 8},
          .28f,
          .63f,
          .65f},
         {L"꽃을 돌보는 주민",
          L"어제 심은 꽃이 폈어. 지나갈 때 한번 봐 줘.",
          {-3, -6},
          {-3, -6},
          .78f,
          .40f,
          .42f},
         {L"배달원",
          L"이 동네는 문 앞에 놓인 화분만 봐도 집을 찾을 수 있어.",
          {0, 7},
          {0, 7},
          .87f,
          .64f,
          .18f},
         {L"학생",
          L"게시판에 옛날 동네 사진이 있어. 우리 집도 나왔더라.",
          {5, -6},
          {5, -6},
          .31f,
          .47f,
          .73f},
         {L"편의점 손님",
          L"늘 먹던 음료가 있으면 하루가 조금 편해져.",
          {14, -3},
          {14, -3},
          .64f,
          .66f,
          .40f},
         {L"쉼터 이웃",
          L"여긴 오후 햇살이 가장 오래 머무는 곳이야.",
          {10, 5},
          {10, 5},
          .83f,
          .47f,
          .57f}}};
    for (int i = 2; i <= 6; ++i)
        residents[i].heading = float(i) * 1.2f;
    residents[3].home = residents[3].position = {-24, -10};
    residents[6].home = residents[6].position = {28, 16};

    buildings = {{-17, -10, 8, 6, 4.4f, .91f, .80f, .65f, L"해온 주택"},
                 {-15, -2, 7, 6, 3.1f, .96f, .84f, .66f, L"온기 빵집"},
                 {-5, -13, 7, 5, 4.8f, .77f, .85f, .79f, L"세탁소"},
                 {16, -9, 7, 7, 3.5f, .82f, .87f, .82f, L"오늘 편의점"},
                 {17, 11, 6, 5, 3.0f, .94f, .77f, .66f, L"꽃집"},
                 {-7, 13, 7, 4, 2.8f, .87f, .83f, .70f, L"우리 집"}};

    // Outer blocks add explorable space while retaining the short tutorial route.
    for (int row = 0; row < 2; ++row)
        for (int col = 0; col < 5; ++col)
        {
            buildings.push_back({-34.f + col * 17.f,
                                 row == 0 ? -25.f : 25.f,
                                 8,
                                 6,
                                 3.5f + (col % 3),
                                 .79f + col * .025f,
                                 .80f - row * .06f,
                                 .73f,
                                 L"동네 상가"});
        }
    buildings.push_back({-35, -6, 7, 9, 4, .82f, .75f, .65f, L"주거동"});
    buildings.push_back({35, 8, 7, 9, 4, .85f, .80f, .69f, L"주거동"});
}

void Tutorial::ClearKeys()
{
    keys.fill(false);
}

bool Tutorial::Blocked(GroundPoint p) const
{
    if (p.x < -HalfWidth || p.x > HalfWidth || p.z < -HalfDepth || p.z > HalfDepth)
        return true;
    for (const auto& b : buildings)
        if (std::abs(p.x - b.x) < b.width * .5f + .32f &&
            std::abs(p.z - b.z) < b.depth * .5f + .32f)
            return true;
    return false;
}

void Tutorial::Update(float dt)
{
    if (paused || completion)
        return;
    animation += dt;
    if (stage < 5)
    {
        elapsed += dt;
        stalled += dt;
    }
    // Cap simulation displacement without under-reporting real active play time.
    dt = (std::min)(dt, .05f);
    GroundPoint oldPlayer = player;
    if (dialogue.empty())
    {
        float sx = float(keys['d']) - float(keys['a']);
        float sy = float(keys['w']) - float(keys['s']);
        float len = std::sqrt(sx * sx + sy * sy);
        if (len > 0)
        {
            // Inverse of the 45-degree camera yaw. Screen up means negative X and Z.
            float dx = (sx - sy) * .70710678f / len * 3.5f * dt;
            float dz = (-sx - sy) * .70710678f / len * 3.5f * dt;
            GroundPoint next = {player.x + dx, player.z};
            if (!Blocked(next))
                player.x = next.x;
            next = {player.x, player.z + dz};
            if (!Blocked(next))
                player.z = next.z;
        }
    }
    float moved = Distance(player, oldPlayer);
    playerSpeed = dt > 0 ? moved / dt : 0;
    if (moved > .0001f)
    {
        playerHeading = std::atan2(player.x - oldPlayer.x, player.z - oldPlayer.z);
        playerStride += moved * 6;
        stepDistance += moved;
        if (stepDistance > .52f)
        {
            stepDistance = 0;
            footsteps.push_back({player, 0});
        }
    }
    for (auto& f : footsteps)
        f.age += dt;
    footsteps.erase(std::remove_if(footsteps.begin(),
                                   footsteps.end(),
                                   [](const Footstep& f)
                                   {
                                       return f.age > .8f;
                                   }),
                    footsteps.end());
    for (int i : {2, 3, 4, 5, 6})
    {
        auto& n = residents[i];
        GroundPoint before = n.position;
        float angle = n.heading;
        GroundPoint next = {n.position.x + std::sin(angle) * .65f * dt,
                            n.position.z + std::cos(angle) * .65f * dt};
        if (Blocked(next) || Distance(next, n.home) > 7.f)
        {
            angle += 1.4f;
            next = {n.position.x + std::sin(angle) * .65f * dt,
                    n.position.z + std::cos(angle) * .65f * dt};
        }
        if (!Blocked(next))
            n.position = next;
        n.heading = angle;
        n.walkSpeed = dt > 0 ? Distance(before, n.position) / dt : 0;
        n.stride += Distance(before, n.position) * 6;
    }
    camera.x += (Clamp(player.x, -37, 37) - camera.x) * (1 - std::exp(-dt * 5));
    camera.z += (Clamp(player.z, -26, 26) - camera.z) * (1 - std::exp(-dt * 5));
    if (stage >= 3 && shadowTime < 0 && Distance(player, bench) < 5)
        shadowTime = 0;
    else if (shadowTime >= 0)
        shadowTime += dt;
}

GroundPoint Tutorial::ObjectivePosition() const
{
    if (stage == 0)
        return residents[0].position;
    if (stage == 1 || stage == 3)
        return bench;
    if (stage == 2)
        return board;
    return residents[7].position;
}

const wchar_t* Tutorial::Objective() const
{
    static const wchar_t* text[] = {L"빵집 사장님에게 인사하기",
                                    L"쉼터의 어르신께 빵 전달하기",
                                    L"골목 게시판의 동네 사진 보기",
                                    L"쉼터의 빈 벤치 조사하기",
                                    L"쉼터 이웃에게 어르신에 대해 묻기",
                                    L"기억 기록: 오늘 빵을 건넨 사람"};
    return text[stage];
}

const wchar_t* Tutorial::TargetName(int target) const
{
    if (target >= 0 && target < 8)
        return residents[target].name;
    return target == 8 ? L"동네 사진 게시판" : L"늘 앉던 자리";
}

int Tutorial::Target() const
{
    int result = -1;
    float best = 1.8f;
    for (int i = 0; i < 10; ++i)
    {
        if (i >= 2 && i <= 6)
            continue; // Background walkers deliberately have no interaction.
        if (i == 1 && stage >= 3)
            continue;
        if (i == 9 && stage < 3)
            continue;
        GroundPoint p = i < 8 ? residents[i].position : (i == 8 ? board : bench);
        float d = Distance(player, p);
        if (d >= best)
            continue;
        bool visible = true;
        for (int j = 1; j <= 8; ++j)
        {
            float t = j / 8.f;
            if (Blocked({player.x + (p.x - player.x) * t, player.z + (p.z - player.z) * t}))
                visible = false;
        }
        if (visible)
        {
            best = d;
            result = i;
        }
    }
    return result;
}

void Tutorial::Say(int who, std::initializer_list<const wchar_t*> lines, int next)
{
    dialogue.assign(lines.begin(), lines.end());
    line = 0;
    speaker = who;
    pendingStage = next;
    ClearKeys();
}

void Tutorial::Interact()
{
    if (!dialogue.empty())
    {
        if (++line < dialogue.size())
            return;
        dialogue.clear();
        if (pendingStage >= 0)
        {
            stage = pendingStage;
            stalled = 0;
            if (stage == 5)
                completion = true;
        }
        pendingStage = -1;
        speaker = -1;
        return;
    }
    int t = Target();
    if (t < 0)
        return;
    if (t == 0 && stage == 0)
        Say(t,
            {L"벤치에 계신 어르신께 이 빵을 전해 줄래?",
             L"늘 같은 시간에 오시거든. 식기 전에 부탁해."},
            1);
    else if (t == 1 && stage == 1)
        Say(t,
            {L"오늘도 잊지 않았구나. 고맙다.",
             L"누군가 기다린다는 건 참 좋은 일이야.",
             L"돌아가는 길에 게시판의 동네 사진도 보고 가렴."},
            2);
    else if (t == 8 && stage == 2)
        Say(t,
            {L"사진 속 벤치에 어르신이 앉아 있다.",
             L"늘 지나가던 곳인데, 이렇게 보니 조금 다르다.",
             L"쉼터 쪽에서 바스락거리는 소리가 난다."},
            3);
    else if (t == 9 && stage == 3)
        Say(t,
            {L"빵 봉투와 손수건만 남아 있다.", L"사람은 없는데, 그림자는 조금 늦게 사라졌다."},
            4);
    else if (t == 7 && stage == 4)
        Say(t,
            {L"어르신? 오늘 저 벤치에는 아무도 없었는데.",
             L"분명 방금 이야기를 나눴는데….",
             L"잊지 않도록 오늘의 일을 기록해 두자."},
            5);
    else if (t == 8)
        Say(t, {L"동네 사람들이 함께 찍은 사진이다. 따뜻한 오후의 풍경."});
    else if (t == 9)
        Say(t, {L"오늘 빵을 건넨 사람. 그 목소리를 기억하고 싶다."});
    else
        Say(t, {residents[t].greeting});
}

void Tutorial::KeyDown(unsigned char raw)
{
    unsigned char key = Lower(raw);
    if (keys[key])
        return;
    keys[key] = true;
    if (key == 27)
    {
        paused = !paused;
        ClearKeys();
        return;
    }
    if ((paused || completion) && key == 'r')
    {
        Reset();
        return;
    }
    if (completion && key == 'e')
    {
        completion = false;
        return;
    }
    if (paused || completion)
        return;
    if (key == 9)
        journal = !journal;
    if (key == 'e')
    {
        Interact();
        keys['e'] = true;
    }
}

void Tutorial::KeyUp(unsigned char key)
{
    keys[Lower(key)] = false;
}

namespace
{
    RenderEffects* activeEffects = nullptr;

    void Color(float r, float g, float b, float a = 1)
    {
        glColor4f(r, g, b, a);
    }

    void Box(float x,
             float y,
             float z,
             float w,
             float h,
             float d,
             float r,
             float g,
             float b,
             int material = 0)
    {
        if (activeEffects)
            activeEffects->Material(material);
        float x0 = x - w / 2, x1 = x + w / 2, z0 = z - d / 2, z1 = z + d / 2;
        glBegin(GL_QUADS);
        Color(r * .76f, g * .76f, b * .76f); // faceted ambient + sun lighting
        glNormal3f(0, 0, 1);
        glVertex3f(x0, y, z1);
        glVertex3f(x1, y, z1);
        glVertex3f(x1, y + h, z1);
        glVertex3f(x0, y + h, z1);
        glNormal3f(0, 0, -1);
        glVertex3f(x1, y, z0);
        glVertex3f(x0, y, z0);
        glVertex3f(x0, y + h, z0);
        glVertex3f(x1, y + h, z0);
        Color(r * .89f, g * .89f, b * .89f);
        glNormal3f(-1, 0, 0);
        glVertex3f(x0, y, z0);
        glVertex3f(x0, y, z1);
        glVertex3f(x0, y + h, z1);
        glVertex3f(x0, y + h, z0);
        glNormal3f(1, 0, 0);
        glVertex3f(x1, y, z1);
        glVertex3f(x1, y, z0);
        glVertex3f(x1, y + h, z0);
        glVertex3f(x1, y + h, z1);
        Color(r, g, b);
        glNormal3f(0, 1, 0);
        glVertex3f(x0, y + h, z0);
        glVertex3f(x0, y + h, z1);
        glVertex3f(x1, y + h, z1);
        glVertex3f(x1, y + h, z0);
        glEnd();
    }

    void Disc(float x, float y, float z, float rx, float rz, float r, float g, float b, float a = 1)
    {
        if (activeEffects && activeEffects->ShadowPass())
            return;
        if (activeEffects)
            activeEffects->Material(0);
        glNormal3f(0, 1, 0);
        Color(r, g, b, a);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(x, y, z);
        for (int i = 0; i <= 32; ++i)
        {
            float t = i * 6.2831853f / 32;
            glVertex3f(x + std::cos(t) * rx, y, z + std::sin(t) * rz);
        }
        glEnd();
    }

    void Ring(GroundPoint p, float radius, float r, float g, float b)
    {
        if (activeEffects && activeEffects->ShadowPass())
            return;
        if (activeEffects)
            activeEffects->Material(0);
        glNormal3f(0, 1, 0);
        Color(r, g, b);
        glLineWidth(3);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 40; ++i)
        {
            float t = i * 6.2831853f / 40;
            glVertex3f(p.x + std::cos(t) * radius, .07f, p.z + std::sin(t) * radius);
        }
        glEnd();
        glLineWidth(1);
    }

    void Ellipsoid(float x,
                   float y,
                   float z,
                   float rx,
                   float ry,
                   float rz,
                   float r,
                   float g,
                   float b,
                   int material = 0)
    {
        if (activeEffects)
            activeEffects->Material(material);
        Color(r, g, b);
        for (int lat = 0; lat < 8; ++lat)
        {
            glBegin(GL_QUAD_STRIP);
            for (int lon = 0; lon <= 12; ++lon)
                for (int k = 0; k < 2; ++k)
                {
                    float a = (lat + k) * 3.14159265f / 8 - 1.5707963f, t = lon * 6.2831853f / 12;
                    float nx = std::cos(a) * std::cos(t), ny = std::sin(a),
                          nz = std::cos(a) * std::sin(t);
                    float len =
                        std::sqrt(nx * nx / (rx * rx) + ny * ny / (ry * ry) + nz * nz / (rz * rz));
                    glNormal3f(nx / rx / len, ny / ry / len, nz / rz / len);
                    glVertex3f(x + nx * rx, y + ny * ry, z + nz * rz);
                }
            glEnd();
        }
    }

    void Person(GroundPoint p,
                float r,
                float g,
                float b,
                float time,
                int type,
                float heading = 0,
                float stride = 0,
                float speed = 0)
    {
        float swing = speed > .02f ? std::sin(stride) * .20f : 0;
        float bob = speed > .02f ? std::abs(std::sin(stride)) * .045f
                                 : std::sin(time * 1.8f + type) * .012f;
        // Transform each anatomical part into world space so it participates in the shadow map.
        auto part = [&](float x,
                        float y,
                        float z,
                        float rx,
                        float ry,
                        float rz,
                        float cr,
                        float cg,
                        float cb)
        {
            float wx = p.x + x * std::cos(heading) + z * std::sin(heading);
            float wz = p.z - x * std::sin(heading) + z * std::cos(heading);
            Ellipsoid(wx, y, wz, rx, ry, rz, cr, cg, cb, 8);
        };
        glDepthMask(GL_FALSE);
        for (int i = 4; i >= 0; --i)
            Disc(p.x,
                 .034f + i * .0004f,
                 p.z,
                 .29f + i * .035f,
                 .22f + i * .028f,
                 .20f,
                 .25f,
                 .28f,
                 .045f);
        glDepthMask(GL_TRUE);
        for (int side : {-1, 1})
        {
            float s = side * .14f, z = swing * side;
            part(s, .12f, z + .06f, .12f, .09f, .20f, .19f, .24f, .28f); // shoes
            part(s, .32f, z * .65f, .105f, .20f, .11f, .27f, .32f, .38f);
            part(s, .55f, z * .20f, .13f, .19f, .14f, .28f, .33f, .40f);
            part(
                side * .33f, .88f + bob, -z * .60f, .11f, .20f, .12f, r * .88f, g * .88f, b * .88f);
            part(side * .36f, .66f + bob, -z, .085f, .12f, .09f, .95f, .75f, .59f);
        }
        part(0, .89f + bob, 0, .29f, .31f, .19f, r, g, b);
        part(0, 1.18f + bob, 0, .09f, .10f, .09f, .95f, .75f, .59f);
        part(0, 1.40f + bob, 0, .23f, .25f, .215f, .96f, .77f, .61f);
        float hair = type == 1 ? .67f : .19f;
        part(0, 1.56f + bob, -.03f, .24f, .14f, .215f, hair, hair * .91f, hair * .83f);
        part(0, 1.40f + bob, .21f, .047f, .056f, .055f, .90f, .65f, .48f);
        for (int side : {-1, 1})
            part(side * .09f, 1.46f + bob, .193f, .023f, .025f, .023f, .15f, .19f, .20f);
        if (type == 0)
            part(0, .83f, .18f, .23f, .24f, .03f, .94f, .90f, .77f);
        if (type == 4 || type == 8)
            part(0, .92f, -.24f, .23f, .25f, .12f, .61f, .39f, .25f);
    }

    void Panel(float x, float y, float w, float h, float r, float g, float b, float a = 1)
    {
        Color(r, g, b, a);
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
        glEnd();
    }
} // namespace

struct TutorialView::Font
{
    HDC dc = nullptr;
    HFONT handle = nullptr;
    HGDIOBJ previous = nullptr;
    int pixelHeight = 0;
    std::map<wchar_t, GLuint> glyphs;

    Font()
    {
        dc = wglGetCurrentDC();
        Resize(19);
    }

    void Resize(int pixels)
    {
        if (pixelHeight == pixels)
            return;
        for (auto item : glyphs)
            glDeleteLists(item.second, 1);
        glyphs.clear();
        if (dc && previous)
            SelectObject(dc, previous);
        if (handle)
            DeleteObject(handle);
        pixelHeight = pixels;
        handle = CreateFontW(-pixels,
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
        if (dc && handle)
            previous = SelectObject(dc, handle);
    }

    ~Font()
    {
        for (auto item : glyphs)
            glDeleteLists(item.second, 1);
        if (dc && previous)
            SelectObject(dc, previous);
        if (handle)
            DeleteObject(handle);
    }

    void Text(
        float x, float y, const std::wstring& s, float r = .94f, float g = .93f, float b = .85f)
    {
        Color(r, g, b);
        glRasterPos2f(x, y);
        Emit(s);
    }

    void WorldText(float x, float y, float z, const std::wstring& s)
    {
        Color(.20f, .28f, .26f);
        glRasterPos3f(x, y, z);
        Emit(s);
    }

    void Emit(const std::wstring& s)
    {
        for (wchar_t ch : s)
        {
            auto it = glyphs.find(ch);
            if (it == glyphs.end())
            {
                GLuint id = glGenLists(1);
                if (!id)
                    continue;
                if (!wglUseFontBitmapsW(dc, ch, 1, id))
                {
                    glDeleteLists(id, 1);
                    continue;
                }
                it = glyphs.emplace(ch, id).first;
            }
            glCallList(it->second);
        }
    }
};

TutorialView::TutorialView() : font(new Font), effects(new RenderEffects)
{
}

TutorialView::~TutorialView()
{
    delete effects;
    delete font;
}

void TutorialView::Draw(const Tutorial& game, int width, int height)
{
    float uiScale = (std::min)(width / 1100.f, height / 720.f);
    font->Resize((std::max)(1, int(19 * uiScale)));
    activeEffects = effects;
    auto world = [&]()
    {
        Box(0, -.4f, 0, 90, .35f, 66, .53f, .66f, .38f, 1);
        Box(0, -.04f, 3, 88, .05f, 7, .40f, .44f, .45f, 2);
        Box(5, -.03f, -2, 6, .05f, 62, .65f, .67f, .63f, 3);
        Box(-12, -.02f, 7, 5, .05f, 46, .69f, .69f, .63f, 3);
        Box(0, -.025f, -18, 86, .05f, 4, .43f, .46f, .47f, 2);
        Box(0, -.025f, 19, 86, .05f, 4, .43f, .46f, .47f, 2);
        Box(-25, -.025f, 0, 4, .05f, 63, .68f, .69f, .63f, 3);
        Box(25, -.025f, 0, 4, .05f, 63, .68f, .69f, .63f, 3);
        for (int x = -21; x < 22; x += 2)
            Box(float(x), .015f, 6.3f, 1.9f, .025f, .12f, .94f, .90f, .78f);
        for (int z = -15; z < 16; z += 2)
            Box(2.2f, .016f, float(z), .10f, .024f, 1.9f, .94f, .90f, .78f);
        for (const auto& b : game.buildings)
        {
            // Cut away foreground buildings when they could hide the player.
            float forward = (b.x - game.player.x + b.z - game.player.z) * .7071f;
            float sideways = std::abs((b.x - game.player.x - b.z + game.player.z) * .7071f);
            if (forward > 0 && forward < b.height * 1.8f + (b.width + b.depth) * .36f &&
                sideways < (b.width + b.depth) * .36f)
            {
                Box(b.x, 0, b.z, b.width, .20f, b.depth, b.r, b.g, b.b);
                continue;
            }
            Box(b.x, 0, b.z, b.width, b.height, b.depth, b.r, b.g, b.b, 4);
            Box(b.x, b.height, b.z, b.width + .4f, .25f, b.depth + .4f, .46f, .49f, .47f, 5);
            Box(b.x + 1,
                b.height + .25f,
                b.z,
                1.1f,
                .65f,
                .8f,
                .76f,
                .79f,
                .76f,
                5); // roof equipment
            for (float x = b.x - b.width / 2 + 1; x < b.x + b.width / 2; x += 1.7f)
            {
                Box(x, 1.4f, b.z + b.depth / 2 + .035f, 1.05f, 1.1f, .08f, .33f, .54f, .55f, 6);
                Box(x, 1.4f, b.z + b.depth / 2 + .09f, .055f, 1.1f, .04f, .80f, .82f, .79f, 5);
                Box(x, 1.37f, b.z + b.depth / 2 + .10f, 1.17f, .08f, .15f, .94f, .90f, .75f);
            }
            Box(b.x, 0, b.z + b.depth / 2 + .07f, .85f, 1.8f, .1f, .38f, .36f, .30f);
            Box(b.x, 2.25f, b.z + b.depth / 2 + .4f, b.width - .5f, .15f, 1.2f, .30f, .59f, .57f);
            for (int i = 0; i < 5; ++i)
                Box(b.x - 2 + i,
                    2.23f,
                    b.z + b.depth / 2 + .43f,
                    .30f,
                    .18f,
                    1.22f,
                    .91f,
                    .85f,
                    .65f);
        }
        // Low props deliberately keep the central walking paths readable.
        for (int i = 0; i < 18; ++i)
        {
            float x = -41.f + i * 4.8f;
            float z = (i % 2) ? -30.f : 30.f;
            Box(x, 0, z, .38f, 2.1f, .38f, .48f, .37f, .24f);
            float sway = std::sin(game.animation + i) * .06f;
            Ellipsoid(x + sway, 2.6f, z, 1.5f, 1.1f, 1.4f, .41f, .62f, .37f, 1);
            Ellipsoid(x + sway, 3.25f, z, 1.05f, .85f, 1.0f, .56f, .72f, .42f, 1);
        }
        for (int i = 0; i < 7; ++i)
        {
            float x = -3.f + i * 2;
            Box(x, 0, -8, .8f, .35f, .8f, .70f, .47f, .34f);
            Box(x, .35f, -8, .75f, .27f, .75f, .41f, .61f, .35f);
            Box(x, .62f, -8, .22f, .19f, .25f, .96f, .70f, .54f);
        }
        // A small sheltered garden and washing line give the neighborhood a lived-in scale.
        Box(11, -.01f, 1, 5, .03f, 5, .60f, .72f, .43f, 1);
        Box(12, 0, 0, .4f, 2.1f, .4f, .49f, .37f, .25f);
        Ellipsoid(
            12 + std::sin(game.animation) * .05f, 2.9f, 0, 1.65f, 1.25f, 1.6f, .46f, .66f, .37f, 1);
        Ellipsoid(12, 3.8f, 0, 1.0f, .8f, .95f, .59f, .74f, .43f, 1);
        Box(-19, 0, 11, .09f, 2.1f, .09f, .47f, .45f, .38f);
        Box(-14, 0, 11, .09f, 2.1f, .09f, .47f, .45f, .38f);
        Color(.45f, .43f, .37f);
        glBegin(GL_LINES);
        glVertex3f(-19, 2, 11);
        glVertex3f(-14, 2, 11);
        glEnd();
        for (int i = 0; i < 4; ++i)
            Box(-18.5f + i * 1.1f,
                1.2f,
                11 + std::sin(game.animation + i) * .10f,
                .8f,
                .8f,
                .025f,
                .93f,
                .85f - i * .05f,
                .72f);
        for (int i = 0; i < 3; ++i)
        {
            float x = -19.f + i * 18;
            Box(x, 0, 7, .15f, 2.7f, .15f, .31f, .39f, .38f);
            Box(x, 2.7f, 7, .50f, .35f, .50f, .99f, .89f, .59f);
        }
        Box(-12, .0f, 1.6f, 2, .65f, .8f, .58f, .39f, .26f);
        for (int i = 0; i < 4; ++i)
            Box(-12.7f + i * .45f, .65f, 1.6f, .34f, .17f, .45f, .91f, .67f, .34f);
        // Bench, empty neighboring seat, and the handkerchief remain through the anomaly.
        Box(8, .0f, 2.8f, 2.6f, .45f, .65f, .58f, .40f, .27f, 7);
        Box(8, .45f, 2.5f, 2.6f, .50f, .12f, .66f, .47f, .30f, 7);
        Box(8.8f, .46f, 2.9f, .36f, .02f, .34f, .92f, .91f, .78f);
        if (game.stage >= 2)
            Box(7.3f, .46f, 2.9f, .36f, .40f, .27f, .87f, .69f, .44f);
        Box(7, 0, -11, .13f, 2.2f, .13f, .50f, .38f, .26f);
        Box(7, 1, -11, 2.5f, 1.3f, .15f, .55f, .42f, .29f);
        Box(7, 1.15f, -10.90f, 2.1f, .96f, .03f, .94f, .90f, .75f);
        if (game.stage >= 3)
        {
            Disc(8, .045f, 3, 2, 1.6f, .42f, .56f, .61f, .28f);
            if (game.shadowTime >= 0 && game.shadowTime < 2)
                Disc(8, .051f, 3, .32f, 1.1f, .20f, .24f, .30f, 1 - game.shadowTime / 2);
        }
        for (int i = 0; i < 8; ++i)
        {
            if (i == 1 && game.stage >= 3)
                continue;
            const auto& n = game.residents[i];
            Person(n.position, n.r, n.g, n.b, game.animation, i, n.heading, n.stride, n.walkSpeed);
        }
        Person(game.player,
               .27f,
               .64f,
               .72f,
               game.animation,
               8,
               game.playerHeading,
               game.playerStride,
               game.playerSpeed);
        if (!effects->ShadowPass())
        {
            glDepthMask(GL_FALSE);
            for (const auto& f : game.footsteps)
            {
                float t = f.age / .8f;
                for (int i = 0; i < 3; ++i)
                    Disc(f.position.x + (i - 1) * t * .16f,
                         .08f + t * .15f,
                         f.position.z + t * .13f,
                         .05f + t * .12f,
                         .04f + t * .10f,
                         .85f,
                         .81f,
                         .67f,
                         (1 - t) * .24f);
            }
            glDepthMask(GL_TRUE);
        }
        Ring(game.player, .49f, .96f, .92f, .68f);
        if (game.stage == 1)
            Box(game.player.x + .48f, .5f, game.player.z, .28f, .38f, .25f, .88f, .68f, .40f);
        if (game.stage < 5)
        {
            GroundPoint p = game.ObjectivePosition();
            float pulse = std::sin(game.animation * (game.stalled > 30 ? 5 : 2)) * .12f;
            Ring(p, .80f + pulse, .96f, .76f, .31f);
            if (!effects->ShadowPass())
                Box(p.x, 2.2f + pulse, p.z, .20f, .50f, .20f, .98f, .80f, .35f);
        }
        int target = game.Target();
        if (target >= 0)
            Ring(target < 8 ? game.residents[target].position
                            : (target == 8 ? Tutorial::board : Tutorial::bench),
                 .57f,
                 .95f,
                 .98f,
                 .84f);
    };
    if (effects->BeginShadow(game.camera.x, game.camera.z))
        world();
    effects->BeginScene(width, height);
    glClearColor(.76f, .83f, .79f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = float(width) / height;
    glOrtho(-12 * aspect, 12 * aspect, -12, 12, -100, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(35, 1, 0, 0);
    glRotatef(-45, 0, 1, 0);
    glTranslatef(-game.camera.x, 0, -game.camera.z);
    world();
    effects->Unlit();
    activeEffects = nullptr;

    font->WorldText(-13.5f, 3.5f, 2, L"온기 빵집");
    font->WorldText(6, 2.5f, -11, L"동네 사진");
    font->WorldText(8, 2.6f, 5, L"작은 쉼터");
    effects->Composite();
    int target = game.Target();

    // Fixed logical UI coordinates are letterboxed within any window aspect ratio.
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float uiW = width / uiScale, uiH = height / uiScale;
    glOrtho(0, uiW, uiH, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Panel(24, 24, 560, 122, .13f, .23f, .24f, .94f);
    font->Text(44, 53, L"해온동  /  늘 앉던 자리", .98f, .81f, .48f);
    font->Text(44, 87, game.Objective());
    std::wstring status = L"WASD 이동   E 상호작용   Tab 기록   Esc 일시정지";
    font->Text(44, 121, status, .73f, .83f, .78f);
    int seconds = int(game.elapsed);
    font->Text(uiW - 230, 48, L"늦은 오후  ·  초여름");
    font->Text(uiW - 230,
               77,
               L"플레이 " + std::to_wstring(seconds / 60) + L"분 " + std::to_wstring(seconds % 60) +
                   L"초");
    if (game.stage == 1)
        font->Text(44, 175, L"소지품: 아직 따뜻한 빵 봉투", .25f, .32f, .28f);
    if (game.stage < 5)
    {
        GroundPoint d = game.ObjectivePosition();
        float dx = d.x - game.player.x, dz = d.z - game.player.z;
        float sx = (dx - dz) * .7071f, sy = (dx + dz) * .4056f;
        float n = std::sqrt(sx * sx + sy * sy);
        float cx = uiW - 93, cy = 132;
        Panel(uiW - 250, 95, 226, 99, .13f, .23f, .24f, .88f);
        font->Text(uiW - 234, 124, L"다음 목적지");
        font->Text(uiW - 234, 163, std::to_wstring(int(Distance(game.player, d))) + L" m");
        if (n > .01f)
        {
            sx /= n;
            sy /= n;
            Color(.98f, .81f, .48f);
            glBegin(GL_TRIANGLES);
            glVertex2f(cx + sx * 23, cy + sy * 23);
            glVertex2f(cx - sx * 12 - sy * 9, cy - sy * 12 + sx * 9);
            glVertex2f(cx - sx * 12 + sy * 9, cy - sy * 12 - sx * 9);
            glEnd();
        }
    }
    if (target >= 0 && game.dialogue.empty() && !game.paused && !game.completion)
    {
        Panel(24, uiH - 83, 540, 54, .13f, .23f, .24f, .94f);
        font->Text(
            44, uiH - 49, L"[E] " + std::wstring(game.TargetName(target)) + L" · 대화 / 조사");
    }
    if (game.journal && game.dialogue.empty())
    {
        Panel(24, 207, 610, 170, .96f, .92f, .81f, .98f);
        font->Text(44, 242, L"오늘의 기록  /  식기 전에", .24f, .32f, .30f);
        font->Text(44, 279, game.Objective(), .24f, .32f, .30f);
        font->Text(44, 315, L"빵집 → 어르신 → 게시판 → 빈 벤치 → 쉼터 이웃", .24f, .32f, .30f);
        font->Text(44, 352, L"서두르지 않아도 괜찮다. 빵은 사라지지 않는다.", .24f, .32f, .30f);
    }
    if (!game.dialogue.empty())
    {
        Panel(24, uiH - 166, uiW - 48, 142, .12f, .21f, .23f, .98f);
        font->Text(46, uiH - 130, game.TargetName(game.speaker), .98f, .81f, .48f);
        font->Text(46, uiH - 90, game.dialogue[game.line]);
        font->Text(46,
                   uiH - 47,
                   L"[E] 계속  ·  " + std::to_wstring(game.line + 1) + L" / " +
                       std::to_wstring(game.dialogue.size()),
                   .72f,
                   .82f,
                   .78f);
        if (game.speaker == 8 && game.stage == 2)
        {
            // In-engine stylized neighborhood photograph, no external bitmap required.
            Panel(uiW / 2 - 175, 220, 350, 220, .96f, .91f, .77f);
            Panel(uiW / 2 - 157, 238, 314, 165, .65f, .77f, .70f);
            Panel(uiW / 2 - 100, 324, 200, 13, .47f, .33f, .23f);
            Panel(uiW / 2 - 30, 288, 26, 36, .56f, .49f, .72f);
            Panel(uiW / 2 - 26, 270, 18, 18, .93f, .75f, .58f);
            font->Text(uiW / 2 - 145, 428, L"늘 같은 자리에 앉아 있던 사람", .24f, .32f, .30f);
        }
    }
    if (game.paused || game.completion)
    {
        Panel(0, 0, uiW, uiH, .10f, .17f, .20f, .65f);
        Panel(uiW / 2 - 330, uiH / 2 - 110, 660, 235, .14f, .24f, .26f, .99f);
        font->Text(uiW / 2 - 300,
                   uiH / 2 - 65,
                   game.paused ? L"잠시 쉬어가기" : L"기억 기록 획득: 오늘 빵을 건넨 사람",
                   .98f,
                   .81f,
                   .48f);
        font->Text(uiW / 2 - 300,
                   uiH / 2 - 17,
                   game.paused ? L"시간과 주민들의 움직임이 멈췄습니다."
                               : L"당연했던 하루에, 기억하고 싶은 일이 생겼다.");
        font->Text(uiW / 2 - 300,
                   uiH / 2 + 35,
                   game.paused ? L"[Esc] 계속하기    [R] 처음부터 다시 시작"
                               : L"[E] 동네 둘러보기    [R] 다시 시작");
        font->Text(uiW / 2 - 300,
                   uiH / 2 + 82,
                   L"창 닫기로 종료 · 저장 기능이 없는 튜토리얼 프로토타입",
                   .72f,
                   .82f,
                   .78f);
    }
}
