#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include "LevelOneView.h"
#include "LevelOne.h"
#include "ModelCache.h"
#include "RenderEffects.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace
{
    constexpr float Pi = 3.14159265f;

    void Panel(
        float x, float y, float width, float height, float r, float g, float b, float alpha = 1)
    {
        glColor4f(r, g, b, alpha);
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + width, y);
        glVertex2f(x + width, y + height);
        glVertex2f(x, y + height);
        glEnd();
    }

    void Circle(LevelPoint point, float y, float radius, float r, float g, float b, float alpha = 1)
    {
        glColor4f(r, g, b, alpha);
        glBegin(GL_LINE_LOOP);

        for (int i = 0; i < 40; ++i)
        {
            float angle = i * Pi / 20;
            glVertex3f(point.x + std::sin(angle) * radius, y, point.z + std::cos(angle) * radius);
        }

        glEnd();
    }

    struct KoreanFont
    {
        HDC dc = nullptr;
        HFONT font = nullptr;
        HGDIOBJ previous = nullptr;
        int size = 0;
        std::map<wchar_t, GLuint> glyphs;

        ~KoreanFont()
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

        void Text(float x,
                  float y,
                  const std::wstring& text,
                  float r = .93f,
                  float g = .94f,
                  float b = .87f)
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
} // namespace

struct LevelOneView::Implementation
{
    RenderEffects effects;
    ModelCache models;
    KoreanFont font;

    void Character(LevelPoint point,
                   float heading,
                   float stride,
                   bool walking,
                   float attack,
                   bool enemy,
                   int kind,
                   bool flash)
    {
        float swing = walking ? std::sin(stride) * .20f : 0;
        float bounce = walking ? std::abs(std::sin(stride)) * .04f : 0;
        float r = enemy ? .45f + kind * .07f : .25f;
        float g = enemy ? .40f : .64f;
        float b = enemy ? .63f : .69f;

        if (flash)
        {
            r = 1;
            g = .80f;
            b = .64f;
        }

        auto body = [&](float x,
                        float y,
                        float z,
                        float sx,
                        float sy,
                        float sz,
                        float red,
                        float green,
                        float blue)
        {
            float wx = point.x + x * std::cos(heading) + z * std::sin(heading);
            float wz = point.z - x * std::sin(heading) + z * std::cos(heading);
            models.Draw(
                ModelKind::Sphere, effects, wx, y, wz, sx, sy, sz, heading, red, green, blue);
        };

        for (int side : {-1, 1})
        {
            body(side * .14f, .11f, swing * side + .06f, .12f, .09f, .19f, .20f, .24f, .28f);
            body(side * .14f, .34f, swing * side * .6f, .11f, .24f, .12f, .26f, .30f, .36f);
            body(side * .32f, .88f + bounce, -swing * side * .5f, .11f, .21f, .12f, r, g, b);
            body(side * .35f, .68f + bounce, -swing * side, .08f, .12f, .09f, .91f, .73f, .57f);
        }

        body(0, .85f + bounce, 0, .28f, .32f, .19f, r, g, b);
        body(0,
             1.37f + bounce,
             0,
             .23f,
             .26f,
             .21f,
             enemy ? .61f : .95f,
             enemy ? .61f : .76f,
             enemy ? .73f : .60f);
        body(0, 1.54f + bounce, -.03f, .24f, .14f, .21f, .24f, .24f, .29f);

        for (int side : {-1, 1})
        {
            body(side * .085f,
                 1.43f + bounce,
                 .197f,
                 .022f,
                 .025f,
                 .024f,
                 enemy ? .94f : .12f,
                 enemy ? .75f : .17f,
                 enemy ? .43f : .20f);
        }

        if (!enemy)
        {
            float swingAngle = heading + (attack > 0 ? (1 - attack / .36f) * 2.3f - 1.15f : .5f);
            models.Draw(ModelKind::Weapon,
                        effects,
                        point.x + std::sin(swingAngle) * .6f,
                        .65f,
                        point.z + std::cos(swingAngle) * .6f,
                        1,
                        1,
                        1,
                        swingAngle);
        }
    }

    void World(const LevelOne& game)
    {
        // Merge adjacent ground tiles into strips; ponds are excluded from the ground surface.
        for (int z = 0; z < LevelOne::Rows; ++z)
        {
            int x = 0;

            while (x < LevelOne::Columns)
            {
                LevelTile tile = game.tiles[z * LevelOne::Columns + x];
                int end = x + 1;

                while (end < LevelOne::Columns && game.tiles[z * LevelOne::Columns + end] == tile)
                {
                    ++end;
                }

                LevelPoint point = game.Center(x, z);

                if (tile != LevelTile::Water)
                {
                    bool road = tile == LevelTile::Road;
                    models.Draw(ModelKind::Box,
                                effects,
                                point.x + (end - x - 1),
                                -.10f,
                                point.z,
                                (end - x) * 2.f,
                                .10f,
                                2,
                                0,
                                road ? .54f : .53f,
                                road ? .56f : .67f,
                                road ? .55f : .40f,
                                road ? 3 : 1);
                }
                else
                {
                    for (int col = x; col < end; ++col)
                    {
                        LevelPoint water = game.Center(col, z);
                        models.Draw(ModelKind::Water, effects, water.x, -.05f, water.z, 2, 1, 2);
                    }
                }

                x = end;
            }
        }

        for (const auto& building : game.buildings)
        {
            float forward =
                (building.position.x - game.player.x + building.position.z - game.player.z) *
                .7071f;
            float sideways = std::abs(
                (building.position.x - game.player.x - building.position.z + game.player.z) *
                .7071f);
            bool cutaway =
                forward > 0 && forward < 13 && sideways < (building.columns + building.rows) * .72f;

            if (cutaway)
            {
                models.Draw(ModelKind::Box,
                            effects,
                            building.position.x,
                            0,
                            building.position.z,
                            building.columns * 2.f - .4f,
                            .25f,
                            building.rows * 2.f - .4f,
                            0,
                            .82f,
                            .80f,
                            .70f,
                            4);
            }
            else
            {
                ModelKind model = ModelKind::Home;

                switch (building.style)
                {
                case BuildingStyle::Shop:
                    model = ModelKind::Shop;
                    break;
                case BuildingStyle::Home:
                    model = ModelKind::Home;
                    break;
                case BuildingStyle::Apartments:
                    model = ModelKind::Apartments;
                    break;
                case BuildingStyle::Utility:
                    model = ModelKind::Utility;
                    break;
                }

                models.Draw(model,
                            effects,
                            building.position.x,
                            0,
                            building.position.z,
                            building.columns * 2.f - .4f,
                            1,
                            building.rows * 2.f - .4f);
            }
        }

        for (const auto& tree : game.trees)
        {
            // Hide nearby foreground crowns, preserving the character silhouette.
            if (std::hypot(tree.x - game.player.x, tree.z - game.player.z) < 2)
            {
                continue;
            }

            models.Draw(ModelKind::Tree, effects, tree.x, 0, tree.z);
            models.Draw(ModelKind::Leaf, effects, tree.x, 3.0f, tree.z);
        }

        // Safe starting plaza: a low circular-looking platform and a warm light.
        models.Draw(ModelKind::Box,
                    effects,
                    game.spawn.x,
                    .01f,
                    game.spawn.z,
                    3.6f,
                    .06f,
                    3.6f,
                    0,
                    .82f,
                    .81f,
                    .70f,
                    3);
        models.Draw(ModelKind::Box,
                    effects,
                    game.spawn.x - 2,
                    0,
                    game.spawn.z,
                    .14f,
                    2.8f,
                    .14f,
                    0,
                    .32f,
                    .39f,
                    .40f,
                    5);
        models.Draw(ModelKind::Sphere,
                    effects,
                    game.spawn.x - 2,
                    2.85f,
                    game.spawn.z,
                    .22f,
                    .20f,
                    .22f,
                    0,
                    .99f,
                    .87f,
                    .49f);

        for (const auto& enemy : game.enemies)
        {
            if (enemy.state != EnemyState::Dead)
            {
                bool moving =
                    enemy.state == EnemyState::Roaming || enemy.state == EnemyState::Chasing;
                Character(enemy.position,
                          enemy.heading,
                          enemy.stride,
                          moving,
                          0,
                          true,
                          enemy.kind,
                          enemy.hitFlash > 0);
            }
        }

        Character(game.player,
                  game.heading,
                  game.stride,
                  game.speed > .02f,
                  game.attackTime,
                  false,
                  0,
                  game.invulnerable > 0 && int(game.time * 16) % 2 == 0);

        for (const auto& drop : game.drops)
        {
            ModelKind kind = drop.kind == DropKind::Potion   ? ModelKind::Potion
                             : drop.kind == DropKind::Memory ? ModelKind::Shard
                                                             : ModelKind::Weapon;
            models.Draw(kind,
                        effects,
                        drop.position.x,
                        .14f + std::sin(game.time * 2) * .07f,
                        drop.position.z,
                        1,
                        1,
                        1,
                        game.time * .7f);
        }
    }

    void Overlays(const LevelOne& game)
    {
        effects.Unlit();
        glDepthMask(GL_FALSE);
        glLineWidth(2);
        Circle(game.spawn, .09f, 5, .65f, .79f, .60f, .45f);
        Circle(game.player, .12f, .46f, .96f, .88f, .55f);

        for (const auto& enemy : game.enemies)
        {
            if (enemy.state == EnemyState::Windup)
            {
                Circle(enemy.position, .10f, 1.35f, 1, .39f, .27f);
            }
        }

        if (game.attackTime > 0)
        {
            glColor4f(1, .90f, .56f, game.attackTime / .36f);
            glBegin(GL_TRIANGLE_STRIP);

            for (int i = 0; i <= 24; ++i)
            {
                float angle = game.heading - 1.4f + i * 2.8f / 24;
                for (float radius : {1.55f, 2.25f})
                {
                    glVertex3f(game.player.x + std::sin(angle) * radius,
                               .65f,
                               game.player.z + std::cos(angle) * radius);
                }
            }

            glEnd();
        }

        for (const auto& dust : game.dust)
        {
            float t = dust.age / .8f;
            Circle(
                dust.position, .12f + t * .15f, .07f + t * .18f, .90f, .84f, .64f, (1 - t) * .5f);
        }

        int drop = game.NearestDrop();
        if (drop >= 0)
        {
            Circle(game.drops[drop].position, .1f, .5f, .92f, .82f, .52f);
        }

        glLineWidth(1);
        glDepthMask(GL_TRUE);
    }

    void Hud(const LevelOne& game, float width, float height)
    {
        Panel(20, 20, 510, 164, .12f, .21f, .23f, .95f);
        font.Text(38, 50, L"레벨 1  /  귀갓길의 잔향", .97f, .81f, .47f);
        font.Text(38,
                  79,
                  game.accomplished ? L"목표 달성 · 자유롭게 사냥하며 계속 성장하세요"
                                    : L"목표: 캐릭터 Lv.5 도달 + 스탯 포인트 1회 배분");
        font.Text(38,
                  108,
                  L"Lv." + std::to_wstring(game.stats.level) + L"   HP " +
                      std::to_wstring(game.stats.health) + L" / " +
                      std::to_wstring(game.stats.MaximumHealth()));
        Panel(38, 120, 470, 10, .24f, .31f, .32f);
        Panel(
            38, 120, 470.f * game.stats.health / game.stats.MaximumHealth(), 10, .44f, .76f, .60f);
        Panel(38, 144, 470, 9, .24f, .31f, .32f);
        Panel(38,
              144,
              470.f * game.stats.experience / game.stats.NextExperience(),
              9,
              .79f,
              .68f,
              .91f);
        font.Text(38,
                  177,
                  L"EXP " + std::to_wstring(game.stats.experience) + L" / " +
                      std::to_wstring(game.stats.NextExperience()) + L"   처치 " +
                      std::to_wstring(game.kills));

        Panel(20, height - 90, width - 40, 70, .12f, .21f, .23f, .95f);
        font.Text(38,
                  height - 62,
                  L"WASD 이동   Space / J 공격   E 줍기   Q 회복   C 스탯   Esc 일시정지");
        font.Text(38,
                  height - 33,
                  L"회복 음료 " + std::to_wstring(game.potions) + L"   기억 조각 " +
                      std::to_wstring(game.fragments) + L"   배분 포인트 " +
                      std::to_wstring(game.stats.points) +
                      (game.stats.weapon ? L"   강화 손전등 장착" : L"   기본 손전등"),
                  .84f,
                  .84f,
                  .68f);

        if (game.toastTime > 0)
        {
            Panel(20, height - 142, width - 40, 40, .16f, .27f, .27f, .94f);
            font.Text(38, height - 115, game.toast, .96f, .86f, .61f);
        }

        if (game.NearestDrop() >= 0)
        {
            font.Text(width / 2 - 70, height / 2 + 90, L"[E] 아이템 획득", .98f, .89f, .59f);
        }

        // Minimap deliberately presents every walkable tile and blocked plot.
        float mapX = width - 208, mapY = 42, cell = 4;
        Panel(mapX - 12, mapY - 22, 200, 184, .12f, .21f, .23f, .95f);
        font.Text(mapX, mapY - 5, L"지도 / 녹색: 쉼터");

        for (int z = 0; z < LevelOne::Rows; ++z)
        {
            for (int x = 0; x < LevelOne::Columns; ++x)
            {
                LevelTile tile = game.tiles[z * LevelOne::Columns + x];
                float r = tile == LevelTile::Building ? .29f
                          : tile == LevelTile::Water  ? .24f
                                                      : .62f;
                float g = tile == LevelTile::Water      ? .49f
                          : tile == LevelTile::Building ? .33f
                                                        : .66f;
                float b = tile == LevelTile::Water ? .64f : .47f;
                Panel(mapX + x * cell, mapY + z * cell, cell - .2f, cell - .2f, r, g, b);
            }
        }

        auto dot = [&](LevelPoint point, float r, float g, float b)
        {
            float x = point.x / LevelOne::TileSize + LevelOne::Columns / 2;
            float z = point.z / LevelOne::TileSize + LevelOne::Rows / 2;
            Panel(mapX + x * cell - 2, mapY + z * cell - 2, 4, 4, r, g, b);
        };

        for (const auto& enemy : game.enemies)
        {
            if (enemy.state != EnemyState::Dead)
            {
                dot(enemy.position, .89f, .39f, .38f);
            }
        }

        dot(game.spawn, .36f, .95f, .52f);
        dot(game.player, 1, .93f, .59f);
        font.Text(mapX, mapY + 154, L"Seed " + std::to_wstring(game.mapSeed));
        font.Text(24,
                  213,
                  models.loadedFromDisk ? L"모델: 파일 캐시 로드"
                  : models.savedToDisk  ? L"모델: 생성 후 파일 저장 완료"
                                        : L"모델: 메모리 전용 (캐시 저장 실패)",
                  .22f,
                  .32f,
                  .28f);

        if (game.statsOpen)
        {
            Panel(width / 2 - 315, 224, 630, 280, .12f, .21f, .23f, .98f);
            float left = width / 2 - 293;
            font.Text(
                left, 258, L"캐릭터 능력치  ·  배분 중에는 전투가 멈춥니다.", .98f, .83f, .52f);
            font.Text(left,
                      296,
                      L"공격 " + std::to_wstring(game.stats.Attack()) + L"   방어 " +
                          std::to_wstring(game.stats.Defense()) + L"   최대 HP " +
                          std::to_wstring(game.stats.MaximumHealth()));
            font.Text(left,
                      332,
                      L"남은 포인트 " + std::to_wstring(game.stats.points) + L"   누적 경험치 " +
                          std::to_wstring(game.stats.totalExperience));
            font.Text(left, 370, L"[1] 힘: 공격 +2   [2] 활력: 최대 HP +12   [3] 방어 +1");
            font.Text(left, 408, L"레벨업 기본 보너스: 최대 HP +16 / 공격 +4 / 방어 +1");
            font.Text(left, 447, L"배분한 스탯은 즉시 적용됩니다. [C / Esc] 닫기");
            font.Text(left, 482, L"지역 번호 1과 캐릭터 레벨은 서로 다릅니다.", .75f, .83f, .78f);
        }

        if (game.paused || game.stats.health <= 0)
        {
            Panel(0, 0, width, height, .10f, .17f, .20f, .68f);
            Panel(width / 2 - 310, height / 2 - 90, 620, 186, .12f, .21f, .23f, .98f);
            font.Text(width / 2 - 282,
                      height / 2 - 48,
                      game.stats.health <= 0 ? L"잠시 쉬어도 괜찮습니다." : L"일시정지",
                      .96f,
                      .83f,
                      .51f);
            font.Text(width / 2 - 282,
                      height / 2 - 5,
                      game.stats.health <= 0 ? L"[R] 쉼터에서 회복 · 경험치와 아이템 유지"
                                             : L"[Esc] 계속   [N] 새 랜덤 맵 + 성장 초기화");
            font.Text(width / 2 - 282,
                      height / 2 + 40,
                      L"게임 진행은 실행 중에만 유지됩니다. 창 닫기로 종료.");
        }
    }
};

LevelOneView::LevelOneView() : implementation(new Implementation)
{
}

LevelOneView::~LevelOneView() = default;

void LevelOneView::Draw(const LevelOne& game, int width, int height)
{
    auto& view = *implementation;
    float scale = (std::min)(width / 1200.f, height / 800.f);
    view.font.Resize((std::max)(1, int(18 * scale)));
    view.effects.SetTime(game.time);

    if (view.effects.BeginShadow(game.camera.x, game.camera.z))
    {
        view.World(game);
    }

    view.effects.BeginScene(width, height);
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
    view.World(game);
    view.Overlays(game);
    view.effects.Composite();

    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float uiWidth = width / scale, uiHeight = height / scale;
    glOrtho(0, uiWidth, uiHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    auto project = [&](LevelPoint point, float y) -> LevelPoint
    {
        float x = point.x - game.camera.x;
        float z = point.z - game.camera.z;
        return {uiWidth / 2 + (x - z) * .70710678f / (24 * aspect) * uiWidth,
                uiHeight / 2 - (y * .819152f - (x + z) * .40558f) / 24 * uiHeight};
    };

    for (const auto& enemy : game.enemies)
    {
        if (enemy.state == EnemyState::Dead)
        {
            continue;
        }

        LevelPoint screen = project(enemy.position, 2);
        Panel(screen.x - 20, screen.z, 40, 5, .20f, .25f, .28f);
        Panel(screen.x - 20,
              screen.z,
              40.f * enemy.health / enemy.maximumHealth,
              5,
              .87f,
              .44f,
              .40f);
    }

    for (const auto& feedback : game.feedback)
    {
        LevelPoint screen = project(feedback.position, 2.2f + (1.4f - feedback.life) * .5f);
        view.font.Text(screen.x - 14,
                       screen.z,
                       feedback.text,
                       feedback.positive ? .83f : 1.f,
                       feedback.positive ? .92f : .78f,
                       .48f);
    }

    view.Hud(game, uiWidth, uiHeight);
}
