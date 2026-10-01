# 코드 리뷰 프롬프트 — 빈 박스 빼기 대상 선택 통일

- 작업 ID: `EMPTY-BOX-TAKE-TARGET`
- 단계: 구현
- 상태: 완료

## 1. 기능 계약과 단계

- 계약: `PROMPT_ARCHITECTURE.md`(EBT-001~016, 승인), 설계 `PROMPT_IMPLEMENTATION.md`, 정본 `.md/Architecture/ServiceFacilityDisplaySystem.md` "빈 박스 빼기 대상 선택". 전체 경로 직접 구현(수직 구현 없음).
- 실행 위치 덮어쓰기: 마스터 결정으로 worktree 대신 메인 트리 브랜치 `work/EMPTY-BOX-TAKE-TARGET`(시작 커밋 `2552f61`)에서 구현·빌드했다.

## 2. 시나리오 ID별 코드·테스트 연결

테스트 파일은 `Private/Tests/ServiceFacilityEmptyBoxTakeAutomationTests.cpp`(신규, 이름 접두사 `BathhouseSim.Service.Facility.EmptyBoxTake.`)와 수정한 `ServiceFacilityDisplayAutomationTests.cpp`다.

| ID | 코드 | 테스트 |
|---|---|---|
| EBT-001, 004, 010 후보 필터 | `FDisplayFacilityTakeSelection::SelectSpaceIndex` | `SelectionRules`; `VanityTraceQueryCueExecute`(001, 004), `ShowerAndFilledBoxUnchanged`(010); `GroupsAndConsumption`(004 query) |
| EBT-002, 003 각 거리·보이는 물품 | `GetAimAngleRadians`, `UDisplaySpaceComponent::GetVisibleItemWorldLocations` | `SelectionRules`(깊이 무관, 묶음 최근접 물품), `VanityTraceQueryCueExecute`(002, 003: 실제 camera trace) |
| EBT-005, 006 대체 묶음·이유 | `SelectSpaceIndex` 대체 규칙, 기존 `EvaluateTake` | `SelectionRules`, `VanityTraceQueryCueExecute`(실패 보고 1회, 무변화) |
| EBT-007 반복 | 변경 없음(기존 held-use) | `RouterRepeatAndKeyGuard`(수정 없이 통과), 연속 빼기 시간 검증은 사용자 PIE |
| EBT-008, 011 채운 박스 | 변경 없음(`SelectSpace` 박스 종류 분기) | `ShowerAndFilledBoxUnchanged`(넣기 프리뷰·강조·실행) |
| EBT-012 소모 전환 | 선택을 매 query 재계산 | `VanityTraceQueryCueExecute`(`ConsumeOneUse` 뒤 `RefreshInteractionQuery`) |
| EBT-013 강조 옵션 끔 | 변경 없음 | `VanityTraceQueryCueExecute` |
| EBT-014 포함 관계 | Content(사전 조사 완료) | 자동화 없음, 사용자 PIE |
| EBT-015 냉장고, EBT-016 집게 | 변경 없음 | 필터의 기존 Display·냉장고·`Cleaning.Litter.Tongs` 회귀 |
| EBT-009 | 변경 없음 | 사용자 PIE |

## 3. 변경 파일과 구현 요약

- 신규 `Private/Service/DisplayFacilityTakeSelection.h/.cpp`: private 순수 helper. 후보 = `TakeableCount > 0`이고 보이는 물품 있음, 거리 = 물품별 조준 각의 최솟값, 정확 동거리는 낮은 SpaceIndex, 후보 없으면 Count > 0 중 최저 index 아니면 전체 최저 index.
- `Public/Service/DisplaySpaceComponent.h`, `Private/Service/DisplaySpaceComponent.cpp`: `GetVisibleItemWorldLocations` 추가(C++ 전용). `GetSlotWorldRelativeTransform`을 재사용하고 `StockVisual`이 relative identity라 component transform만 곱한다. bounds 중심 보정 포함.
- `Public/Service/DisplayFacilityTargetComponent.h`, `Private/Service/DisplayFacilityTargetComponent.cpp`: static `SelectClosestSpace` 삭제, `SelectSpace` 빈 박스 분기를 helper 입력 조립으로 교체. 마지막 반환 루프에 `FacilityRouted` 조건을 추가했다(설계에 없던 방어 조건, 대체 index가 항상 routed 묶음 중에서 나오므로 동작 차이 없음).
- 테스트: `ServiceFacilityAutomationTestSupport.h`에 `AimAtPoint` 추가. `ServiceFacilityDisplayAutomationTests.cpp`는 설계 6.2대로 `GroupsAndConsumption`의 VANI-012 블록을 EBT-004 결과(key 0·가능·이유 없음, execute 미호출)로, `RouterRepeatAndKeyGuard`는 `SelectClosestSpace` 단언 두 개 삭제만 했다.
- 조정값 원본 원칙: 새 수치·상수 없음. 테스트 허용오차(1e-9, 1e-3)와 EBT-003 오프셋 2cm는 fixture 기하 검증용이며 동작 조정값이 아니다.
- `0_ARCHITECTURE.md` 상태 줄만 갱신했다(Source 반영). `ServiceFacilityDisplaySystem.md`는 설계 절과 구현이 같아 수정하지 않았다.

## 4. 클래스 크기·책임 변화

- `DisplaySpaceComponent.cpp` 456 → 474줄(accessor 하나), router cpp 205 → 187줄. 새 상태·UPROPERTY·UFUNCTION·delegate·Tick 없음. 선택 규칙은 helper에만 있다.

## 5. Blueprint·API·Core Redirect 영향

- reflected 변경 없음. 삭제·추가 API는 C++ 전용이고 사용처는 테스트뿐이었다. Core Redirect 불필요. Content·Config 변경 없음.

## 6. 빌드와 검증

- `git diff --check` 이상 없음(CRLF 경고만). Editor 미실행 확인 후 빌드.
- UE 5.8 `BathhouseSimEditor Win64 Development` 빌드 성공. 로그 `Saved/ebt_build.log`.
- Automation 필터 `BathhouseSim.Service+BathhouseSim.Interaction+BathhouseSim.Cleaning.Litter.Tongs+BathhouseSim.Towel.Display`: 59개 완료, 실패 0, EXIT CODE 0. 로그 `Saved/ebt_auto.log`, 리포트 `Saved/Automation/Reports/2026-10-01/EMPTY-BOX-TAKE-TARGET`. 신규 `SelectionRules`, `VanityTraceQueryCueExecute`, `ShowerAndFilledBoxUnchanged` 모두 Success.
- 빌드 시점 Source 식별값: HEAD `2552f6175dc7fc762a7f6246f2806e2a1c434908`, `git diff HEAD -- Source Config` SHA-256 `80A279FB3EBCD2A63D6A9FA64578BD5F244AE70F574645094A0631F39B0F8405`. 이 diff는 추적 파일 변경만 포함한다. 신규 untracked 3파일(`DisplayFacilityTakeSelection.h/.cpp`, `ServiceFacilityEmptyBoxTakeAutomationTests.cpp`)은 해시에 빠져 있으니 커밋 뒤 HEAD로 대조해야 한다.

## 7. 리뷰 중점과 미검증

- 4.1 식·후보 조건·정확 동거리·대체 순서가 정본과 같은지, `GetVisibleItemWorldLocations`의 개수·transform이 `RefreshStockVisual`과 같은지.
- query·execute·focus가 같은 선택과 key를 쓰고 새 상태가 없는지. 채운 박스·냉장고·집게 경로에 diff가 없는지.
- 기존 테스트 수정이 6.2 범위에 머물렀는지.
- 미검증: 실제 BP asset(`BP_Vanity`, `BP_Shower`)의 진열 보정과 router Box에서의 체감 선택, EBT-007·009 연속 빼기 시간 경과, EBT-014. 모두 사용자 PIE 항목(`PROMPT_UNREAL.md`).
