# PROMPT_IMPLEMENTATION — UNBOX-SPAWN-VIEW 플레이어 앞 생성 위치를 카메라 시선 기준으로

- 작업 ID: `UNBOX-SPAWN-VIEW`
- 단계: 아키텍처
- 상태: 완료

## 0. 재작업 기록

- 2026-10-01 사용자 지시(마스터 전달): "코드상수 자체 작성하지 말고 원본 데이터가 위치한 곳을 참조하도록 해야 해. 값 바꾸면 문서도 바꿔야 하잖아."
- 첫 설계 커밋 `1deaced` 대비 바뀐 범위:
  - 위치 계산과 무리 모양에 쓰이는 수치를 전부 조정값 원본(`UShopSettings`, `ALitterTongsActor`)으로 옮긴다. 순수 helper는 값을 인자로만 받는다(4절, 5절).
  - 이 문서와 Architecture 정본(`ShopSystem.md`, `CleaningLitterSystem.md`, `InteractionSystem.md`)에서 값을 지우고 원본 위치만 적었다.
  - 자동화는 기대값을 설정 원본에서 계산한다(9절).
- 유지: 단계 구조, 시선 앞 기하, P10 방법, 공유 범위, 금지 범위, PIE 시나리오, Content·Config 변경 없음.

## 1. 입력과 단계

- 기능 계약: 같은 폴더 `PROMPT_ARCHITECTURE.md`(USV-001~021, 2026-10-01 사용자 승인, P9·P10·P2 정정 포함), 선택 기록 `QNA_FEATURE_SPEC.md`, Editor 사실 `REPORT_UNREAL_DISCOVERY.md`.
- 단계: 수직 구현 없음(Q6 B). 개봉과 집게 봉투 묶기를 이 작업 하나에서 USV-001~021 전체로 구현·검증한다.
- 작업 위치: worktree `C:\UnrealProjects\BathhouseSim\.claude\worktrees\unbox`, 브랜치 `work/UNBOX-SPAWN-VIEW`. 기능 명세 승인 커밋 `fc5b59f`, 첫 아키텍처 커밋 `1deaced`. "현재 코드 상수"는 `fc5b59f`의 Source를 말한다.
- 정본(이 단계에서 반영): `.md/Architecture/ShopSystem.md` Catalog And Settings·Cluster Layout·World Placement·Transaction·Blueprint/API·Verification, `.md/Architecture/CleaningLitterSystem.md` Tongs·Front Drop Placement·Blueprint/API·Verification, `.md/Architecture/InteractionSystem.md` Source Scope·Player View-Front Spawn Geometry·Dependencies, `.md/0_ARCHITECTURE.md` Shop 요약·상태.

## 2. 목적, 수용 기준, 비목표

목적: 상자 개봉(LMB)과 봉투 묶기(RMB)로 새로 생기는 물체가 입력 순간의 카메라 시선 앞(pitch 포함)에 보이게 한다. 기존 발바닥 기준 정면 규칙은 막혔을 때의 2단계(바닥 정면)로 남긴다.

수용 기준(관찰 결과, `PROMPT_ARCHITECTURE.md` 5·8절):

- 1단계로 생긴 무리(봉투는 봉투 하나)는 카메라에서 시선 방향으로 잰 가장 가까운 부분이 생성 거리 설정값이고, 시선 ray에 좌우·위아래 가운데 정렬된다. 카메라를 감싸거나 닿지 않는다.
- 막히면 시선 당김(하한까지) → 바닥 정면 → (개봉만) 머리 위 → (개봉만) 최후. 봉투는 바닥 정면까지 실패하면 `봉투를 놓을 공간이 없음`이고 아무것도 바뀌지 않는다.
- 1~3단계 모두 벽·바닥·천장·기존 물체 안, 벽·천장 너머에 생기지 않는다. 개봉은 손님 몸 안에도 생기지 않는다. 봉투는 손님을 막는 대상으로 보지 않는다(P2 정정).
- 바닥 정면 단계로 생겨도 무리가 카메라를 감싸지 않는다(P10). 작은 무리와 봉투의 바닥 정면 위치는 현재와 같다.
- 발밑을 보면 플레이어 몸 자리와 겹쳐도 그 자리를 쓴다(P9).
- 생긴 뒤 물리·튐·Pawn 무시·개봉 항상 성공·봉투 실패 계약은 그대로다.
- 모든 동작 수치는 4절 원본에서만 읽는다. 기본값에서의 결과는 현재 코드와 같다(새 단계 제외).

비목표: 생기는 물체의 종류·수량·크기, 무리 모양 규칙(값을 옮기기만 하고 기본값·알고리즘은 그대로), 튐 세기, 추가 속도·연출·HUD 문구, G 내려놓기·Q 회수·배송 지점, 머리 위·최후 쌓기 규칙, pitch 제한·FOV·capsule, depenetration 설정 변경.

## 3. 설계 결정 (기능 명세 12절 "설계에 맡김"과 값 원본)

| 항목 | 결정 | 근거 |
|---|---|---|
| 정렬 기준과 "가장 가까운 부분" | 선 자세 box마다 support function으로 축 투영 범위를 정확히 구한다. 시선 축 V 투영 최솟값 = 생성 거리, 카메라 right·up 축(`FRotationMatrix(V.Rotation())`의 Y·Z) 투영 범위의 중앙 = 카메라 위치 | box 8꼭짓점과 같은 결과를 닫힌 식으로 얻고, 화면 기준 가운데 정렬이 정확하다. AABB 중심 정렬은 pitch가 있을 때 화면 중앙에서 벗어난다 |
| P10 방법 | 바닥 정면 translation 뒤, 무리 윗면 ≤ 카메라 Z − 카메라 여유이면 그대로, 아니면 수평 전방 투영 최솟값이 카메라 여유가 되도록 수평 전방으로 민다. 민 자리도 기존 검사를 통과해야 하며 실패하면 다음 시도 → 머리 위 | 카메라와 무리가 평면 하나로 분리되면 감쌀 수 없다. 윗면 조건으로 작은 무리와 봉투는 현재 위치·당김 그대로다 |
| 시도 횟수와 비용 | 단계별 시도 수는 조정값이다. 시선 앞 시도 수 기본값은 현재 바닥 정면 시도 수(`FrontLayoutAttempts`)와 같다 | 기본값에서 첫 설계 대비 비용 변화 없음. 시도 수를 올리면 query 수가 비례해 늘어난다(정본 World Placement) |
| 조정값 원본 | 개봉: `UShopSettings`(Project Settings `Bathhouse Shop`, `Config/DefaultGame.ini` `[/Script/BathhouseSim.ShopSettings]`). 봉투: `ALitterTongsActor` UPROPERTY(`BP_LitterTongs` Class Defaults). 기존 값 의미는 바꾸지 않는다 | 기능 명세 7절 "조정 위치는 기존 값과 같다". 한 값에 두 의미를 주지 않는다 |
| 값 전달 | 개봉: private `FShopUnboxingTuning::FromSettings(const UShopSettings&)` 한 곳이 모든 값을 읽는다. 봉투: `ALitterTongsActor::BuildTieDropRequest(Context, OutRequest)` 한 곳이 모든 값을 읽는다. helper·placement 함수는 인자로만 받는다 | 원본을 읽는 지점이 시스템마다 하나라 테스트도 같은 함수로 기대값을 만든다 |
| 계산 공유 | 순수 기하(시선 앞 translation, P10 push, 당김 거리열)만 Interaction private helper로 공유한다. 단계 순서·충돌·시야·손님 검사는 각 시스템이 소유한다 | 개봉은 무리·여유 shape·손님 검사·항상 성공, 봉투는 단일 shape·쓰레기/얼룩 무시·실패 반환으로 검사 집합이 다르다 |
| 시야 검사 기준점 | 시선 앞은 카메라 위치, 바닥 정면은 capsule 중심(현재), 머리 위는 capsule 윗면 중심(현재) | 시선 앞은 카메라 기준이다. 바닥 정면은 현재 결과를 바꾸지 않는다 |
| 카메라 입력 | 두 경로 모두 `FHeldEquipmentUseContext::CameraOrigin/CameraDirection`(입력 순간 `UPlayerEquipmentUseComponent::BuildContext`가 카메라 component에서 채움). 봉투의 `FindComponentByClass<UCameraComponent>` 조회를 없앤다 | 입력 순간 값이 기준(5.1)이고, 테스트가 카메라를 명시적으로 줄 수 있다 |

## 4. 값 원본 목록

### 4.1 개봉: `UShopSettings` (`Public/Shop/ShopSettings.h`, Config=Game, Category `Shop|Unboxing`)

- 모두 `UPROPERTY(Config, EditAnywhere)`. 기존 property 뒤에 선언한다.
- 기본값 정본은 header다. 값마다 `static constexpr` `Default<Name>` 상수 하나를 두고 member 초기값과 getter의 비유한 fallback이 그 상수를 같이 쓴다. 기존 `UnboxForwardDistanceCm`·`UnboxOverlapDepthCm` getter의 리터럴 fallback도 이 방식으로 바꾼다(현재 C++ 기본값 그대로).
- 유효 범위는 UPROPERTY `ClampMin`/`ClampMax`와 getter clamp가 같은 선언부에서 정한다. 거리 getter는 하한 이상으로, 간격은 0 초과로, 시도 수·단 수는 1 이상으로 정리한다.
- Config 키는 추가하지 않는다. 키가 없으면 C++ 기본값을 쓰므로 현재 동작이 유지된다.

| property | 의미 | 기본값 원본 |
|---|---|---|
| `UnboxViewDistanceCm` | 1단계 시작 거리: 카메라 → 무리의 가장 가까운 부분(시선 방향) | 기능 명세 7절 "개봉 생성 거리" 새 기본 |
| `UnboxViewMinDistanceCm` | 1단계 당김 하한 | 기능 명세 7절 "개봉 시선 당김 간격·하한"의 하한 |
| `UnboxViewPullStepCm` | 1단계 당김 간격 | 기능 명세 7절 같은 행의 간격 |
| `UnboxViewLayoutAttempts` | 1단계 거리당 layout 시도 수 | 현재 상수 `FrontLayoutAttempts`(ShopUnboxingPlacement.cpp) |
| `UnboxForwardDistanceCm` (기존) | 2단계 바닥 정면 수평 중심 거리. 의미 유지, 주석만 갱신 | 기존 C++ 기본값 |
| `UnboxMinForwardDistanceCm` | 2단계 당김 하한 | 현재 바닥 정면 loop의 하한 리터럴(`FMath::Max(0.0f, …)`) |
| `UnboxForwardPullStepCm` | 2단계 당김 간격 | 현재 바닥 정면 loop의 감소 리터럴 |
| `UnboxForwardLayoutAttempts` | 2단계 거리당 layout 시도 수 | 현재 상수 `FrontLayoutAttempts` |
| `UnboxForwardFloorClearanceCm` | 2단계 최저 바닥면의 발바닥 위 높이 | 현재 상수 `FloorClearanceCm` |
| `UnboxCameraClearanceCm` | P10 카메라 여유 | 신규 값. 초기 기본값은 4.3 |
| `UnboxOverheadClearanceCm` | 3단계 capsule 윗면 위 시작 여유 | 현재 상수 `PlayerTopClearanceCm` |
| `UnboxOverheadStepCm` | 3단계 단 간격 | 현재 상수 `UpperHeightStepCm` |
| `UnboxOverheadStepCount` | 3단계 단 수 | 현재 상수 `UpperHeightSteps` |
| `UnboxOverheadLayoutAttempts` | 3단계 단당 layout 시도 수 | 현재 상수 `UpperLayoutAttempts` |
| `UnboxClusterPlacementAttempts` | 무리: 물품당 배치 시도 수 | 현재 상수 `MaxPlacementAttempts`(ShopUnboxingCluster.cpp) |
| `UnboxClusterMinElevationDegrees`, `UnboxClusterMaxElevationDegrees` | 무리: 고도각 크기 범위(하한 ≤ 상한, 0~90 범위 clamp) | 현재 상수 `MinimumElevationDegrees`, `MaximumElevationDegrees` |
| `UnboxClusterDepthToleranceCm` | 무리: 다른 쌍 깊이 허용 오차 | 현재 상수 `PairDepthToleranceCm` |
| `UnboxClusterPairDepthExtentRatio` | 무리: 쌍 목표 깊이 상한 = 비율 × 두 물품 half extent 최솟값 | 현재 `GetPairTargetDepth`의 비율 리터럴 |
| `UnboxOverlapDepthCm` (기존) | 겹침 깊이 D, 여유 shape `Dc` | 기존(Config 저장값 유지) |

`Private/Shop/ShopUnboxingTuning.h/.cpp`(신규): `struct FShopUnboxingTuning`(위 값 전체, getter로 정리된 값)과 `static FShopUnboxingTuning FromSettings(const UShopSettings&)`. Settings를 읽는 곳은 이 함수와 기존 Settings 사용처(장바구니·배송)뿐이다.

### 4.2 봉투: `ALitterTongsActor` (`Public/Cleaning/LitterTongsActor.h`, EditDefaultsOnly, BlueprintReadOnly, Category `Litter Tongs`)

- 기본값 정본은 header 초기값, 실제 값은 `/Game/Bathhouse/Blueprints/Cleaning/BP_LitterTongs` Class Defaults다. 새 property는 C++ 기본값을 상속한다.
- runtime fallback 상수를 두지 않는다. 단계 값이 무효(비유한, 간격 ≤ 0, 하한 ≤ 0, 거리 < 하한)면 `Find`가 그 단계만 건너뛴다. `IsDataValid`는 새 값의 무효 조합을 오류로 보고한다. 기존 바닥 정면 값의 현재 무효 처리(`Minimum <= 0`, `Forward < Minimum`, 비유한이면 그 단계 없음)는 같다.

| property | 의미 | 기본값 원본 |
|---|---|---|
| `TieViewDistanceCm` | 1단계 시작 거리: 카메라 → 봉투의 가장 가까운 부분 | 기능 명세 7절 "봉투 생성 거리" |
| `TieViewMinDistanceCm` | 1단계 당김 하한 | 기능 명세 7절 "봉투 시선 당김 간격·하한"의 하한 |
| `TieViewPullStepCm` | 1단계 당김 간격 | 기능 명세 7절 같은 행의 간격 |
| `TieForwardDistanceCm` (기존) | 2단계 바닥 정면 수평 거리. 의미 유지, 주석 갱신 | 기존 |
| `TieMinForwardDistanceCm` (기존) | 2단계 당김 하한. 의미 유지 | 기존 |
| `TieForwardPullStepCm` | 2단계 당김 간격 | 현재 `TrashBagDropPlacement.cpp` loop의 감소 리터럴 |
| `TieFloorClearanceCm` | 2단계 봉투 밑면의 발바닥 위 높이(중심 = 발바닥 + half Z + 이 값) | 현재 `CandidateCenter` 식의 더하기 리터럴 |
| `TieCameraClearanceCm` | P10 카메라 여유 | 신규 값. 초기 기본값은 4.3 |

`ALitterTongsActor::BuildTieDropRequest(const FHeldEquipmentUseContext& Context, FTrashBagDropRequest& OutRequest) const`(public, 비 reflected, header는 `struct FTrashBagDropRequest;` 전방 선언): context 카메라 값과 위 값 전체를 채운다. 실행 경로와 자동화가 같이 쓴다.

### 4.3 원본이 없던 신규 값의 초기 기본값

이 표는 아직 원본이 없는 값의 최초 입력이다. 구현 뒤에는 header 초기값이 정본이며, 이후 정본 문서와 이 표를 값 변경 때 고치지 않는다.

| 값 | 초기 C++ 기본값 | 근거 |
|---|---|---|
| `UnboxCameraClearanceCm`, `TieCameraClearanceCm` | 10cm | UE 기본 near clip 거리 수준. 이보다 가까우면 카메라에 닿아 잘려 보인다 |

### 4.4 코드에 남는 수(예외)와 근거

| 수 | 위치 | 근거 |
|---|---|---|
| 부동소수 판정 `KINDA_SMALL_NUMBER`, `IsNearlyZero`, 무리 `AxisEpsilon` | helper·cluster·placement | 동작 조정이 아니라 0 판정과 수치 안정이다 |
| yaw 무작위 범위(전체 원), 고도 부호(±) | cluster | 회전 정의역이다 |
| 여유 shape의 `Dc/2`, 투영 범위 중앙의 1/2 | placement·helper | 밑면을 고정한 채 위로 Dc만큼 키우는 중심·반높이 기하, 중앙점 정의다 |
| support function 계수(1, `|Axis·local_k|`) | helper·cluster | box 투영 정의다 |
| 설정 유효 범위(ClampMin·ClampMax, getter clamp, 고도각 0~90) | `UShopSettings`, `ALitterTongsActor` 선언부 | 설정 정의의 일부이며 한 선언부에만 있다 |
| `Default*` 상수 | `UShopSettings` | 기본값 정본 자체다 |
| 테스트 seed, 허용 오차, fixture 기하 | Tests | 제품 값이 아니다. fixture 위치는 설정값 기준 상대 배치로 만든다(9절) |

범위 밖(이번 작업에서 손대지 않음): `ShopUnboxingPlacement.cpp`·`ShopUnboxingCluster.cpp` 밖의 Shop 상수(배송 Tick 간격, 배송 지점 `MaxSearchHeightCm`·`DropGapCm` 기본값 등)와 다른 시스템의 상수.

## 5. 대상 파일과 책임 변화

### 5.1 신규: 공유 순수 기하

`Source/BathhouseSim/Private/Interaction/PlayerViewFrontPlacement.h/.cpp` (world·UObject 접근 없음, 조정값 없음. 정본 `InteractionSystem.md` Player View-Front Spawn Geometry)

```cpp
struct FViewFrontBox
{
	FVector Center = FVector::ZeroVector;      // collision shape 중심
	float YawDegrees = 0.0f;                   // 선 자세, yaw만
	FVector HalfExtent = FVector::ZeroVector;  // local half extent (>= 0)
};

namespace PlayerViewFrontPlacement
{
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
		const FVector& UnitHorizontalForward, float CameraClearanceCm);

	// Start' = max(Start, Min). Start', Start'-Step, ... (> Min) 뒤 마지막 Min 한 번.
	// 비유한 입력이나 Step <= 0이면 빈 배열.
	void BuildPullDistances(float StartCm, float MinCm, float StepCm, TArray<float>& OutDistances);
}
```

- 반환 형태(out param·작은 struct)는 구현이 정해도 되지만 의미는 위와 같아야 한다. 이 파일에 기본값·상수 조정값을 두지 않는다.
- 개봉 바닥 정면의 현재 거리열(시작에서 간격씩, 하한에서 멈춤, 하한 포함)과 봉투 기본값 거리열은 `BuildPullDistances` 결과와 같다. 봉투 시작·하한 차이가 간격의 배수가 아니면 마지막 하한을 한 번 더 시도하는 차이만 생긴다(의도된 정규화).

### 5.2 개봉 (Shop)

`Private/Shop/ShopUnboxingCluster.h/.cpp`

- `BuildLayout(HalfExtents, DepthCm, const FShopUnboxClusterTuning& Tuning, FRandomStream&, OutItems)`. `FShopUnboxClusterTuning`은 `UnboxCluster*` 다섯 값이며 `FShopUnboxingTuning`의 멤버다. 파일 안의 `MaxPlacementAttempts`, 고도각 두 상수, `PairDepthToleranceCm`, `GetPairTargetDepth` 비율 리터럴을 지운다(`AxisEpsilon`은 4.4 예외로 남김).
- 알고리즘과 난수 소비 순서는 바꾸지 않는다. 기본값에서 같은 seed는 이전과 같은 layout이어야 한다.

`Private/Shop/ShopUnboxingPlacement.h/.cpp`

```cpp
enum class EShopUnboxPlacementStage : uint8 { None, ViewFront, FloorFront, Overhead, FinalStack };

struct FShopUnboxingPlacementRequest
{
	FVector CameraOrigin = FVector::ZeroVector;
	FVector CameraDirection = FVector::ForwardVector; // pitch 포함, 내부에서 정규화
	FVector FootLocation = FVector::ZeroVector;
};

static bool FindSpawnTransforms(UWorld& World, AActor& Player, const UCapsuleComponent& PlayerCapsule,
	AShopDeliveryBoxActor& Box, const TArray<FShopUnboxItemShape>& Items,
	const FShopUnboxingPlacementRequest& Request, const FShopUnboxingTuning& Tuning,
	FRandomStream& RandomStream, TArray<FTransform>& OutTransforms, FText& OutFailureReason,
	EShopUnboxPlacementStage* OutStage = nullptr);
```

- 기존 위치 인자(`FootLocation`, `ViewYaw`, `ForwardDistanceCm`, `OverlapDepthCm`)를 request·tuning으로 옮긴다. 기존 시그니처는 남기지 않는다(호출자는 transaction과 테스트뿐).
- 파일 상단 상수(`FrontLayoutAttempts`, `UpperLayoutAttempts`, `UpperHeightSteps`, `UpperHeightStepCm`, `PlayerTopClearanceCm`, `FloorClearanceCm`)와 loop 리터럴을 지우고 tuning 값을 쓴다. 여유 shape는 `Tuning`의 겹침 깊이(getter에서 이미 범위 정리됨)를 그대로 `Dc`로 쓰고 별도 clamp 리터럴을 두지 않는다.
- 입력 검증: request 벡터 NaN, 방향 0이면 기존 `InvalidUnboxRequest` 실패.
- 수평 전방 F = (V.X, V.Y, 0) 정규화. 0에 가까우면 바닥 정면 단계를 건너뛴다. 물품 half extent probe의 yaw는 F의 yaw(없으면 0)를 쓴다(값에 영향 없음).
- 단계(처음 성공에서 반환하고 `OutStage`에 기록). 상세 규칙 정본은 `ShopSystem.md` World Placement다.
  1. ViewFront: `BuildPullDistances(UnboxViewDistanceCm, UnboxViewMinDistanceCm, UnboxViewPullStepCm)`의 d마다 layout 최대 `UnboxViewLayoutAttempts`회. layout → `ComputeViewFrontTranslation(…, CameraOrigin, V, d)` → 물품마다 기존 `IsCandidateSafe`(여유 shape 수평·위만, 환경, 손님 `ECC_Pawn`, 시야). 시야 기준점 = `CameraOrigin`.
  2. FloorFront: 기존 정면 단계(`BuildPullDistances(UnboxForwardDistanceCm, UnboxMinForwardDistanceCm, UnboxForwardPullStepCm)`, d마다 `UnboxForwardLayoutAttempts`회, `GetClusterTranslation`으로 XY 중심 = 발바닥 + F × d, 최저 바닥면 = 발바닥 + `UnboxForwardFloorClearanceCm`, 시야 기준점 capsule 중심). translation 뒤 world box로 `GetCameraClearancePushCm(…, CameraOrigin, F, UnboxCameraClearanceCm)`를 구해 0보다 크면 translation += F × push. 그 다음 같은 `IsCandidateSafe`.
  3. Overhead(`UnboxOverhead*`), 4. FinalStack: 현재 규칙 그대로, 값만 tuning에서 읽는다.
- `TryMakeLayout`은 단계별 translation 계산만 바꿔 끼우는 형태로 정리한다(callback 또는 단계 인자). 안전 검사 코드를 단계마다 복제하지 않는다.
- 환경·손님·시야 검사는 player와 상자를 무시한다(현재). 따라서 플레이어 몸 자리와 겹치는 시선 앞 자리는 허용된다(P9).
- 아래 방향 여유는 시선 앞 단계에도 두지 않는다(정본 World Placement 근거).
- 크기: 현재 391줄. 시선 기하는 5.1, 값 읽기는 `ShopUnboxingTuning`에 두고 이 파일에는 단계 orchestration만 더한다. 500줄을 넘기지 않는다.

`Private/Shop/ShopUnboxingTransaction.cpp`

- request를 `Context.CameraOrigin`, `Context.CameraDirection.GetSafeNormal()`, 기존 발바닥(Pawn 위치 − capsule 반높이)으로 채우고 tuning은 `FShopUnboxingTuning::FromSettings(*Settings)`로 만든다. `CameraOrigin.ContainsNaN()`이면 기존 `MissingUnboxContext` 실패.
- 나머지(seed `FMath::Rand()`, 생성·활성화·rollback·consume 순서)는 바꾸지 않는다.

`Public/Shop/ShopSettings.h`, `Private/Shop/ShopSettings.cpp`: 4.1의 property·`Default*` 상수·getter. `UnboxForwardDistanceCm` 주석은 "2단계 바닥 정면의 발바닥 기준 수평 중심 거리"로 바꾼다.

### 5.3 봉투 (Cleaning)

`Private/Cleaning/TrashBagDropPlacement.h/.cpp`

```cpp
enum class ETrashBagDropStage : uint8 { None, ViewFront, FloorFront };

struct FTrashBagDropRequest
{
	FVector CameraOrigin = FVector::ZeroVector;
	FVector CameraDirection = FVector::ForwardVector;
	float ViewDistanceCm = 0.0f;          // TieViewDistanceCm
	float ViewMinDistanceCm = 0.0f;       // TieViewMinDistanceCm
	float ViewPullStepCm = 0.0f;          // TieViewPullStepCm
	float FloorForwardDistanceCm = 0.0f;  // TieForwardDistanceCm
	float FloorMinForwardDistanceCm = 0.0f; // TieMinForwardDistanceCm
	float FloorPullStepCm = 0.0f;         // TieForwardPullStepCm
	float FloorClearanceCm = 0.0f;        // TieFloorClearanceCm
	float CameraClearanceCm = 0.0f;       // TieCameraClearanceCm
};

static bool Find(UWorld& World, APawn* UserPawn, const TArray<AActor*>& IgnoredActors,
	TSubclassOf<ATrashBagActor> BagClass, const FTrashBagDropRequest& Request,
	FTransform& OutTransform, ETrashBagDropStage* OutStage = nullptr);
```

- request 멤버 초기값은 0이며 의미 있는 기본값을 갖지 않는다. 값은 항상 `BuildTieDropRequest`가 채운다. 0이면 무효로 그 단계를 건너뛴다.
- 카메라는 request에서만 읽는다. `UCameraComponent` 조회를 제거한다. capsule은 바닥 정면 발바닥과 시야 기준점에만 쓴다.
- 회전 = 플레이어 Actor yaw(현재). shape·template = `ATrashBagActor::BuildClassCollisionQuery`(현재), root→bounds 중심 offset 역산도 현재 방식.
- 후보 하나의 검사(두 단계 공통 함수 하나): 후보 shape와 겹치는 `AWaterStainActor`·`ALitterActor`를 무시 목록에 더한 뒤, 단계 시야 기준점 → shape 중심 Visibility line trace 막힘 없음, `FacilityPlacementCollision::HasBlockingOverlap`(template 응답이라 Pawn은 막지 않음) 없음. 무시: IgnoredActors(플레이어·집게).
- 단계:
  1. ViewFront: 시선 값 세 개로 거리열. 봉투 box 하나(`Center` = yaw 회전 기준 bounds 중심, `HalfExtent` = Shape box)로 `ComputeViewFrontTranslation`. 시야 기준점 = `CameraOrigin`.
  2. FloorFront: 현재 규칙(바닥 정면 값 세 개로 거리열, 중심 = 발바닥 + F × d + up × (half Z + `FloorClearanceCm`), 시야 기준점 capsule 중심). `GetCameraClearancePushCm(…, CameraClearanceCm)` 적용. F가 0이면 건너뛴다.
- 벡터 NaN·방향 0·template 실패·`BagClass` 없음이면 false. 성공 시 `OutTransform` scale = template relative scale(현재).

`Public/Cleaning/LitterTongsActor.h`, `Private/Cleaning/LitterTongsActor.cpp`

- 4.2 property 추가, `BuildTieDropRequest` 추가. 기존 `TieForwardDistanceCm`·`TieMinForwardDistanceCm`는 이름·값 유지, 주석만 바닥 정면 단계로 갱신.
- `ExecuteSecondaryEquipmentUse`: `BuildTieDropRequest(Context, Request)` → `Find`. 실패 문구·성공 뒤 `BagCount = 0` 순서·query는 바꾸지 않는다.
- `IsDataValid`: 새 property의 무효 조합(간격 ≤ 0, 하한 ≤ 0, 거리 < 하한, 여유 < 0)을 오류로 보고한다.
- 크기: 현재 397줄, 약 30줄 추가만 허용.

### 5.4 책임 변화 요약

| 항목 | 판단 |
|---|---|
| 기존 책임 | 개봉 위치·무리 helper(Shop private), 봉투 위치 helper(Cleaning private) |
| 신규 책임 | 카메라 시선 앞 기하와 P10 분리 판정(공유 helper), 값 읽기 지점(`FShopUnboxingTuning::FromSettings`, `BuildTieDropRequest`) |
| 상태 owner | 없음(순수 계산) |
| authoring owner | 개봉 `UShopSettings`(Project Settings·`DefaultGame.ini`), 봉투 `ALitterTongsActor` CDO(`BP_LitterTongs`) |
| 실행 owner | 기존 `FShopUnboxingTransaction::Open`, `ALitterTongsActor::ExecuteSecondaryEquipmentUse` |
| 의존 방향 | Shop → Interaction private helper, Cleaning → Interaction private helper. 두 시스템은 이미 Interaction에 의존하며 새 module 없음 |
| 거부한 대안 | Placement에 helper 추가(설비 배치 책임과 무관), 개봉·봉투 단계 orchestration 통합(검사 집합·실패 계약이 달라 분기만 늘어남), 두 경로 공통 전역 설정(조정 위치는 기존 값과 같아야 함, 기능 명세 7절) |

## 6. lifecycle, rollback, 전역 설정

- 위치 계산은 Actor 생성 전 순수 query라 rollback 대상이 없다. 개봉 transaction의 생성 실패 rollback·consume 순서, 봉투의 성공 뒤 `BagCount = 0`은 그대로다.
- 카메라 값은 입력 press 순간 `BuildContext`가 만든 context 한 벌을 쓴다. 조정값은 입력마다 원본에서 새로 읽는다(Project Settings·BP 변경이 다음 입력부터 반영).
- Project·World·Collision·Input·Nav·physics 설정 변경 없음.

## 7. Blueprint/API, Core Redirect, Content·Config

- reflected 추가: 4.1·4.2 표의 신규 property. 추가만이며 rename·삭제 없음, Core Redirect 불필요. 기존 property 의미 변경 없음(주석만).
- Config 변경 없음: 새 `UShopSettings` property는 `DefaultGame.ini`에 키가 없으면 C++ 기본값을 쓰고, 기본값 = 현재 코드 상수라 동작이 같다. 기존 `UnboxOverlapDepthCm` 저장값은 그대로다.
- Content 변경 없음: `BP_LitterTongs`는 새 property를 C++ 기본값으로 상속하므로 resave하지 않는다. 기존 Tie 두 값은 BP 저장값이 있으면 그대로 쓰인다.
- 따라서 Editor 작업 단계는 `PROMPT_UNREAL.md`에 "Content 변경 없음"을 선언하고 `git status`로 Content가 그대로임을 보이면 생략 대상이다(`AGENT_WORKFLOW.md`).
- PIE 뒤 조정은 Project Settings `Bathhouse Shop`의 `Shop|Unboxing` 값과 `BP_LitterTongs` Class Defaults에서 한다. 조정은 사용자 PIE 이후 별도 지시가 있을 때만 한다.

## 8. 기능 명세 7절 기본값 표와 달라지는 점

명세 문서는 수정하지 않는다. 사용자 결과는 같고 조정 가능 범위만 넓어진다.

- 개봉·봉투 시선 당김 간격이 조정값이 된다(`UnboxViewPullStepCm`, `TieViewPullStepCm`). 첫 설계는 코드 상수였다.
- 명세에서 "현재 값 그대로"인 바닥 정면 값(개봉 당김 간격·하한·바닥 여유·시도 수, 봉투 당김 간격·바닥 여유)과 머리 위 쌓기 값(여유·단 간격·단 수·시도 수)이 조정값이 된다. 기본값은 현재 코드 상수와 같다.
- 명세 7절에 없는 값이 조정값으로 생긴다: 시선 앞 시도 수, P10 카메라 여유(개봉·봉투), 무리 모양 다섯 값. 무리 모양 값은 옮기기만 하며 기본값에서 모양은 같다(명세 11절 비목표 유지).
- 명세 7절의 기본값 수치는 명세 원문이 최초 근거로 남고, 구현 뒤 기본값 정본은 C++ header, 실제 조정값 정본은 `DefaultGame.ini`·`BP_LitterTongs`다.

## 9. 구현 금지 범위

- Content·Config 수정, Blueprint graph·asset 값으로 위치 보정, `BP_LitterTongs`·Settings asset resave.
- 4절 표의 값을 코드 상수·리터럴로 다시 쓰는 것. helper 기본 인자로 값을 넣는 것. 4.4 예외 밖의 새 수치 상수.
- `FShopUnboxingCluster` 알고리즘·난수 소비 순서, 겹침 깊이 계약, 여유 shape 방향(수평·위만), 머리 위·최후 쌓기 규칙, 개봉 항상 성공, 봉투 실패 계약 변경.
- 생성 순간 속도·impulse 추가, Pawn 응답·CCD·depenetration 설정 변경.
- 봉투에 손님(`ECC_Pawn`) 검사 추가(P2 정정 위반), 플레이어 몸과의 겹침을 막힘으로 판정(P9 위반).
- 테스트를 통과시키려고 환경·손님·시야 검사를 완화하거나 바닥 정면 단계의 작은 무리·봉투 결과를 바꾸는 것.
- 기존 reflected property rename·삭제, 기존 Settings 값의 의미 재사용.
- G 내려놓기, Q 회수, 배송 지점, 다른 생성 경로 수정. `UPlayerCarryComponent`·`UPlayerInteractionComponent`·`UPlayerEquipmentUseComponent` 수정(이미 context에 카메라 값이 있다).
- 새 module·plugin·전역 설정 추가.

설계와 맞지 않는 엔진 동작(예: 카메라 component 방향이 입력 순간 시선과 다름)이 드러나면 우회하지 않고 `QNA_IMPLEMENTATION.md`로 멈춰 보고한다.

## 10. 자동화 테스트 (seed 고정, 기대값은 원본에서 계산)

공통:

- 무작위가 있는 테스트는 `FRandomStream`을 고정 seed로 만든다. 분포를 보는 시나리오는 seed 목록(예 1~20)을 돌려 모두 단언한다.
- 기대값은 숫자로 쓰지 않는다. 개봉은 `FShopUnboxingTuning::FromSettings(*GetDefault<UShopSettings>())`, 봉투는 `Tongs->BuildTieDropRequest(Context, Request)`로 얻은 값으로 계산한다. 카메라 위치는 테스트 capsule 중심 + 카메라 offset처럼 fixture에서 명시한다.
- 특정 값이 필요한 테스트(예: 겹침 깊이 범위 순회, 무효 값)는 Settings·CDO 값을 바꾸고 scope guard로 되돌린다. 범위 끝값은 property의 `ClampMin`/`ClampMax` metadata에서 읽는다.
- 벽·천장 fixture는 설정값 기준 상대 위치로 둔다(예: 벽 = 카메라 앞 `UnboxViewDistanceCm` + 무리 깊이의 절반 지점).
- 단계는 `OutStage`로 단언한다. Shop fixture 공용 함수는 필요하면 `Tests/ShopUnboxShapeTestSupport.h`로 옮긴다. 허용 오차는 테스트 상수로 둔다(제품 값 아님).

### 10.1 신규

`Private/Tests/PlayerViewFrontPlacementAutomationTests.cpp` — `BathhouseSim.Interaction.ViewFrontPlacement.Geometry` (순수 helper라 거리·여유·간격은 테스트 입력이다)

- 단일 box에서 yaw 0·45, pitch 0·±45·−80·+80·−90: 결과 box 8꼭짓점 brute force로 V 투영 최솟값 = 입력 거리, right·up 투영 범위 중앙 = 0.
- `FShopUnboxingCluster::BuildLayout`(고정 seed, 설정 원본 tuning) 결과에 같은 단언, 카메라가 어느 box 안에도 없음.
- `GetCameraClearancePushCm`: 윗면이 카메라 − 여유 아래이면 0, 카메라 높이를 덮는 무리는 push 뒤 수평 투영 최솟값 = 여유, 이미 앞이면 0.
- `BuildPullDistances`: 간격 배수·비배수 시작, 시작 = 하한, 시작 < 하한, 하한 0, 간격 ≤ 0·NaN(빈 배열).

`Private/Tests/ShopUnboxViewFrontAutomationTests.cpp` — `BathhouseSim.Shop.UnboxViewFront.*` (넓은 바닥, `ACharacter` player, 실제 `DA_FacilityPlacement_*` shape, 현재 설정 원본)

| 테스트 | 시나리오 | 단언 |
|---|---|---|
| `.OpenAndRepresentative` | USV-001, 002, 012, 015 | pitch 0, 천장 없음. 샤워기 1개·SHOP-033 구성 4개·설비와 박스 혼합: seed 목록 모두 ViewFront, 모든 꼭짓점 V 투영 ≥ `UnboxViewDistanceCm`(허용 오차)이고 최솟값 = 그 값, 카메라가 어떤 물품 box 안에도 없음, 샤워기 1개 중심 Z = 카메라 Z. 장바구니 최대 수량(`CartTotalQuantityLimit`)의 설비: 단계와 무관하게 카메라가 box 안에 없고, ViewFront 결과는 같은 거리 단언, ViewFront 비율을 AddInfo로 기록(과반 미만이면 실패) |
| `.PitchAndFloor` | USV-003~006 | 샤워기 1개 −45·−80·+45(천장 없음), 대표 4개 −45: ViewFront면 d가 시선 거리열 안, 원래 밑면 > 바닥 윗면, −80은 중심 수평 거리 < capsule 반지름 + 물품 반폭(몸 자리 허용), +45는 중심 Z > 카메라 Z. 대표 4개가 FloorFront면 최저 밑면 = 발바닥 + `UnboxForwardFloorClearanceCm` |
| `.BlockedStages` | USV-007~010 | 낮은 천장·위 보기·4개(고정 seed) → FloorFront, 모든 윗면 + Dc/2 < 천장. 시선 앞 거리 안의 벽·샤워기 → ViewFront, d < `UnboxViewDistanceCm`, 벽 이쪽(여유 포함). 가까운 벽·4개 → FloorFront 또는 Overhead, 벽 너머 없음. 사방 cage → Overhead 또는 FinalStack |
| `.FloorFrontUnchanged` | 2단계 보존 | 눈높이 판으로 시선 단계를 막음: 샤워기 1개 FloorFront, XY 중심 = 발바닥 + `UnboxForwardDistanceCm`, 최저 밑면 = 발바닥 + `UnboxForwardFloorClearanceCm`. `UnboxOverlapDepthCm`의 ClampMin·ClampMax와 중간 여러 값에서 모두 FloorFront 채택(2026-09-28 재검토 회귀 방지) |
| `.CameraClearance` | USV-013, P10 | (a) 천장 없음, 급한 아래 시선, 최대 수량 설비, seed 목록: 시선 단계는 바닥에 막혀 FloorFront(또는 Overhead). FloorFront면 (윗면 ≤ 카메라 − `UnboxCameraClearanceCm` 또는 수평 투영 최솟값 ≥ 그 값)이고 최저 밑면 = 발바닥 + `UnboxForwardFloorClearanceCm`, 수평 투영 최솟값 = 여유인 결과가 1번 이상(push 경로 실행 확인). (b) 낮은 천장, pitch 0, 최대 수량 설비: 단계와 무관하게 카메라가 box 안에 없음, ViewFront면 V 투영 최솟값 ≥ 하한, FloorFront면 (a)의 분리 조건, 모든 윗면 + Dc/2 < 천장(FinalStack 제외) |
| `.GuestAndSeed` | USV-011, 014 | ViewFront 후보 자리에 손님 `ACharacter`: 결과가 Dc 여유 shape로 손님과 겹치지 않음. 샤워기 3개 같은 seed 두 번 = 같은 결과, 다른 seed = 다른 위치·yaw |
| `.EnvironmentClearance` | P2 여유 | ViewFront 기준 후보 옆 수평 Dc 안 벽, 위 Dc 안 천장 → 그 후보 기각(재배치된 결과가 여유 shape로 막힘 없음) |
| `.TuningSource` | 값 원본 | Settings 값을 guard로 바꾸면(시선 거리·간격·바닥 여유·머리 위 단 간격·카메라 여유·무리 시도 수) 결과가 바뀐 값을 따른다. 즉 코드에 남은 고정 값이 없음을 확인 |
| `.RoomPhysics` | USV-021 | 닫힌 방(physics world)에서 최대 수량 설비를 위 시선으로 transform을 구해 생성·활성화, tick 뒤 모두 벽 안·바닥 위·천장 아래, 최저 밑면이 바닥 근처 |

`Private/Tests/CleaningLitterTieViewFrontAutomationTests.cpp` — `BathhouseSim.Cleaning.Litter.TieViewFront` (Cleaning fixture, 바닥 box 추가, 기대값은 `BuildTieDropRequest`)

- USV-017: pitch 0 → ViewFront, 중심 = 카메라 + V × (`ViewDistanceCm` + 봉투 반폭), 회전 = 플레이어 yaw.
- USV-018: 발 앞 아래 시선, 후보 자리에 쓰레기·물 얼룩 → ViewFront, 밑면 > 바닥, 쓰레기·얼룩이 막지 않음.
- 손님 `ACharacter`가 후보 자리에 있어도 성공(P2 정정).
- 눈높이 판으로 시선 단계를 막음 → FloorFront, 중심 = 발바닥 + F × `FloorForwardDistanceCm` + up × (half Z + `FloorClearanceCm`).
- USV-019: 앞 전 높이 벽(하한 거리 안) → 실패, 실제 `ExecuteSecondaryEquipmentUse`로 `봉투를 놓을 공간이 없음`, BagCount·월드 봉투 수 불변.
- pitch −90(수평 성분 0) → ViewFront 성공, FloorFront 시도 없음.
- 값 원본: CDO 값을 guard로 바꾸면(시선 거리·간격·바닥 여유·카메라 여유) 결과가 따른다. 무효 조합이면 해당 단계만 건너뛰고 `IsDataValid`가 Invalid.

### 10.2 갱신할 기존 테스트

단언 의도는 유지하고 어느 단계를 검증하는지 명시한다. 기존 숫자 기대값(예: 바닥 정면 거리·바닥 여유·당김 간격·무리 허용 오차·Settings 기본값)은 원본 읽기로 바꾼다.

- `ShopAutomationTests.cpp` `BathhouseSim.Shop.FreshInstallTrashAndUnboxing`: 바닥 정면 단언은 시선 단계를 막는 판을 두고 FloorFront로 검증, 열린 바닥은 ViewFront 단언 추가. 벽·위로 쌓기·cage 부분은 request·tuning으로 바꾸고 단계 단언. transaction 경로의 `OpenContext`에 `CameraOrigin`(player capsule 중심 + 카메라 offset) 설정.
- `ShopUnboxingScatterAutomationTests.cpp`: `UnboxingClusterLayout`(허용 오차·고도각을 tuning에서), `UnboxingSettings` 계열 fallback 단언(`UShopSettings::Default*` 상수와 비교), `UnboxingPawnAvoidance`, `UnboxingEnvironmentClearance`, `UnboxingDepthPlacementObservation`(D 순회는 metadata 범위), `UnboxingPhysics` 방 테스트를 request·tuning으로 바꾼다.
- `CleaningLitterAutomationTestSupport.h` `Context()`: `CameraOrigin`·`CameraDirection`을 `Player.Camera`에서 채운다.
- `CleaningLitterToolAutomationTests.cpp`: `TongsAndBag`의 "blocked tie" 벽을 눈높이까지 덮게 한다. `BagScaleAndFrontDrop`의 `Find` 호출을 `BuildTieDropRequest` 결과로 바꾸고, 바닥 정면 기대값(높이, 당김 한 단계, offset mesh 중심)은 시선 단계를 막는 판을 두어 FloorFront로, request 값에서 계산해 검증한다. "vertical camera" 단언은 ViewFront 성공으로 바꾼다.

## 11. 빌드와 실행 명령

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
  -ReportExportPath="$ProjectRoot\Saved\Automation\Reports\<날짜>\UNBOX-SPAWN-VIEW-<이름>" -log
```

`<Filter>`는 차례로 실행한다(한 번에 `+`로 이어도 된다).

1. `BathhouseSim.Interaction.ViewFrontPlacement`
2. `BathhouseSim.Shop`
3. `BathhouseSim.Cleaning`
4. 회귀: `BathhouseSim.Service.Shop`, `BathhouseSim.Service.Amenity.Shop`, `BathhouseSim.Interaction.Equipment`(실제 `UPlayerEquipmentUseComponent`로 개봉 성공을 보는 테스트)

빌드 경고 0 증가, 위 필터 전부 통과가 구현 완료 조건이다. 실패·환경 Fatal은 로그 경로와 함께 `PROMPT_REVIEW.md`에 적는다.

## 12. 코드 리뷰 기준

- 4절 값이 원본(`UShopSettings`, `ALitterTongsActor`) 밖에 수치로 남아 있지 않다. 남은 수는 4.4 예외뿐이다(`ShopUnboxingPlacement.cpp`, `ShopUnboxingCluster.cpp`, `TrashBagDropPlacement.cpp`, `PlayerViewFrontPlacement.cpp` 리터럴 점검).
- 원본을 읽는 지점이 `FShopUnboxingTuning::FromSettings`와 `BuildTieDropRequest` 둘뿐이다.
- 기본값이 `fc5b59f`의 상수와 같고, 기본값에서 바닥 정면·머리 위·무리 결과가 이전과 같다(`.FloorFrontUnchanged`, 같은 seed 무리, 봉투 FloorFront).
- "가장 가까운 부분" 정의가 helper 한 곳에만 있고 개봉·봉투가 같은 함수를 쓴다.
- 기존 property 의미·기본값 불변, 신규 property 추가만, Config·Content diff 없음.
- 시선 단계의 시야 기준점은 카메라, 바닥 정면은 capsule 중심, 머리 위는 capsule 윗면 중심.
- 봉투 경로에 Pawn 검사가 없고, 두 경로 모두 플레이어 몸 겹침을 허용한다. 카메라 값은 context에서만 읽는다.
- `ShopUnboxingPlacement.cpp` 500줄 이하, 안전 검사 단계별 복제 없음.

## 13. 사용자 PIE 관찰 항목

DefaultMap, 기본 캐릭터. 수용 기준은 "화면 정면에 보임, 카메라를 감싸지 않음, 바닥·벽·천장 안이나 너머가 아님"이다. 거리 기대치는 기능 명세 8절과 현재 설정값을 따른다. 천장 높이 조건이 맞는 장소가 없으면 장소 부족으로 기록한다.

| 순서 | 시나리오 | 관찰 |
|---|---|---|
| 1 | USV-001, 002, 015 | 트인 곳에서 수평으로 보고 샤워기 1개·대표 상자·혼합 상자 개봉: 화면 중앙 눈높이, 가장 가까운 면이 시선 거리 설정만큼 앞에 생겨 튀며 떨어짐. 화면 아래 밖에 생기지 않음 |
| 2 | USV-012, 013 | 최대 수량 설비: 높은 천장에서는 화면 정면, 낮은 천장에서는 앞쪽 바닥(화면이 무리에 묻히지 않음) 또는 머리 위 |
| 3 | USV-003~006 | 비스듬히 아래, 거의 발밑, 비스듬히 위로 개봉: 화면 중앙 부근에 생김, 바닥에 파고들지 않음, 발밑에서는 몸 자리와 겹쳐도 밀리지 않고 걸어 나감 |
| 4 | USV-007~011 | 낮은 천장 위 보기, 가까운 벽, 벽에 붙음, 구석, 손님 앞: 천장·벽 너머 없음, 손님 몸 안 생성 없음, 개봉 항상 성공 |
| 5 | USV-014, 016, 021 | 같은 상자 두 번 배치 다름, 생긴 물품 E 들기·배치·버리기, 위에서 떨어진 물품이 맵 밖으로 빠지지 않음 |
| 6 | USV-017~019 | 봉투: 수평으로 보면 화면 중앙 가까이, 발 앞은 쓰레기·얼룩이 있어도 생김, 벽 앞은 `봉투를 놓을 공간이 없음`과 개수 유지 |
| 7 | USV-020 | G 내려놓기, Q 회수, 배송 도착 위치가 이전과 같음 |

조정이 필요하면 Project Settings `Bathhouse Shop`의 `Shop|Unboxing` 값, `BP_LitterTongs` Class Defaults의 `Litter Tongs` 값을 바꾸고 관찰과 함께 보고한다. 문서는 고칠 필요가 없다.
