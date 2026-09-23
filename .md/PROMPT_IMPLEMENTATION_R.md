# 구현 재작업 프롬프트 — Bath Water Operations deficit transaction과 관리 UI 계약

## 재검토 결론

`UBathWaterConditionComponent`, `UBathWaterOperationsSubsystem`, 독립 utility Actor, Customer actual-bather identity와 native Widget 계층이라는 책임 분리는 유지한다. UE 5.8 build와 현재 focused automation은 통과했지만, deficit 상태의 목표 수온 request가 보존 설정을 반대로 낮추는 transaction 오류, 실제 footprint와 다른 지도 투영, 승인된 관리 화면 정보 계약 누락이 있어 Unreal Editor 단계로 넘기지 않는다.

Source, `PROMPT_REVIEW.md`, `PROMPT_UNREAL.md`를 아래 결과와 일치하게 수정하고 다시 코드 리뷰를 요청한다. `Content/`, `Config/`, Level, Blueprint/WBP와 `.md/Unreal/*`은 이 재작업에서 수정하지 않는다.

## P1 — deficit 상태의 목표 수온 증가 request가 기존 설정을 낮추지 않게 수정

대상:

- `Source/BathhouseSim/Private/Facility/BathWaterOperationsSubsystem.cpp`
- `Source/BathhouseSim/Private/Tests/BathWaterOperationsAutomationTests.cpp`

현재 `RequestTargetTemperature()`는 현재 heating/cooling demand를 이미 충족하지 못하는 world에서 같은 방향으로 더 멀리 움직이는 request가 들어오면 `Available`로 계산한 절대 최대값을 그대로 commit한다. 예를 들어 목표 40°C, 가열 요구량 100, 예기치 않은 provider 소실로 total 0인 상태에서 50°C를 요청하면 feasible target이 ambient 20°C가 되어 보존된 40°C 설정까지 반환한다. circulation request가 현재값을 하한으로 보존하는 것과도 불일치한다.

필수 수정:

1. 현재 target과 requested target이 ambient의 같은 쪽이고 요청이 ambient에서 더 멀어지는 방향이면 capacity clamp가 현재 승인값을 넘어 반대 방향으로 낮추지 못하게 한다.
2. heating 쪽 증가 request는 최소 현재 target, cooling 쪽 증가 request는 최대 현재 target을 보존한다.
3. 현재 world가 deficit이어도 ambient 쪽으로 이동해 demand를 낮추는 request는 계속 허용한다.
4. ambient를 횡단하는 request는 기존 종류 demand를 0으로 반환한 뒤 반대 종류의 남은 용량까지만 이동하는 기존 계약을 유지한다.
5. 같은 방향 증가 request가 capacity 때문에 현재 승인값을 넘을 수 없으면 `bSucceeded=true`, `bWasLimited=true`, `CommittedValue=CurrentValue`, 정확한 `LimitedKind`를 반환한다. `RequiredAdditionalPoints`는 `max(RequestedCandidateAggregateUsed - TotalCapacity, 0)`으로 정의한다.
6. `CommittedValue`가 현재값과 같으면 condition setter, `DataRevision` 증가, registry mutation publication과 operations delegate broadcast를 수행하지 않는다. 동일 값을 다시 요청한 일반 no-op에도 같은 publication 규칙을 적용한다.
7. heating과 cooling 양쪽에 대해 current value 보존, 감소 허용, ambient 횡단, shortage 계산과 no-op publication을 native automation으로 검증한다.

## P1 — 지도 footprint 위치·크기·Yaw를 한 번의 투영으로 계산

대상:

- `Source/BathhouseSim/Private/UI/BathWaterMapWidget.cpp`
- 필요 시 `Source/BathhouseSim/Public/UI/BathWaterMapWidget.h`
- `Source/BathhouseSim/Private/Tests/BathWaterOperationsAutomationTests.cpp`

현재 `LayoutTile()`은 회전된 footprint의 screen-axis AABB 크기를 먼저 계산한 뒤 같은 tile에 render angle을 다시 적용한다. yaw 0인 300×240cm Bath도 이미 `width=240`, `height=300`으로 계산된 상태에서 -90도를 다시 적용하므로 X/Y 비율이 뒤집힌다. 임의 yaw에서는 AABB와 render rotation이 중복 적용된다. 또한 `TransformVectorNoScale()`과 unscaled extent 조합이 footprint component/Actor의 실제 scale을 tile size에 반영하지 않는다.

필수 수정:

1. 지도는 Zone의 실제 world X:Y 비율을 보존하는 중앙 content rect를 사용한다. `PixelsPerWorldUnit`은 `min(CanvasWidth / ZoneWorldYLength, CanvasHeight / ZoneWorldXLength)`의 uniform scale 하나로 계산하고 남는 Canvas 영역은 중앙 letterbox로 둔다.
2. footprint 네 world corner는 `FootprintTransform.TransformPosition(LocalCorner)`로 계산해 Actor/root/component scale을 모두 포함한다.
3. Zone world 중심과 회전축을 기준으로 네 corner를 같은 projection helper로 변환한다. world `+X=screen up`, world `+Y=screen right` 계약을 유지한다.
4. tile position은 projected corner 네 개의 평균, width/height와 render angle은 투영된 인접 edge의 길이와 방향에서 도출한다. screen-axis AABB를 계산한 뒤 render angle을 다시 적용하지 않는다.
5. 네 world corner 중 하나라도 Zone world bounds 밖이면 해당 Bath를 지도에서 제외한다.
6. yaw 0/90/임의 각도, 300×240cm 같은 비정방형 footprint, Actor/component non-unit scale과 Zone 밖 corner를 실제 widget/projection helper automation으로 검증한다. Canvas와 Zone aspect가 달라도 yaw와 footprint 비율이 유지되고 남는 영역만 letterbox인지 확인한다.

## P1 — 승인된 관리 화면 데이터와 상태 계약을 native Widget에 완성

대상:

- `Source/BathhouseSim/Public|Private/UI/BathWaterCapacitySummaryWidget.*`
- `Source/BathhouseSim/Public|Private/UI/BathWaterBathTileWidget.*`
- `Source/BathhouseSim/Public|Private/UI/BathWaterDetailWidget.*`
- `Source/BathhouseSim/Public|Private/UI/BathWaterManagementScreenWidget.*`
- 필요 시 `Source/BathhouseSim/Public/Facility/BathWaterOperationsTypes.h`
- 필요 시 `Source/BathhouseSim/Private/Facility/BathWaterOperationsSubsystem.cpp`
- `.md/PROMPT_UNREAL.md`
- `Source/BathhouseSim/Private/Tests/BathWaterOperationsAutomationTests.cpp`

현재 capacity summary의 native 계약은 TextBlock 세 개뿐이라 승인 기능 계약의 종류별 진행 막대와 정상/가득 참/deficit 상태 표현을 데이터에 맞게 구동할 수 없다. detail에는 선택 Bath 이름 binding이 없고, tile/detail은 실제 용량 부족 종류와 이유를 표현할 데이터가 없다. `EBathWaterThermalStatus::SuspendedByCapacity`만으로는 circulation deficit으로 정화가 중지됐지만 target이 ambient인 Bath 같은 상태를 구분할 수 없다. `FeedbackText`도 이전 Bath의 제한 결과가 다른 Bath 선택 뒤 남을 수 있다.

필수 수정:

1. 순환·가열·냉각의 used/total/deficit 진행 막대와 상태를 C++이 적용할 최소 native 계약을 추가한다. WBP는 layout/style만 소유한다.
2. detail에 선택 Bath 식별 이름을 제공한다.
3. tile/detail이 normal, circulation deficit, heating deficit와 cooling deficit을 domain snapshot 결과로 구분하게 한다. capacity 상태는 상호 배타적인 단일 enum으로 합치지 않는다. 한 Bath가 여러 deficit을 동시에 가질 수 있어야 한다.
   - `FBathWaterBathSnapshot`은 최소한 `bCirculationCapacityDeficit`, `bHeatingCapacityDeficit`, `bCoolingCapacityDeficit`에 해당하는 독립 상태를 제공한다.
   - subsystem은 해당 Bath의 kind별 demand가 0보다 크고 world aggregate가 그 kind에서 deficit일 때만 해당 상태를 설정한다.
   - `EBathWaterThermalStatus`는 실제 수온 운동 상태로 유지하고 capacity 상태와 합치지 않는다.
   - tile/detail은 여러 deficit 이유를 동시에 표시할 수 있고 UI에서 global capacity 규칙을 재계산하지 않는다.
4. detail에 현재 Bath의 용량 부족 종류와 부족 이유를 지속 표시하고, slider 제한 결과에는 limited kind와 부족 포인트를 표시한다.
5. selection/context 변경, target 제거와 deficit 해소 때 stale `FeedbackText`가 남지 않게 명시적으로 정리한다.
6. 온도 표시는 승인 계약의 소수점 한 자리와 `°C`를 사용한다.
7. 빈 지도 상태, 선택 해제 상태와 선택 표시를 C++ 상태에 따라 구동할 수 있게 한다.
8. 추가되는 exact `BindWidget` 이름·타입과 optional presentation hook을 `.md/PROMPT_UNREAL.md`에 실제 Source와 동일하게 갱신한다.
9. automation은 reflected property 존재만 확인하지 말고 snapshot 적용, deficit 상태, selection 전환과 slider limited feedback을 실행해 검증한다.

Capacity summary 상태와 progress 계산은 다음으로 통일한다.

- `Deficit`: `DeficitPoints > epsilon`
- `Full`: deficit이 아니고 `TotalPoints > 0`이며 `UsedPoints >= TotalPoints - epsilon`
- `Normal`: 그 외
- progress는 `TotalPoints > 0 ? clamp(UsedPoints / TotalPoints, 0, 1) : 0`
- ProgressBar가 1로 clamp되더라도 deficit text와 상태 표현을 숨기지 않는다.

## P2 — UI refresh와 registry revision 계약을 실제 구현과 맞춤

대상:

- `Source/BathhouseSim/Public|Private/UI/BathWaterManagementScreenWidget.*`
- `Source/BathhouseSim/Public|Private/UI/BathWaterMapWidget.*`
- `Source/BathhouseSim/Public|Private/UI/BathWaterBathTileWidget.*`
- `Source/BathhouseSim/Public|Private/UI/BathWaterDetailWidget.*`
- `Source/BathhouseSim/Public|Private/UI/BathWaterCapacitySummaryWidget.*`
- `Source/BathhouseSim/Public|Private/Facility/BathWaterOperationsSubsystem.*`
- `Source/BathhouseSim/Private/Tests/BathWaterOperationsAutomationTests.cpp`

현재 root는 `bSnapshotDirty`를 사용하지 않고 매 frame 전체 snapshot을 모든 child에 다시 적용한다. TextBlock, Slider, tile layout과 render transform도 값 비교 없이 반복 갱신된다. provider register/unregister도 bath topology revision을 올려 utility 변화만으로 map tile 전체를 재생성한다. invalid weak entry prune는 topology/data revision과 notification을 남기지 않아 직접 component 소실 시 stale tile을 제거하지 못할 수 있다.

필수 수정:

1. polling은 허용하되 child별 presentation cache를 비교해 실제 변경값만 UMG에 적용한다.
2. Bath set/Zone geometry가 바뀔 때만 map tile set을 rebuild하고 utility capacity 변화만으로 tile을 재생성하지 않는다.
3. 연속 수위·수온·오염도는 tile/detail의 해당 값만 갱신한다.
4. `bSnapshotDirty`와 revision을 실제로 사용하거나 불필요한 상태를 제거해 단일 refresh 정책으로 정리한다.
5. invalid provider/bath weak entry 정리가 총용량·Bath topology와 revision에 반영되게 하고, callback 재진입에서도 mutation publication이 유실되지 않게 한다.
6. 동일 snapshot 반복 적용에서 rebuild/text/slider update가 증가하지 않고 provider-only 변화가 bath tile을 교체하지 않는 automation을 추가한다.

## P2 — utility/bath placement·recovery 통합 자동화 보강

대상:

- `Source/BathhouseSim/Private/Tests/BathWaterOperationsAutomationTests.cpp`
- 필요한 기존 test probe/helper
- 실제 결함이 확인되면 해당 Facility/Placement Source

현재 신규 tests는 capacity component와 payload 값, reflected Widget property 존재만 확인한다. `ABathWaterUtilityFacilityActor` 자체는 테스트에서 사용되지 않으며 staged register/unregister, recovery 거부, rollback, final publication, typed payload import 실패와 computer/map 동작도 실행하지 않는다.

전체 suite의 기존 실패 중 `Placement.ActorReplacementTransaction`과 `Placement.ActorReplacementFailureAtomicity`는 신규 utility와 Bath가 사용하는 Actor replacement transaction과 직접 관련된다. 반면 `Placement.SettingsZoneLeaseAndCompatibility`의 grid 10/20cm와 `GridVisual` material 실패는 이번 Bath Water transaction과 직접 관계가 없으며, 그 테스트의 Definition runtime/data validation 계약만 신규 utility Definition과 관련된다.

필수 검증:

1. utility pre-placed register와 duplicate no-op, 정상 placement commit 전 미등록/commit 후 1회 등록을 검증한다.
2. demand를 깨는 recovery가 hold 시작 전에 거부되고 actor/provider/item이 불변인지 검증한다.
3. recovery silent stage, rollback, success publication과 unexpected EndPlay deficit/resume을 검증한다.
4. typed payload capacity round trip뿐 아니라 mismatched kind, invalid number와 Definition/class mismatch import가 item과 staged Actor를 보존하는지 검증한다.
5. Bath recovery hold/cancel/stage/rollback에서 condition state, demand와 registry identity가 정확히 한 번 복원되는지 검증한다.
6. 두 ActorReplacement 실패의 fixture를 현재 Definition 계약에 맞게 수정하거나, 최소한 같은 success/failure/rollback transaction을 실제 utility/bath actor로 통과시키는 독립 focused integration test를 추가한다. 유효 계약을 약화하거나 실패를 expected 처리하지 않는다.
7. `SettingsZoneLeaseAndCompatibility`의 grid/material 실패는 기존 Placement 문제로 분리 보고한다. 이번 재작업 완료 조건으로 억지로 수정하지 않되, 신규 utility Definition의 runtime/data validation은 독립 검증한다.

## 문서 정합성

대상:

- `.md/0_ARCHITECTURE.md`
- `.md/Architecture/UISystem.md`
- `.md/PROMPT_REVIEW.md`
- `.md/PROMPT_UNREAL.md`

현재 새 Operations/Management UI 문서는 Source 구현 완료로 표시하지만 상위 architecture map과 `UISystem.md`는 아직 Source 미구현으로 남아 있다. 재작업 뒤 실제 구현 상태, 정확한 Widget 계약과 검증 결과로 동기화한다. 빌드가 up-to-date였다는 사실과 실제 실행한 automation 결과를 구분하고, 시나리오 범위 나열을 테스트 통과 증거로 대신하지 않는다.

## 보존할 계약

- `UBathWaterStateComponent` 물 양 정본과 accepted flow sample
- `UBathWaterConditionComponent` 수온·오염도·weak active-bather owner
- `UBathWaterOperationsSubsystem` world aggregate/request owner
- utility를 customer Facility registry에 넣지 않는 독립 Actor 경계
- Customer actual segment begin 실패 시 flag 미commit과 공통 end cleanup
- sample computer 호환, `PrimaryUseAction`과 `Computer > Placement > Equipment` 우선순위
- 기존 reflected 이름·enum ordinal; Core Redirect 미추가
- Content/Config/StateTree/Blueprint/WBP 미수정

## 재검증

- `git diff --check`
- UE 5.8 `BathhouseSimEditor Win64 Development` 실제 compile 또는 up-to-date 여부를 정확히 보고
- `BathhouseSim.BathWater` 전부 성공
- `BathhouseSim.Customer` 전부 성공
- utility/bath transaction과 management UI focused automation 전부 성공
- 전체 `BathhouseSim` suite에서 신규/기존 실패를 정확히 분리한다. 두 ActorReplacement 실패는 해소하거나 실제 utility/bath actor의 동등한 focused 통합 검증을 제공하고, SettingsZone grid/material 실패는 별도 기존 Placement 상태로 보고
- failure/ensure/NaN transform/duplicate registration과 per-Tick log spam 없음
