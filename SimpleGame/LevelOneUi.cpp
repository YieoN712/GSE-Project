#include "LevelOneUi.h"
#include "LevelOne.h"
#include <algorithm>

void LevelOneUi::Hud(const LevelOne& game, float width, float height, bool loaded, bool saved)
{
    UiPanel(20, 20, 510, 164, .12f, .21f, .23f, .95f);
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
    UiPanel(38, 120, 470, 10, .24f, .31f, .32f);
    UiPanel(38, 120, 470.f * game.stats.health / game.stats.MaximumHealth(), 10, .44f, .76f, .60f);
    UiPanel(38, 144, 470, 9, .24f, .31f, .32f);
    UiPanel(
        38, 144, 470.f * game.stats.experience / game.stats.NextExperience(), 9, .79f, .68f, .91f);
    font.Text(38,
              177,
              L"EXP " + std::to_wstring(game.stats.experience) + L" / " +
                  std::to_wstring(game.stats.NextExperience()) + L"   처치 " +
                  std::to_wstring(game.kills));

    UiPanel(20, height - 90, width - 40, 70, .12f, .21f, .23f, .95f);
    font.Text(
        38, height - 62, L"WASD 이동   Space / J 공격   E 줍기   Q 회복   C 스탯   Esc 일시정지");
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
        UiPanel(20, height - 142, width - 40, 40, .16f, .27f, .27f, .94f);
        font.Text(38, height - 115, game.toast, .96f, .86f, .61f);
    }

    if (game.NearestDrop() >= 0)
    {
        font.Text(width / 2 - 70, height / 2 + 90, L"[E] 아이템 획득", .98f, .89f, .59f);
    }

    // Minimap deliberately presents every walkable tile and blocked plot.
    float mapX = width - 208, mapY = 42, cell = 4;
    UiPanel(mapX - 12, mapY - 22, 200, 184, .12f, .21f, .23f, .95f);
    font.Text(mapX, mapY - 5, L"지도 / 녹색: 쉼터");

    for (int z = 0; z < LevelOne::Rows; ++z)
    {
        for (int x = 0; x < LevelOne::Columns; ++x)
        {
            LevelTile tile = game.tiles[z * LevelOne::Columns + x];
            float r = tile == LevelTile::Building ? .29f : tile == LevelTile::Water ? .24f : .62f;
            float g = tile == LevelTile::Water ? .49f : tile == LevelTile::Building ? .33f : .66f;
            float b = tile == LevelTile::Water ? .64f : .47f;
            UiPanel(mapX + x * cell, mapY + z * cell, cell - .2f, cell - .2f, r, g, b);
        }
    }

    auto dot = [&](LevelPoint point, float r, float g, float b)
    {
        float x = point.x / LevelOne::TileSize + LevelOne::Columns / 2;
        float z = point.z / LevelOne::TileSize + LevelOne::Rows / 2;
        UiPanel(mapX + x * cell - 2, mapY + z * cell - 2, 4, 4, r, g, b);
    };

    for (const auto& enemy : game.enemies)
    {
        if (enemy.state != EnemyState::Dead && game.scene.IsVisible(enemy.Id()))
        {
            dot(enemy.position, .89f, .39f, .38f);
        }
    }

    dot(game.spawn, .36f, .95f, .52f);
    dot(game.Player().position, 1, .93f, .59f);
    font.Text(mapX, mapY + 154, L"Seed " + std::to_wstring(game.mapSeed));
    font.Text(24,
              213,
              loaded  ? L"모델: 파일 캐시 로드"
              : saved ? L"모델: 생성 후 파일 저장 완료"
                      : L"모델: 메모리 전용 (캐시 저장 실패)",
              .22f,
              .32f,
              .28f);

    if (game.statsOpen)
    {
        UiPanel(width / 2 - 315, 224, 630, 280, .12f, .21f, .23f, .98f);
        float left = width / 2 - 293;
        font.Text(left, 258, L"캐릭터 능력치  ·  배분 중에는 전투가 멈춥니다.", .98f, .83f, .52f);
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
        UiPanel(0, 0, width, height, .10f, .17f, .20f, .68f);
        UiPanel(width / 2 - 310, height / 2 - 90, 620, 186, .12f, .21f, .23f, .98f);
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

void LevelOneUi::Draw(const LevelOne& game, int width, int height, bool loaded, bool saved)
{
    float scale = (std::min)(width / 1200.f, height / 800.f);
    float aspect = float(width) / height;
    float uiWidth = width / scale, uiHeight = height / scale;
    font.Resize((std::max)(1, int(18 * scale)));
    font.Begin(uiWidth, uiHeight);
    if (!game.FindPlayer())
    {
        font.Text(38, 50, L"레벨 종료 · [R] 다시 시작");
        return;
    }
    auto project = [&](LevelPoint point, float y) -> LevelPoint
    {
        float x = point.x - game.camera.x;
        float z = point.z - game.camera.z;
        return {uiWidth / 2 + (x - z) * .70710678f / (24 * aspect) * uiWidth,
                uiHeight / 2 - (y * .819152f - (x + z) * .40558f) / 24 * uiHeight};
    };

    for (const auto& enemy : game.enemies)
    {
        if (enemy.state == EnemyState::Dead || !game.scene.IsVisible(enemy.Id()))
        {
            continue;
        }

        auto matrix = game.scene.WorldMatrix(enemy.Id());
        LevelPoint screen = project({matrix[12], matrix[14]}, matrix[13] + 2);
        UiPanel(screen.x - 20, screen.z, 40, 5, .20f, .25f, .28f);
        UiPanel(screen.x - 20,
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
        font.Text(screen.x - 14,
                  screen.z,
                  feedback.text,
                  feedback.positive ? .83f : 1.f,
                  feedback.positive ? .92f : .78f,
                  .48f);
    }

    Hud(game, uiWidth, uiHeight, loaded, saved);
}
