# Utility Fuel System

## Status And Scope

- [UtilityLaborSystem.md](UtilityLaborSystem.md)의 하위 문서다. 재료, 공급함, 삽, 연료 설비(보일러·쿨러), 투입 판정 Volume, 자동 열림 문과 연료 transaction을 소유한다.
- 드라이아이스와 쿨러를 보일러와 같은 규칙으로 확장했다. 보일러 동작은 유지 계약이다. 구현 상태(2026-09-26): 이 설계의 Source 구현과 코드 재작업이 끝났고 코드 리뷰는 사용자 지시로 승인됐다. Editor 단계에서 다섯 Blueprint(보일러·쿨러·순환기·삽·드라이아이스 공급함) 저장·Compile까지 진행했으나 DefaultMap instance의 새 Class Default 상속, 드라이아이스 공급함 외부 actor 저장, Data Validation, 새 프로세스 재로드와 직접 PIE 수용이 남아 통합 승인 전이다(`.md/PROMPT_INTEGRATION_REVIEW.md`, `.md/USER_UNREAL.md`).
- 대상 시나리오: 보일러 유지 LAB-003~011, 019, 024, 040~049, 051; 쿨러 LAB-026~028, 052~058, 060~062; 공통 LAB-059.

## Source

- `UtilityFuelTypes.h`: `EUtilityFuelKind { None=0, Coal=1, DryIce=2 }`(append), `FUtilityFuelLoad`, `EUtilityFuelFailure`(기존 값 유지), 재료 표시 이름 helper.
- `UtilityFuelSupplyActor.*`, `UtilityShovelActor.*`, `UtilityFuelIntakeVolumeComponent.*`, `UtilityFuelDoorComponent.*`, private `UtilityFuelTransaction.*`.
- 신규 `BathWaterFuelUtilityFacilityActor.*`, `BathWaterCoolerFacilityActor.*`. 기존 `BathWaterBoilerFacilityActor.*`는 fuel intermediate의 자식이 된다.
- legacy `UtilityFuelIntakeComponent.*`: `UStaticMeshComponent` 자식, 보일러 전용 선택 외형. serialized 호환 때문에 이름·부모·reflected layout을 바꾸지 않는다.
- private `UtilityLaborInputGuard.*`(신규): 컴퓨터 입력 capture·배치 모드 소유를 판정하던 `ValidateOwnerInput`을 연료 transaction과 레버 노동이 공유하도록 옮긴 비-UObject helper.

## Fuel Kinds And Supply

- `FUtilityFuelLoad`는 None/0 또는 Coal·DryIce 중 하나의 finite 양수 한 회분이다. 혼합과 추가 적재는 없다.
- `AUtilityFuelSupplyActor`: `SupplyMesh`가 query target이다. `FuelKind`(Coal 또는 DryIce)와 `ScoopPoints=25`는 EditDefaultsOnly이며 공급함 class가 한 삽 양의 정본이다. 무제한이고 이동·회수·carry 기능이 없다.
- `BP_CoalSupply`와 신규 `BP_DryIceSupply`는 같은 native class의 Blueprint 자식이다. 재료별 native class를 만들지 않는다.
- LMB held-use Apply 한 번(`Instant`)은 held 삽 상태로 행동 하나를 고른다(2026-09-28 E에서 이동). 빈 삽이면 퍼담기, 같은 kind 적재면 전량 반환, 다른 kind 적재면 재료 불일치 거부·내용 보존이다. secondary(F) 행동은 없다.
- TargetName은 재료 표시 이름(석탄 공급함 / 드라이아이스 공급함)을 쓴다. validation은 FuelKind가 Coal 또는 DryIce, ScoopPoints finite 양수, SupplyMesh QueryOnly·Visibility Block·Navigation off다.

## Shovel

`AUtilityShovelActor`는 `IPlayerInteractable`, `IPhysicalCarryable`만 구현한다. 삽은 장비(`IHeldEquipmentUsable`)가 아니다. 삽 LMB는 대상의 held-use Apply로 처리되며 빈 공간에서는 아무 일도 없다.

- `WorldMesh` physical root, `LoadVisual` collision-free child. 기존 carry·FreeDrop·exact FixedSlot·CCD·Pawn Ignore·약한 release 계약을 유지한다.
- `CanAcceptLoad`는 빈 삽에 Coal 또는 DryIce 한 회분을 허용한다.
- 재료별 외형: EditDefaultsOnly `LoadAppearances : TMap<EUtilityFuelKind, FUtilityShovelLoadAppearance>`. 항목은 선택 `Mesh`와 선택 `Material`을 가지며 적용 시 `LoadVisual`에 mesh(지정 시)와 material slot 0 override(지정 시)를 적용한다. 빈 삽이면 `LoadVisual`을 숨긴다.
- validation: Coal과 DryIce 항목이 모두 있고 각 항목이 Mesh나 Material 중 하나 이상을 지정하며, 두 항목이 같은 mesh·material 조합이 아니어야 한다. 들기·내려놓기·거치 상태 모두 같은 표현을 쓴다.
- 재료는 carry 전환, drop, 거치, fall recovery에서 같은 Actor에 보존되며 시간에 따라 사라지지 않는다.

## Fuel Utility Actors

`ABathWaterFuelUtilityFacilityActor : ABathWaterLaborUtilityFacilityActor`(신규, Abstract):

- 생성자에서 `FuelIntakeVolume`, `FuelDoorPivot`, `FuelDoorMesh`, `FuelDoorPresentation`을 만든다. 이름·class는 현재 보일러와 같다. Volume과 문 pivot은 `SceneRoot` 하위, 문 mesh는 pivot 하위다. 문 표현에 pivot을, Volume에 문 표현을 주입한다.
- `virtual EUtilityFuelKind GetAcceptedFuelKind() const`와 `virtual EBathWaterCapacityKind GetRequiredCapacityKind() const`를 자식이 고정값으로 override한다.
- validation: labor intermediate 규칙 + Capacity kind 일치, 유효 Volume과 `SceneRoot` 부착, 문 pivot `SceneRoot` 부착, 문 mesh 지정·pivot 부착·NoCollision·Navigation off, 문 표현 값.
- `ABathWaterBoilerFacilityActor`: Coal·Heating. legacy `FuelIntake`(선택 외형)를 계속 생성한다. mesh가 있으면 `SceneRoot` 부착·NoCollision·Navigation off를 요구한다.
- `ABathWaterCoolerFacilityActor`(신규): DryIce·Cooling. legacy `FuelIntake`는 없다.

## Input And Trace

- 삽 월드 작업은 target의 held-use Apply(LMB, `Instant`)다. LMB `Started` 한 번이 fresh trace/query/execute 한 번이며 누르고 있어도 반복하지 않는다. 계약은 [HeldTargetUseSystem.md](HeldTargetUseSystem.md) Shovel Targets다.
- 공급함 `SupplyMesh`와 `FuelIntakeVolume`이 focus target이다. query는 transaction 평가 결과만 사용하고 상태를 바꾸지 않는다.
- 실패 문구는 삽 필요(빈손·다른 도구), 빈 삽, 재료 불일치, 가득 참, 회수 중, 미설치, 조준 불일치를 구분한다. 문구의 재료·설비 이름은 target 설비와 재료 종류에서 만든다.
- E·F·RMB는 삽 작업을 실행하지 않는다. 대상의 E primary는 이름만 보이고 행동이 없다. 삽 E pickup과 exact slot E take/store는 기존 carry coordinator를 쓴다.
- generic Interaction/Character에 concrete cast나 연료 mutation을 넣지 않는다. target이 held object를 판별하는 target-side 패턴을 유지한다.
- execute는 같은 Interaction trace 거리·channel로 fresh single-hit를 재검증한다. 기대 component는 공급함 `SupplyMesh`, 투입은 해당 설비의 `FuelIntakeVolume`이다.
- owner input 검사(컴퓨터 capture, 배치 모드)는 `UtilityLaborInputGuard`를 쓴다.

## Atomic Fuel Transaction

`FUtilityFuelTransaction`은 synchronous game-thread helper다. 모든 진입은 `FPlayerInteractionContext`를 쓴다.

- `EvaluateScoop`, `EvaluateReturn`, `EvaluateInsert`는 side-effect 없는 평가다. `EvaluateInsert`는 intake Volume의 owner를 `ABathWaterFuelUtilityFacilityActor`로 보고 `Load.Kind == GetAcceptedFuelKind()`를 요구한다. 다르면 `WrongFuel`이다. 보일러 전용 cast를 남기지 않는다.
- `EvaluateInsert` 성공 조건: held 삽 identity·authoring, owner input, 받는 재료 적재, 설비 authoring, Volume 일치, placed domain active·non-staged, placed clock active, labor block 없음, 현재값 < 최대.
- execute: 평가 → fresh hit → 삽과 target guard → guard 안 재평가 → candidate 계산 → silent commit → publish. 퍼담기는 empty→한 회분, 반환은 일치 한 회분→empty, 투입은 `min(Max, Current + Load.Points)`와 empty다.
- `Current == Max`면 거부, 조금이라도 공간이 있으면 한 회분 전부 소비하고 초과분은 폐기한다. 실패 시 재료·가동수치 변화가 없고 자연 감소는 계속된다.
- post-commit 파괴는 재료를 되살리지 않는다. 외부 event를 두 상태 변경 사이에 방송하지 않는다.

## Fuel Door Presentation

2026-09-30: 같은 알고리즘의 범용 `UOpeningPresentationComponent`(Interaction/Presentation)가 세탁기·건조기 뚜껑용으로 추가된다. 이 class의 부모와 reflected layout은 바꾸지 않는다(BP_Boiler 호환).

`UUtilityFuelDoorComponent`는 보일러·쿨러 공통이며 문 자세만 표현한다. domain 상태, 투입 판단과 collision을 소유하지 않는다.

- `FuelIntakeVolume`이 `IPlayerInteractionFocusObserver`로 받은 알림을 자기 설비의 문에 전달한다. 다른 설비 문에는 전달하지 않는다.
- source별 최신 query의 `bHeldApplyVisible && bCanHeldApply`를 weak key로 저장하고, 하나라도 true면 목표 열림이다. `bCanHeldApply`는 `EvaluateInsert` 성공이므로 문 열림과 지금 LMB 투입 가능이 같은 판정이다. 가득 참은 닫힘, 자연 감소로 공간이 생기면 다음 query refresh에서 열린다.
- 투입 성공 후 빈 삽, 도구 변경·거치·drop, 시선·거리 이탈, 회수 Hold 시작은 query 변화나 focus 종료로 닫힘을 만든다. 컴퓨터 진입은 suppression의 query clear가 focus 종료를 보낸다.
- alpha [0,1] 선형 이동. 열기 `1/OpenSeconds`, 닫기 `1/CloseSeconds`, 목표 전환 시 현재 alpha에서 방향 전환, 0초는 즉시. alpha ≠ target일 때만 Tick한다.
- 회전은 `Baseline * AxisAngle(LocalRotationAxis, OpenAngleDegrees * alpha)`, baseline은 배치한 닫힘 자세다.
- authoring은 EditDefaultsOnly, 설비 Blueprint class가 정본이다: `LocalRotationAxis`, `OpenAngleDegrees`, `OpenSeconds=0.2`, `CloseSeconds=0.2`.
- construction·BeginPlay·EndPlay는 source를 비우고 닫힘을 즉시 적용한다. EndPlay는 pivot 참조와 baseline을 유지한다. staged·회수 변환은 collision off로 focus 종료가 되어 닫힌다.
- 문 mesh는 NoCollision·Navigation off다. 판정은 고정 Volume이 맡아 문 회전이 조준을 끊지 않고 아무것도 밀거나 막지 않는다.
- CallInEditor `PreviewOpenPose()`/`RestoreClosedPose()`는 Editor world 전용 transient 표시이며 PreSave·construction·BeginPlay가 닫힘을 복원한다.

## Blueprint/API Contracts

- 신규 reflected: `EUtilityFuelKind::DryIce`, `FUtilityShovelLoadAppearance`, 삽 `LoadAppearances`, fuel·cooler native class.
- 기존 reflected 이름 유지: `FuelIntake`, `FuelIntakeVolume`, `FuelDoorPivot`, `FuelDoorMesh`, `FuelDoorPresentation`, `SupplyMesh`, `WorldMesh`, `LoadVisual`, `FuelKind`, `ScoopPoints`.
- 삽의 기존 `LoadVisual` mesh는 `LoadAppearances`가 mesh를 지정하지 않는 재료의 기본 mesh로 남는다.
- Editor: `BP_DryIceSupply` 생성, `BP_UtilityShovel.LoadAppearances` 두 항목, `BP_Cooler` 투입 Volume·문 authoring.

## Verification

- 기존 보일러 연료·문·실제 조준 경로 automation은 fuel intermediate 이동 뒤 그대로 통과해야 한다.
- 쿨러: DryIce 퍼담기·반환·투입, 교차 투입(석탄→쿨러, 드라이아이스→보일러)과 교차 반환 거부, 90+25 초과 폐기, 가득 참 후 감소 재열림, 드라이아이스 삽만 문 열림, 두 쿨러 독립, 냉각 소진·재개, 회수 payload.
- 삽: 재료별 외형 전환과 drop·거치 보존, `LoadAppearances` validation.
- 공급함: DryIce authoring validation, 재료별 TargetName.
