# Unreal 사전 조사 보고서 — 급수·배수 시스템과 욕탕 물

## 상태

읽기 전용 조사를 완료했다. UE 5.8 `BathhouseSim` Editor의 MCP asset 조회로 Class Default, Level instance, 관련 mesh/DataAsset/StateTree 연결과 로그를 확인했다. Asset 수정, Blueprint/StateTree Compile 요청, Save와 PIE는 수행하지 않았다.

기능 계약은 `.md/QNA_FEATURE_SPEC.md` Q1~Q27까지 답변됐지만 `.md/PROMPT_ARCHITECTURE.md`의 상태 문구는 아직 Q1~Q19만 반영됐다고 적혀 있다. 기능 명세 단계에서 Q20~Q27을 정본에 반영한 뒤 아키텍처로 넘겨야 한다.

## 조사 기준선과 capability

- Project: `C:/UnrealProjects/BathhouseSim/BathhouseSim.uproject`
- EngineAssociation: `5.8`
- 활성 plugin: `ModelContextProtocol`, `EditorToolset`, `StateTree`, `GameplayStateTree`
- Level: `/Game/Maps/DefaultMap`
- 조사 시작/종료 PIE: `false`
- 조사 시작/종료 시 target package의 Editor dirty 상태: 모두 `false`
- 조사 종료 시 열린 asset 없음. 조사 전용으로 시작한 Editor PID `25804`는 검증 후 종료했고 PID 소멸을 확인했다.
- 사용한 읽기 기능: asset search/dependency/reference/tag, Blueprint parent/CDO/graph/component, UObject property, Level actor/component, StaticMesh bounds/material/collision, Editor/로그 상태 조회
- StateTree 전용 hierarchy 조회 tool은 제공되지 않았다. 따라서 asset schema/reference는 확인했지만 state/task/transition/binding 내부 트리는 MCP로 직접 확인하지 못했다.

Git 작업 트리는 조사 전부터 변경돼 있었으며, 특히 `BP_Bath.uasset`, `NS_HoneyBeam.uasset`, 기능 문서 변경은 사용자 소유 상태로 보존했다. Editor package dirty와 Git modified는 서로 다른 기준이다.

## `/Game/Bathhouse/Blueprints/Facility/BP_Bath`

### Blueprint와 Class Default

- Parent Class: `/Script/BathhouseSim.BathhouseFacilityActor`
- Blueprint 자체 변수: 없음
- Construction Script: 비어 있음
- Event Graph: 연결되지 않은 기본 Event 선언만 존재
- 직접 dependency: `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Bath`, `/Game/Bathhouse/Meshes/Bath/SM_Bath_old`, `/Script/BathhouseSim`
- `Bath_01` body/details/hardware/water mesh와 Niagara는 현재 `BP_Bath` dependency가 아니다.

현재 CDO component 구성:

```text
PackagePhysicalRoot (root, Box)
└─ SceneRoot
   ├─ FacilityVisual (StaticMesh)
   ├─ FacilitySlotA
   ├─ FacilitySlotB
   ├─ FacilitySlotC
   └─ PlacementFootprint (Box)

FacilityPlacement (ActorComponent)
BathWaterState (ActorComponent)
```

급수밸브, 배수레버, 물 표면, Niagara, 급수·배수용 별도 interaction target component는 존재하지 않는다.

### 현재 시각·충돌·배치 값

- `FacilityVisual`: `/Game/Bathhouse/Meshes/Bath/SM_Bath_old`, Relative Scale `(3,3,3)`, visible, Movable
- CDO `FacilityVisual` collision: `BlockAllDynamic`, `QueryAndPhysics`, WorldDynamic, Navigation relevant
- `PlacementFootprint`: Extent `(150,120,38)`, Relative Location `(0,0,38)`, NoCollision, Navigation 비활성
- 기존 `.md/Unreal/PlacementSystem.md`의 Bath footprint X extent `145`와 실제 CDO `150`은 불일치한다.
- `BathWaterState`: `Empty`, `NormalizedAmount=0`, tick 비활성, auto activate 비활성
- Facility Type `Bath`, Facility Number `-1`, enabled `true`
- `DA_FacilityPlacement_Bath`: Stable ID `Facility.Bath`, tag `Facility.Placeable`, placed class `BP_Bath`, recovery item class `BP_PlaceableFacilityItem`, recovery mesh `None`, locker slots `0`

### 슬롯

- 세 슬롯의 CDO Action 위치: A `(0,-60,2.5)`, B `(0,0,2.5)`, C `(0,60,2.5)`
- CDO Approach Offset: `(150,0,-72.5)`
- Facing Rotation: Yaw `180`
- 세 슬롯 모두 enabled/Available이며 Navigation에는 관여하지 않는다.

## `DefaultMap`의 선배치 욕탕

두 인스턴스가 존재한다.

- `Bath`: Actor Location `(1842,0,0)`, `FacilityVisual=SM_Bath_old`, visual Relative Yaw `-90`, Scale `(3,3,3)`, 수위 `Empty/0`
- `Bath2`: Actor Location `(1750,-638,0)`, `FacilityVisual=None`, visual Relative Yaw `0`, Scale `(3,3,3)`, 수위 `Empty/0`
- `Bath2`의 세 슬롯 Approach Offset은 `(150,0,0)`으로 CDO의 Z `-72.5`와 다르다.
- 두 인스턴스의 `FacilityVisual` collision response에서 `Visibility`와 `Camera`가 `Ignore`로 저장돼 있다.
- 두 인스턴스 모두 별도 valve/drain/water/Niagara component가 없다.

따라서 첫 수직 구현 전에 `Bath2`의 누락 mesh와 Approach override를 유지할 의도인지 migration 오류인지 결정해야 한다. 상호작용 trace가 `Visibility`를 사용한다면 현재 욕탕 visual 자체는 hit target이 될 수 없으므로 신규 조작부는 명시적인 interaction hit collision을 가져야 한다.

## 신규 욕탕 mesh 후보

현재 `/Game/Bathhouse/Meshes/Bath/Bath_01/StaticMeshes`에는 다음 4개가 있다.

| Asset | Local Bounds Min | Local Bounds Max | 확인 결과 |
|---|---:|---:|---|
| `SM_Bath_01_Body` | `(-228.3,-178.3,0)` | `(228.3,264.25,161.2)` | 본체 후보 |
| `SM_Bath_01_Details` | `(67.5,51.5,32.6)` | `(124.5,178.56,80.75)` | 세부 외관 후보 |
| `SM_Bath_01_Hardware` | `(-155.5,-126.1,35.2)` | `(128,247.5,180.7013)` | hardware가 하나의 결합 StaticMesh |
| `SM_Bath_01_Water` | `(-181.25,-131.25,86.5)` | `(181.25,131.25,87.1165)` | 물 표면 후보 |

- Water mesh material slot: `M_Water_Turquoise`, `M_Water_Ripple`
- asset 이름 `Valve` 또는 `Drain`으로 검색되는 별도 project asset은 없다.
- Hardware가 하나의 결합 mesh라서 현재 asset 그대로는 급수밸브와 배수레버를 서로 독립적으로 회전시키는 두 component source가 되지 못한다.
- 네 mesh의 bounds는 공통 authored coordinate frame을 사용한 정황이 있으나, bounds만으로 실제 pivot과 의도한 회전축을 확정할 수 없다.
- 0%·100% 물 위치와 0% 숨김 상태는 현재 `BP_Bath`에 authoring돼 있지 않다. 수직 구현용 CDO/instance 값을 새로 authoring해야 한다.

## Niagara

- 조사 후보 `/Game/Niagaras/NS_HoneyBeam`은 현재 referencer가 0이며 `BP_Bath`에 연결돼 있지 않다.
- 이 System은 2개 active emitter와 2개 mesh renderer를 사용하고 `/Game/Material/M_HoneyBeam`, `/Game/Prop/SM_Cylinder`에 의존한다.
- 이름, material과 renderer 구성상 급수 물줄기용으로 확정할 근거가 없다. 사용할 System을 별도로 지정하거나 이 asset을 물줄기용으로 authoring할지 Editor 단계 입력에서 명시해야 한다.
- 현재 욕탕에는 Niagara component와 relative transform/auto-activate 값이 없으므로 기본 비활성 계약도 새로 authoring해야 한다.

## 고객 루틴과 StateTree 연결

- `/Game/Bathhouse/Blueprints/Customer/BP_BathhouseCustomerAIController`
  - Parent: `/Script/BathhouseSim.BathhouseCustomerAIController`
  - `CustomerStateTree`는 brain component이며 `/Game/Bathhouse/AI/ST_CustomerRoutine`을 참조
  - linked tree override 없음
  - `bStartLogicAutomatically=false`, AIController `bStartAILogicOnPossess=true`
- `ST_CustomerRoutine` asset schema tag: `/Script/GameplayStateTreeModule.StateTreeAIComponentSchema`
- Editor load 로그에서 `ST_CustomerRoutine` compile 성공을 확인했다.
- asset dependency에는 native BathhouseSim/StateTree module, customer Blueprint와 montage 자산이 포함된다.

현재 Source에는 `Hold Customer Facility`, `Get Customer Facility Target`, `Snap Customer Facility Point`, `Start Customer Bath Stay`, `BathStayExpired` condition/event와 `BathDwell` 처리 Task가 존재한다. `UCustomerSessionComponent::TryReserveFacility()`가 시설 선택을 맡고, bath stay 만료 event가 StateTree를 진행시킨다. 수위 유효성, 탐색 구간 제한시간, 실제 입욕 누적시간 계약은 현재 native API/DataAsset에 없다.

`DA_CustomerRoutine_Default`의 실제 관련 값:

- `BathStayDurationSeconds=10`
- `BathDwellMinSeconds=10`, `BathDwellMaxSeconds=20`
- `FacilityRetryIntervalSeconds=0.5`, `MaxNavigationRetries=3`
- `PreShowerSeconds=5`, `MainShowerSeconds=5`

승인 기능 계약과 `CustomerSystem.md`는 전체 bath stay를 60초로 전제하지만 실제 DataAsset은 10초다. 수직 구현 전에 60초로 migration해야 한다. MCP 기능 부족으로 `ST_CustomerRoutine` 내부 state/task/binding 배치는 확인하지 못했으므로, 이후 Editor 작업 프롬프트에는 기존 bath branch를 직접 열어 대조하는 명시적 절차가 필요하다.

## 로그 판정

- MapCheck: 오류 0, 경고 0
- `ST_CustomerRoutine` compile: 성공
- startup/조회 로그에서 프로젝트 Blueprint compile error, StateTree binding error, missing component/property load error 또는 Ensure는 확인되지 않았다.
- 조사 중 잘못된 controller asset 경로와 존재하지 않는 property 이름을 조회해 발생한 `LogScript` 경고는 MCP 조사 요청 오류이며 프로젝트 asset 오류가 아니다.
- 플랫폼 SDK, profiling DLL과 UE UnifiedErrorTest 메시지는 이번 기능과 무관한 실행 환경 로그다.

## 아키텍처 단계에 넘길 결론

1. 현재 `BathWaterState`는 enum/normalized amount 저장용 최소 component일 뿐이며 연속 수위 처리, 조작부 상태, 표현 binding, 이용 가능성 publication을 아직 제공하지 않는다.
2. 첫 수직 구현은 기존 `FacilityVisual` 하나를 전제로 하면 안 되며 Body/Details/Hardware/Water와 두 독립 조작부를 명시적으로 authoring해야 한다.
3. 기존 combined Hardware에서 회전 부품을 분리한 mesh 또는 별도 회전용 표현 asset이 필요하다. pivot/local axis는 Editor에서 실제 분리 asset을 기준으로 확정해야 한다.
4. Water mesh의 0%·100% transform, Niagara System/transform과 interaction hit collision은 모두 미작성 상태다.
5. 손님 수위 필터는 기존 Facility subsystem의 슬롯 배타성을 보존하면서 예약 전, 이동 중, 입장 직전과 사용 중 하락을 같은 authoritative 수위 기준으로 처리해야 한다.
6. 전체 60초 timer와 실제 입욕 누적 timer를 분리해야 하며 knockdown pause와 욕탕 강제 퇴장/재탐색 경계를 기존 session interruption 계약에 연결해야 한다.
7. StateTree asset 내부 편집량은 직접 조사하지 못했으므로 native 확장 후 Unreal Editor 단계에서 실제 bath branch/task binding 확인을 필수 검증으로 둔다.
8. `Bath2` 누락 visual/Approach override와 DataAsset 10초 값은 기능 구현 전에 migration 대상으로 명시해야 한다.

## 미확정 Editor 사실

- 분리된 급수밸브·배수레버 asset의 실제 pivot, 닫힌 transform과 local 회전축
- 최종 물줄기 Niagara System과 배치 transform
- Water mesh의 최종 0%·100% relative transform
- `ST_CustomerRoutine` 내부 bath state hierarchy, transition과 현재 property binding

이 항목은 현재 asset에 값이 없거나 MCP가 StateTree 내부 구조를 노출하지 않아 추측하지 않았다.
