# PROMPT_ARCHITECTURE — PLACEMENT-FOOTPRINT-PREVIEW 배치 미리보기의 footprint 공간 표시

- 작업 ID: `PLACEMENT-FOOTPRINT-PREVIEW`
- 단계: 기능 명세
- 상태: 완료

## 1. 목적과 범위

설비 아이템을 들고 배치 미리보기를 볼 때, 지금 보이는 비주얼 메시 미리보기에 더해 그 설비의 Placement Footprint가 바닥(XY 평면)에서 차지하는 영역을 함께 보여 준다. 메시 교체(모델링 작업)로 footprint와 메시 크기가 크게 달라져도 플레이어가 두 가지를 화면에서 바로 알 수 있게 한다.

- 어디까지가 이 설비 자리인지
- 왜 놓을 수 없는지

함께, 조사에서 드러난 쿨러·순환기·보일러 footprint 결함을 이번 작업에서 고친다(Q10 B).

사용자 요청 원문(2026-10-02): "Placement Footprint 가 실제 비주얼 메시보다 넓은 경우가 생길거임. 근데 현재 배치프리뷰에는 비주얼메시만 표현되다보니깐 placement footprint 가 비주얼메시의 크기랑 꽤 다를경우에는 ux 가 불편할수있음. 비주얼메시 프리뷰와 더불어서 placement footprint 의 xy평면상에서의 공간프리뷰도 보여줬으면 좋겠다."

사용자 지시(2026-10-02): "값은 기본적으로 원본참조로 작성임." 이 문서의 수치는 근거 인용이나 사용자 판단용 제안값이다. 정본에는 조정값 원본 위치만 남긴다([AGENT_WORKFLOW.md](../../AGENT_WORKFLOW.md) 조정값 원본 원칙).

범위:

- 플레이어가 배치용 설비 아이템을 든 동안의 배치 미리보기. 활성 Definition 16종 전체가 대상이다([Unreal/PlacementSystem.md](../../Unreal/PlacementSystem.md) Definition 계약 표).
- footprint 영역 표시의 모양·색·높이·가림과 기존 미리보기 동작과의 연동
- 표시 모양을 바꾸는 조정값과 조정 범위
- 쿨러 footprint를 정수 칸으로, 순환기·보일러의 회전 제거, grid 비정수 footprint를 잡는 Data Validation(Q10 B)

범위 밖은 9절에 둔다.

## 2. 현재 동작

근거는 다음 셋이다. 아래 수치는 조사 시점 값의 인용이며 원본은 괄호의 위치다.

- [Architecture/PlacementSystem.md](../../Architecture/PlacementSystem.md)의 Generic Native Preview, Preview Without Aim, Compatible Zone Grid Presentation
- `REPORT_UNREAL_DISCOVERY.md`(이하 조사 보고)
- 사용자 PIE 관찰(2026-10-02)

미리보기 메시:

- 배치 설비 Blueprint의 class-default static mesh들을 복제한다. 모든 material slot이 유효/무효 미리보기 재질로 바뀐다(원본: Project Settings `Facility Placement`의 `ValidPreviewMaterial`·`InvalidPreviewMaterial`).
- 사용자 PIE 관찰로는 유효 상태에서 **초록 반투명**이다("반투명이야").
- 조사 보고 3절은 재질 그래프만 보고 "사실상 불투명"이라고 추론했다. 이 명세는 그 추론 대신 사용자 관찰을 따른다. 화면 외형은 숨김 Editor로 판정할 수 없다(조사 보고 8절).

배치 판정:

- 구역 포함·겹침·바닥 지지를 메시가 아니라 배치 설비의 `PlacementFootprint` 상자로 판정한다.
- 겹침은 들고 있는 설비의 footprint 상자와 다른 물체의 blocking collision 사이에서 판정한다.
- 그래서 footprint가 메시보다 넓으면 "메시는 아무것에도 닿지 않는데 놓을 수 없음(무효)"이 생기고, 원인 영역이 화면에 보이지 않는다.

footprint 계약과 설비별 관계:

- footprint 바닥은 설비 Actor local Z=0(설치 바닥)에 있다. XY 크기는 grid 간격(원본: Project Settings `Facility Placement > Grid Size Cm`)의 정수배가 계약이다.
- 설비별 footprint와 미리보기 메시의 관계(조사 보고 2절):
  - footprint가 메시보다 넓음: Vanity(사방 같은 여백), Bath, Shower·Washer·Dryer(한 축 양쪽 여백). Shower 메시는 바닥에서 떠 있는 판이다.
  - footprint와 메시 외곽이 같음(7종): ClothesLocker_4·_8, DrinkFridge, MassageChair, RestBench, Television, ScrubTable
  - 메시가 footprint보다 큼(3종): Circulator, Boiler, Cooler
  - ClothesLocker_1은 사용자가 작업 트리에서 수정 중이다. 사용자 소유 변경이라 이 작업 근거로 쓰지 않는다.
- 설치 바닥보다 아래로 내려가는 미리보기 메시는 Bath뿐이다(1cm 미만).

결함(Q10 B로 이번에 수정):

- Cooler: 실제 판정 footprint가 grid 정수배가 아니다(조사 시 1.5×2.5칸). Data Validation은 root scale을 반영하지 않아 이를 잡지 못한다. 그래서 snap해도 footprint 변이 grid 선과 반 칸 어긋난다. footprint 축은 Actor 축과 90° 다르다.
- Circulator·Boiler: 메시와 footprint를 담은 하위 root(`SceneRoot`)가 Actor 축에서 조금 돌아 있다(조사 시 −1°). 그래서 snap해도 footprint 변이 grid 선과 평행하지 않다.

grid:

- 미리보기 세션 동안 호환 공간 전체에 중립색 grid가 보이며 조준·유효성과 무관하다.
- 높이·선 두께·강조 간격의 원본은 구역 Blueprint(`BP_FacilityPlacementZone`, `BP_BathhouseSpace`) Class Default와 공간 instance의 `GridZOffsetCm`·`GridLineThicknessCm`·`MajorGridIntervalCells`다. 색·불투명도의 원본은 `MI_FacilityPlacementGrid`다.

입력과 미리보기 숨김:

- LCtrl을 누른 동안만 위치를 grid에 맞추고(snap), Mouse Wheel이 Yaw를 돌린다. 누적 Yaw는 LCtrl을 놓거나 조준을 잃어도 유지된다.
- (EXP-U3 D4) 후보 위치를 계산하지 못하면 미리보기가 숨는다(거리 밖, 벽·천장·계단, 구역 없는 곳). 후보가 있으면 조준 자리에 무효 상태로 보인다. 공간 불허·락커 한도·구역 밖·겹침·바닥 지지 부족도 이 경우다.

## 3. 목표 동작

용어:

- **footprint 표시**: 배치 미리보기 동안 바닥에 그려지는 footprint XY 점유 영역 표현
- **메시 미리보기**: 기존 설비 메시 미리보기

동작:

1. 배치용 설비 아이템을 들고 배치 가능한 공간 바닥을 조준하면 메시 미리보기와 footprint 표시가 함께 보인다.
2. footprint 표시는 반투명 채움과 선명한 외곽선으로 된 사각형 하나다(Q1 A). 그 설비 `PlacementFootprint`의 XY 사각형이다.
   - 크기·중심·방향은 배치 확정 판정에 쓰는 실제 world footprint와 같다.
   - 보정 없이 실제 판정 영역을 그대로 보여 준다.
3. footprint 표시는 조준한 공간의 설치 바닥 높이에 수평으로 놓이고, 그 공간의 grid 표시보다 위에 보인다. 높이(Z 부피)는 표시하지 않는다.
4. 메시 미리보기와 footprint 표시는 같은 후보 위치·Yaw를 쓰므로 언제나 함께 움직이고 함께 돈다. LCtrl snap 중에는 footprint 네 변이 grid 선에 맞는다. Q10 B 수정 뒤에는 16종 모두가 그렇다.
5. 색은 메시 미리보기와 같은 유효·무효 색이며, 같은 판정 결과로 메시와 함께 바뀐다(Q2 A). 색 원본은 메시 미리보기 재질 하나다.
6. 다른 물체와는 일반 물체처럼 가려진다(Q3 A). 벽이나 설치된 설비 같은 불투명 물체 안·뒤로 들어간 부분은 보이지 않고, 보이는 바닥 부분만 보인다(grid와 같은 방식).
7. 메시 미리보기 외형(재질·반투명·색)은 지금 그대로다(옛 Q5 A).
   - 자기 메시 아래에 놓인 footprint 부분은 반투명 메시를 통해 보인다.
   - 겹친 부분이 충분히 알아볼 수 있게 보이는지는 PIE 관찰 항목이다(FPV-002·010·011). 부족하면 footprint 쪽 조정값으로 맞춘다.
8. footprint가 메시와 같거나 작은 설비도 항상 표시한다(Q4 A).
9. 메시 미리보기가 숨으면(D4) footprint 표시도 숨는다. 메시가 다시 보이면 같은 새 위치에 함께 보인다.
10. footprint 표시는 배치 판정·안내 문구·확정 결과를 바꾸지 않는다. 판정 결과가 바뀌는 것은 Q10 B로 고치는 쿨러·순환기·보일러 세 설비뿐이다(FPV-016).
11. 설치된 설비의 footprint와 Q 회수 대상의 footprint는 표시하지 않는다(Q6 A, Q7 A).

## 4. 사용자 승인 사항과 미확정 확인

- 답변 일자: 2026-10-02. S4 아니오, 자동 승인이다(근거는 `QNA_FEATURE_SPEC.md` 결정 요약).
- 결정:
  - Q1 A 채움+외곽선, Q2 A 메시와 같은 유효·무효 색, Q3 A 일반 가림, Q4 A 항상 표시
  - Q5 옛 사본 답 A: 메시 미리보기 외형 유지 + footprint 표시 추가
  - Q6 A·Q7 A 설치·회수 대상 표시 없음, Q8 A 공통 조정값 한 벌
  - Q9 A 한 작업 공통 구현, 대표 설비 Vanity
  - Q10 B 세 설비 결함 이번에 수정
  - S2 범위 밖 asset 변경은 멈추지 않고 진행하고 단계 보고에 기록. `BP_ClothesLocker` 사용자 변경은 제외
  - S3 PIE 통과 후 병합 예
- 사전 조사 정정: 조사 보고의 "미리보기 사실상 불투명" 추론을 근거로 만들었던 새 Q5는 잘못된 전제라 철회했다. 사용자 관찰(반투명)과 옛 Q5 답 A를 적용한다.
- 미확정 사항 없음.

## 5. 수용 시나리오 (Given/When/Then)

공통 Given: 플레이어가 배치용 설비 아이템을 들고 있고, 그 설비를 허용하는 공간이 있다. 유효 상태는 메시 미리보기의 유효 색, 무효 상태는 무효 색이다.

### FPV-001 조준하면 메시와 footprint가 함께 보인다
- Given 허용 공간의 빈 바닥이 배치 거리 안에 있다.
- When 그 바닥을 조준한다.
- Then
  - 메시 미리보기와 footprint 표시가 함께 보인다.
  - footprint 표시는 반투명 채움과 외곽선으로 된, 바닥에 수평으로 깔린 사각형이다.
  - grid보다 위에 있어 grid 선에 묻히거나 깜빡이지 않는다.
  - 둘 다 유효 색이다.

### FPV-002 footprint가 메시보다 넓으면 넓은 영역이 보인다 (대표: Vanity)
- Given Vanity 아이템을 들고 홀에 있다.
- When 빈 바닥을 조준한다.
- Then
  - footprint 표시가 메시 외곽보다 사방으로 같은 폭만큼 넓게 보인다.
  - 크기는 Vanity footprint 칸 수만큼의 grid 칸과 같다.
  - 메시 아래에 놓인 footprint 부분과 외곽선도 반투명 메시를 통해 보인다.
- PIE 관찰: 메시와 겹친 부분의 footprint가 경계를 알아볼 만큼 보이는지 적는다.

### FPV-003 LCtrl snap과 해제
- Given FPV-002 상태다.
- When LCtrl을 누른 채 조준을 천천히 옮긴 뒤 LCtrl을 놓는다.
- Then
  - 누르는 동안 메시와 footprint 표시가 함께 칸 단위로 이동하고, footprint 네 변이 grid 선과 겹친다.
  - 놓으면 둘이 함께 자유 이동으로 돌아가며 Yaw는 그대로다.

### FPV-004 회전
- Given Vanity를 들고 있다(가로·세로가 다른 footprint).
- When Mouse Wheel을 한 단계씩 돌린다.
- Then
  - 메시와 footprint 표시가 같은 각도로 함께 돌고, footprint 표시의 긴 변이 메시와 같은 방향으로 바뀐다.
  - 회전 후에도 footprint 표시는 바닥 위에 수평이다.
  - 90° 단위로 돌린 뒤 snap하면 네 변이 grid 선에 맞는다.

### FPV-005 footprint 여백 때문에 겹침 무효
- Given 설치된 다른 설비나 벽 옆에서, Vanity 메시는 닿지 않지만 footprint 여백은 닿는 자리가 있다.
- When 그 자리를 조준하고 LMB를 누른다.
- Then
  - 메시 미리보기와 footprint 표시가 무효 색으로 보인다.
  - footprint 여백이 다른 설비·벽 발밑까지 닿아 있는 것이 보인다. 불투명 물체 안·뒤로 들어간 부분은 가려진다.
  - 기존 겹침 안내 문구가 그대로 나오고, 설치되지 않으며, 아이템은 손에 남는다.

### FPV-006 기타 무효 사유
- Given 다음 중 하나가 성립하는 자리가 있다: 구역 밖으로 걸침, 공간 불허 설비, 락커 한도, 바닥 지지 부족.
- When 그 자리를 조준한다.
- Then
  - 메시와 footprint 표시가 그 자리에 무효 색으로 함께 보인다(숨지 않음).
  - 사유별 기존 안내 문구가 그대로 나온다.
  - 구역 밖으로 걸친 경우 footprint 표시가 벽을 넘는 부분은 벽에 가려진다.

### FPV-007 조준 없음(D4)과 복귀
- Given FPV-002 상태에서 Yaw를 한 번 이상 돌려 두었다.
- When 하늘·벽·천장·계단·배치 거리 밖을 조준했다가 다시 구역 바닥을 조준한다.
- Then
  - 조준이 없는 동안 메시와 footprint 표시가 함께 사라지고, 다른 곳에 footprint만 남지 않는다.
  - 다시 조준하면 둘이 새 후보 위치에 함께 나타나며 Yaw가 유지된다.
  - grid는 내내 그대로다.

### FPV-008 확정 성공
- Given 유효한 자리를 조준하고 있다.
- When LMB로 설치한다.
- Then
  - 설비가 설치되고 메시 미리보기·footprint 표시·grid가 모두 사라진다.
  - 설치된 설비 주위에 footprint 표시가 남지 않는다.

### FPV-009 세션 종료 경로
- Given FPV-002 상태다.
- When 다음 중 하나가 일어난다: G로 아이템을 떨어뜨림, 컴퓨터 사용, 그 밖의 이유로 손의 물건이 바뀜.
- Then
  - 메시 미리보기와 footprint 표시가 함께 사라진다.
  - 다시 들면 처음부터 정상 표시된다.

### FPV-010 footprint가 메시와 같거나 작은 설비
- Given 다음 두 설비를 하나씩 든다.
  - footprint와 메시 외곽이 같은 설비(예: DrinkFridge)
  - 메시가 footprint보다 큰 설비(예: Boiler)
- When 각각 빈 바닥을 조준한다.
- Then
  - 두 설비 모두 footprint 표시가 실제 판정 영역 크기로 놓이고, 반투명 메시를 통해 보인다.
  - 메시가 큰 설비는 메시가 footprint 표시 밖으로 튀어나와 보인다.
- PIE 관찰: 메시와 크기가 같은 설비에서 footprint 경계를 알아볼 수 있는지 적는다.

### FPV-011 메시가 바닥에 닿지 않거나 바닥 아래로 내려가는 설비
- Given 다음 두 설비를 하나씩 든다.
  - Shower(메시가 바닥에서 떠 있음)
  - Bath(메시 바닥면이 설치 바닥보다 조금 아래)
- When 목욕공간 바닥을 조준한다.
- Then
  - Shower는 메시 아래 바닥에 footprint 표시가 보인다.
  - Bath는 반투명 메시를 통해 footprint 전체가 보인다.
  - footprint 표시가 Bath 메시 바닥면과 겹쳐 깜빡이지 않는다.
- PIE 관찰: Bath 메시 아래 footprint가 알아볼 만큼 보이는지 적는다.

### FPV-012 다른 공간·층
- Given 홀, 목욕공간, 지하 작업공간에 각각 허용 설비가 있다.
- When 각 공간 바닥을 조준한다(넓힘으로 늘어난 바닥 포함).
- Then 각 공간의 설치 바닥 높이에 footprint 표시가 놓이고, 동작은 FPV-001~007과 같다.

### FPV-013 판정 회귀 없음
- Given 이 기능 전에 유효였던 자리와 무효였던 자리를 하나씩 안다. Cooler·Circulator·Boiler는 이 시나리오에서 제외한다(FPV-016).
- When 같은 설비로 같은 자리를 다시 조준하고 LMB를 누른다.
- Then 유효·무효 판정과 설치 결과가 이전과 같다.

### FPV-014 활성 설비 전체
- Given 활성 Definition 16종의 아이템이 있다.
- When 하나씩 들고 허용 공간 바닥을 조준한다.
- Then
  - 모든 설비에서 footprint 표시가 그 설비의 실제 판정 footprint와 같은 크기·방향으로 보이고, 메시와 함께 움직인다.
  - LCtrl snap 중에는 16종 모두 footprint 네 변이 grid 선에 맞는다.
  - 미리보기 초기화에 실패하는 설비가 새로 생기지 않는다.

### FPV-015 미리보기 초기화 실패
- Given 미리보기를 만들 수 없는 잘못된 Definition이 있다(기존 실패 조건).
- When 그 아이템을 든다.
- Then 기존과 같이 미리보기·footprint 표시 없이 실패하고 설치를 허용하지 않는다.

### FPV-016 쿨러·순환기·보일러 footprint 수정 (Q10 B)
- Given 쿨러, 순환기, 보일러 아이템이 있다.
- When 각각 들고 허용 공간 바닥을 조준하고 LCtrl snap으로 옮긴다.
- Then
  - 쿨러:
    - footprint 표시가 지금 판정 영역을 덮는 가장 작은 정수 칸이다(지금 1.5×2.5칸 → 2×3칸).
    - 메시와의 상대 위치·방향은 그대로다. snap하면 네 변이 grid 선에 맞는다.
  - 순환기·보일러:
    - 메시와 footprint가 Actor 축과 평행해진다(약 1° 회전 제거).
    - snap하면 footprint 네 변이 grid 선과 평행하게 맞고, 메시도 함께 반듯하게 보인다.
  - 판정 변화:
    - 세 설비의 배치 판정 결과는 이 수정 때문에 이전과 달라질 수 있다. 쿨러는 판정 영역이 넓어져, 전에 놓이던 좁은 자리가 무효가 될 수 있다.
    - 판정은 수정된 footprint로 하고, footprint 표시는 그 영역과 같다.
  - 이미 놓인 설비와 회수:
    - Level에 이미 놓인 세 설비 instance도 같은 Blueprint 수정을 따른다. 순환기·보일러가 반듯해지고 쿨러 footprint가 넓어진다.
    - 세 설비의 회수·재배치·잔량 보존은 이전과 같다.
- Given grid 간격의 정수배가 아닌 world footprint를 가진 배치 설비 Blueprint가 있다. 여기에는 root scale이나 하위 component scale 때문에 정수배가 아니게 된 경우도 포함한다.
- When Data Validation을 실행한다.
- Then
  - 그 Blueprint(또는 Definition)가 오류로 보고된다.
  - 수정 뒤 활성 16종은 이 오류가 없다.
  - 검사 대상은 지금과 같이 활성 Definition의 배치 설비다.

## 6. Editor authoring 기대와 유지 계약

사용자가 조정하는 값(제안값은 `QNA_FEATURE_SPEC.md` 기본값 표, 정본에는 원본 위치만 남긴다):

- 모든 설비 공통 한 벌이다(Q8 A). 저장 위치·형식은 설계에 맡긴다.
- footprint 표시의 채움 불투명도와 외곽선 불투명도(0~1)
- 외곽선 두께(cm, 바닥 평면에서 사각형 안쪽으로)
- 바닥 위 높이(cm, 조준한 공간의 설치 바닥 기준 위쪽, grid 표시보다 위)
- 유효·무효 색은 따로 두지 않는다. 메시 미리보기 재질 색을 따른다.

footprint 크기 원본:

- 크기의 원본은 계속 배치 설비 Blueprint의 `PlacementFootprint`와 grid 간격 설정 하나다.
- 표시를 위해 설비별 크기를 따로 입력하지 않는다.

Q10 B Editor 결과:

- `BP_Cooler`: footprint world 크기가 grid 정수배(2×3칸)다. 설계가 정한 방법으로 authoring한다(extent·scale 등).
- `BP_Circulator`·`BP_Boiler`: 메시와 footprint를 담은 하위 root의 Yaw가 0이다.

유지:

- Definition 내용
- 다른 설비 Blueprint의 footprint·collision·Navigation
- grid 재질·값, Level 배치
- 메시 미리보기 재질과 그 외형(반투명·색), 복제 대상

S2 처리:

- 위 범위 밖 asset을 바꿔야 하면 멈추지 않고 진행한다.
- 해당 단계 보고(`REPORT_UNREAL_EDITOR.md` 등)에 asset, 바꾼 값, 이유를 기록한다.
- 단, 사용자 작업 트리 변경이 있는 `Content/Bathhouse/Blueprints/Facility/BP_ClothesLocker.uasset`은 저장·커밋하지 않는다.

## 7. 실패·취소·복구 계약

- 메시 미리보기가 없는 모든 경우 footprint 표시도 없다(초기화 실패, 세션 종료, 확정 성공). footprint 표시만 남는 상태는 없다.
- footprint 표시를 준비하지 못한 경우(표현 재질 누락 등):
  - 배치 판정·확정 결과를 바꾸지 않는다.
  - 판정과 다른 표시를 보이지 않는다(예: 무효인데 유효 색 footprint).
  - 실패를 로그로 알린다.
  - 메시 미리보기만 남길지, 미리보기 전체를 실패시킬지는 설계에 맡긴다.
- 확정 실패 시 아이템·미리보기·footprint 표시는 실패 직전과 같다.

## 8. 수직 구현 판단

- 판단 기준상 다음에 해당한다.
  - Content(표현 재질, 세 설비 Blueprint)와 C++가 함께 바뀐다.
  - 모든 설비에 공통 적용된다.
  - 화면 표현이 수용 기준이다.
- 사용자 결정은 Q9 A다. 한 작업 안에서 공통 구현하고 별도 확장 작업을 두지 않는다.
- PIE 순서:
  1. 대표 설비 **Vanity**(홀, 사방 같은 여백, 가로·세로가 다름)로 FPV-001~009
  2. FPV-010~015로 16종 전체
  3. FPV-016으로 세 설비 수정
- 사용자 작업 트리의 ClothesLocker_1 변경은 사용자 소유다. 이 작업은 그 asset을 저장·커밋하지 않는다.

## 9. 비목표

- footprint 높이(Z 부피)·3D 상자 표시
- 무효 사유별 영역 강조와 안내 문구 변경. 예: 겹친 부분만 다른 색, 구역 밖 부분만 강조
- 메시 미리보기 재질·외형 변경
- 쿨러·순환기·보일러 밖의 footprint·메시 크기 수정과 배치 판정 규칙 변경
- 설치된 설비 footprint 표시(Q6 A), Q 회수 중 표시(Q7 A)
- 플레이어가 footprint 표시를 켜고 끄는 입력
- Definition 밖 Actor의 footprint 수정. 대상은 출구·건조 자리·신발장, opt-out 수건 더미·수거통이다.
- `.md/Unreal/PlacementSystem.md` 기록 불일치 정리(조사 보고 6절). Editor 단계가 정본을 갱신할 때 처리한다.
- 컴퓨터 욕탕 관리 지도의 욕탕 타일 크기 문제. 별도 작업 `BUG-2026-10-02_bath_water_map_grid_scale_mismatch`에서 다룬다.
- 배송 상자 개봉 위치, 회수 아이템 크기·물리

## 설계에 맡김

- footprint 표시를 그리는 수단(별도 component, decal, 재질, 메시 등)과 소유자
- 메시 미리보기와 후보 transform·숨김·유효성 갱신을 공유하는 방법, 색을 메시 미리보기 재질 색에 연동하는 방법
- grid·Bath 메시 바닥면과의 높이 차이, 반투명 메시와 반투명 footprint 사이의 그리기 순서, 면 방향 처리
- 쿨러 footprint를 정수 칸으로 만드는 authoring 방법
- grid 비정수 footprint 검사가 root scale과 하위 component scale을 반영하는 방법
- 조정값의 저장 위치·형식, 표현 준비 실패의 구체 처리
- 자동화 테스트 구성. 화면 가림·비침 확인은 PIE 체크리스트로 보낸다(FBK-003).
