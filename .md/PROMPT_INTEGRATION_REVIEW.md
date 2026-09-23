# 통합 검토 프롬프트 — Bath Water Operations 관리 화면

## 결론

상태는 **관리 화면 재작업 저장·자동 PIE 확인 완료, 실제 LMB와 기능 수용 대기**다. 기존 사용자 PIE에서는 패널/텍스트가 거의 흰색으로 겹치고 지도 격자가 보이지 않았으며 욕탕 타일 선택이 되지 않았다. 기존 sample 화면의 LMB 클릭은 사용자 테스트로 통과했으므로 입력 매핑은 바꾸지 않았다. 이번 컴퓨터 Widget 재작업에는 앞선 사용자 요청 범위에 한해 Unreal Editor API/Python fallback을 적용했다.

WBP 5개의 패널·글자 대비, 간격, tile 색·글자 크기를 수정해 개별 Compile/Save했다. `WBP_BathWaterMap`에 타일보다 뒤에 놓이는 `GridCanvas`를 추가했고, native `BathWaterMapWidget`은 Zone의 grid size와 major interval을 사용해 격자선과 경계를 생성한다. 타일 표기는 내부 UObject 이름 대신 Actor 표시명이다. 실제 타일 0개의 원인은 Map WBP의 Class Default가 아니라 화면에 중첩된 `BathMap.BathTileWidgetClass`가 `None`으로 저장된 것이었다. 이를 명시적으로 연결해 저장했다. UE 5.8 DLL 링크 뒤 새 Editor PIE에서 욕탕 타일 2개와 격자 렌더를 확인했다.

## 저장된 Content·Level

- Utility: `BP_Circulator`, `BP_Boiler`, `BP_Cooler`와 대응 `DA_FacilityPlacement_*` 3개. Capacity는 각각 Circulation/Heating/Cooling `100`이다.
- Utility visual: 공통 `/Game/Bathhouse/Meshes/SM_Facility_sample`; Boiler/Cooler footprint 100×60×120cm, Circulator 120×80×120cm. Recovery mesh는 `None`이며 native Cube fallback이다.
- `BP_Bath`: inherited `BathWaterCondition` 1개; circulation `100`, heating/cooling `5 points/°C`, cleaning `1 point/s`, contamination `0.1 point/s/actual bather`, target control `0.5°C/s`, natural return `0.05°C/s`. Bath Water Settings는 ambient `20°C`, target `10~50°C`, step `1°C`로 별도 Config 변경이 없다.
- Widget: `/Game/Bathhouse/UI/WBP_BathWaterManagementScreen`, `WBP_BathWaterCapacitySummary`, `WBP_BathWaterMap`, `WBP_BathWaterBathTile`, `WBP_BathWaterDetail`. 각각 대응 native Widget class를 상속한다. 필수 `BindWidget` 이름/타입, Map tile class와 clipping을 재로드 후 검사했다. 실제 hierarchy 정본은 [InteractionUISystem.md](Unreal/InteractionUISystem.md)다.
- `BP_BathhouseComputer.ScreenWidget.WidgetClass`: 새 management WBP. World Space, Draw Size `1024×576`, Hardware Input `false` 유지.
- DefaultMap exact computer actor `ManagedBathPlacementZone`: exact `BP_FacilityPlacementZone_C_UAID_F02F7433CA36D1FF02_1155169559`. 저장 대상은 해당 external actor package `/Game/__ExternalActors__/Maps/DefaultMap/7/EH/E4FLO971KSWUJ40H7W7PHK`이며 `DefaultMap.umap`은 저장하지 않았다.

## 확인 결과

- WBP 5개와 computer BP를 개별 Compile/Save하고 같은 Editor에서 패키지 재로드했다. 복제 원본 Widget GUID map 정리 후 다섯 WBP를 강제 저장·재로드했고 해당 재로드 구간에 GUID `Ensure` 또는 Blueprint compile error는 없었다.
- WBP 5개와 computer BP의 Editor Data Validation 6/6 `VALID`, errors/warnings 0건이다. 재작업 WBP 5개를 개별 Compile/Save하고 새 DLL을 적용한 Editor에서 검증했다.
- Utility Definition 3개도 새 Editor에서 Data Validation `VALID`다. 각 Definition은 recovery mesh `None`에 따른 native Cube fallback 경고만 있다.
- computer Widget Class, Draw Size/Space/Hardware Input과 Level Zone reference가 재로드 후 유지된다.
- PIE RenderTarget 1024×576에서 어두운 panel, 전체 폭 utility summary, Zone 경계·격자, Bath 타일 2개와 detail을 확인했다. 타일 Button `OnClicked` 이벤트를 호출하자 Bath2가 선택되고 detail 값·두 slider 활성화가 갱신됐다. 실제 플레이어 LMB hit test는 미검증이다.
- 별도 Unreal Python commandlet 프로세스에서 다섯 WBP의 native parent, Map tile class, computer screen class와 `1024×576`을 디스크로부터 다시 읽었다. 검사 스크립트는 성공했으나 로컬 Zen/DDC cache에 writable node가 없어 commandlet 프로세스 종료 코드는 `1`이었다. 이는 Asset assertion 실패가 아니며 정상 Editor 재시작 검증을 대체하지 않는다.
- 첫 PIE 종료 중 Bath unregister 알림이 월드 teardown 중 위젯 재생성을 시도해 `ensure` 1건이 발생했다. `RefreshSnapshot`의 world teardown guard를 추가하고 DLL 재빌드했다. 새 Editor PIE 종료 로그에서 같은 `ensure`는 재발하지 않았다.
- `Save All`과 컴퓨터 Focus/Input graph 변경은 하지 않았다. 사용자 소유 Editor는 사용자가 닫았고, 검증용 agent-owned background Editor만 개별 종료·재시작했다.

## 잔여 위험과 통합 리뷰 재개 조건

1. `.md/PROMPT_UNREAL.md`의 PIE 14개 시나리오로 capacity/demand, 지도 투영, 선택/슬라이더/feedback, utility와 Bath recovery transaction, focus/input 회귀를 직접 검증한다. 자동 PIE의 RenderTarget과 Button 이벤트만으로 기능 수용을 선언하지 않는다.
2. 실제 월드 화면에서 타일·슬라이더 hit target을 플레이어 LMB로 확인한다. 자동 RenderTarget과 Button 이벤트는 통과했지만 화면 조준/포커스 입력 경로는 대신하지 못한다. 격자선은 타일 아래, Zone 경계 안에만 나타나고 클릭을 막지 않아야 한다.
3. authoring 과정의 이전 compile/reload 로그에는 원본 sample Widget GUID fixup과 transient graph reload 메시지가 있었다. 최종 강제 저장 후 새 Editor 재로드에서는 해당 GUID `Ensure`가 재발하지 않았다. 실제 플레이 시 로그도 살핀다.

위 조건이 완료되기 전에는 최종 통합 승인하지 않는다. 다른 Placement fixture 오류와 현 작업의 Widget 결과는 분리해서 보고한다.
