# 코드 리뷰 프롬프트 — 욕탕 수면 가시성과 급수 Niagara 종료

## 범위

사용자 PIE에서 확인된 두 표현 결함의 내부 버그 수정이다. 급수·배수 상태 owner, 유량 계산, interaction과 asset 연결은 변경하지 않는다.

## 변경 파일

- `Source/BathhouseSim/Private/Facility/BathhouseBathFacilityActor.cpp`
- `Source/BathhouseSim/Private/Tests/BathWaterAutomationTests.cpp`

## 수정 계약

- `UpdateWaterSurface()`가 normalized amount를 기준으로 `Visibility`와 `Hidden In Game`을 함께 설정한다.
  - `0`: invisible, hidden in game
  - `>0`: visible, not hidden in game
- Blueprint에 저장된 `bHiddenInGame` 값이 런타임 수위 표현을 막지 못한다.
- 급수 flow가 꺼지는 모든 경로는 `UNiagaraComponent::DeactivateImmediate()`를 사용한다.
  - player close
  - full auto-close
  - recovery begin/commit
  - recovery cancel snapshot이 inactive였던 경우
  - EndPlay
- 논리 물 양, 유량, 밸브·레버 상태와 recovery snapshot 계약은 바꾸지 않는다.

## 자동화 변경

`BathhouseSim.BathWater.ControlAxisMotionAndPlanePresentation`에 다음 회귀 검사를 추가했다.

- 0%에서 plane은 `IsVisible=false`, `bHiddenInGame=true`
- 양수에서 plane은 `IsVisible=true`, `bHiddenInGame=false`
- 다시 0%가 되면 두 숨김 조건이 모두 복원됨

## 검증 상태

- `git diff --check` 통과
- UE 5.8 `BathhouseSimEditor Win64 Development` 빌드 성공
- focused `BathhouseSim.BathWater` automation 3건 성공
  - `ControlAxisMotionAndPlanePresentation`
  - `CustomerSearchActualTimeAndInvalidation`
  - `StateThresholdFlowAndFreeze`
- 실제 `BP_Bath`의 수면과 `NS_HoneyBeam` 시각 결과는 새 DLL로 Editor를 다시 연 뒤 PIE 확인 필요

## 리뷰 중점

- `SetHiddenInGame`과 `SetVisibility`가 동일한 `bHasVisibleWater`에서 파생되는지
- Niagara를 즉시 끄는 경로가 recovery cancel의 원래 active snapshot 복원을 훼손하지 않는지
- 수면 transform, material, collision과 물 상태 계산이 변하지 않았는지
- 새 reflected API, Content/Config 또는 Core Redirect가 없는지

단순 내부 표현 버그 수정이며 구조·책임·Public API가 바뀌지 않아 Architecture 정본은 수정하지 않았다.
