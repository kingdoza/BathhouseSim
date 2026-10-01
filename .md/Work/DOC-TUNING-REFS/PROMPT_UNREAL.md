# PROMPT_UNREAL — DOC-TUNING-REFS Editor 작업
- 작업 ID: `DOC-TUNING-REFS`
- 단계: 구현
- 상태: 완료

## 1. 작업 필요 여부

- Content·Config 변경 없음. 모든 신규 property의 C++ 기본값은 기존 코드 상수와 같다(원본 위치만 데이터로 이동). 저장·재저장·Core Redirect 불필요. 구현 단계는 Content·Config를 수정하지 않았다(`git status`로 확인).
- 마스터는 아래 읽기 확인 1건(3.6 삽 CDO)만 Editor 단계에서 수행하거나, 마스터 판단으로 사용자 PIE 관찰로 대체할 수 있다. Editor 작업 단계 전체를 생략해도 저장할 asset은 없다.

## 2. 읽기 전용 Editor 확인 (저장 금지)

- 대상: `/Game/.../BP_UtilityShovel`(경로는 Editor에서 `AUtilityShovelActor` 파생 BP로 찾는다)의 CDO.
- 확인: `ThrowImpulseStrength`, `UpwardThrowImpulseStrength`가 `AUtilityShovelActor` C++ 기본값(`GetDefault<AUtilityShovelActor>()`)과 같은가. 설계 단계의 이름표 검사에서는 두 property 이름이 BP asset에 없어 override 흔적이 없었다.
- 이유: 이전에는 삽이 이 property를 읽지 않아 값이 무시되었고, 이번부터 getter가 property를 반환한다. BP CDO가 C++ 기본값과 다르면 삽 G 놓기 속도가 바뀐다.
- 다르면 저장하지 않고 멈춰 사용자에게 어느 값을 쓸지 묻는다(값 선택은 사용자 결정).
- 같은 이유로 다른 carryable은 이미 자기 property를 읽고 있었으므로 확인 대상이 아니다.

## 3. 신규 reflected property (BP는 C++ 기본값 상속, 변경 불필요)

| 소유 | property | 상속 BP/위치 |
|---|---|---|
| `UShopSettings` | `DeliveryAttemptIntervalSeconds` | Project Settings > Bathhouse Shop (Config 키 없음, `Default*` 상수가 기본) |
| `UShopScreenWidget` | `CountdownRefreshIntervalSeconds` | `/Game/Bathhouse/UI/Shop/WBP_ShopScreen` |
| `ACleaningDirectorActor` | `SpawnClearanceFloorOffsetCm` | `BP_CleaningDirector` |
| `AWaterStainActor` | `PlacementClearHeightToleranceCm` | `BP_WaterStain` |
| `ALitterActor` | `PlacementClearHeightToleranceCm` | `BP_Litter` |

## 4. 사용자 PIE 회귀 관찰 항목 (동작 불변 확인)

- 상점 주문 후 배송 도착 알림과 배송 상자 생성이 이전과 같은 시간에 일어난다. 상점 화면 주문 목록 남은 시간이 이전과 같은 간격으로 갱신된다.
- 쓰레기·물 얼룩이 이전과 같이 생성되고 바닥 바로 위의 얇은 물체가 있는 곳에서는 생성되지 않는다.
- 설비를 배치하면 발밑의 쓰레기·물 얼룩이 이전과 같이 정리된다.
- 삽·상자·설비 아이템을 G로 놓을 때 속도(앞·위)가 이전과 같다. 삽은 위 2절 확인 결과가 같음일 때만 같아야 한다.

## 5. 갱신할 Unreal 정본·금지

- `.md/Unreal/*System.md` 갱신 없음(asset 변경 없음). authoring owner 이름 반영은 마스터 판단.
- Blueprint에서 구현하면 안 되는 로직: 없음(값 읽기는 모두 C++).
