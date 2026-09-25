# Utility Labor System

## Status And Scope

- 2026-09-24 Source 수직 구현을 반영했다. 이전 UE 5.8 빌드에서 당시 C++ compile과 `.lib` link는 통과했으나 실행 중인 UnrealEditor가 DLL을 잠가 최종 link는 완료하지 못했다. 이후 테스트 보강·분할분은 아직 재빌드하지 않았다. automation은 미실행이며 코드 리뷰와 별도 Editor 단계가 남아 있다.
- 입력: `.md/PROMPT_ARCHITECTURE.md`, `.md/QNA_FEATURE_SPEC.md` Q29~Q47, `.md/REPORT_UNREAL_DISCOVERY.md`.
- 첫 수직 대상: 보일러, 석탄 공급함, 공용 삽과 거치, 투입구, 원형 계기, 용량/UI/회수 연계.
- 대상 시나리오: LAB-001~025, LAB-037~039. LAB-026~036은 쿨러·수동 순환기 후속 확장이다.
- 보일러 수직 단계 동안 기존 쿨러·순환기 공급은 기존 동작을 유지한다. 특히 기존 순환기 정격 100은 가열 검증의 준비 조건이다. 전체 노동 기능 완료나 최종 순환기 기본 상태로 간주하지 않는다.
- 새 전역 정책 선택 UI, 범용 연료 프레임워크, 전체 설비 노동 선행 구현은 만들지 않는다.

## Source And Ownership

신규 `Public/Utility`, `Private/Utility`:

- `UtilityFuelTypes.h`: `EUtilityFuelKind { None, Coal }`, `FUtilityFuelLoad { Kind, Points }`, 실패 코드.
- `UtilityOperationComponent.*`: `UUtilityOperationComponent`, 가동 잔량·게임시간 감소·0 경계.
- `UtilityFuelIntakeComponent.*`: `UUtilityFuelIntakeComponent`, 메시 기반 투입 endpoint.
- `UtilityGaugeComponent.*`: `UUtilityGaugeComponent`, 가동 상태를 바늘 transform으로 표현.
- `BathWaterBoilerFacilityActor.*`: `ABathWaterBoilerFacilityActor`, 기존 utility Actor의 native 자식 composition root.
- `UtilityShovelActor.*`: `AUtilityShovelActor`, physical carry/equipment use와 삽 적재 상태.
- `UtilityFuelSupplyActor.*`: `AUtilityFuelSupplyActor`, 무제한 석탄 공급과 F 반환.
- private `UtilityFuelTransaction.*`: 검증·재진입 방어·silent 변경·publication을 한 호출로 조율하는 비-UObject helper.

기존 수정: Facility의 utility Actor/Capacity/Operations/types/payload, Interaction의 carry kind enum, UI의 capacity summary와 필요한 deficit 표시. 기존 Character 입력, StateTree, 물 계산은 재구현하지 않는다.

구현 파일은 `Public/Utility`, `Private/Utility` 및 기존 Facility/Interaction/UI 경로에 있다. Editor asset 변경은 이 Source 단계에서 하지 않았다. 리뷰 대상은 `.md/PROMPT_REVIEW.md`, Editor 작업 대상은 `.md/PROMPT_UNREAL.md`를 따른다.

| 대상 | 기존 책임 | 추가 책임 / 판단 |
|---|---|---|
| utility base Actor | 배치·회수와 provider 연결 | optional labor lifecycle hook만 확장. 잔량/입력 로직은 분리 |
| boiler native child | 신규 | Operation, Intake, Gauge와 시각 subobject 조립 |
| Operation component | 신규 | 잔량·시간·투입 허용 상태의 정본. 독립 수명이 있어 Component 선택 |
| Capacity component | 정격 종류·값 | 명시적으로 주입한 Operation의 가동 여부 조회 |
| Operations subsystem | 전체 provider와 욕탕 요구량 | 설치/가동 집계 분리. 별도 subsystem 생성 불필요 |
| 삽 Actor | 신규 | 한 재료 한 회분. 기존 carry/equipment interface 구현 |
| 공급함 Actor | 신규 | 공급 종류와 한 회분 설정, 반환 target. 유한 inventory 없음 |
| FuelTransaction helper | 신규 | 두 owner의 원자적 변경. world registry/Tick 불필요 |
| Gauge component | 신규 | 바늘 표현과 delegate lifecycle. domain 정본을 갖지 않음 |

## Boiler Composition And Vertical Boundary

`ABathWaterBoilerFacilityActor : ABathWaterUtilityFacilityActor`를 추가한다.

- inherited `Capacity`, `FacilityPlacement`, `PackagePhysicalRoot`, `SceneRoot`, `VisualMesh`, `PlacementFootprint` 이름을 유지한다.
- 신규 native subobject: `Operation`, `FuelIntake`, `GaugeFace`, `GaugeNeedlePivot`, `GaugeNeedleMesh`, `GaugePresentation`.
- `FuelIntake`와 `GaugeFace`, `GaugeNeedlePivot`은 `SceneRoot` 하위, `GaugeNeedleMesh`는 pivot 하위다.
- `Operation`과 `GaugePresentation`은 비공간 ActorComponent다.
- native boiler는 Heating을 요구하며 다른 kind이면 validation/register 실패다.
- base의 native `GetUtilityOperation()` 기본 null / `RequiresLaborOperation()` 기본 false를 child가 override한다. boiler의 필수 Operation 누락은 비가동·등록 실패로 처리하며 항시 공급 fallback하지 않는다.
- `GaugeFace`와 `GaugeNeedleMesh`에는 실제 mesh를 지정한다. 둘 다 NoCollision/Navigation off이며 face/pivot은 `SceneRoot` 아래, needle은 pivot 아래여야 한다. 등록 전 runtime authoring validation도 이 조건을 검사한다.
- 기존 base 순환기·쿨러는 이번 수직 단계에서만 기존 공급 계약을 유지한다. 사용자 조정 가능한 `AlwaysOn` 정책 flag를 추가하지 않는다.
- `BP_Boiler`를 이 native child로 reparent한다. 기존 BP object path와 Definition의 BP 참조는 유지한다.
- 쿨러의 드라이아이스, 순환기 왕복 component/진행 UI는 후속 설계 대상이다. 순환기 최대 잔량에서 시작 허용 여부도 그 단계에서 확정한다.

## Operation State And Time

`UUtilityOperationComponent`는 다음 상태를 소유한다.

- 잔량 anchor 값과 마지막 게임시간, placed-clock 활성 여부.
- labor blocked 상태: 회수 Hold, 변환 transition, domain inactive 또는 EndPlay.
- 변경 revision, 마지막 publication의 가동 여부와 원자적 mutation guard.
- `MaxOperationPoints=100`, `DecayPointsPerSecond=1`: EditDefaultsOnly, Blueprint 종류별 정본. 최대는 finite >0, 감소는 finite >=0.
- 신규 선배치/신규 설치의 잔량은 항상 0. InitialPoints authoring 옵션은 만들지 않는다.

API 의미:

- `GetRemainingPoints()` / `GetOperationSnapshot()`: 게임시간으로 투영한 현재값. query에서 delegate나 저장 mutation 없음.
- `CanAcceptFuel(Load, Context)`: 설치 상태·kind·최대 여부를 side-effect 없이 검증.
- silent settle/add와 publish는 transaction helper/설비 lifecycle만 호출하는 native 내부 경로.
- `StartPlacedClock()`, `StopPlacedClock()`: 초기화/등록 성공·회수 stage에 대응, idempotent.
- `OnOperationChanged`: 잔량/표현 갱신. `OnOperatingChanged`: 0↔양수 기여 변경만 알림.

현재값은 `max(0, AnchorPoints - DecayRate * elapsed game time)`이다. Tick 횟수와 무관하게 같은 게임시각은 같은 값이다. 투입·export·중단 경계는 같은 시각으로 settle하고 다음 anchor를 만든다. 같은 시간 두 번 감소시키지 않는다.

- 물리 계기 표현을 위해 placed 양수 동안 component Tick. 0 또는 packaged/staged면 Tick 중지.
- Tick은 현재값과 이전 표시를 비교해 갱신하고 0 경계를 정확히 한 번 publish한다.
- 투입은 같은 게임시각의 mutation 전 projected operating 상태와 commit 후 상태를 비교한다. exhaustion Tick 뒤와 그 직전 투입이 모두 capacity edge를 정확히 한 번 알린다.
- capacity 조회도 같은 pure projected 값을 읽으므로 Tick 순서가 뒤여도 소진 설비가 공급 중이라고 오판하지 않는다.
- pause에는 감소가 없고 world time dilation을 따른다. computer focus는 pause가 아니므로 감소한다.
- 잔량이 작아도 양수면 정격 전부, 정확히 0이면 0. 임의 큰 epsilon으로 연료를 조기 소진시키지 않는다.
- Q Hold는 clock을 멈추지 않는다. Hold cancel에는 과거 잔량 snapshot을 복구하지 않는다.
- authoring 변경은 PIE 시작 전만 지원. runtime 최대값 변경/수치 migration UI는 범위 밖이다.

## Fuel And Shovel

`AUtilityShovelActor`는 `IPlayerInteractable`, `IPhysicalCarryable`, `IHeldEquipmentUsable`을 구현한다.

- `WorldMesh`를 physical root, `LoadVisual`을 collision-free child로 둔다.
- `FuelLoad`는 None/0 또는 Coal/finite positive 한 회분만 허용한다. revision과 mutation guard를 함께 소유한다.
- `EPhysicalCarryKind` 끝에 `Shovel` 추가. 기존 ordinal을 이동하지 않는다.
- 기존 `FreeDrop|FixedSlot`, exact assigned slot, carrier, last-safe 위치, CCD, Pawn Ignore와 약한 질량 무시 release를 그대로 구현한다.
- `HeldTransform`은 삽 class default에서 위치·회전만 적용한다. Root scale은 보존한다.
- 별도 공통 carry Actor/Component를 추가하지 않는다. 기존 wrench/mop의 interface 구현을 참고하되 공격/청소 행동은 상속하지 않는다.
- 적재 mesh/material은 삽 `LoadVisual`에 지정하고 empty면 숨긴다. 들기/내려놓기/거치 상태 모두 같은 표현을 사용한다.
- `EndEquipmentUse`/`CancelEquipmentUse`는 완료된 적재/투입을 되돌리지 않는다.
- carrier EndPlay/fall recovery는 같은 삽을 이동하며 내용물을 보존한다. 실제 삽 Actor 파괴 시 외부 참조만 해제하고 복제 Actor를 만들지 않는다.

`AUtilityFuelSupplyActor`는 `IPlayerInteractable`을 구현한다.

- `SupplyMesh`를 실제 query target으로 사용한다. 공급함 이동/회수/carry 기능은 없다.
- `FuelKind=Coal`, `ScoopPoints=25`를 EditDefaultsOnly로 지정한다. 한 삽 양의 authoring 정본은 공급함 class이며 삽/투입구에 같은 값을 중복 저장하지 않는다.
- 퍼담을 때 종류와 양을 삽에 복사한다. 이후 공급함 소멸이나 다른 공급함 반환에도 삽 내용은 독립적이다.
- F 반환은 일치 kind의 공급함이면 가능하며 유한 재고를 생성하지 않는다.
- 첫 단계에서는 Coal만 사용한다. 후속 DryIce는 enum append와 별도 공급함 class default로 확장한다.

## Input And Trace

- LMB는 기존 `IHeldEquipmentUsable`의 Instant `BeginEquipmentUse`에서 한 번 처리한다. Update/Triggered/Release에서 재실행하지 않는다.
- 공급함과 intake는 `IPlayerInteractable`로 focus target이 된다. LMB row는 held 삽 query가 authoritative하다.
- 빈손/다른 도구이면 target query가 disabled equipment 안내를 제공하되 기존 held equipment query 우선순위를 바꾸지 않는다.
- F 반환은 공급함의 `ExecuteSecondaryInteraction`에서 같은 FuelTransaction 경로를 호출한다.
- 삽 E pickup과 exact slot E take/store는 기존 carry coordinator를 사용한다.
- `UUtilityFuelIntakeComponent : UStaticMeshComponent, IPlayerInteractable`의 실제 hit만 투입을 허용한다. body hit를 intake hit로 대체하지 않는다.
- generic Interaction/Character에 공급함·보일러 concrete cast나 연료 mutation을 넣지 않는다.
- execute 시 현재 camera의 같은 Interaction trace 거리/channel로 fresh single-hit를 재검증한다. cached FocusHit만 믿거나 blocker를 건너뛰지 않는다.
- 가능하면 기존 generic trace/context refresh를 재사용한다. 읽기 전용 trace API가 필요하면 native generic API로만 노출하고 새 channel/distance authoring을 만들지 않는다.
- player/carry/held identity, suppression, active placement/computer owner와 target validity를 execute 진입에서 확인한다.
- Q row는 기존 포커스 Actor supplemental intent 경로를 통해 intake를 바라볼 때도 합성되는지 검증한다.

## Atomic Fuel Transaction

`FUtilityFuelTransaction`은 synchronous game-thread helper다. 전역 연료 subsystem은 없다.

1. interactor, 실제 held 삽 identity, fresh hit, target, domain mode와 종류/용량을 검증한다.
2. 삽과 target guard를 획득한다. 그 뒤 callback 없이 현재 게임시각/잔량/revision을 다시 검사한다.
3. 퍼담기는 empty→한 회분, 반환은 일치 한 회분→empty, 투입은 `min(Max, Current + Load.Points)`와 empty를 candidate로 계산한다.
4. 양쪽 값을 silent commit한다. 실패 가능한 사전 준비는 이 이전에 끝낸다.
5. 완성된 상태만 publish한다. guard 중 callback의 재진입 mutation은 거부한다. 외부 callback 뒤 참조는 재검증한다.

- `Current == Max`이면 거부, 조금이라도 공간이 있으면 한 회분 전부 소비하고 초과분 폐기.
- 검증 실패 시 적재와 주입 증가는 모두 없다. 설치 상태 자연 감소는 실패와 관계없이 계속된다.
- 이미 성공한 투입 뒤 listener가 Actor를 파괴하는 것은 post-commit 소실이다. 재료를 되살려 중복하지 않는다.
- supply에는 finite count가 없어 삽 state만 commit하지만 동일한 validation/result 계약을 사용한다.
- 외부 표현 event를 두 상태 변경 사이에 방송하지 않는다.

## Capacity Split

기존 `UBathWaterUtilityCapacityComponent`가 정격을 소유하고 optional Operation을 native로 주입받는다. missing required Operation은 active 0이다.

`UBathWaterOperationsSubsystem` 집계:

- Installed = registered provider의 정격 합.
- Active = registered provider 중 labor 가동 중인 정격 합. 수직 단계의 명시적 legacy provider는 기존 정격을 제공.
- Used = 기존 욕탕 요구량 합.
- 설정 request와 `CanRemoveProvider`는 Installed 사용. water effect용 `IsCapacitySatisfied`는 Active 사용.
- 회수로 Active < Used가 되더라도 Installed 기준을 만족하면 회수를 허용한다.
- provider 구조/0 경계는 capacity/data revision만 변경한다. bath topology는 변경하지 않는다.
- positive→positive 잔량 감소는 gauge만 갱신하며 world capacity publication을 반복하지 않는다.

reflected `FBathWaterCapacitySnapshot`은 기존 필드를 보존한다.

- `TotalPoints` = Installed의 기존 canonical 저장 필드, `UsedPoints` 유지.
- `ActivePoints`, `InstalledDeficitPoints=max(Used-Total,0)` 추가.
- `DeficitPoints=max(Used-Active,0)`, `IsSatisfied()`는 실제 효과 충족 의미.
- 새 snapshot default Active=0. 테스트/수동 snapshot 생성도 active 값을 명시한다.
- 기존 bath deficit flag는 active deficit을 나타낸다. 종류별 설치/가동 부족을 UI가 구분할 수 있도록 해당 snapshot 정보도 전달한다.
- 가동 부족으로 slider 상한이 줄거나 요청이 거부되면 안 된다. 설치용량 부족 시 기존 감량 허용·다른 bath 보존 규칙은 그대로 유지한다.

## Gauge Presentation

`UUtilityGaugeComponent : UActorComponent`는 Operation과 NeedlePivot을 native로 주입받는다.

- authored pivot relative rotation과 표시 회전을 구분한다. Construction preview, 반복 construction, BeginPlay, 같은 pivot 재구성에서 이미 적용한 표시 자세를 baseline으로 다시 저장하지 않는다.
- `LocalRotationAxis`, `ZeroAngleDegrees`, `MaxAngleDegrees`, `ActiveStartRatio=1/3`을 EditAnywhere로 노출한다.
- axis finite/nonzero, ratio [0,1), 각도 finite를 검증한다. 축은 normalize하고 음의 축도 지원한다.
- `v==0 ? p=0 : p=a+(1-a)*v/Max`를 계산해 `Angle=Lerp(ZeroAngle,MaxAngle,p)`로 적용한다.
- 회전은 `Baseline * AxisAngle(LocalAxis, Angle)`를 매번 산출하며 누적 회전하지 않는다.
- pivot location이 회전 중심이며 NeedleMesh relative location으로 실제 바늘 길이/중심을 맞춘다.
- 위치와 scale을 회전 update가 변경하지 않는다. 값 변경 즉시 적용하며 tween/noise는 추가하지 않는다.
- `GaugeFace`, needle는 NoCollision/Navigation false. intake는 QueryOnly, Visibility Block, Navigation false. body collision/Nav 계약은 기존 그대로다.
- `AUtilityShovelActor::HasValidAuthoring()`은 WorldMesh/LoadVisual mesh와 parent, world root QueryAndPhysics/WorldStatic Block/Pawn Ignore/CCD, LoadVisual NoCollision/Navigation off와 held unit scale을 검사한다. invalid shovel은 pickup, slot bind, equipment-use와 transaction에서 거부된다. Data Validation은 world root의 authoring collision 상태도 검사한다.
- native `OnConstruction`은 신규 0 바늘 자세와 authoring validation만 제공한다. runtime 잔량을 Editor serialized 값에 기록하지 않는다.
- gauge binding 해제/EndPlay는 idempotent. actor Tick은 추가하지 않는다.

## Recovery And Payload

base utility에 optional labor lifecycle를 연결하되 기존 `FFacilityActorConversionTransaction` 순서를 사용한다.

1. Hold begin: 기존 Installed 회수 gate 재검증, labor block. clock과 provider는 유지.
2. Cancel: labor block 해제. 현재 게임시각으로 감소한 잔량 유지.
3. Export: 동일 synchronous conversion의 게임시각으로 계산한 잔량을 typed payload에 저장.
4. Stage unregister: labor clock settle/중단, provider silent unregister와 기존 collision/domain 비활성화.
5. 실패 rollback: 임시 item 제거, 기존 provider/domain/collision 복원. Hold 시작값으로 rewind하지 않고 stage 시점 잔량부터 설치 clock 재개. stage가 여러 frame을 넘도록 바꾸지 않는다.
6. 성공: 원본 제거, payload만 남고 감소 없음. capacity publication 한 번.
7. 재설치: import를 BeginPlay 전에 수행하므로 `bOperationStateImported`로 보존값 reset 방지. staged에서는 clock off. 원본 item 소비와 placed commit이 모두 성공한 뒤 clock 활성화.

`UBathWaterUtilityPlacementInstanceData`에 `bHasOperationState`, `RemainingOperationPoints` 추가. 기존 Kind/CapacityPoints/Definition 유지. runtime anchor time/delegate/Actor reference는 payload에 넣지 않는다.

- 기존 payload에 새 필드가 없으면 신규 empty state로 해석한다. 이번 기능 이후 boiler export는 반드시 operation state를 가진다.
- nonfinite/음수/현재 maximum 초과 잔량이나 mismatched kind는 import 거부, 원본 item 보존. 묵시적 손실 clamp 금지.
- same-world per-class authoring을 사용하며 최대/감소속도는 CDO 정본이다. instance override를 제공하지 않아 포장 중 설정 복사 문제를 만들지 않는다.
- stage/cancel/destroy 실패와 callback 재진입은 기존 fault-injection 회수 tests로 검증한다.
- 욕탕의 empty water 회수 freeze와 utility 연료 감소를 혼동하지 않는다.

## UI And Editor Handoff

- Summary의 기존 `*CapacityText`에 `예약 {Used} / 가동 {Active} / 설치 {Total}`를 native로 작성한다.
- 기존 `*CapacityStatusText`는 설치 부족과 가동 부족의 값/이름을 모두 표시한다. 설치 부족을 먼저 쓰되 가동 부족도 숨기지 않는다.
- 기존 Bar는 예약/설치 비율을 유지하고 의미를 명시한다. 0 설치와 초과 예약을 text/status가 정확히 보여야 한다.
- cache 비교는 Active/InstalledDeficit 포함. 소진은 capacity/data revision을 갱신하되 map tile을 재생성하지 않는다.
- LMB 삽 row는 Instant, progress hidden. F return과 Q hold는 기존 행. 이번에는 순환기 진행 표시를 구현하지 않는다.
- 신규 UI Widget class/BindWidget는 불필요하다. 기존 1024×576 WBP의 text wrapping/높이만 Editor에서 점검한다.

Editor 대상: 기존 `/Game/Bathhouse/Blueprints/Facility/BP_Boiler`, `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Boiler`; 신규 `/Game/Bathhouse/Blueprints/Utility/BP_UtilityShovel`, `BP_CoalSupply`, 기존 fixed-slot BP의 전용 instance.

- `BP_Boiler` reparent 후 inherited component/값 보존과 Definition 연결을 확인한다. 보고서에서 CDO/instance Definition=None이므로 실제 저장 연결이 필수다.
- 삽, 적재 석탄, 투입구, 분리 계기/바늘은 mesh 슬롯과 pivot만 설계로 확정한다. 미확인 asset 이름을 최종 외형으로 지정하지 않는다.
- 자산 미확보는 Editor의 구체적인 제작/선택 작업이다. 고품질 art는 범위 밖이나 기존 Cube 하나로 표현 검증을 통과시키지 않는다.
- 시각적 부품 배치와 WBP 작업이 MCP에서 불가하면 실제 확인 근거와 exact path를 `USER_UNREAL.md`에 인계한다. 본 설계 단계에서 미리 미완료 큐를 작성하지 않는다.
- Editor 저장 후 Unreal 정본은 Editor 에이전트가 갱신한다. 현재 조사 불일치를 아키텍처가 저장 완료로 기록하지 않는다.

## Compatibility And Verification

- 기존 reflected 이름 삭제/rename 없음. subclass 추가와 `BP_Boiler` reparent이므로 Core Redirect 불필요. compile/save/reload는 필수.
- `EPhysicalCarryKind::Shovel`만 append. 기존 Input/Collision/Nav/StateTree/module 변경 없음.
- 신규 component/Actor와 payload를 포함한 첫 native build 뒤 Editor 재시작으로 layout을 갱신한다.
- 잔량 0/양수/최대, non-identity gauge quaternion과 construction/runtime/configure/rebind/EndPlay lifecycle, 게임시간·pause, Tick 전/후 0 경계 투입, operation/fuel delegate 재진입과 carry/slot 보존을 automation으로 검사한다.
- Boiler/Shovel visual slot 각각의 누락, hierarchy, collision/navigation, CCD/Pawn 응답과 held unit scale을 authoring automation으로 검사한다.
- public `PlaceItemAsFacility()` success와 injected failure rollback에서 payload, item consumption, provider·clock 수명 및 재설치 후 감소 재개를 utility recovery automation으로 검증한다.
- Installed 기준 request/recovery, Active 기준 effect, unexpected loss와 재개, UI cache와 provider-only topology 보존을 검사한다.
- 회수 Hold 소모·0 도달·실패 rollback·포장 장시간·재설치 import 순서·기존 payload 호환을 검사한다.
- 실제 플레이어 E/F/G/LMB/Q와 월드 계기, 물 가열/소진/재개는 LAB-001~025/037~039 PIE로 검증한다.
- 다음 단계 프롬프트는 `.md/PROMPT_IMPLEMENTATION.md`. Source 구현/코드 리뷰/Editor/통합 리뷰/사용자 수직 승인 뒤에만 후속 확장한다.
