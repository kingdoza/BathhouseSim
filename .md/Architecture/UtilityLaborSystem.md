# Utility Labor System

## Status And Scope

- 2026-09-26 설계: 보일러 수직 구현 완료·사용자 승인 뒤 쿨러 전체 확장과 순환기 레버 수직을 설계했다. 입력은 `.md/PROMPT_ARCHITECTURE.md`(쿨러·순환기 노동 가동 확장)와 `.md/QNA_FEATURE_SPEC.md` Q29~Q61이다. 기능 계약과 설계 진행은 2026-09-26 사용자가 승인했다.
- 구현 상태(2026-09-26): 이 설계의 Source 구현과 코드 재작업이 끝났고 코드 리뷰는 사용자 지시로 승인됐다. Editor 단계에서 다섯 Blueprint(보일러·쿨러·순환기·삽·드라이아이스 공급함) 저장·Compile까지 진행했으나 DefaultMap instance의 새 Class Default 상속, 드라이아이스 공급함 외부 actor 저장, Data Validation, 새 프로세스 재로드와 직접 PIE 수용이 남아 통합 승인 전이다(`.md/PROMPT_INTEGRATION_REVIEW.md`, `.md/USER_UNREAL.md`).
- 설계 당시 출발점은 보일러 수직 구현(쿨러·순환기는 Operation 없이 항상 공급하는 base Blueprint)이었다. 구현 입력은 당시 `.md/PROMPT_IMPLEMENTATION.md`였다.
- 이번 단계 완료 대상: 보일러 유지(LAB-001~025, 037~049, 051), 공통(LAB-050, 059, 063~065), 쿨러(LAB-026~028, 052~058, 060~062), 순환기 수직(LAB-029~031, 033~036, 066~072). `순환기 후속`(LAB-032, 073~075)은 순환기 수직 사용자 승인 뒤 단계다. 다만 취소·회수 규칙 자체는 이번 구현에 포함한다.
- 이번 단계 뒤에는 어떤 utility도 Operation 없이 가동용량을 제공하지 않는다. 검증에 필요한 순환은 실제 순환기 노동으로 준비하며 automation만 test probe로 잔량을 준비할 수 있다.
- 새 전역 정책 UI, 범용 연료 프레임워크, 순환기 이외 레버 설비, 공급함 재고는 만들지 않는다.

## Documents

- 이 문서: 설비 class 계층, Operation·시간, 용량 집계, 계기, 회수 payload, 공통 UI·Editor·호환성.
- [UtilityFuelSystem.md](UtilityFuelSystem.md): 재료 종류, 공급함, 삽, 연료 설비(보일러·쿨러), 투입 판정 Volume, 문, 연료 transaction.
- [UtilityLeverSystem.md](UtilityLeverSystem.md): 순환기 조작부, 레버 왕복 상태 기계, 보상, 취소·복귀와 진행 표시.

Source는 `Public/Utility`, `Private/Utility`와 기존 Facility/Interaction/UI의 최소 확장이다. 잔량은 Operation, 적재는 삽, 레버 왕복은 레버 노동 component, 전역 용량은 Operations subsystem이 소유한다.

## Class Hierarchy And Ownership

```text
ABathWaterUtilityFacilityActor            (Facility, 기존) 배치·회수·provider 연결
└─ ABathWaterLaborUtilityFacilityActor    (Utility, 신규) Operation + 바늘 계기 조립
   ├─ ABathWaterFuelUtilityFacilityActor  (Utility, 신규) 투입 Volume + 문, 받는 재료
   │  ├─ ABathWaterBoilerFacilityActor    (기존) Heating·Coal, legacy FuelIntake 외형
   │  └─ ABathWaterCoolerFacilityActor    (신규) Cooling·DryIce
   └─ ABathWaterCirculatorFacilityActor   (신규) Circulation, 조작부 Volume + 레버
```

| 대상 | 책임 | 판단 |
|---|---|---|
| utility base | 배치·회수·provider와 labor lifecycle hook | 모든 utility가 Operation을 요구한다. Operation 없는 legacy 항상 공급 경로를 삭제한다 |
| labor intermediate | `Operation`, `GaugeNeedlePivot`, `GaugeNeedleMesh`, `GaugePresentation` 생성·주입, 공통 validation | 세 설비 공통 조립을 한 곳에 둔다. base에 계기 로직을 누적하지 않는다 |
| fuel intermediate | `FuelIntakeVolume`, `FuelDoorPivot`, `FuelDoorMesh`, `FuelDoorPresentation`, `GetAcceptedFuelKind()` | 보일러·쿨러 차이는 재료와 용량 종류뿐이라 공통화한다 |
| boiler | Heating·Coal 고정, legacy `FuelIntake` 선택 외형 | 기존 `BP_Boiler` parent class를 유지해 reparent가 필요 없다 |
| cooler | Cooling·DryIce 고정 | `BP_Cooler`를 reparent한다 |
| circulator | Circulation 고정, `LeverOperatingVolume`, `LeverPivot`, `LeverMesh`, `LeverLabor` | `BP_Circulator`를 reparent한다 |
| Operation | 잔량·시간·가동 경계·labor block·보상 적용 | 기존 component. 레버 보상용 단일 owner API만 추가한다 |

- 기존 boiler subobject(`Operation`, 바늘 3종, 투입 Volume·문 4종)는 **이름·class를 바꾸지 않고** 생성 위치만 부모 class 생성자로 옮긴다. default subobject와 property는 이름으로 연결되므로 serialized layout은 바뀌지 않는다. 그래도 기존 export를 가진 native class 변경이므로 CoreSystem의 복사본 로드 gate를 적용한다.
- 모든 주입은 생성자에서 native로 하고 runtime `FindComponentByClass`에 의존하지 않는다. 주입 참조는 owner 수명 동안 유지하며 EndPlay가 지우지 않는다.
- 각 native 설비는 고정 capacity kind를 검증한다. kind가 맞지 않거나 필수 부품이 없으면 Data Validation 오류이며 등록·배치·회수·노동이 모두 거부된다.
- `GaugeFace` native subobject는 삭제한다. 별도 계기판은 Blueprint가 선택적으로 추가·삭제하는 일반 static mesh이며 native가 참조하거나 검증하지 않는다.

필수·선택 구성:

| 설비 | 필수 | 선택 |
|---|---|---|
| 보일러 | Operation, 바늘 pivot·mesh, 투입 Volume, 문 pivot·mesh | Blueprint 계기판, legacy `FuelIntake` 외형 |
| 쿨러 | Operation, 바늘 pivot·mesh, 투입 Volume, 문 pivot·mesh | Blueprint 계기판 |
| 순환기 | Operation, 바늘 pivot·mesh, 조작부 Volume, 레버 pivot·mesh | Blueprint 계기판 |

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
- `ApplyLaborReward(Points, OutFailure)`: 레버 왕복 완료용 단일 owner 변경. placed clock active·labor block 없음·guard 미사용·finite 양수를 확인하고 같은 게임시각으로 settle한 뒤 `min(Max, Current + Points)`를 silent commit하고 한 번 publish한다. 0→양수 capacity edge를 투입과 같은 규칙으로 알린다.
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

## Capacity Split

기존 `UBathWaterUtilityCapacityComponent`가 정격을 소유하고 Operation을 native로 주입받는다. Operation이 없으면 authoring 오류이며 active 0이다.

`UBathWaterOperationsSubsystem` 집계:

- Installed = registered provider의 정격 합.
- Active = registered provider 중 Operation 잔량이 양수인 정격 합. Operation 없는 provider는 가동 0이며 legacy 항상 공급 경로는 없다.
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

`UUtilityGaugeComponent : UActorComponent`는 labor intermediate가 Operation과 `GaugeNeedlePivot`을 주입한다. 세 설비가 같은 규칙을 쓴다.

- authored pivot relative rotation을 baseline으로 두고 표시 회전과 구분한다. 규칙은 private `UtilityPivotRotation` helper가 소유하며 문·레버와 공유한다. construction preview, 반복 construction, BeginPlay, 같은 pivot 재구성에서 이미 적용한 표시 자세를 baseline으로 다시 저장하지 않는다.
- `LocalRotationAxis`, `ZeroAngleDegrees`, `MaxAngleDegrees`, `ActiveStartRatio=1/3`을 EditAnywhere로 노출한다. axis finite/nonzero, ratio [0,1), 각도 finite를 검증하며 축은 normalize하고 음의 축도 지원한다.
- `v==0 ? p=0 : p=a+(1-a)*v/Max`, `Angle=Lerp(ZeroAngle,MaxAngle,p)`를 적용한다. 회전은 `Baseline * AxisAngle(LocalAxis, Angle)`로 매번 산출하며 누적하지 않는다.
- pivot location이 회전 중심이며 NeedleMesh relative location으로 바늘 길이/중심을 맞춘다. 위치와 scale은 회전 update가 바꾸지 않는다. tween/noise는 없다.
- 바늘 mesh는 필수이며 NoCollision·Navigation off다. 별도 계기판은 native 구성요소가 아니므로 바늘 표시와 validation은 계기판 유무와 무관하다.
- native `OnConstruction`은 신규 0 바늘 자세와 authoring validation만 제공한다. runtime 잔량을 Editor serialized 값에 기록하지 않는다.
- EndPlay는 Operation delegate binding만 해제하고 주입 참조와 baseline을 유지한다. 반복 호출에 안전하다.

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

- 기존 payload에 새 필드가 없으면 신규 empty state로 해석한다. 보일러·쿨러·순환기 export는 반드시 operation state를 가진다. 레버 왕복 중 회수가 시작되면 왕복은 보상 없이 취소되며 payload에는 그 시점 잔량만 들어간다.
- nonfinite/음수/현재 maximum 초과 잔량이나 mismatched kind는 import 거부, 원본 item 보존. 묵시적 손실 clamp 금지.
- same-world per-class authoring을 사용하며 최대/감소속도는 CDO 정본이다. instance override를 제공하지 않아 포장 중 설정 복사 문제를 만들지 않는다.
- stage/cancel/destroy 실패와 callback 재진입은 기존 fault-injection 회수 tests로 검증한다.
- 욕탕의 empty water 회수 freeze와 utility 연료 감소를 혼동하지 않는다.

## UI

- Summary의 순환·가열·냉각 `*CapacityText`는 `예약 {Used} / 가동 {Active} / 설치 {Total}`를 native로 작성한다. `*CapacityStatusText`는 설치 부족을 먼저 쓰되 가동 부족도 숨기지 않는다. Bar는 예약/설치 비율이다. 기존 9개 binding과 cache 규칙을 유지한다.
- 조준 HUD는 target query의 E row 하나로 행동 또는 실패 이유를 표시한다. 삽 작업은 진행 표시가 없다. 순환기 왕복은 E row 진행 막대를 쓰며 [UtilityLeverSystem.md](UtilityLeverSystem.md)가 규칙을 정한다. Q 회수 진행은 기존 recovery row다.
- E row 진행 막대는 기존 Hold mode 또는 신규 query 필드 `bPrimaryProgressVisible`이 true일 때 표시한다. 계약은 [InteractionSystem.md](InteractionSystem.md)와 [UISystem.md](UISystem.md)에 있다. 신규 Widget class나 BindWidget은 없다.

## Editor Handoff

Editor 대상과 순서:

1. 복사본 로드 gate 통과 보고 확인. 그 전에는 `BP_Boiler`를 열거나 저장하지 않는다.
2. `BP_Boiler`: native `GaugeFace`가 사라진다. 사용자가 삭제하기 전까지 기존 임시 계기판 외형을 유지해야 하므로 Blueprint component `GaugeFacePlate`(static mesh)를 추가해 `.md/Unreal/UtilityLaborSystem.md`에 기록된 Cube·Location `(0,-32,90)`·Scale `(0.30,0.02,0.30)`·NoCollision을 재현한다. `GaugeFace` 이름은 재사용하지 않는다.
3. `BP_Cooler` → `ABathWaterCoolerFacilityActor`, `BP_Circulator` → `ABathWaterCirculatorFacilityActor` reparent. 기존 Capacity kind·정격·footprint·본체 mesh 보존을 확인한다.
4. 쿨러: 투입 Volume·문 pivot/mesh·바늘 pivot/mesh 임시 authoring. 순환기: 조작부 Volume·레버 pivot/mesh·바늘 pivot/mesh 임시 authoring. 임시 표현이라도 문 열림·닫힘, 레버 위·아래, 바늘 위치를 구분할 수 있어야 한다.
5. `DA_FacilityPlacement_Cooler`, `DA_FacilityPlacement_Circulator`와 두 Blueprint의 Definition 연결을 확인·저장한다.
6. 신규 `/Game/Bathhouse/Blueprints/Utility/BP_DryIceSupply`(공급함 native 자식, `FuelKind=DryIce`), `BP_UtilityShovel`의 재료별 적재 외형.
7. DefaultMap: 드라이아이스 공급함 배치, 선배치 쿨러·순환기 instance가 새 Class Default를 상속하는지 확인. World Partition 외부 actor 저장 제약은 `USER_UNREAL.md` 기준을 따른다.
8. 세 Blueprint에서 별도 계기판을 삭제한 상태로 Compile·Save·재로드·Data Validation이 통과하는지 확인한다(LAB-063). 확인 뒤 원래 외형을 복원할지는 사용자 결정이다.

MCP로 불가한 작업은 exact path와 근거를 `USER_UNREAL.md`에 인계한다. Unreal 정본은 Editor 단계가 갱신한다.

## Compatibility And Verification

- rename·delete 대상: native `GaugeFace` subobject 삭제뿐이다. 같은 class(`UStaticMeshComponent`)의 default subobject 제거이므로 size mismatch 유형은 아니지만, 기존 `BP_Boiler` CDO와 DefaultMap 보일러 instance에 남은 export 처리 방식(무시·고아 객체·경고)은 엔진 동작으로 확인되지 않았다. 구현 단계가 `BP_Boiler` 파일 복사본으로 **로드 → Compile → 복사본 Save → 재로드**를 먼저 수행해 Fatal·유령 component가 없음을 확인한다. 실패하면 삭제를 진행하지 않고 설계로 되돌린다.
- boiler subobject의 부모 class 이동, 새 intermediate class, 새 enum 값 `EUtilityFuelKind::DryIce`(append), 새 query 필드는 이름·layout 호환이며 Core Redirect가 필요 없다.
- `BP_Cooler`, `BP_Circulator`는 reparent 전까지 Operation이 없어 validation 오류·용량 0이 된다. 이 기간 DefaultMap 로드는 가능해야 하며 fatal이 아니다.
- legacy 항상 공급 삭제로 base utility를 직접 spawn하던 automation은 labor 설비와 test probe 잔량 준비로 바꾼다.
- 로드 검증 순서: `BP_Boiler` 복사본(로드·Compile·Save·재로드) → 원본 비저장 로드 → `BP_Cooler`·`BP_Circulator` 비저장 로드 → DefaultMap. 앞 단계가 실패하면 멈춘다.
- automation: 계층 이동 뒤 보일러 기존 test 전부, 쿨러 연료 흐름·문·교차 재료, 레버 상태 기계·실제 조준 경로, 세 설비 authoring, 용량 legacy 제거, 전체 `BathhouseSim` 회귀.
- PIE: 이번 단계 대상 LAB를 실제 입력으로 검증한다. `순환기 후속`은 통과 처리하지 않는다.
