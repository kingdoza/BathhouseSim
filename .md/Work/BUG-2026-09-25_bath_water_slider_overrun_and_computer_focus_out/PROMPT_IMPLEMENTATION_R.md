# PROMPT_IMPLEMENTATION_R — 욕탕 관리 슬라이더 한계 초과·컴퓨터 클릭 없는 포커스아웃

- 작업 ID: `BUG-2026-09-25_bath_water_slider_overrun_and_computer_focus_out`
- 단계: 코드 리뷰
- 상태: 완료
- 출처: 코드 리뷰 1회차

## 리뷰 범위와 결론

- 범위: `git diff cd47d47 9db857f`. Source 식별값은 `PROMPT_REVIEW.md`의 SHA-256 `97090aa8…0ef2`와 같음을 다시 계산해 확인했다(재빌드 안 함). Automation 로그(`Saved/Logs/BathhouseSim.log`)는 필터 전체 Success다.
- 결론: 구현 재검토. production 코드(`UBathWaterDetailWidget`, Build.cs, friend 두 줄)와 컴퓨터 포커스 테스트는 승인 수준이다. 아래 R1·R2만 고친다.

## 유지되는 승인 범위 (재작업 금지)

- `Private/UI/BathWaterDetailWidget.cpp`·`Public/UI/BathWaterDetailWidget.h`
  - 같은 callback 안 확정값 쓰기
  - `bWritingSliderValue` guard. UE 5.8 `USlider::HandleOnValueChanged`가 broadcast 전에 `Value`를 바꾸므로 중첩 `SetValue` 뒤 `USlider::Value`와 `SSlider` 속성이 모두 확정값으로 남는다.
  - cache gate 앞 polling 동기화
  - 실패 분기(domain → `CachedSnapshot`)
  - 피드백 규칙 불변
- `BathhouseSim.Build.cs`, `FirstPersonCharacter.h`·`BathhouseComputerActor.h` friend, `BathWaterOperationsAutomationTestSupport.h` 이동
- `ComputerKeyboardFocusAutomationTests.cpp` 전체
- `BathWaterSliderInputAutomationTests.cpp`에서 R1 대상 줄을 뺀 나머지: pointer drag 테스트, 순환 구간, SLD-004, polling, 실패 경로
- `PROMPT_UNREAL.md`의 "Content 변경 없음" 판정. `git diff cd47d47 9db857f -- Content Config` 0건을 확인했다.

## Findings

### R1 (중요 — 조정값 원본 원칙, 테스트 기대값이 튜닝 원본과 분리됨)

- 위치: `Source/BathhouseSim/Private/Tests/BathWaterSliderInputAutomationTests.cpp` 324~330행, 360·378행, 381행 메시지
- 현상
  - 가열·냉각 기대 한계를 `Ambient ± HeatingHeadroomC(10)/CoolingHeadroomC(5)`로 단언한다.
  - 설치용량에는 `+0.25°C` 여유를 더한다.
- 문제
  - domain은 가능한 변화량을 `FloorToFloat(MaxDelta / TargetTemperatureStepC) * TargetTemperatureStepC`로 내린다(`Private/Facility/BathWaterOperationsSubsystem.cpp` 272~273, 288~289행).
  - 따라서 기대값은 "Step이 10과 5를 나누고 Step > 0.25"라는 숨은 가정에 기대고 있다.
  - 현재 원본 `UBathWaterSettings::TargetTemperatureStepC`는 `1.0`이다(`Public/Facility/BathWaterSettings.h` 50행, `Config/`에 override 없음). 테스트는 우연히 통과한다. Step을 0.25나 0.3으로 조정하면 production이 맞아도 실패한다.
  - 주석 "A quarter of a degree … step quantization"과 메시지 "in 0.5 degree steps"는 현재 원본(1.0)과도 맞지 않는다.
- 수정 방향
  - `Settings->GetTargetTemperatureStepC()`를 읽는다.
  - 여유는 Step의 정수배로 정한다. 예: `HeadroomC = Step * N`. N은 `Ambient ± HeadroomC`가 `[Min, Max]` 안에 들도록 settings에서 계산하고, 1 이상으로 둔다.
  - 설치용량 여유는 한 step 미만으로 둔다. 예: `PerC * (HeadroomC + Step * 0.5f)`.
  - 기대 한계는 `Ambient ± HeadroomC`로 둔다.
  - domain 공식(`FloorToFloat`)을 테스트에 복제하지 않는다.
  - 기존 범위 guard(fixture가 `[Min, Max]` 밖이면 실패)는 유지한다.
  - 주석·메시지에서 `0.5`·"quarter" 같은 수치를 지우고 "TargetTemperatureStepC 단위"로 쓴다.
- 금지: `UBathWaterSettings` 기본값·Config 변경, production 변경, 테스트에서 settings CDO 값을 바꾸는 우회

### R2 (낮음 — 문서 수치 복제가 원본과 불일치)

- 위치: `PROMPT_UNREAL.md` 사용자 PIE 관찰 표 SLD-003 행 "손잡이가 0.5°C 단위 확정값에 멈춤"
- 문제
  - 실제 단위는 `TargetTemperatureStepC`(현재 1.0)이다. 사용자가 PIE에서 1°C 단위 이동을 실패로 오인할 수 있다.
  - 문서는 수치를 복제하지 않고 원본 위치를 가리켜야 한다(`AGENT_WORKFLOW.md` 조정값 원본 원칙).
- 수정 방향: "`UBathWaterSettings.TargetTemperatureStepC`(Project Settings > Game > Bath Water) 단위의 확정값"으로 바꾼다. `PROMPT_REVIEW.md`의 SLD-003 설명에 같은 표현이 있으면 함께 맞춘다.
- 범위 밖(아키텍처 소유, 마스터에게 별도 보고): 같은 `0.5°C` 표기가 다음 두 곳에 있다. 구현 단계는 고치지 않는다.
  - `PROMPT_IMPLEMENTATION.md` 1절 SLD-003·9절 2번
  - `.md/Architecture/BathWaterManagementUISystem.md` 98행

## 재검증 조건

1. 변경 파일은 `BathWaterSliderInputAutomationTests.cpp`, `PROMPT_UNREAL.md`, (필요 시) `PROMPT_REVIEW.md`뿐이다. production Source·Build.cs·Content·Config diff는 없다.
2. 빌드한다(`UE_BUILD_POLICY.md` 진입점). 이어서 headless Automation 필터 `BathhouseSim.BathWater+BathhouseSim.Computer+BathhouseSim.Interaction.HeldTargetUse.InputOwners`를 실행해 전부 Success여야 한다.
3. `PROMPT_REVIEW.md`의 Source 식별값과 빌드·Automation 결과를 갱신한다. R1 수정 뒤 SLD-003 기대값이 어떤 Step 값에도 성립하는 근거를 한두 줄로 적는다(값을 실제로 바꿔 실행할 필요는 없다).
4. 재검증은 같은 리뷰어가 R1·R2 diff만 본다.
