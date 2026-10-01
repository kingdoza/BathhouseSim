# PROMPT_UNREAL — UNBOX-SPAWN-VIEW 플레이어 앞 생성 위치를 카메라 시선 기준으로

- 작업 ID: `UNBOX-SPAWN-VIEW`
- 단계: 구현
- 상태: 완료

## 1. Editor 작업 필요 여부

**Content 변경 없음.** Editor 작업 단계는 생략 대상이다(`AGENT_WORKFLOW.md`: Content 변경 없음 선언 + `git status`로 Content 그대로 확인).

- `git status --short -- Content Config` 출력 없음(구현 완료 시점).
- 새 `UShopSettings` property는 `Config/DefaultGame.ini`에 키가 없으면 C++ header 기본값(`UShopSettings::Default*`)을 쓴다. 기존 `UnboxOverlapDepthCm=20.0` 저장값은 그대로다.
- `ALitterTongsActor`의 새 `Tie*` property는 `/Game/Bathhouse/Blueprints/Cleaning/BP_LitterTongs`가 C++ 기본값을 상속한다. resave하지 않는다. 기존 `TieForwardDistanceCm`·`TieMinForwardDistanceCm`는 BP 저장값이 있으면 그대로 쓰인다.
- 갱신할 `.md/Unreal/*System.md` 없음(asset 구조 변화 없음).
- 사용자 PIE 이후 값 조정은 Project Settings `Bathhouse Shop`의 `Shop|Unboxing` 값과 `BP_LitterTongs` Class Defaults의 `Litter Tongs` 값이다. 조정은 별도 지시가 있을 때만 하며 Config·BP 저장이 필요하면 그때 Editor 작업으로 다룬다.
- Blueprint에서 구현하면 안 되는 로직: 생성 위치 계산, 단계 순서, 충돌·시야 검사 전부 C++이다. Blueprint는 값 authoring만 한다.

## 2. 사용자 PIE 관찰 항목 (DefaultMap, 기본 캐릭터)

수용 기준: 화면 정면에 보임, 카메라를 감싸지 않음, 바닥·벽·천장 안이나 너머가 아님. 거리 기대치는 기능 명세 8절과 현재 설정값을 따른다.

| 순서 | 시나리오 | 관찰 | 기대 결과 |
|---|---|---|---|
| 1 | USV-001, 002, 015 | 트인 곳에서 수평으로 보고 샤워기 1개·대표 상자·혼합 상자 개봉(LMB) | 화면 중앙 눈높이, 가장 가까운 면이 시선 거리 설정만큼 앞에 생겨 튀며 떨어짐. 화면 아래 밖에 생기지 않음 |
| 2 | USV-012, 013 | 최대 수량 설비 개봉 | 높은 천장에서는 화면 정면, 낮은 천장에서는 앞쪽 바닥(화면이 무리에 묻히지 않음) 또는 머리 위. 천장이 아주 낮아 무리가 들어가지 않으면 최후 쌓기라 카메라가 물품에 묻힐 수 있음(알려진 설계 범위, 있으면 기록) |
| 3 | USV-003~006 | 비스듬히 아래, 거의 발밑, 비스듬히 위로 개봉 | 화면 중앙 부근에 생김, 바닥에 파고들지 않음, 발밑에서는 몸 자리와 겹쳐도 밀리지 않고 걸어 나감 |
| 4 | USV-007~011 | 낮은 천장 위 보기, 가까운 벽, 벽에 붙음, 구석, 손님 앞 | 천장·벽 너머 없음, 손님 몸 안 생성 없음, 개봉 항상 성공 |
| 5 | USV-014, 016, 021 | 같은 상자 두 번 배치 비교, 생긴 물품 E 들기·배치·버리기, 위에서 떨어진 물품 | 배치가 매번 다름, 들기·배치·버리기 정상, 맵 밖으로 빠지지 않음 |
| 6 | USV-017~019 | 집게 봉투 묶기(RMB): 수평, 발 앞(쓰레기·얼룩 위), 벽 앞 | 수평이면 화면 중앙 가까이, 발 앞은 쓰레기·얼룩이 있어도 생김, 벽 앞은 `봉투를 놓을 공간이 없음`과 개수 유지 |
| 7 | USV-020 | G 내려놓기, Q 회수, 배송 도착 위치 | 이전과 같음 |

조정이 필요하면 위 값들을 바꾸고 관찰과 함께 보고한다. 문서는 고칠 필요가 없다.
