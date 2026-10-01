# Editor 작업 — COMPUTER-WHEEL-SCROLL 컴퓨터 화면 스크롤 영역 마우스 휠 스크롤

- 작업 ID: `COMPUTER-WHEEL-SCROLL`
- 단계: Editor 작업
- 상태: 완료

(Editor 워커 전문을 마스터가 저장)

## 결론

- 남은 작업은 Unreal 정본 문서 갱신 3건이었고, 모두 반영했습니다. 입력 `PROMPT_UNREAL.md`가 Content 변경 없음을 선언했기 때문입니다.
- asset·Level 수정, 저장, Compile은 없습니다. 이 단계에서 완료한 것은 문서 갱신뿐입니다.
- 실제 동작 수용은 사용자 PIE에서 확인해야 합니다.

## 입력과 기준선

- 입력: `PROMPT_UNREAL.md`(단계 구현, 상태 완료), `PROMPT_IMPLEMENTATION.md` 9절·12절, `REPORT_UNREAL_DISCOVERY.md`(단계 Editor 사전 조사, 상태 완료).
- 브랜치 `work/COMPUTER-WHEEL-SCROLL`, HEAD `c1f00d9`, 리뷰 승인 커밋 `2a52b8b`.
- 시작 시 `git status`: clean.
- Content·Config 변경 없음: `git diff 2c24374 HEAD -- Content Config` 결과가 비어 있습니다. 따라서 사전 조사의 사실이 현재 Content와 같습니다.
- `FEEDBACK_BACKLOG.md`: 일반화 항목이 없어 적용할 Active 항목이 없습니다.

## 실행 경로

- Unreal Editor를 띄우지 않았고 MCP·Python도 쓰지 않았습니다. 근거는 다음 두 가지입니다.
  - 사전 조사 이후 Content 변경이 없습니다.
  - 기록할 사실이 모두 `REPORT_UNREAL_DISCOVERY.md` 1~5절에 있습니다.
- 자동화 이름과 로그 경로는 Source에서 확인했습니다(`Source/BathhouseSim/Private/Tests/ComputerWheelScrollAutomationTests.cpp`).
  - `ScreenWheelContentContract` 테스트 등록: 638행
  - 영역별 `WheelScrollMultiplier` Info 로그: 690행
  - IA·mapping에 modifier·trigger가 0개인지 검사: 700~714행
- Compile, Validation, Save, 재로드: 해당 없음(asset 변경 없음).

## 변경 대상 — `.md/Unreal/InteractionUISystem.md`

1. "컴퓨터 연결" 절
   - `BP_BathhouseComputer` `ScreenWidget.WidgetClass`를 `/Game/Bathhouse/UI/WBP_ComputerScreenRoot.WBP_ComputerScreenRoot_C`로 정정했습니다. 낡은 `WBP_BathWaterManagementScreen_C` 기술을 대체합니다. CDO와 DefaultMap instance가 같고 override가 없다는 점도 적었습니다.
   - root hierarchy를 한 줄 추가했습니다: `RootSize`, `TabBar`, `ScreenSwitcher`(index 0 `ManagementScale` > `ManagementScreen`, index 1 `ShopScreen`). 조상 widget이 hit test를 막지 않는다는 점도 적었습니다.
2. 새 소절 "컴퓨터 화면 스크롤 영역"
   - ScrollBox 4개의 WBP 경로와 내용을 표로 정리했습니다. 모두 세로이고 중첩이 없으며, 다른 WBP에는 ScrollBox가 없습니다.
   - 휠 계약 설정: `ConsumeMouseWheel=WhenScrollingPossible`, `AnimateWheelScrolling=false`. Architecture `ComputerSystem.md` `Screen Wheel Scroll` 절로 링크했습니다.
   - 이동량은 원본 위치만 적었습니다.
     - 원본: 각 ScrollBox `WheelScrollMultiplier`, 엔진 cvar `Slate.GlobalScrollAmount`.
     - 계산식: 한 칸 = cvar × multiplier, 단위는 ScrollBox local unit.
     - 현재 값을 보는 법: Editor에서 직접 읽거나, 자동화 `ScreenWheelContentContract`의 Info 로그를 보거나, Editor 콘솔에서 확인합니다.
   - `DetailScroll`은 `ManagementScale` 안이라 화면 px로는 더 작게 움직입니다.
   - 이동량 조정은 해당 ScrollBox의 `WheelScrollMultiplier`만 바꿉니다. WBP에 휠 graph, `OnMouseWheel` override, ScrollBox 직접 조작을 만들지 않습니다.
   - 슬라이더 위 휠은 `DetailScroll`로 올라갑니다.
3. 새 소절 "공용 휠 입력"
   - `IMC_FirstPerson`의 `MouseWheelAxis → IA_PlacementRotate`(Axis1D, modifier·trigger 없음)는 유일한 휠 mapping이자 공용 휠 입력입니다. 컴퓨터를 쓰지 않을 때는 배치 회전, 사용 중에는 화면 스크롤로 쓰이며 분기는 C++가 합니다.
   - `IMC_Default`·`IMC_MouseLook`에는 mapping이 없습니다.
   - 아키텍처 설계 없이 modifier·trigger나 두 번째 휠 mapping을 추가하지 않습니다. 자동화가 이 전제를 load 검증합니다.

- 이 문서의 기존 기록(Bath Water 관리 화면, 서비스, 세신 HUD)은 바꾸지 않았습니다.
- 문서는 65줄로 분리 검토 기준(300줄)보다 짧습니다. `0_UNREAL.md` 라우팅("Input Mapping, Widget hierarchy")도 그대로 맞습니다.

## 남은 항목과 큐

- `USER_UNREAL.md`: 추가하거나 제거한 항목이 없습니다.
- 화면 작업 후보: 없습니다.
- 미측정(사전 조사와 같음): `ManagementScale` 배율과 탭 바 높이. 문서에는 "화면 px로는 더 작다"는 정성 기술만 적었습니다.
- 작업 트리에 내가 만들지 않은 미추적 `.md/Work/EXPANSION-PURCHASE/`가 있습니다. 건드리지 않았습니다.

## 사용자 PIE 관찰 항목 (`PROMPT_IMPLEMENTATION.md` 12절 1~17)

선행 조건: 이 작업의 build를 사용자 Editor에 반영합니다. 모든 시나리오는 DefaultMap 컴퓨터에 빈손으로 E를 눌러 진입하고, 시점 전환이 끝나 커서가 보이는 상태에서 시작합니다. 상점 상품은 20종이라 목록이 넘칩니다.

1. CWS-003·001·002
   - 진입 직후 아무것도 클릭하지 않고, 커서를 상품 카드 사이 빈 곳에 둔 채 휠을 아래로 굴리면 상품 목록만 스크롤됩니다.
   - 위로 굴리면 돌아갑니다.
   - 장바구니·주문 패널, 잔액, 탭, 시점·캐릭터는 그대로입니다.
2. CWS-004: 담기 버튼·이름·가격 위에서 휠을 굴리면 스크롤만 되고 장바구니 수량·잔액은 그대로입니다.
3. CWS-005: 맨 끝에서 같은 방향으로 굴리면 튕기지 않고 멈춥니다. 반대 방향 한 칸은 즉시 움직입니다.
4. CWS-006: 장바구니에 상품이 1종뿐일 때 장바구니 위에서 휠을 굴려도 아무것도 움직이지 않습니다.
5. CWS-007: 장바구니가 넘칠 만큼 담은 뒤 그 위에서 휠을 굴리면 장바구니만 움직입니다.
6. CWS-008: 대기 주문이 넘칠 때 주문 목록 위에서 휠을 굴리면 주문 목록만 움직이고 남은 시간은 계속 갱신됩니다.
7. CWS-009: 관리 탭에서 욕탕을 선택하고 상세 문구 위에서 휠을 굴리면 상세 영역만 스크롤되고 지도·용량 요약은 그대로입니다.
8. CWS-010: 순환도·목표 수온 슬라이더 위에서 휠을 굴리면 상세 영역이 스크롤되거나 끝이면 멈춥니다. 슬라이더 값·욕탕 설정·용량 표시는 그대로입니다.
9. CWS-011: 탭 버튼, 주문 버튼, 잔액 문구, 지도 위에서 휠을 굴려도 변화가 없고 선택·탭도 그대로입니다.
10. CWS-012: 커서가 모니터 바깥 월드에 있을 때 휠을 굴려도 화면·시점·캐릭터에 변화가 없습니다.
11. CWS-013: 진입 전환 중이나 E로 나가는 전환 중에 휠을 굴려도 스크롤 위치가 그대로이고 진입·이탈은 정상입니다.
12. CWS-014: 상품 목록을 중간까지 내린 뒤 E로 나갔다가 재진입해도, 관리 탭에 다녀와도 같은 위치입니다.
13. CWS-015: 휠로 스크롤한 직후 커서 아래 카드의 담기 버튼을 누르면 그 상품이 담깁니다.
14. CWS-016: 휠을 몇 번 쓴 뒤 E를 누르면 휠을 안 썼을 때와 같은 결과입니다(ESC가 아닌 E로 판정).
15. CWS-017: 컴퓨터 밖에서 설비 아이템의 배치 프리뷰가 보일 때 휠을 굴리면 기존처럼 회전합니다.
16. CWS-018: 빈손으로 모니터를 보며 휠을 굴려도 아무 일 없습니다.
17. CWS-020: 컴퓨터 사용 중 휠을 쓴 뒤 이동키·Space·Shift·F·G·Q·LCtrl을 눌러도 아무 일 없습니다.

- 이동량 감각은 수용 실패 사유가 아닙니다. 조정이 필요하면 Editor 단계가 해당 ScrollBox의 `WheelScrollMultiplier`만 바꿉니다.
- 실패하면 시나리오 ID, 커서 위치, 휠 방향·칸 수, 움직인 영역을 기록합니다. 휠이 전혀 반응하지 않으면 `showdebug enhancedinput`에서 `IA_PlacementRotate` 값이 보이는지도 함께 적습니다.

## 종료 상태

- Editor 프로세스를 시작하지 않았습니다.
- `git status`: ` M .md/Unreal/InteractionUISystem.md`, `?? .md/Work/EXPANSION-PURCHASE/`(이 작업 범위 밖).
- 커밋하지 않았습니다.
