# PROMPT_UNREAL — COMPUTER-WHEEL-SCROLL

- 작업 ID: `COMPUTER-WHEEL-SCROLL`
- 단계: 구현
- 상태: 완료

## 상태
**Content 변경 없음.** asset 생성·수정·저장·Compile 없음. `git status`에 `Content/`, `Config/` 변경이 없다. Editor 작업 단계는 아래 문서 갱신만 남으며, 마스터가 생략하고 마스터가 직접 문서를 갱신해도 된다.

## 갱신할 Unreal 정본(문서만, `PROMPT_IMPLEMENTATION.md` 9절)
1. `.md/Unreal/InteractionUISystem.md` "컴퓨터 연결": `BP_BathhouseComputer` `ScreenWidget.WidgetClass`를 `/Game/Bathhouse/UI/WBP_ComputerScreenRoot.WBP_ComputerScreenRoot_C`로 고친다(근거 `REPORT_UNREAL_DISCOVERY.md`).
2. 같은 문서에 스크롤 영역 authoring 기록: `WBP_ShopScreen`의 `ProductScroll`·`CartScroll`·`OrderScroll`, `WBP_BathWaterDetail`의 `DetailScroll`. 세로, 중첩 없음, `ConsumeMouseWheel=WhenScrollingPossible`, `AnimateWheelScrolling=false`, `WheelScrollMultiplier`는 값을 복제하지 않고 각 ScrollBox의 `WheelScrollMultiplier` 프로퍼티(WBP)를 원본으로 참조하며 현재 값은 Editor 단계가 읽어 기록한다(자동화 C는 finite·양수만 확인하고 값은 `AddInfo`로 남긴다). 한 칸 = 엔진 cvar `Slate.GlobalScrollAmount` × 그 ScrollBox의 `WheelScrollMultiplier`, ScrollBox local unit. `DetailScroll`은 `ManagementScale` 안이라 화면 px로는 더 작다.
3. 같은 문서 입력 절: `IMC_FirstPerson`의 `MouseWheelAxis → IA_PlacementRotate`(Axis1D, modifier·trigger 없음)는 공용 휠 intent다. 컴퓨터 미사용 시 배치 회전, Active 시 화면 스크롤. 아키텍처 설계 없이 modifier·trigger나 두 번째 휠 mapping을 추가하지 않는다(자동화 C가 지킴).

## 사용자 PIE 관찰 항목
`PROMPT_IMPLEMENTATION.md` 12절 1~17번 그대로(CWS-001~020). 선행: 이 브랜치 build를 사용자 Editor에 반영. 핵심:
- CWS-003·001·002: 진입 직후 클릭 없이 상품 카드 사이 빈 곳에서 휠 → 상품 목록만 스크롤, 반대로 돌아감.
- CWS-004·015: 담기 버튼 위 휠은 스크롤만, 스크롤 직후 담기 클릭은 정상.
- CWS-005~008: 끝에서 정지·즉시 반대 이동, 장바구니·주문 목록은 넘칠 때만 각자 스크롤.
- CWS-009~011: 상세 영역 스크롤, 슬라이더 위 휠은 값 불변, 영역 밖 무반응.
- CWS-012·013: 화면 밖 월드·전환 중 휠 무반응.
- CWS-014·016: 재진입 후 스크롤 위치 유지, 휠 후 E 이탈 정상.
- CWS-017·018·020: 컴퓨터 밖 배치 회전 유지, 빈손 모니터 휠 무반응, 사용 중 이동·행동 키 차단 유지.
- 휠이 전혀 반응하지 않으면 `showdebug enhancedinput`으로 `IA_PlacementRotate` 값을 함께 기록. 이동량 감각은 수용 실패가 아니며 필요하면 해당 ScrollBox `WheelScrollMultiplier`만 Editor 단계에서 조정한다.

## Blueprint에서 구현하면 안 되는 것
휠 분기·주입·hover gate는 C++(`AFirstPersonCharacter::MouseWheelInput`, `UPlayerComputerUseComponent`)에 있다. WBP에 휠 이벤트 graph, `OnMouseWheel` override, ScrollBox 직접 조작을 만들지 않는다.
