# 통합 검토 — 급수·배수 시스템과 욕탕 물 수직 구현

## 결론

상태는 **Editor 부분 완료**다. Customer routine Data Asset과 `BP_Bath` authoring은 저장·재로드까지 완료됐다. StateTree BathLoop authoring과 실제 플레이 수용 검증은 남아 있다.

## 이번 Editor 저장 변경

- `/Game/Bathhouse/Blueprints/Facility/BP_Bath`
  - Parent: `/Script/BathhouseSim.BathhouseBathFacilityActor`
  - `BathWaterState`: Fill `6.666667`, Drain `10.0`
  - `WaterSurfaceMesh`: `SM_Bath_01_Water`, Scale `(0.65,0.65,1)`
  - Water marker: Empty Z `-70`, Full Z `-15`
  - `FillValveControl`: Engine Cube, `30×8×8cm`, Y축 `90°`, `0.5s`
  - `DrainLeverControl`: Engine Cube, `6×8×30cm`, Y축 `45°`, `0.5s`
  - `FillFlowNiagara`: `/Game/Niagaras/NS_HoneyBeam`, `AutoActivate=false`
- `.md/Unreal/BathWaterSystem.md`
- `.md/Unreal/FacilitySystem.md`
- `.md/Unreal/0_UNREAL.md`
- `.md/USER_UNREAL.md`

Cube 두 개와 `NS_HoneyBeam`은 사용자가 명시적으로 승인한 임시 표현이다. 기존 `FacilityVisual`, `PlacementFootprint`, `FacilitySlotA~C`와 슬롯 transform은 유지했다.

## 저장·재로드 검증

- `BP_Bath` Compile 성공
- `BP_Bath` 패키지만 개별 Save 성공; `Save All` 미사용
- 새 UE 5.8 Editor 프로세스에서 parent, component asset, transform, 수위 marker와 유량 값 재로드 확인
- 재로드 후 `BP_Bath`와 `/Game/Maps/DefaultMap` 모두 dirty false
- 기존 Level의 `Bath` 인스턴스에도 저장된 Class Default가 반영됨을 확인
- Level 및 World Partition external actor는 변경·저장하지 않음

최초 reparent 직후 필수 asset과 marker를 넣기 전 발생한 native validation 오류는 최종 authoring 뒤 재Compile에서 해소됐다. 별도 Data Validation 실행 도구는 현재 MCP에 없었으며, 최종 Compile과 새 프로세스 로드 로그에는 해당 Bath validation 오류가 없다.

## 알려진 경고

`SM_Bath_01_Water`의 translucent `M_Water_Turquoise`를 Nanite mesh에 사용하는 경고가 남아 있다. 이번 범위에서는 기존 mesh/material을 수정하지 않았다. 실제 물 렌더링에 문제가 있으면 component의 Nanite 사용 금지 또는 비-Nanite 수면 mesh를 별도 검토한다.

## 남은 작업

- `/Game/Bathhouse/AI/ST_CustomerRoutine` BathLoop state/task/transition/binding 수동 구성과 Compile
- 급수·배수, 수면, 조작부, Niagara, 임계 수위, Customer 반복 이용과 Q Hold 회수의 PIE 수용 검증
- 임시 Cube/`NS_HoneyBeam`의 최종 표현 에셋 교체는 미관 요구가 확정될 때 별도 진행

정확한 수동 절차는 `.md/USER_UNREAL.md`, 저장된 Editor 계약은 `.md/Unreal/BathWaterSystem.md`를 따른다.
