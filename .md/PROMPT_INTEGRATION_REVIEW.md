# 통합 검토 인계 — 보일러 노동 가동 수직 구현

## 상태

**부분 완료, 통합 승인 불가.** 사용자 지시로 코드 리뷰 승인 관문만 MCP 실행에서 건너뛰었다. UE 5.8 공식 `Build.bat`은 `Target is up to date`, `Result: Succeeded`였다. Blueprint/Level 단계는 Unreal MCP로, 이후 사용자가 별도로 요청한 Capacity Summary 레이아웃은 Unreal Editor Python API로 진행했다. Computer Use와 Save All은 사용하지 않았다.

## 저장·재로드된 Content

- `/Game/Bathhouse/Blueprints/Facility/BP_Boiler`: `/Script/BathhouseSim.BathWaterBoilerFacilityActor`로 reparent. 기존 Definition, Capacity Heating 100, 본체 mesh·footprint를 보존하고 inherited `FuelIntake`, `GaugeFace`, `GaugeNeedlePivot`, `GaugeNeedleMesh`, `GaugePresentation`, `Operation`을 승인된 Cube 임시 표현으로 설정했다. warnings-as-errors Compile·개별 Save·새 Editor 재로드 성공.
- `/Game/Bathhouse/Blueprints/Utility/BP_UtilityShovel`: native `UtilityShovelActor` 자식으로 생성. `WorldMesh`·`LoadVisual`에 Cube, 물리 root/child 계약을 설정했다. warnings-as-errors Compile·개별 Save·재로드 성공.
- `/Game/Bathhouse/Blueprints/Utility/BP_CoalSupply`: native `UtilityFuelSupplyActor` 자식으로 생성. `SupplyMesh=SM_Facility_sample`, FuelKind Coal, ScoopPoints 25를 확인했다. warnings-as-errors Compile·개별 Save·재로드 성공.
- `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Boiler`: 읽기 전용 확인만 했다. `PlacedFacilityClass=BP_Boiler_C`, `RecoveryItemClass=BP_PlaceableFacilityItem_C`, `RecoveryItemMesh=None`을 유지한다.
- `/Game/Bathhouse/UI/WBP_BathWaterCapacitySummary`: 기존 3열·9개 BindWidget과 Bar·native 로직을 유지하고 제목·레이블을 한국어로, 값 13pt/상태 10.5pt의 줄바꿈·여백을 조정했다. Unreal Editor Python API로 해당 WBP만 개별 Save, 새 프로세스 재로드와 `BS_UP_TO_DATE` Compile·오류/경고 0건 확인. 메인 컴퓨터 WBP와 Map은 저장하지 않았다.
- 현재 상태를 `.md/Unreal/BathWaterSystem.md`, `PlacementSystem.md`, `InteractionUISystem.md`, 신규 `UtilityLaborSystem.md` 및 `0_UNREAL.md` 라우팅에 반영했다.

## Level 저장 실패와 PIE

- `DefaultMap`에 CoalSupply `(550,-850,0)`, UtilityShovel `(425,-800,45)`, ShovelSlot `(425,-800,45)`을 임시 배치하고 슬롯 `AssignedItem`을 그 삽 인스턴스로 지정했으나, 신규/기존 World Partition actor에 대한 MCP `save_actor`가 모두 외부 패키지 `Asset does not exist`를 반환했다. `/Game/Maps/DefaultMap` 개별 저장도 이 external actor들을 영속화하지 않았다.
- 따라서 세 임시 신규 actor를 제거하고 agent-owned Editor를 재시작했다. 재로드 후 세 actor가 없음을 확인했다. 기존 보일러 인스턴스는 재시작 후 새 Blueprint 투입구·계기 메시를 정상 상속했고, 별도 Level override는 저장하지 않았다.
- 첫 PIE는 stale live 보일러 인스턴스의 `FuelIntake.StaticMesh=None`으로 invalid utility 경고가 났다. Editor 재시작 후 재로드된 두 번째 PIE에서는 같은 `LogBathWaterUtility` 경고가 재발하지 않았다. 실제 LMB/G/E/F/Q 입력과 연료·가동·계기 시각 수용은 검증하지 않았다.
- 대상 Blueprint dirty는 해제됐다. `DefaultMap` 및 외부 액터에 이번 작업의 디스크 변경은 없다. 이번 MCP에는 native Data Validation 실행 기능과 WidgetTree layout 편집 기능이 없어 후자는 별도 승인된 Editor Python API 단계에서 수행했다.
- 사용자가 열어 둔 Editor에서 CoalSupply `(550,-850,0)`, UtilityShovel `(425,-800,45)`, ShovelSlot `(425,-800,45)`을 다시 배치하고 `AssignedItem`을 정확한 삽 actor로 지정했다. PIE 시작·종료의 새 유틸리티 초기화 오류는 없었다. 세 actor는 아직 외부 패키지에 저장되지 않았으므로 저장·재로드 검증 전이며 Editor를 닫으면 안 된다. MCP 저장 실패를 재시도하지 않고 세 actor를 선택한 채 사용자 저장 단계로 인계했다.

## 남은 항목

`.md/USER_UNREAL.md`에 DefaultMap의 세 actor 배치·exact fixed-slot 참조와 World Partition 외부 actor 저장, WBP 1024×576 실제 화면 잘림 확인, Data Validation 및 LAB 직접 입력 수용을 기록했다. 레벨 저장·재로드와 직접 PIE 수용 전에는 통합 승인하지 않는다.
