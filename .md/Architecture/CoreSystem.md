# Core System

## Responsibilities

Core System은 Source 루트와 모듈 공통 규칙을 문서화한다.

- `BathhouseSim` 런타임 모듈의 빌드 의존성 관리
- Unreal primary game module 등록
- Source/Content/Config 경계 규칙
- Core Redirect 검토 기준
- 시스템 문서 작성 기준

## Source Scope

```text
Source/BathhouseSim/
  BathhouseSim.Build.cs
  BathhouseSim.cpp
  BathhouseSim.h
```

`Core`는 현재 별도 C++ 폴더가 아니라 문서상 공통 경계다.

## Module Rules

`BathhouseSim.Build.cs`는 현재 1인칭 조작, customer gameplay loop와 native/world-space UI에 실제 사용되는 런타임 의존성을 유지한다.

- `Core`
- `CoreUObject`
- `Engine`
- `InputCore`
- `EnhancedInput`

UE 5.8 customer loop 구현은 실제 include/use site와 함께 다음 runtime 의존성을 사용한다.

- `AIModule`
- `GameplayTasks`
- `NavigationSystem`
- `GameplayTags`
- `StateTreeModule`
- `GameplayStateTreeModule`
- `UMG`

Placement는 `UFacilityPlacementSettings`를 위해 runtime `DeveloperSettings` 의존성을 실제 include/use site와 함께 사용한다. typed `FFacilityPlacementPayload`는 CoreUObject 기반의 instanced data UObject로 구성해 신규 module/plugin을 요구하지 않는다. GameplayTags와 NavigationSystem은 기존 의존성을 재사용한다.

Bath Water는 기존 `DeveloperSettings` 의존성으로 전역 입욕 임계치를 노출하고, native `UNiagaraComponent` 급수 표현을 위해 runtime `Niagara` module을 실제 include/use site와 함께 추가한다. 수면 위치 보간과 control 회전은 Engine component/transform API를 사용하며 별도 fluid, physics 또는 animation module을 추가하지 않는다.

`BathhouseSim.uproject`에는 UE 5.8 `StateTree`, `GameplayStateTree` plugin이 활성화되어 있다. Computer의 `UWidgetComponent`, `UWidgetInteractionComponent`와 native sample widget은 기존 `UMG`/`InputCore` 의존성으로 구현되어 있다. direct API 사용처가 없는 `StateTreeEditorModule`, `Slate`, `SlateCore`는 runtime module에 추가하지 않는다.

Editor target 빌드에서만 `UnrealEd`를 private dependency로 추가한다(`if (Target.bBuildEditor)`). 현재 사용처는 `WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS`로 감싼 Blueprint load automation(`Private/Tests/UtilityBoilerBlueprintLoadAutomationTests.cpp`) 하나다. runtime·game target 코드는 `UnrealEd`에 의존하지 않으며, 새 editor-only 사용처도 같은 조건부 include·compile 경계를 지켜야 한다.

## Runtime Entry

`BathhouseSim.cpp`는 `IMPLEMENT_PRIMARY_GAME_MODULE`로 `BathhouseSim` 모듈을 등록한다.

현재 Core System은 게임플레이 상태를 소유하지 않는다. 플레이어 입력, 카메라, 이동, sprint 상태는 Character System이 소유한다.

## Source Boundaries

- `Source/BathhouseSim/Public`에는 외부 모듈 또는 Blueprint native parent가 참조할 수 있는 public header를 둔다.
- `Source/BathhouseSim/Private`에는 cpp 구현과 private helper를 둔다.
- include 경로는 모듈 include root 기준으로 작성한다. 예: `#include "Character/FirstPersonCharacter.h"`
- `#include "Public/..."` 형태는 사용하지 않는다.

## Content And Config Boundaries

- `Content/`는 Blueprint와 asset 데이터이며, 명시 지시 없이 수정하거나 resave하지 않는다.
- `Config/DefaultEngine.ini`는 GameMode/Pawn/Controller 연결, 프로젝트 공통 collision channel 또는 Core Redirect가 필요할 때만 수정한다.
- `Intermediate/`, `Binaries/`, `Saved/`, `DerivedDataCache/`는 문서 기준이나 설계 판단의 출처로 사용하지 않는다.

## Core Redirect Policy

UCLASS/USTRUCT/UENUM rename 또는 삭제가 Blueprint/native asset 참조를 깨뜨릴 수 있으면 `Config/DefaultEngine.ini`의 `[CoreRedirects]`를 검토한다.

Core System은 고정된 native class inventory를 유지하지 않는다. 구현이 변해도 이 문서는 모듈, Source 경계, redirect 판단 기준만 제공한다.

현재 native type과 Blueprint/API 계약은 각 `.md/Architecture/*System.md`의 `Key Classes`, `Blueprint/API Contracts`를 기준으로 확인한다. 외부 asset 또는 이전 이식 단계의 Blueprint가 예전 type 이름을 참조한다면, Editor migration 전 Core Redirect 설계가 필요하다.

- Core Redirect는 이름·경로 변경만 해결한다. serialized export가 있는 UCLASS의 이름을 유지한 채 native 부모 class를 바꾸거나 custom native `Serialize` 형식을 바꾸면 기존 asset이 serial size mismatch로 로드되지 않으며 redirect로 복구할 수 없다.
- 이런 변경은 기존 class·subobject layout을 유지하고 새 class·새 subobject를 추가하는 방식으로 설계한다. 기존 요소의 제거는 모든 asset이 새 구조로 저장·재로드된 뒤 별도 단계로 다룬다.
- 기존 export를 가진 native class 변경은 Editor 작업 전에 해당 asset의 파일 복사본을 비저장 로드해 Fatal이 없음을 먼저 확인한다. 시작 맵이 그 asset을 참조하면 엔진 기본 템플릿 맵으로 headless 검증한다.

## System Documents

현재 구현 시스템 문서는 실제 Source 디렉터리에 맞춰 작성하고 확정 target 문서는 implementation status를 명시한다.

- `CharacterSystem.md`: `Source/BathhouseSim/Public/Character`, `Source/BathhouseSim/Private/Character`
- `CameraSystem.md`: `Source/BathhouseSim/Public/Camera`, `Source/BathhouseSim/Private/Camera`
- `InteractionSystem.md`: player trace, primary/secondary intent와 equipment-use 경계
- `PhysicalCarrySystem.md`: Interaction Source 안의 fixed slot, free-drop transaction과 physical item recovery 경계
- `FacilitySystem.md`: facility slot과 counter queue 경계
- `BathWaterSystem.md`: 욕탕 급수·배수 상태, control/수면 표현과 Customer 입욕 가능성 경계
- `BathWaterOperationsSystem.md`: utility 용량 원장과 욕탕 수온·오염도 domain 경계
- `BathWaterManagementUISystem.md`: computer management context, Zone 지도와 native Widget 계층
- `UtilityLaborSystem.md`: `Public/Utility`, `Private/Utility`의 설비 노동 hub(계층·Operation·용량·계기·회수).
- `UtilityFuelSystem.md`: Utility 하위 재료·공급함·삽·연료 설비·투입 Volume·문 경계.
- `UtilityLeverSystem.md`: Utility 하위 순환기 조작부·레버 왕복 경계.
- `ShopSystem.md`: `Public/Shop`, `Private/Shop`의 상품 목록·장바구니·주문·배송·상자·개봉·쓰레기통 경계.
- `PlacementSystem.md`: 설비 mode/preview/placement/recovery, 확장 단계와 락커 capacity lease 경계
- `EconomySystem.md`: wallet과 cash claim 경계
- `CustomerSystem.md`: StateTree routine과 customer session 경계
- `UISystem.md`: native Widget/Widget Blueprint 경계
- `CleaningSystem.md`: water stain spawn과 wet mop cleaning 경계
- `TowelSystem.md`: towel inventory, atomic transfer와 processing 경계
- `TowelPresentationSystem.md`: Towel 하위 Stack/Pile/Slot world presentation 경계
- `ComputerSystem.md`: world monitor, focus/input session과 sample screen 경계
- `CombatSystem.md`: 범용 equipment use, melee attack과 health 경계
- `CustomerRecoverySystem.md`: customer knockdown, soft interruption과 restartable StateTree Task 경계
- `CoreSystem.md`: Source 루트, 모듈/문서/redirect 공통 규칙

`Source/BathhouseSim/Private/Tests`는 system이 아니라 focused native automation test 경로다.

새 Source 하위 디렉터리를 추가하면 같은 이름의 `*System.md`를 추가하고 책임, 핵심 클래스, runtime flow, 의존성, Blueprint/API 계약, 수동 검토 지점을 문서화한다.

Cleaning/Towel/Computer, Combat/Customer Recovery, Physical Carry와 Bath Water Operations는 현재 runtime module dependency 안에서 구현한다. physics, curve, collision, AI/Navigation, StateTree, UMG와 DeveloperSettings는 이미 선언된 dependency를 사용하며 새 dependency는 실제 include/use site가 확인되지 않는 한 추가하지 않는다.

## Class Growth Policy

- Actor/Character는 default subobject 조립, reflected getter와 상위 flow만 담당한다.
- 독립적인 타이머, Tick, delegate lifecycle, physics snapshot과 transaction guard는 응집된 Component/Subsystem으로 분리한다.
- input Character에 damage, cleaning, carry, computer와 customer routine 상태를 복제하지 않는다.
- `UCustomerSessionComponent`는 domain resource/timer owner로 유지하고 ragdoll physics와 StateTree pause/restart lifecycle은 신규 Customer component에 둔다.
- queue MoveTo/도착 회전/overflow wander와 recovery gate의 async lifecycle은 `UCustomerSessionComponent`나 이미 400줄을 넘은 `CustomerStateTreeTasks` 구현에 누적하지 않고 `UCustomerQueueNavigationComponent`와 `CustomerQueueStateTreeTasks` 파일로 분리한다. Counter는 FIFO/assignment owner로만 유지한다.
- `UPlayerInteractionComponent`는 focus/query/result 표시 경계를 유지하고 concrete weapon/cleaning mutation은 equipment actor와 domain owner에 위임한다. 이미 500줄을 넘었으므로 추가는 commit 지점의 generic focus observer 알림에 한정하고 concrete target 판별·표현 상태·Tick을 넣지 않는다.
- 모든 소지품을 통합하는 공통 physical carry Actor/Component는 만들지 않고 `IPhysicalCarryable`을 유지한다. Placement 전용 `APlaceableFacilityItemActor`는 허용하되 다른 item domain의 기반 클래스로 확장하지 않는다. generic fixed slot은 world interaction Actor로, carry reference commit owner는 `UPlayerCarryComponent`에 두고 Actor 교체와 snapshot/rollback mechanics는 Placement의 private non-UObject helper로 분리한다.
- 재사용 가능한 held motion은 carry 소유권과 분리된 표현 Component로 유지한다.
- 설비 placement/recovery의 session·preview·rollback은 `UPlayerFacilityPlacementComponent`에 두고 contents/water/slot 조건은 원래 domain owner가 판정한다.
- 범용 Facility Actor에 물 control·표현을 누적하지 않는다. 기존 `UBathWaterStateComponent`는 authoritative water owner로 확장하고 욕탕 전용 Actor가 control, 수면과 Niagara를 조립한다.
- 300줄을 넘은 `UBathWaterStateComponent`에는 수온·오염도·용량 원장을 누적하지 않는다. 이 component는 실제 유입·유출 flow sample만 추가하고, 물 condition은 `UBathWaterConditionComponent`, world aggregate는 `UBathWaterOperationsSubsystem`으로 분리한다.
- 순환기·보일러·쿨러는 customer slot을 가진 범용 `ABathhouseFacilityActor`에 조건 분기를 추가하지 않고 placement/recovery 계약을 구현한 독립 utility Actor로 둔다.
- utility base에 연료·계기·문·레버 로직을 누적하지 않는다. Operation과 바늘은 labor intermediate, 투입 Volume·문은 fuel intermediate, 조작부·레버는 circulator가 조립한다. 연료 transaction, pivot 회전 baseline과 owner input guard는 private helper가 맡는다. 문 열림은 전용 표현 component, 레버 왕복은 레버 노동 component가 소유한다.
- 설치 락커 용량, customer lease와 임시 action-slot 후보는 `ULockerCapacitySubsystem`에 두며 Customer Session이나 설비 Actor에 전역 합계를 복제하지 않는다.
- 상점: cart는 PlayerState component, 주문·배송은 world subsystem, 개봉 위치·transaction은 private helper가 맡는다. 이미 600줄을 넘은 `UPlayerCarryComponent`에는 기존 placement 소모를 일반화한 consume commit만 추가하고 상점 판정을 넣지 않는다. Widget은 cart·주문·돈을 보관하지 않는다.

## Manual Review Points

- 새 모듈 의존성을 추가할 때 실제 include/use site가 있는지 확인한다.
- StateTree/GameplayStateTree plugin과 runtime module을 UE 5.8 기준으로 확인한다.
- `DeveloperSettings`가 `UFacilityPlacementSettings` 실제 사용과 일치하고 placement payload가 불필요한 신규 module/plugin을 추가하지 않는지 확인한다.
- `UBathWaterSettings`가 기존 `DeveloperSettings`를 재사용하고 `Niagara` dependency가 실제 native component 사용에만 추가되는지 확인한다.
- Bath Water Operations가 기존 `UMG`, `DeveloperSettings`와 placement payload 경계만 사용하며 새 module을 불필요하게 추가하지 않는지 확인한다.
- UCLASS/USTRUCT/UENUM rename/delete 시 Blueprint 참조와 Core Redirect 필요 여부를 확인한다.
- Config 변경은 실제 gameplay 연결 또는 migration 목적이 분명할 때만 수행한다.
- 문서가 Source 구조와 어긋나면 Source 재대조 후 시스템 문서를 갱신한다.
