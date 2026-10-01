# PROMPT_IMPLEMENTATION — UNBOX-SPAWN-VIEW 플레이어 앞 생성 위치를 카메라 시선 기준으로

- 작업 ID: `UNBOX-SPAWN-VIEW`
- 단계: 아키텍처
- 상태: 완료

## 1. 입력과 단계

- 기능 계약: 같은 폴더 `PROMPT_ARCHITECTURE.md`(USV-001~021, 2026-10-01 사용자 승인, P9·P10·P2 정정 포함), 선택 기록 `QNA_FEATURE_SPEC.md`, Editor 사실 `REPORT_UNREAL_DISCOVERY.md`.
- 단계: 수직 구현 없음(Q6 B). 개봉과 집게 봉투 묶기를 이 작업 하나에서 USV-001~021 전체로 구현·검증한다.
- 작업 위치: worktree `C:\UnrealProjects\BathhouseSim\.claude\worktrees\unbox`, 브랜치 `work/UNBOX-SPAWN-VIEW`, 아키텍처 시작 커밋 `fc5b59f`.
- 정본 갱신(이 단계에서 반영함): `.md/Architecture/ShopSystem.md` Catalog And Settings·World Placement·Transaction·Verification, `.md/Architecture/CleaningLitterSystem.md` Tongs·Front Drop Placement·Verification, `.md/Architecture/InteractionSystem.md` Source Scope·Player View-Front Spawn Geometry·Dependencies, `.md/0_ARCHITECTURE.md` Shop 요약.

## 2. 목적, 수용 기준, 비목표

목적: 상자 개봉(LMB)과 봉투 묶기(RMB)로 새로 생기는 물체가 입력 순간의 카메라 시선 앞(pitch 포함)에 보이게 한다. 기존 발바닥 기준 정면 규칙은 막혔을 때의 2단계(바닥 정면)로 남긴다.

수용 기준(관찰 결과, `PROMPT_ARCHITECTURE.md` 5·8절):

- 1단계로 생긴 무리(봉투는 봉투 하나)는 카메라에서 시선 방향으로 잰 가장 가까운 부분이 생성 거리이고, 시선 ray에 좌우·위아래 가운데 정렬된다. 카메라를 감싸거나 닿지 않는다.
- 막히면 시선 당김(하한까지) → 바닥 정면 → (개봉만) 머리 위 → (개봉만) 최후. 봉투는 바닥 정면까지 실패하면 `봉투를 놓을 공간이 없음`이고 아무것도 바뀌지 않는다.
- 1~3단계 모두 벽·바닥·천장·기존 물체 안, 벽·천장 너머에 생기지 않는다. 개봉은 손님 몸 안에도 생기지 않는다. 봉투는 손님을 막는 대상으로 보지 않는다(P2 정정).
- 바닥 정면 단계로 생겨도 무리가 카메라를 감싸지 않는다(P10). 작은 무리와 봉투의 바닥 정면 위치는 현재와 같다.
- 발밑을 보면 플레이어 몸 자리와 겹쳐도 그 자리를 쓴다(P9).
- 생긴 뒤 물리·튐·Pawn 무시·개봉 항상 성공·봉투 실패 계약은 그대로다.

비목표: 생기는 물체의 종류·수량·크기, 무리 모양(`FShopUnboxingCluster`), 튐 세기, 추가 속도·연출·HUD 문구, G 내려놓기·Q 회수·배송 지점, 머리 위·최후 쌓기 규칙, pitch 제한·FOV·capsule, depenetration 설정 변경.

## 3. 설계 결정 (기능 명세 12절 "설계에 맡김")

| 항목 | 결정 | 근거 |
|---|---|---|
| 정렬 기준과 "가장 가까운 부분" | 선 자세 box마다 support function으로 축 투영 범위를 정확히 구한다. 시선 축 V 투영 최솟값 = 생성 거리, 카메라 right·up 축(`FRotationMatrix(V.Rotation())`의 Y·Z) 투영 범위의 중앙 = 카메라 위치 | box 8꼭짓점과 같은 결과를 닫힌 식으로 얻고, 화면 기준 가운데 정렬이 정확하다. AABB 중심 정렬은 pitch가 있을 때 화면 중앙에서 벗어난다 |
| P10 방법 | 바닥 정면 단계 translation 뒤, 무리 윗면 ≤ 카메라 Z − 10cm이면 그대로, 아니면 수평 전방 투영 최솟값이 카메라 앞 10cm가 되도록 수평 전방으로 민다. 민 자리도 기존 검사를 통과해야 하며 실패하면 다음 시도 → 머리 위 | 카메라와 무리가 평면 하나로 분리되면 감쌀 수 없다. 윗면 조건으로 작은 무리(설비 1개 윗면 약 80cm)와 봉투는 현재 위치·당김 그대로다 |
| 시도 횟수와 비용 | 시선 당김: 거리마다 layout 최대 4회(바닥 정면과 같음). 머리 위·최후 그대로. worst case (8 × 4 + 11 × 4 + 8 × 3) = 100 layout × 10 물품 × 3 query, 첫 실패 물품에서 중단 | 기존 68 layout 대비 약 1.5배, 클릭 한 번의 동기 처리로 허용 |
| 조정값 | 기존 값은 의미를 바꾸지 않는다. 시선 단계 값을 새로 추가한다. 당김 간격 10cm는 두 경로 모두 지금처럼 코드 상수다 | 기존 `UnboxForwardDistanceCm`·`TieForwardDistanceCm`·`TieMinForwardDistanceCm`는 바닥 정면 단계에서 현재 의미 그대로 쓰인다. 같은 값에 두 의미를 주지 않는다 |
| 계산 공유 | 순수 기하(시선 앞 translation, P10 push, 당김 거리열)만 Interaction private helper로 공유한다. 단계 순서·충돌·시야·손님 검사는 각 시스템이 소유한다 | 개봉은 무리·여유 shape·손님 검사·항상 성공, 봉투는 단일 shape·쓰레기/얼룩 무시·실패 반환으로 검사 집합이 다르다. "가장 가까운 부분" 정의는 한 곳에만 둔다 |
| 시야 검사 기준점 | 시선 앞 단계는 카메라 위치, 바닥 정면은 capsule 중심(현재), 머리 위는 capsule 윗면 중심(현재) | 시선 앞은 카메라 기준으로 정해지므로 카메라에서 보이지 않는 자리를 막는다. 바닥 정면은 현재 결과를 바꾸지 않는다 |
| 카메라 입력 | 두 경로 모두 `FHeldEquipmentUseContext::CameraOrigin/CameraDirection`(입력 순간 `UPlayerEquipmentUseComponent::BuildContext`가 카메라 component에서 채움)을 쓴다. 봉투의 `FindComponentByClass<UCameraComponent>` 조회를 없앤다 | 입력 순간 값이 기준(5.1)이고, 테스트가 카메라를 명시적으로 줄 수 있다 |

## 4. 대상 파일과 책임 변화

### 4.1 신규: 공유 순수 기하

`Source/BathhouseSim/Private/Interaction/PlayerViewFrontPlacement.h/.cpp` (world·UObject 접근 없음, 정본 `InteractionSystem.md` Player View-Front Spawn Geometry)

```cpp
struct FViewFrontBox
{
	FVector Center = FVector::ZeroVector;      // collision shape 중심
	float YawDegrees = 0.0f;                   // 선 자세, yaw만
	FVector HalfExtent = FVector::ZeroVector;  // local half extent (>= 0)
};

namespace PlayerViewFrontPlacement
{
	inline constexpr float PullStepCm = 10.0f;
	inline constexpr float CameraClearanceCm = 10.0f;

	// (p - Origin)·UnitAxis의 모든 box 점에 대한 [Min, Max]. box마다 Center·Axis ± (Ex|Axis·X_yaw| + Ey|Axis·Y_yaw| + Ez|Axis.Z|)
	void GetProjectionRange(TConstArrayView<FViewFrontBox> Boxes, const FVector& Origin, const FVector& UnitAxis,
		double& OutMin, double& OutMax);

	// Boxes에 더할 translation. 결과에서 V 투영 Min = DistanceCm, right/up 투영 범위 중앙 = 0(카메라).
	// right/up = FRotationMatrix(UnitViewDirection.Rotation())의 Y/Z 축(수직 V에서도 정의됨).
	FVector ComputeViewFrontTranslation(TConstArrayView<FViewFrontBox> Boxes, const FVector& CameraOrigin,
		const FVector& UnitViewDirection, float DistanceCm);

	// P10. world 좌표 Boxes 기준. 윗면 Max Z <= CameraOrigin.Z - CameraClearanceCm 이거나
	// 수평 전방 투영 Min >= CameraClearanceCm 이면 0, 아니면 CameraClearanceCm - 투영 Min.
	float GetCameraClearancePushCm(TConstArrayView<FViewFrontBox> Boxes, const FVector& CameraOrigin,
		const FVector& UnitHorizontalForward);

	// Start' = max(Start, Min). Start', Start'-10, ... (> Min) 뒤 마지막 Min 한 번. 비유한 입력이면 빈 배열.
	void BuildPullDistances(float StartCm, float MinCm, TArray<float>& OutDistances);
}
```

- 반환 형태(out param·작은 struct)는 구현이 정해도 되지만 의미는 위와 같아야 한다.
- 개봉 바닥 정면의 기존 거리열(`UnboxForwardDistanceCm` → 0, 0 포함)과 봉투의 기본값 거리열(60, 50, 40, 30)은 `BuildPullDistances`로 같은 결과다. 봉투 값이 10의 배수 간격이 아니면 마지막 하한을 한 번 더 시도하게 되는 차이만 있다(의도된 정규화).

### 4.2 개봉 (Shop)

`Private/Shop/ShopUnboxingPlacement.h/.cpp`

```cpp
enum class EShopUnboxPlacementStage : uint8 { None, ViewFront, FloorFront, Overhead, FinalStack };

struct FShopUnboxingPlacementRequest
{
	FVector CameraOrigin = FVector::ZeroVector;
	FVector CameraDirection = FVector::ForwardVector; // pitch 포함, 내부에서 정규화
	FVector FootLocation = FVector::ZeroVector;
	float ViewDistanceCm = 100.0f;        // UnboxViewDistanceCm
	float ViewMinDistanceCm = 30.0f;      // UnboxViewMinDistanceCm
	float FloorForwardDistanceCm = 100.0f; // UnboxForwardDistanceCm (의미 유지)
	float OverlapDepthCm = 8.0f;
};

static bool FindSpawnTransforms(UWorld& World, AActor& Player, const UCapsuleComponent& PlayerCapsule,
	AShopDeliveryBoxActor& Box, const TArray<FShopUnboxItemShape>& Items,
	const FShopUnboxingPlacementRequest& Request, FRandomStream& RandomStream,
	TArray<FTransform>& OutTransforms, FText& OutFailureReason,
	EShopUnboxPlacementStage* OutStage = nullptr);
```

- 기존 `FootLocation`·`ViewYaw`·`ForwardDistanceCm`·`OverlapDepthCm` 위치 인자를 request로 옮긴다. 기존 시그니처는 남기지 않는다(호출자는 transaction과 테스트뿐).
- 입력 검증: request 벡터 NaN, 방향 0, 거리·깊이 비유한·음수는 기존 `InvalidUnboxRequest` 실패다. 시선 거리 < 하한이면 하한으로 올린다.
- 수평 전방 F = (V.X, V.Y, 0) 정규화. 0에 가까우면 바닥 정면 단계를 건너뛴다. 물품 half extent probe의 yaw는 F의 yaw(없으면 0)를 쓴다(값에 영향 없음).
- 단계(처음 성공에서 반환하고 `OutStage`에 기록):
  1. ViewFront: `BuildPullDistances(ViewDistance, ViewMin)`의 d마다 layout 최대 4회. layout(무리 local 중심·yaw·HalfExtents) → `ComputeViewFrontTranslation(…, CameraOrigin, V, d)` → 물품마다 기존 `IsCandidateSafe`(여유 shape 수평·위만, 환경, 손님 `ECC_Pawn`, 시야). 시야 기준점 = `CameraOrigin`.
  2. FloorFront: 기존 정면 단계 그대로(`BuildPullDistances(FloorForwardDistance, 0)`, d마다 4회, `GetClusterTranslation`으로 XY 중심 = 발바닥 + F × d, 최저 바닥면 = 발바닥 + 20, 시야 기준점 capsule 중심). translation 뒤 world box로 `GetCameraClearancePushCm(…, CameraOrigin, F)`를 구해 0보다 크면 translation += F × push. 그 다음 같은 `IsCandidateSafe`.
  3. Overhead, 4. FinalStack: 현재 코드 그대로.
- `TryMakeLayout`은 단계별 translation 계산만 바꿔 끼우는 형태로 정리한다(callback 또는 단계 인자). 안전 검사 코드를 단계마다 복제하지 않는다.
- 환경·손님·시야 검사는 player와 상자를 무시한다(현재). 따라서 플레이어 몸 자리와 겹치는 시선 앞 자리는 허용된다(P9).
- 아래 방향 여유는 시선 앞 단계에도 두지 않는다. 원래 extent 밑면의 바닥 overlap은 기각되고, 튐으로 아래로 밀리면 바닥 collision과 CCD가 받는다(기존 Escape Guard와 같음).
- 크기: 현재 391줄. 시선 기하는 4.1 helper에 두고 이 파일에는 단계 orchestration만 더한다. 500줄을 넘기지 않는다.

`Private/Shop/ShopUnboxingTransaction.cpp`

- request를 `Context.CameraOrigin`, `Context.CameraDirection.GetSafeNormal()`, 기존 발바닥(Pawn 위치 − capsule 반높이), Settings의 세 거리와 겹침 깊이로 채운다. `CameraOrigin.ContainsNaN()`이면 기존 `MissingUnboxContext` 실패.
- 나머지(seed `FMath::Rand()`, 생성·활성화·rollback·consume 순서)는 바꾸지 않는다.

`Public/Shop/ShopSettings.h`, `Private/Shop/ShopSettings.cpp`

- 추가(Config, EditAnywhere, Category `Shop`, 기존 property 뒤에 선언):
  - `UnboxViewDistanceCm` = 100, `meta=(ClampMin="0.0")`. 주석(tooltip): 카메라에서 무리의 가장 가까운 부분까지 시선 방향 거리, 1단계 시선 앞.
  - `UnboxViewMinDistanceCm` = 30, `meta=(ClampMin="0.0")`. 주석: 시선 당김 하한.
- getter: `GetUnboxViewMinDistanceCm()` = 유한이면 max(0, v), 아니면 30. `GetUnboxViewDistanceCm()` = 유한이면 max(하한, v), 아니면 max(하한, 100).
- `UnboxForwardDistanceCm`는 이름·기본값·getter 유지, 주석만 "바닥 정면 단계(2단계) 발바닥 기준 수평 중심 거리"로 바꾼다.

### 4.3 봉투 (Cleaning)

`Private/Cleaning/TrashBagDropPlacement.h/.cpp`

```cpp
enum class ETrashBagDropStage : uint8 { None, ViewFront, FloorFront };

struct FTrashBagDropRequest
{
	FVector CameraOrigin = FVector::ZeroVector;
	FVector CameraDirection = FVector::ForwardVector;
	float ViewDistanceCm = 60.0f;            // TieViewDistanceCm
	float ViewMinDistanceCm = 30.0f;         // TieViewMinDistanceCm
	float FloorForwardDistanceCm = 60.0f;    // TieForwardDistanceCm (의미 유지)
	float FloorMinForwardDistanceCm = 30.0f; // TieMinForwardDistanceCm (의미 유지)
};

static bool Find(UWorld& World, APawn* UserPawn, const TArray<AActor*>& IgnoredActors,
	TSubclassOf<ATrashBagActor> BagClass, const FTrashBagDropRequest& Request,
	FTransform& OutTransform, ETrashBagDropStage* OutStage = nullptr);
```

- 카메라는 request에서만 읽는다. `UCameraComponent` 조회를 제거한다. capsule은 바닥 정면 발바닥과 시야 기준점에만 쓴다.
- 회전 = 플레이어 Actor yaw(현재). shape·template = `ATrashBagActor::BuildClassCollisionQuery`(현재), root→bounds 중심 offset 역산도 현재 방식.
- 후보 하나의 검사(두 단계 공통 함수 하나): 후보 shape와 겹치는 `AWaterStainActor`·`ALitterActor`를 무시 목록에 더한 뒤, 단계 시야 기준점 → shape 중심 Visibility line trace 막힘 없음, `FacilityPlacementCollision::HasBlockingOverlap`(template 응답이라 Pawn은 막지 않음) 없음. 무시: IgnoredActors(플레이어·집게).
- 단계:
  1. ViewFront: `BuildPullDistances(ViewDistance, ViewMin)`. 봉투 box 하나(`Center`=yaw 회전 기준 bounds 중심, `HalfExtent`=Shape box)로 `ComputeViewFrontTranslation`. 시야 기준점 = `CameraOrigin`.
  2. FloorFront: 현재 규칙 그대로(`BuildPullDistances(FloorForward, FloorMin)`, 중심 = 발바닥 + F × d + up × (half Z + 5), 시야 기준점 capsule 중심). P10 push를 같은 helper로 적용한다(봉투 윗면 약 45cm라 항상 0). F가 0이면 이 단계를 건너뛴다.
- 요청 검증: 벡터 NaN·방향 0·template 실패·`BagClass` 없음이면 false. 거리 값은 호출자가 정리해 넘긴다(아래).
- 성공 시 `OutTransform` scale = template relative scale(현재).

`Public/Cleaning/LitterTongsActor.h`, `Private/Cleaning/LitterTongsActor.cpp`

- 추가(EditDefaultsOnly, BlueprintReadOnly, Category `Litter Tongs`, 기존 Tie 값 뒤): `TieViewDistanceCm` = 60 `ClampMin="1.0"`, `TieViewMinDistanceCm` = 30 `ClampMin="1.0"`. 기존 `TieForwardDistanceCm`·`TieMinForwardDistanceCm`는 이름·값 유지, 의미는 바닥 정면 단계(주석 갱신).
- `ExecuteSecondaryEquipmentUse`: request를 `Context.CameraOrigin`·`Context.CameraDirection`과 네 값으로 만든다. 시선 값 정리: 하한 = 유한이면 max(1, v) 아니면 30, 거리 = 유한이면 max(하한, v) 아니면 max(하한, 60). 바닥 정면 값은 현재처럼 그대로 넘기고 `Find`가 현재 조건(`Minimum <= 0`·`Forward < Minimum`·비유한)이면 바닥 정면 단계만 건너뛴다.
- 실패 문구·성공 뒤 `BagCount = 0` 순서·query는 바꾸지 않는다.
- `IsDataValid`: `TieViewMinDistanceCm < 1` 또는 `TieViewDistanceCm < TieViewMinDistanceCm`이면 오류.
- 크기: 현재 397줄, 약 20줄 추가만 허용.

### 4.4 책임 변화 요약

| 항목 | 판단 |
|---|---|
| 기존 책임 | 개봉 위치 helper(Shop private), 봉투 위치 helper(Cleaning private) |
| 신규 책임 | 카메라 시선 앞 기하와 P10 분리 판정 |
| 상태 owner | 없음(순수 계산). 조정값 authoring owner: 개봉 `UShopSettings`, 봉투 `ALitterTongsActor` CDO |
| 실행 owner | 기존 `FShopUnboxingTransaction::Open`, `ALitterTongsActor::ExecuteSecondaryEquipmentUse` |
| 의존 방향 | Shop → Interaction private helper, Cleaning → Interaction private helper. 두 시스템은 이미 Interaction에 의존하며 새 module 없음 |
| 거부한 대안 | Placement에 helper 추가(346줄 문서, 설비 배치 책임과 무관), 개봉·봉투 단계 orchestration 통합(검사 집합·실패 계약이 달라 분기만 늘어남) |

## 5. lifecycle, rollback, 전역 설정

- 위치 계산은 Actor 생성 전 순수 query라 rollback 대상이 없다. 개봉 transaction의 생성 실패 rollback·consume 순서, 봉투의 성공 뒤 `BagCount = 0`은 그대로다.
- 카메라 값은 입력 press 순간 `BuildContext`가 만든 context 한 벌을 쓴다. 계산 중 다시 읽지 않는다.
- Project·World·Collision·Input·Nav·physics 설정 변경 없음. `Config/DefaultGame.ini` 변경 없음(새 Settings 값은 C++ 기본값 사용, 기존 `UnboxOverlapDepthCm=20` 유지).

## 6. Blueprint/API, Core Redirect, Content

- reflected 추가: `UShopSettings::UnboxViewDistanceCm`, `UnboxViewMinDistanceCm`, `ALitterTongsActor::TieViewDistanceCm`, `TieViewMinDistanceCm`. 추가만이며 rename·삭제 없음, Core Redirect 불필요.
- 기존 property 의미 변경 없음(주석만 바꿈).
- Content 변경 없음. `BP_LitterTongs`는 새 property를 C++ 기본값으로 상속하므로 resave하지 않는다. Editor 작업 단계는 `PROMPT_UNREAL.md`에 "Content 변경 없음"을 선언하고 `git status`로 Content가 그대로임을 보이면 생략 대상이다(`AGENT_WORKFLOW.md`).
- PIE 뒤 조정은 Project Settings `Bathhouse Shop`(UnboxView*)과 `BP_LitterTongs` Class Defaults(TieView*)에서 한다. 이 조정은 사용자 PIE 이후 별도 지시가 있을 때만이다.

## 7. 구현 금지 범위

- Content·Config 수정, Blueprint graph·asset 값으로 위치 보정, `BP_LitterTongs`·Settings asset resave.
- `FShopUnboxingCluster`, 겹침 깊이 계약, 여유 shape 방향(수평·위만), 머리 위·최후 쌓기, 개봉 항상 성공, 봉투 실패 계약 변경.
- 생성 순간 속도·impulse 추가, Pawn 응답·CCD·depenetration 설정 변경.
- 봉투에 손님(`ECC_Pawn`) 검사 추가(P2 정정 위반), 플레이어 몸과의 겹침을 막힘으로 판정(P9 위반).
- 테스트를 통과시키려고 환경·손님·시야 검사를 완화하거나, 바닥 정면 단계의 작은 무리·봉투 위치(발바닥 + 20, 반높이 + 5, 기존 거리열)를 바꾸는 것.
- 기존 reflected property rename·삭제, 기존 Settings 값의 의미 재사용.
- G 내려놓기, Q 회수, 배송 지점, 다른 생성 경로 수정. `UPlayerCarryComponent`·`UPlayerInteractionComponent`·`UPlayerEquipmentUseComponent` 수정(이미 context에 카메라 값이 있다).
- 새 module·plugin·전역 설정 추가.

설계와 맞지 않는 엔진 동작(예: 카메라 component 방향이 입력 순간 시선과 다름)이 드러나면 우회하지 않고 `QNA_IMPLEMENTATION.md`로 멈춰 보고한다.

## 8. 자동화 테스트 (seed 고정)

공통: 무작위가 있는 테스트는 `FRandomStream`을 고정 seed로 만든다(예 31415, 4242). 분포를 보는 시나리오는 seed 1~20을 돌려 모두 단언한다. 카메라 위치는 테스트 capsule 중심 + (0, 0, 70)처럼 명시하고, 단계는 `OutStage`로 단언한다. Shop fixture 공용 함수는 필요하면 `Tests/ShopUnboxShapeTestSupport.h`로 옮긴다.

### 8.1 신규

`Private/Tests/PlayerViewFrontPlacementAutomationTests.cpp` — `BathhouseSim.Interaction.ViewFrontPlacement.Geometry`

- box 60³ yaw 0, V 수평, O = (0, 0, 166), d = 100 → 중심 (130, 0, 166). yaw 45 → 중심 X = 100 + 30√2.
- pitch 0, ±45, −80, +80, −90에서 결과 box 8꼭짓점 brute force: V 투영 최솟값 = d(±0.01), right·up 투영 범위 중앙 = 0(±0.01).
- `FShopUnboxingCluster::BuildLayout`(seed 4242, 60³ × 4, D 20) 결과에 같은 단언, 카메라가 어느 box 안에도 없음.
- `GetCameraClearancePushCm`: 윗면이 카메라 − 10 아래이면 0, 카메라 높이를 덮는 무리는 push 뒤 수평 투영 최솟값 = 10(±0.01), 이미 앞이면 0.
- `BuildPullDistances`: (100, 30) → 100…30 8개, (100, 0) → 11개 마지막 0, (65, 30) → 65, 55, 45, 35, 30, (30, 30) → [30], (20, 30) → [30], NaN → 빈 배열.

`Private/Tests/ShopUnboxViewFrontAutomationTests.cpp` — `BathhouseSim.Shop.UnboxViewFront.*` (넓은 바닥, `ACharacter` player, 실제 `DA_FacilityPlacement_*` shape, D 20)

| 테스트 | 시나리오 | 단언 |
|---|---|---|
| `.OpenAndRepresentative` | USV-001, 002, 012, 015 | pitch 0, 천장 없음. 샤워기 1개·SHOP-033 구성 4개·설비 5 + 박스 5: seed 1~20 모두 ViewFront, 모든 꼭짓점 V 투영 ≥ 100 − 0.5이고 최솟값 = 100(±0.5), 카메라가 어떤 물품 box 안에도 없음, 샤워기 1개 중심 Z = 카메라 Z(±1). 설비 10개: seed 1~20에서 단계와 무관하게 카메라가 box 안에 없고, ViewFront 결과는 같은 거리 단언, ViewFront 비율을 AddInfo로 기록(과반 미만이면 실패) |
| `.PitchAndFloor` | USV-003~006 | 샤워기 1개 −45·−80·+45(천장 없음), 대표 4개 −45: ViewFront면 d ∈ [30, 100], 원래 밑면 > 바닥 윗면, −80은 중심 수평 거리 < capsule 반지름 + 물품 반폭(몸 자리 허용), +45는 중심 Z > 카메라 Z. 대표 4개 −45가 FloorFront면 최저 밑면 = 발바닥 + 20 |
| `.BlockedStages` | USV-007~010 | 천장 250·pitch +45·4개(고정 seed) → FloorFront, 모든 윗면 + Dc/2 < 천장. 수평 150cm 앞 벽·샤워기 → ViewFront, d < 100, 벽 이쪽(여유 포함). 50cm 앞 벽·4개 → FloorFront 또는 Overhead, 벽 너머 없음. 사방 cage → Overhead 또는 FinalStack |
| `.FloorFrontUnchanged` | 2단계 보존 | 눈높이 판(바닥 위 110~400, 앞 20~400)으로 시선 단계를 막음: 샤워기 1개 FloorFront, XY 중심 = 발바닥 + 100, 최저 밑면 = 발바닥 + 20. D = 0, 8, 20, 30, 50 모두 FloorFront 채택(2026-09-28 재검토 회귀 방지) |
| `.CameraClearance` | USV-013, P10 | (a) 천장 없음, pitch −60, 설비 10개, seed 1~20: 시선 단계는 바닥에 막혀 FloorFront(또는 Overhead). FloorFront면 (윗면 ≤ 카메라 − 10 또는 수평 투영 최솟값 ≥ 10)이고 최저 밑면 = 발바닥 + 20, 수평 투영 최솟값 = 10(±0.5)인 결과가 1번 이상(push 경로 실행 확인). (b) 천장 250, pitch 0, 설비 10개, seed 1~20: 단계와 무관하게 카메라가 box 안에 없음, ViewFront면 V 투영 최솟값 ≥ 하한, FloorFront면 (a)의 분리 조건, 모든 윗면 + Dc/2 < 천장(FinalStack 제외) |
| `.GuestAndSeed` | USV-011, 014 | ViewFront 후보 자리에 손님 `ACharacter`: 결과가 Dc 여유 shape로 손님과 겹치지 않음. 샤워기 3개 같은 seed 두 번 = 같은 결과, 다른 seed = 다른 위치·yaw |
| `.EnvironmentClearance` | P2 여유 | ViewFront 기준 후보 옆 수평 Dc 안 벽, 위 Dc 안 천장 → 그 후보 기각(재배치된 결과가 여유 shape로 막힘 없음) |
| `.RoomPhysics` | USV-021 | 닫힌 방(천장 바닥 위 320, physics world)에서 설비 10개 pitch +45로 transform을 구해 생성·활성화, 3초 tick 뒤 모두 벽 안·바닥 위·천장 아래, 최저 밑면이 바닥 5cm 이내 |

`Private/Tests/CleaningLitterTieViewFrontAutomationTests.cpp` — `BathhouseSim.Cleaning.Litter.TieViewFront` (Cleaning fixture, 바닥 box 추가)

- USV-017: pitch 0 → ViewFront, 중심 X = 60 + 봉투 반폭, Z = 카메라 Z(±0.1), 회전 = 플레이어 yaw.
- USV-018: pitch −70, 후보 자리에 쓰레기·물 얼룩 → ViewFront, 밑면 > 바닥, 쓰레기·얼룩이 막지 않음.
- 손님 `ACharacter`가 후보 자리에 있어도 성공(P2 정정).
- 눈높이 판으로 시선 단계를 막음 → FloorFront, 현재 값(중심 X = 60, Z = 발바닥 + 반높이 + 5).
- USV-019: 앞 50cm 전 높이 벽 → 실패, 실제 `ExecuteSecondaryEquipmentUse`로 `봉투를 놓을 공간이 없음`, BagCount·월드 봉투 수 불변.
- pitch −90(수평 성분 0) → ViewFront 성공, FloorFront 시도 없음.
- `IsDataValid`: TieView 하한 0, 거리 < 하한이면 Invalid.

### 8.2 갱신할 기존 테스트

시그니처 변경과 1단계 변경으로 다음은 고쳐야 한다. 단언 의도는 유지하고 어느 단계를 검증하는지 명시한다.

- `ShopAutomationTests.cpp` `BathhouseSim.Shop.FreshInstallTrashAndUnboxing`: "100cm forward·바닥 + 20" 단언은 시선 단계를 막는 판을 두고 FloorFront로 검증, 열린 바닥은 ViewFront 단언 추가. 벽·위로 쌓기·cage 부분은 request로 바꾸고 단계 단언. transaction 경로의 `OpenContext`에 `CameraOrigin`(player capsule 중심 + (0, 0, 70)) 설정.
- `ShopUnboxingScatterAutomationTests.cpp`: `UnboxingPawnAvoidance`, `UnboxingEnvironmentClearance`, `UnboxingDepthPlacementObservation`, `UnboxingPhysics`(방 테스트)를 request로 바꾼다. "front stage" 단언은 FloorFront를 강제한 상태로 유지하거나 ViewFront 단언으로 명확히 나눈다.
- `CleaningLitterAutomationTestSupport.h` `Context()`: `CameraOrigin`·`CameraDirection`을 `Player.Camera`에서 채운다.
- `CleaningLitterToolAutomationTests.cpp`: `TongsAndBag`의 "blocked tie" 벽을 눈높이까지 덮게 한다. `BagScaleAndFrontDrop`의 `Find` 호출을 request로 바꾸고, 바닥 정면 기대값(Z 30, 10cm 당김 X 50, offset mesh 중심 (60, 0, 55))은 시선 단계를 막는 판을 두어 FloorFront로 검증, "vertical camera" 단언은 ViewFront 성공으로 바꾼다.

## 9. 빌드와 실행 명령

경로는 변수로 둔다. 작업 브랜치 worktree에서 실행한다. C++ 빌드는 다른 작업과 순차로 진행하므로 마스터가 정한 순서를 따른다. 권한 요구는 `AGENT_WORKFLOW.md` UE 5.8 Build Policy와 같다.

```powershell
$ProjectRoot = 'C:\UnrealProjects\BathhouseSim\.claude\worktrees\unbox'
$Uproject = Join-Path $ProjectRoot 'BathhouseSim.uproject'

& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
  BathhouseSimEditor Win64 Development `
  -Project="$Uproject" `
  -WaitMutex `
  -NoHotReloadFromIDE

& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  "$Uproject" /Engine/Maps/Templates/Template_Default `
  -unattended -nullrhi -NoSplash -NoSound -DDC-ForceMemoryCache `
  -ExecCmds="Automation RunTests <Filter>; Quit" -TestExit="Automation Test Queue Empty" `
  -ReportExportPath="$ProjectRoot\Saved\Automation\Reports\2026-10-01\UNBOX-SPAWN-VIEW-<이름>" -log
```

`<Filter>`는 차례로 실행한다(한 번에 `+`로 이어도 된다).

1. `BathhouseSim.Interaction.ViewFrontPlacement`
2. `BathhouseSim.Shop`
3. `BathhouseSim.Cleaning`
4. 회귀: `BathhouseSim.Service.Shop`, `BathhouseSim.Service.Amenity.Shop`, `BathhouseSim.Interaction.Equipment`(실제 `UPlayerEquipmentUseComponent`로 개봉 성공을 보는 테스트)

빌드 경고 0 증가, 위 필터 전부 통과가 구현 완료 조건이다. 실패·환경 Fatal은 로그 경로와 함께 `PROMPT_REVIEW.md`에 적는다.

## 10. 코드 리뷰 기준

- "가장 가까운 부분" 정의가 helper 한 곳에만 있고 개봉·봉투가 같은 함수를 쓴다.
- 기존 property 의미·기본값 불변, 새 property 4개만 추가, Config·Content diff 없음.
- 바닥 정면 단계의 작은 무리·봉투 결과가 이전 코드와 같다(8.1 `.FloorFrontUnchanged`, 봉투 FloorFront 단언).
- 시선 단계의 시야 기준점은 카메라, 바닥 정면은 capsule 중심, 머리 위는 capsule 윗면 중심.
- 봉투 경로에 Pawn 검사가 없고, 두 경로 모두 플레이어 몸 겹침을 허용한다.
- 카메라 값은 context에서만 읽는다(`FindComponentByClass<UCameraComponent>` 제거).
- `ShopUnboxingPlacement.cpp` 500줄 이하, 안전 검사 단계별 복제 없음.

## 11. 사용자 PIE 관찰 항목

DefaultMap, 기본 캐릭터. 수치는 기대치이고 수용 기준은 "화면 정면에 보임, 카메라를 감싸지 않음, 바닥·벽·천장 안이나 너머가 아님"이다. 천장 높이 조건이 맞는 장소가 없으면 장소 부족으로 기록한다.

| 순서 | 시나리오 | 관찰 |
|---|---|---|
| 1 | USV-001, 002, 015 | 트인 곳에서 수평으로 보고 샤워기 1개·대표 상자·혼합 상자 개봉: 화면 중앙 눈높이 약 1m 앞에 생겨 튀며 떨어짐. 화면 아래 밖에 생기지 않음 |
| 2 | USV-012, 013 | 설비 10개: 높은 천장에서는 화면 정면, 낮은 천장에서는 앞쪽 바닥(화면이 무리에 묻히지 않음) 또는 머리 위 |
| 3 | USV-003~006 | 약 45° 아래, 거의 발밑(−80°), 약 45° 위로 개봉: 화면 중앙 부근에 생김, 바닥에 파고들지 않음, 발밑에서는 몸 자리와 겹쳐도 밀리지 않고 걸어 나감 |
| 4 | USV-007~011 | 낮은 천장 위 보기, 1.5m 앞 벽, 벽에 붙음, 구석, 손님 앞: 천장·벽 너머 없음, 손님 몸 안 생성 없음, 개봉 항상 성공 |
| 5 | USV-014, 016, 021 | 같은 상자 두 번 배치 다름, 생긴 물품 E 들기·배치·버리기, 위에서 떨어진 물품이 맵 밖으로 빠지지 않음 |
| 6 | USV-017~019 | 봉투: 수평으로 보면 화면 중앙 약 60cm 앞, 발 앞(−70°)은 쓰레기·얼룩이 있어도 생김, 벽 앞은 `봉투를 놓을 공간이 없음`과 개수 유지 |
| 7 | USV-020 | G 내려놓기, Q 회수, 배송 도착 위치가 이전과 같음 |

조정이 필요하면 `UnboxViewDistanceCm`·`UnboxViewMinDistanceCm`(Project Settings `Bathhouse Shop`), `TieViewDistanceCm`·`TieViewMinDistanceCm`(`BP_LitterTongs`)의 값과 관찰을 함께 보고한다.
