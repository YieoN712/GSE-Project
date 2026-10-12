#include "TutorialUi.h"
#include "Tutorial.h"
#include "Dependencies/glew.h"
#include <algorithm>
#include <cmath>

namespace
{
    float Distance(GroundPoint a, GroundPoint b)
    {
        return std::hypot(a.x - b.x, a.z - b.z);
    }
} // namespace

void TutorialUi::Prepare(int width, int height)
{
    float scale = (std::min)(width / 1100.f, height / 720.f);
    canvas.Resize((std::max)(1, int(19 * scale)));
}

void TutorialUi::DrawWorldLabels()
{
    canvas.WorldText(-13.5f, 3.5f, 2, L"온기 빵집");
    canvas.WorldText(6, 2.5f, -11, L"동네 사진");
    canvas.WorldText(8, 2.6f, 5, L"작은 쉼터");
}

void TutorialUi::Draw(const Tutorial& game, int width, int height)
{
    float uiScale = (std::min)(width / 1100.f, height / 720.f);
    int target = game.Target();
    // Fixed logical UI coordinates are letterboxed within any window aspect ratio.
    float uiW = width / uiScale, uiH = height / uiScale;
    canvas.Begin(uiW, uiH);
    if (!game.FindPlayer())
    {
        canvas.Text(44, 53, L"레벨 종료 · [R] 다시 시작");
        return;
    }
    UiPanel(24, 24, 560, 122, .13f, .23f, .24f, .94f);
    canvas.Text(44, 53, L"해온동  /  늘 앉던 자리", .98f, .81f, .48f);
    canvas.Text(44, 87, game.Objective());
    std::wstring status = L"WASD 이동   E 상호작용   Tab 기록   Esc 일시정지";
    canvas.Text(44, 121, status, .73f, .83f, .78f);
    int seconds = int(game.elapsed);
    canvas.Text(uiW - 230, 48, L"늦은 오후  ·  초여름");
    canvas.Text(uiW - 230,
                77,
                L"플레이 " + std::to_wstring(seconds / 60) + L"분 " +
                    std::to_wstring(seconds % 60) + L"초");
    if (game.stage == 1)
        canvas.Text(44, 175, L"소지품: 아직 따뜻한 빵 봉투", .25f, .32f, .28f);
    if (game.stage < 5)
    {
        GroundPoint d = game.ObjectivePosition();
        float dx = d.x - game.Player().position.x, dz = d.z - game.Player().position.z;
        float sx = (dx - dz) * .7071f, sy = (dx + dz) * .4056f;
        float n = std::sqrt(sx * sx + sy * sy);
        float cx = uiW - 93, cy = 132;
        UiPanel(uiW - 250, 95, 226, 99, .13f, .23f, .24f, .88f);
        canvas.Text(uiW - 234, 124, L"다음 목적지");
        canvas.Text(
            uiW - 234, 163, std::to_wstring(int(Distance(game.Player().position, d))) + L" m");
        if (n > .01f)
        {
            sx /= n;
            sy /= n;
            glColor3f(.98f, .81f, .48f);
            glBegin(GL_TRIANGLES);
            glVertex2f(cx + sx * 23, cy + sy * 23);
            glVertex2f(cx - sx * 12 - sy * 9, cy - sy * 12 + sx * 9);
            glVertex2f(cx - sx * 12 + sy * 9, cy - sy * 12 - sx * 9);
            glEnd();
        }
    }
    if (target >= 0 && game.dialogue.empty() && !game.paused && !game.completion)
    {
        UiPanel(24, uiH - 83, 540, 54, .13f, .23f, .24f, .94f);
        canvas.Text(
            44, uiH - 49, L"[E] " + std::wstring(game.TargetName(target)) + L" · 대화 / 조사");
    }
    if (game.journal && game.dialogue.empty())
    {
        UiPanel(24, 207, 610, 170, .96f, .92f, .81f, .98f);
        canvas.Text(44, 242, L"오늘의 기록  /  식기 전에", .24f, .32f, .30f);
        canvas.Text(44, 279, game.Objective(), .24f, .32f, .30f);
        canvas.Text(44, 315, L"빵집 → 어르신 → 게시판 → 빈 벤치 → 쉼터 이웃", .24f, .32f, .30f);
        canvas.Text(44, 352, L"서두르지 않아도 괜찮다. 빵은 사라지지 않는다.", .24f, .32f, .30f);
    }
    if (!game.dialogue.empty())
    {
        UiPanel(24, uiH - 166, uiW - 48, 142, .12f, .21f, .23f, .98f);
        canvas.Text(46, uiH - 130, game.TargetName(game.speaker), .98f, .81f, .48f);
        canvas.Text(46, uiH - 90, game.dialogue[game.line]);
        canvas.Text(46,
                    uiH - 47,
                    L"[E] 계속  ·  " + std::to_wstring(game.line + 1) + L" / " +
                        std::to_wstring(game.dialogue.size()),
                    .72f,
                    .82f,
                    .78f);
        if (game.speaker == 8 && game.stage == 2)
        {
            // In-engine stylized neighborhood photograph, no external bitmap required.
            UiPanel(uiW / 2 - 175, 220, 350, 220, .96f, .91f, .77f);
            UiPanel(uiW / 2 - 157, 238, 314, 165, .65f, .77f, .70f);
            UiPanel(uiW / 2 - 100, 324, 200, 13, .47f, .33f, .23f);
            UiPanel(uiW / 2 - 30, 288, 26, 36, .56f, .49f, .72f);
            UiPanel(uiW / 2 - 26, 270, 18, 18, .93f, .75f, .58f);
            canvas.Text(uiW / 2 - 145, 428, L"늘 같은 자리에 앉아 있던 사람", .24f, .32f, .30f);
        }
    }
    if (game.paused || game.completion)
    {
        UiPanel(0, 0, uiW, uiH, .10f, .17f, .20f, .65f);
        UiPanel(uiW / 2 - 330, uiH / 2 - 110, 660, 235, .14f, .24f, .26f, .99f);
        canvas.Text(uiW / 2 - 300,
                    uiH / 2 - 65,
                    game.paused ? L"잠시 쉬어가기" : L"기억 기록 획득: 오늘 빵을 건넨 사람",
                    .98f,
                    .81f,
                    .48f);
        canvas.Text(uiW / 2 - 300,
                    uiH / 2 - 17,
                    game.paused ? L"시간과 주민들의 움직임이 멈췄습니다."
                                : L"당연했던 하루에, 기억하고 싶은 일이 생겼다.");
        canvas.Text(uiW / 2 - 300,
                    uiH / 2 + 35,
                    game.paused ? L"[Esc] 계속하기    [R] 처음부터 다시 시작"
                                : L"[E] 동네 둘러보기    [R] 다시 시작");
        canvas.Text(uiW / 2 - 300,
                    uiH / 2 + 82,
                    L"창 닫기로 종료 · 저장 기능이 없는 튜토리얼 프로토타입",
                    .72f,
                    .82f,
                    .78f);
    }
}
