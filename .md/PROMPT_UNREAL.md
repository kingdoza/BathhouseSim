# Unreal 작업 프롬프트 — 보일러 노동 가동 수직 구현

## 진입 조건과 현재 상태

- 이번 재개에서는 사용자 지시로 코드 리뷰 승인 관문을 건너뛴다. UE 5.8 native build/restart 조건은 유지한다. 이전 MCP 단계에서 BP_Boiler의 FacilityPlacement.Definition만 저장·재로드 확인했으며 나머지 Editor 작업은 하지 않았다.
- 기준 정본: .md/PROMPT_ARCHITECTURE.md, .md/PROMPT_IMPLEMENTATION.md, .md/Architecture/UtilityLaborSystem.md, .md/REPORT_UNREAL_DISCOVERY.md.
- 범위는 LAB-001~025, LAB-037~039다. LAB-026~036은 미구현으로 남긴다.
- Unreal MCP 도구만 사용한다. Computer Use, 셸/바이너리 asset 편집, Save All을 사용하지 않는다. 필요한 MCP 기능이 없으면 정확한 작업을 .md/USER_UNREAL.md에 넘긴다.

## 대상 asset과 저장 allowlist

| 경로 | 작업 |
|---|---|
| /Game/Bathhouse/Blueprints/Facility/BP_Boiler | parent를 /Script/BathhouseSim.BathWaterBoilerFacilityActor로 변경, inherited 설정 보존 |
| /Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Boiler | Placed class BP_Boiler와 기존 BP_PlaceableFacilityItem recovery class 확인 |
| /Game/Bathhouse/Blueprints/Utility/BP_UtilityShovel | 생성, parent /Script/BathhouseSim.UtilityShovelActor |
| /Game/Bathhouse/Blueprints/Utility/BP_CoalSupply | 생성, parent /Script/BathhouseSim.UtilityFuelSupplyActor |
| /Game/Bathhouse/Blueprints/Interaction/BP_PhysicalCarryFixedSlot | class는 유지하고 아래 map에 삽 전용 instance 추가 |
| /Game/Maps/DefaultMap 및 위 대상의 external actor | 기존 Boiler(현재 위치 299,-885,0) 연결 수정, Supply·Shovel·Slot instance 배치 |
| /Game/Bathhouse/UI/WBP_BathWaterCapacitySummary | hierarchy와 bindings 유지, 글자 layout 조정이 필요할 때만 저장 |

표 밖의 package는 저장하지 않는다. 이번 수직 단계는 기능 검증용 임시 표현이며 단일 Cube 사용을 허용한다. 삽 WorldMesh와 적재 석탄 LoadVisual, FuelIntake, GaugeFace, GaugeNeedleMesh에는 기존 `/Engine/BasicShapes/Cube`를 사용하고 component scale/transform으로 역할을 구분한다. SupplyMesh에는 바닥 pivot Cube인 `/Game/Bathhouse/Meshes/SM_Facility_sample`을 사용한다. 새 mesh package를 제작·저장하지 않는다. 최종 보일러 모델은 투입구와 계기판이 본체에 통합되고 움직이는 시각 부품은 바늘뿐이라는 사용자 결정을 따른다.

## BP_Boiler와 Definition

- 기존 asset 경로, Capacity=Heating 100, VisualMesh, PlacementFootprint와 모든 inherited component 이름/transform을 보존한다. 중복 root/Capacity/placement component 또는 graph gameplay를 추가하지 않는다.
- BP_Boiler CDO의 FacilityPlacement.Definition을 DA_FacilityPlacement_Boiler에 연결한다. 조사 당시 CDO와 DefaultMap instance 모두 None이었다. 기존 instance에 상속되지 않으면 그 한 Boiler만 연결하고 저장한다.
- DA_FacilityPlacement_Boiler의 PlacedFacilityClass=BP_Boiler, RecoveryItemClass=/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem을 확인한다. 승인 없이 footprint, recovery mesh 또는 크기를 바꾸지 않는다.
- native subobject Operation, FuelIntake, GaugeFace, GaugeNeedlePivot, GaugeNeedleMesh, GaugePresentation이 각각 하나인지 확인한다.

## Intake와 gauge

- FuelIntake는 임시 Cube를 크기·위치로 조정해 투입구의 별도 hit target으로 둔다. QueryOnly, Visibility Block, Navigation off이며 본체 mesh가 가리지 않아 실제 LMB trace가 이 component에 맞아야 한다. 최종 본체 모델에 투입구 외형이 통합돼도 이 입력 판정 역할은 유지한다.
- GaugeFace는 임시로 납작한 Cube mesh를 지정하는 고정 표현이다. GaugeNeedleMesh도 얇은 Cube로 설정한다. 둘 다 NoCollision, Navigation off이고 NeedleMesh는 GaugeNeedlePivot child다. pivot relative transform으로 회전 중심과 바늘 길이를 맞춘다. 최종 본체 모델에 계기판 외형이 통합되면 GaugeFace의 중복 표시를 제거하되 현재 C++의 필수 mesh authoring 계약을 충족한다.
- GaugePresentation의 LocalRotationAxis, ZeroAngleDegrees, MaxAngleDegrees, ActiveStartRatio=1/3을 선택 mesh의 local 좌표계에 맞춘다. 회전축/원점을 확인하고 추정값을 저장하지 않는다. 0은 zero 표시, 최소 양수는 전체 범위의 1/3 부근, 50/100은 2/3이어야 한다.
- Operation defaults는 최대 100, 감소 1/sec다. BP/Level에서 초기 잔량을 넣지 않는다.

## 삽·공급함·거치

- BP_UtilityShovel에서 WorldMesh는 물리 root, LoadVisual은 child 표현으로 설정한다. LoadVisual은 NoCollision/Navigation off, empty에서 숨김. native root collision, CCD, Pawn Ignore와 held 위치/회전 계약을 보존한다.
- 이번 단계에서는 WorldMesh와 LoadVisual에 Cube를 각각 지정하고 크기·위치로 빈 상태와 석탄 적재 상태를 구분한다. 최종 삽 silhouette는 수용 조건이 아니다. LoadVisual은 world/held/slot 상태에서 같은 삽의 적재 상태를 유지해야 한다.
- BP_CoalSupply defaults: FuelKind=Coal, ScoopPoints=25. SupplyMesh가 실제 trace target이며 QueryOnly, Visibility Block, Navigation off.
- DefaultMap에 BP_UtilityShovel instance 하나를 만들고 fixed slot의 AssignedItem에 그 정확한 actor reference를 지정한다. bStartOccupied=true, ItemAnchor에 외형을 맞춘다. 중복 삽/slot 연결은 허용하지 않는다.
- Supply와 slot은 접근 가능하며 placement zone, 욕탕, 문과 동선을 막지 않는 위치에 둔다. 저장 뒤 transform을 기록한다.

## Capacity Summary Widget

- /Game/Bathhouse/UI/WBP_BathWaterCapacitySummary의 기존 9개 BindWidget 이름/타입을 유지한다.
- circulation/heating/cooling 각각에 “예약 Used / 가동 Active / 설치 Total”이 읽히고 설치 부족과 가동 부족 status가 잘리지 않아야 한다. Bar는 예약/설치 비율이다.
- /Game/Bathhouse/Blueprints/Computer/BP_BathhouseComputer 및 DefaultMap의 기존 World Space 화면 1024×576을 확인한다. 필요할 때만 allowlist WBP를 수정/개별 Save한다. Widget graph에 계산, 입력, domain logic을 넣지 않는다.
- LMB 삽 action은 instant이며 progress가 없어야 한다. F 반환과 기존 Q 회수 prompt 행을 보존한다.

## Compile, Save, reload

1. UE 5.8에서 대상 BP의 parent/component/property를 확인하고 Data Validation을 실행한다. BP_Boiler의 GaugeFace/GaugeNeedleMesh asset, exact attachment와 NoCollision/Navigation off; BP_UtilityShovel의 WorldMesh/LoadVisual asset, attachment, root QueryAndPhysics/WorldStatic Block/CCD/Pawn Ignore, child NoCollision/Navigation off가 검증되어야 한다. Data Validation은 BP CDO를 확인하고, 실제 placed/held actor는 런타임 authoring validation이 등록·pickup·use 전에 같은 asset/hierarchy 계약을 차단하는지 확인한다.
2. BP_Boiler, BP_UtilityShovel, BP_CoalSupply 및 변경 WBP를 warnings-as-errors로 개별 Compile한다.
3. allowlist package만 개별 저장한다. DefaultMap 저장은 새 Supply/Shovel/Slot과 기존 boiler 연결만 포함한다. 다른 dirty package를 함께 저장하지 않는다.
4. 새 Editor session에서 reload해 parent, Definition, exact slot assignment, mesh/collision, widget bindings와 transforms를 재확인한다.
5. Compile/save/reload 전 값을 Unreal 정본에 저장 완료 상태로 기록하지 않는다.

## PIE 수용 기준

- 새 Boiler 설치 시 Installed 100, Active 0. Installed 상한까지 목표 예약을 허용하고 Active 부족 중 가열 효과를 멈춘다.
- 빈 삽으로 LMB 한 번에 Coal 25를 즉시 퍼담는다. 누르고 있어도 반복되지 않는다. 이미 찬 삽은 내용 보존/추가 거부.
- 실제 intake에 LMB: 0→25, 90→100(초과 폐기, 삽 전체 소모). full, 다른 component/body hit, 300cm 초과, 가림, recovery Hold에서는 삽/설비 값을 바꾸지 않는다.
- 삽 G drop/재획득과 exact fixed slot E 왕복에서도 Coal 25를 보존한다. 같은 Coal supply의 F 반환은 즉시 비우고 Boiler 잔량을 바꾸지 않는다.
- 25는 10초 후 15, 25초 후 0. 양수일 때 정격 100, 정확히 0에서 Active 0이며 Installed/예약은 유지된다. 재투입 후 computer 재입력 없이 가열 재개.
- Q recovery Hold는 투입을 막고 시간을 계속 흐르게 한다. 취소해도 감소값을 복구하지 않는다. 두 Boiler/예약 조합으로 Installed 기준 회수 gate를 확인한다.
- 성공 회수는 잔량을 payload에 보존하고 포장 중 줄이지 않는다. cancel/placement failure는 원래 item·잔량 유지, capacity 추가 없음. 재설치 뒤 보존값부터 감소를 재개한다.
- computer 또는 placement가 입력을 소유하면 LMB와 삽 사용이 겹치지 않는다. 순환 공급이 있는 상태에서 가열/정지/재개 및 LAB-025 물·입욕자 규칙을 본다.
- 임시 Cube 바늘의 0, 25%, 50%, 소진 회전 각도가 계약식과 일치해야 한다. 최종 원형 계기판의 외형 판독성은 이번 임시 표현 수용 조건이 아니다. 1024×576 summary의 세 capacity와 부족 원인, computer 재진입 후 값 유지도 확인한다.
- Output Log에 BP 오류, duplicate registration, NaN transform, tick/input spam이 없어야 한다.

## Unreal 정본과 결과물

실제 저장·reload가 끝난 값으로 .md/Unreal/BathWaterSystem.md, PlacementSystem.md, InteractionUISystem.md를 갱신한다. 새 .md/Unreal/UtilityLaborSystem.md를 만들고 .md/Unreal/0_UNREAL.md 라우팅 표에 추가한다. 미지원 시각 작업은 한국어 .md/USER_UNREAL.md에 exact path, 조작, 기대 결과, 재개 조건을 기록한다. 저장 asset, Compile/Validation/PIE, dirty package와 미완료 항목은 .md/PROMPT_INTEGRATION_REVIEW.md로 인계한다.
