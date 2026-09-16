# Unreal 확인 프롬프트 — 욕탕 수면 가시성과 급수 Niagara 종료

## 상태와 경계

- C++ 수정, UE 5.8 Editor 타깃 빌드와 focused `BathhouseSim.BathWater` automation 3건은 완료됐다.
- 이전 DLL로 열려 있던 Editor는 닫혔으므로 새 Editor 프로세스를 시작해 확인한다.
- Content, Level, Blueprint, StateTree와 Project Settings 변경은 필요 없다.
- `BP_Bath`를 Compile/Save하거나 `Save All`을 사용하지 않는다.

## PIE 확인

`/Game/Maps/DefaultMap`에서 기존 `Bath`를 사용한다.

1. 빈 욕탕의 급수밸브를 연다.
2. `WaterSurfaceMesh`가 즉시 표시되기 시작하고 Empty에서 Full marker 방향으로 약 15초 동안 상승하는지 확인한다.
3. 만수 전에 급수밸브를 닫는다.
4. `NS_HoneyBeam` 표현이 닫는 프레임에 즉시 사라지고 수면 높이는 유지되는지 확인한다.
5. 다시 열어 남은 높이부터 상승하며 만수에서 자동으로 닫히고 flow가 즉시 사라지는지 확인한다.
6. 배수레버를 열어 약 10초에 0%로 내려가고, 0%에서 수면이 완전히 숨겨지는지 확인한다.
7. 급수와 배수를 동시에 열면 순유량 `-3.333333%/s`가 적용되는지 확인한다.

## 수용 조건

- 급수밸브 open + 배수레버 closed에서 물 양과 수면이 증가한다.
- 수면은 0%에서만 숨고 양수에서는 Blueprint의 기존 `Hidden In Game` 저장값과 무관하게 보인다.
- player close, full auto-close와 recovery 시작 시 flow 잔상이 남지 않는다.
- valve/lever 회전, interaction prompt, 수위 marker, collision, Navigation과 reservation threshold 동작은 변하지 않는다.

Editor authoring 변경이 없으므로 `.md/Unreal/*`와 `.md/USER_UNREAL.md`는 갱신하지 않는다. PIE 결과만 통합 리뷰에 전달한다.
