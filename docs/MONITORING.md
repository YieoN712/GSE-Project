# 로컬 월드 모니터

게임 빌드·실행 후 별도 브라우저 창에서 http://127.0.0.1:8765/ 를 엽니다.
HTML은 빌드 시 실행 파일 옆 monitor/index.html로 복사됩니다. 직접 파일을 열지 말고 위 주소를 사용하세요.
추가 런타임은 필요 없습니다. --tutorial은 튜토리얼 주민, 기본 레벨 1은 동네 주민 8명과 외곽 몹을 제공합니다.
--no-monitor로 비활성화합니다.
8765 포트 사용 중이면 콘솔 오류를 남기고 게임만 계속 실행합니다.

## JSON 계약 · schemaVersion 1

- 읽기 전용 GET /api/snapshot. 브라우저가 약 0.5초마다 JSON을 요청합니다. WebSocket/외부 POST 서버는 아닙니다.
- 메인 스레드에서 상태를 복사해 통신 스레드에 전달합니다. 통신 스레드는 Actor에 접근하지 않습니다.
- sequence: 프로세스 내 증가 번호(재실행 시 초기화). timestamp: Unix 밀리초.
- scene, foreground, paused, simulationSeconds: 현재 레벨 및 실행 상태.
- npcs[]: id, townId, name, state, present, active, visible, position.
- mobs[]: 공통 Actor 필드와 state, health, maximumHealth, timer.
- ID는 정밀도 보존용 문자열. 위치는 부모 변환을 반영한 X/Y/Z 월드 좌표입니다.
  활성·표시는 부모 플래그, NPC present는 이야기상 부재도 반영합니다.
- counts: npcs, presentNpcs, mobs, aliveMobs. 사망 후 재생성 대기 몹은 등록 수에만 포함합니다.
- player: 플레이어 상태. 제거됐으면 null.
- towns[]: 여러 마을을 수용하는 배열. 현재 해온의 로드된 구역만 제공합니다.
  population은 현장 NPC 수이며 전체 도시·미로드 구역 인구가 아닙니다. 플레이어·몹 제외입니다.
- 레벨 1 atmosphere는 외곽 보고 여부를 반영한 분위기, rumors는 유통 중인 소문 배열입니다.
  heardRumors는 플레이어가 들은 소문 여부입니다(NeighborhoodState::Rumors 순서).
- tendencies.neighborsMet/outskirtsReports는 인사한 이웃 수와 전한 소식 수입니다.
  reputation.trust/label은 동네 신뢰와 호칭입니다. NPC는 role/greeted도 제공합니다.
  튜토리얼 사회 상태는 아직 null입니다. 빈 소문 배열은 소문 없음, null은 미연동을 뜻합니다.

127.0.0.1 전용이며 상태 변경 API는 없습니다. 브라우저 전환 시 기존 게임 정책대로
시뮬레이션은 멈추지만 상태 제공은 계속됩니다. 끊김·갱신 지연·수신 정지는 표시하고 마지막 데이터를 유지합니다.
게임 빌드·실행 확인은 사용자 수행입니다. 화면 로직 검사: node tests/MonitorDashboardTests.js
