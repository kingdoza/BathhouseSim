# PROMPT_ARCHITECTURE — PLACEMENT-FOOTPRINT-PREVIEW 배치 미리보기의 footprint 공간 표시

- 작업 ID: `PLACEMENT-FOOTPRINT-PREVIEW`
- 단계: 기능 명세
- 상태: 보류 — 초안. Editor 사전 조사 결과 반영과 사용자 질문지 답변 전이라 다음 단계 입력이 아님, 책임 단계: Editor 사전 조사 → 기능 명세, 재개 조건: `REPORT_UNREAL_DISCOVERY.md` 완료 후 질문지 갱신·사용자 답변·확정

## 1. 목적과 범위

설비 아이템을 들고 배치 미리보기를 볼 때, 지금 보이는 비주얼 메시 미리보기에 더해 그 설비의 Placement Footprint가 바닥(XY 평면)에서 차지하는 영역을 함께 보여 준다. 메시 교체(모델링 작업)로 footprint와 메시 크기가 크게 달라져도 플레이어가 "어디까지가 이 설비 자리인지"와 "왜 놓을 수 없는지"를 화면에서 바로 알 수 있게 한다.

사용자 요청 원문(2026-10-02): "Placement Footprint 가 실제 비주얼 메시보다 넓은 경우가 생길거임. 근데 현재 배치프리뷰에는 비주얼메시만 표현되다보니깐 placement footprint 가 비주얼메시의 크기랑 꽤 다를경우에는 ux 가 불편할수있음. 비주얼메시 프리뷰와 더불어서 placement footprint 의 xy평면상에서의 공간프리뷰도 보여줬으면 좋겠다."

범위:

- 플레이어가 배치용 설비 아이템을 든 동안의 배치 미리보기(활성 Definition 16종 전체, [Unreal/PlacementSystem.md](../../Unreal/PlacementSystem.md) Definition 계약 표)
- footprint 영역 표시의 모양·색·높이·가림, 기존 미리보기 동작과의 연동
- 표시 모양을 바꾸는 조정값과 그 조정 범위

범위 밖은 9절에 둔다.

## 2. 현재 동작 (정본 근거)

[Architecture/PlacementSystem.md](../../Architecture/PlacementSystem.md) Generic Native Preview, Preview Without Aim, Compatible Zone Grid Presentation, [Unreal/PlacementSystem.md](../../Unreal/PlacementSystem.md) 기준이다.

- 미리보기는 배치 설비 Blueprint의 class-default static mesh들을 복제한 반투명 메시 하나뿐이다. 모든 material slot이 유효(초록)/무효(빨강) 미리보기 재질로 바뀐다.
- 배치 판정(구역 포함·겹침·바닥 지지)은 메시가 아니라 배치 설비의 `PlacementFootprint` 상자로 한다. 겹침은 footprint 상자와 다른 물체의 blocking collision으로 판정한다. 그래서 메시보다 footprint가 넓으면 "메시는 아무것도 닿지 않는데 놓을 수 없음(빨강)"이 생기고, 플레이어는 원인 영역을 볼 수 없다.
- footprint XY 크기는 grid 간격의 정수배이며(grid 간격 원본: Project Settings `Facility Placement > Grid Size Cm`), footprint 바닥은 설비 Actor local Z=0(설치 바닥)에 있다.
- 미리보기 세션 동안 호환 구역 전체에 중립색 grid가 보인다. 이 grid는 조준·유효성과 무관하다.
- LCtrl을 누른 동안만 위치가 grid에 맞춰지고(snap), Mouse Wheel이 Yaw를 돌리며, 누적 Yaw는 LCtrl을 놓거나 조준을 잃어도 유지된다.
- (EXP-U3 D4) 후보 위치를 계산하지 못하면(거리 밖, 벽·천장·계단, 구역 없는 곳) 미리보기가 숨고, 후보가 있으면 불허·한도·구역 밖·겹침·바닥 지지 부족이어도 조준 자리에 무효 색으로 보인다.
- 현재 Unreal 정본에 기록된 footprint 크기(일부): Bath `(145,120)` 반폭, 락커 1칸 `(30,20)`, 4칸 `(30,70)`, 8칸 `(30,140)`, Washer/Dryer `(30,25)`, Circulator `(60,40)`, Boiler/Cooler `(50,30)`. 메시 크기와의 실제 차이와 나머지 6종(DrinkFridge, Vanity, MassageChair, RestBench, Television, ScrubTable)의 값은 정본에 없어 Editor 사전 조사로 확인한다(10절).

## 3. 목표 동작

용어: 이 문서에서 **footprint 표시**는 배치 미리보기 동안 바닥에 그려지는 footprint XY 점유 영역 표현이다. **메시 미리보기**는 기존 반투명 설비 메시다.

1. 배치용 설비 아이템을 들고 배치 가능한 구역 바닥을 조준하면 메시 미리보기와 footprint 표시가 함께 보인다.
2. footprint 표시는 그 설비 `PlacementFootprint`의 XY 사각형 하나다. 크기·중심·방향은 실제로 놓일 설비의 footprint와 같다(배치 확정 시 판정에 쓰는 영역과 같은 영역).
3. footprint 표시는 조준한 구역의 설치 바닥 높이에 수평으로 놓이고, 그 구역의 grid 표시보다 위에 보인다. 높이(Z 방향 부피)는 표시하지 않는다.
4. 메시 미리보기와 footprint 표시는 같은 후보 위치·Yaw를 쓰므로 언제나 함께 움직이고 함께 돈다. LCtrl snap 중에는 footprint 가장자리가 grid 선에 맞는다(footprint가 grid 간격의 정수배이므로).
5. 유효·무효 상태는 메시 미리보기와 같은 판정 결과를 따른다. 표시 색과 연동 방식은 Q2 답을 따른다.
6. 메시 미리보기가 숨으면(D4) footprint 표시도 숨고, 메시가 다시 보이면 같은 새 위치에 함께 보인다.
7. footprint 표시는 배치 판정·안내 문구·확정 결과를 바꾸지 않는다. 같은 자리의 유효·무효는 이 기능 전과 같다.
8. 표시 모양(채움·외곽선·칸 강조 등)은 Q1, 가림 처리는 Q3, footprint가 메시보다 작거나 같을 때의 표시는 Q4를 따른다.

## 4. 사용자 승인 사항과 미확정 확인

- 확정 전이다. 결정 대기 항목은 `QNA_FEATURE_SPEC.md`의 Q1~Q9, 전제 P1~P12, S2~S4다.
- 질문지는 Editor 사전 조사 결과를 반영해 갱신한 뒤 사용자에게 전달한다. 갱신 시 바뀔 수 있는 부분: 2절 수치 근거, 8절 대표 시나리오 설비, 기본값 표 제안값, Q4 추천 근거.
- 답변 후 이 절에 결정 요약(답변 일자, 자동/명시 승인)을 적는다.

## 5. 수용 시나리오 (Given/When/Then)

공통 Given: 플레이어가 배치용 설비 아이템을 들고 있고, 그 설비를 허용하는 공간이 있다. "유효 상태"·"무효 상태"의 표시 색은 Q2 답을 따른다.

### FPV-001 조준하면 메시와 footprint가 함께 보인다
- Given 허용 공간의 빈 바닥이 배치 거리 안에 있다.
- When 그 바닥을 조준한다.
- Then 메시 미리보기와 footprint 표시가 함께 보이고, footprint 표시는 바닥에 수평으로 깔린 사각형이며 grid보다 위에 있어 grid 선에 묻히거나 깜빡이지 않는다. 둘 다 유효 상태다.

### FPV-002 footprint가 메시보다 넓으면 넓은 영역이 보인다
- Given 대표 설비(8절)처럼 footprint가 메시보다 넓은 설비를 들고 있다.
- When 빈 바닥을 조준한다.
- Then footprint 표시가 메시 외곽보다 바깥까지 보이며, 그 크기는 footprint 칸 수 × grid 간격과 같다. 자기 반투명 메시 아래에 놓인 부분도 메시를 통해 보인다.

### FPV-003 LCtrl snap과 해제
- Given FPV-001 상태다.
- When LCtrl을 누른 채 조준을 천천히 옮긴 뒤 LCtrl을 놓는다.
- Then 누르는 동안 메시와 footprint 표시가 함께 칸 단위로 이동하고 footprint 네 변이 grid 선과 겹친다. 놓으면 둘이 함께 자유 이동으로 돌아가며 Yaw는 그대로다.

### FPV-004 회전
- Given 가로·세로 길이가 다른 footprint를 가진 설비(예: 락커 4칸)를 들고 있다.
- When Mouse Wheel을 한 단계씩 돌린다.
- Then 메시와 footprint 표시가 같은 각도로 함께 돌고, footprint 표시의 긴 변 방향이 메시와 같은 방향으로 바뀐다. 회전 후에도 footprint 표시는 바닥 위에 수평이다.

### FPV-005 footprint 여백 때문에 겹침 무효
- Given 설치된 다른 설비 옆에서, 들고 있는 설비의 메시는 그 설비에 닿지 않지만 footprint는 닿는 자리가 있다.
- When 그 자리를 조준하고 LMB를 누른다.
- Then 메시 미리보기와 footprint 표시가 무효 상태로 보이고 footprint 표시가 다른 설비 쪽으로 겹쳐 들어간 것이 보인다(가림은 Q3). 기존 겹침 안내 문구가 그대로 나오고 설치되지 않으며 아이템은 손에 남는다.

### FPV-006 기타 무효 사유
- Given 구역 밖으로 걸침, 공간 불허 설비, 락커 한도, 바닥 지지 부족 중 하나가 성립하는 자리가 있다.
- When 그 자리를 조준한다.
- Then 메시와 footprint 표시가 그 자리에 무효 상태로 함께 보이고(숨지 않음), 사유별 기존 안내 문구가 그대로 나온다. 구역 밖으로 걸친 경우 footprint 표시가 구역 경계(벽)를 넘는 부분은 Q3에 따라 벽에 가려진다.

### FPV-007 조준 없음(D4)과 복귀
- Given FPV-001 상태에서 Yaw를 한 번 이상 돌려 두었다.
- When 하늘·벽·천장·계단·배치 거리 밖을 조준했다가 다시 구역 바닥을 조준한다.
- Then 조준이 없는 동안 메시와 footprint 표시가 함께 사라지고, 다른 곳에 footprint만 남지 않는다. 다시 조준하면 둘이 새 후보 위치에 함께 나타나며 Yaw가 유지된다. grid는 내내 그대로다.

### FPV-008 확정 성공
- Given 유효한 자리를 조준하고 있다.
- When LMB로 설치한다.
- Then 설비가 설치되고 메시 미리보기·footprint 표시·grid가 모두 사라진다. 설치된 설비 주위에 footprint 표시가 남지 않는다.

### FPV-009 세션 종료 경로
- Given FPV-001 상태다.
- When G로 아이템을 떨어뜨리거나, 컴퓨터를 사용하거나, 다른 이유로 손의 물건이 바뀐다.
- Then 메시 미리보기와 footprint 표시가 함께 사라지고 다시 들면 처음부터 정상 표시된다.

### FPV-010 footprint가 메시보다 작거나 같은 설비
- Given footprint와 메시 외곽이 거의 같거나 메시가 footprint보다 큰 설비를 들고 있다.
- When 빈 바닥을 조준한다.
- Then Q4 답에 따라 표시한다(추천안 A: 항상 표시되어 메시 아래 바닥에서 footprint 사각형이 보인다).

### FPV-011 다른 공간·층
- Given 홀, 목욕공간, 지하 작업공간에 각각 허용 설비가 있다.
- When 각 공간 바닥을 조준한다(넓힘으로 늘어난 바닥 포함).
- Then 각 공간의 설치 바닥 높이에 footprint 표시가 놓이고 동작은 FPV-001~007과 같다.

### FPV-012 판정 회귀 없음
- Given 이 기능 전에 유효였던 자리와 무효였던 자리를 하나씩 안다.
- When 같은 설비로 같은 자리를 다시 조준하고 LMB를 누른다.
- Then 유효·무효 판정과 설치 결과가 이전과 같다.

### FPV-013 활성 설비 전체
- Given 활성 Definition 16종의 아이템이 있다.
- When 하나씩 들고 허용 공간 바닥을 조준한다.
- Then 모든 설비에서 footprint 표시가 그 설비 footprint 크기(칸 수)와 같게 보이고 메시와 함께 움직인다. 미리보기 초기화에 실패하던 설비가 새로 생기지 않는다.

### FPV-014 미리보기 초기화 실패
- Given 미리보기를 만들 수 없는 잘못된 Definition이 있다(기존 실패 조건).
- When 그 아이템을 든다.
- Then 기존과 같이 미리보기·footprint 표시 없이 실패하고 설치를 허용하지 않는다.

Q6·Q7이 B면 시나리오를 추가한다: FPV-015(설치된 설비 footprint 표시), FPV-016(Q 회수 중 대상 footprint 표시).

## 6. Editor authoring 기대와 유지 계약

- 조정값(기본값 표는 `QNA_FEATURE_SPEC.md`): footprint 표시의 채움 불투명도, 외곽선 두께(cm, 바닥 평면 기준), 외곽선 불투명도, 유효·무효 색(Q2), 바닥 위 높이(cm, 구역 설치 바닥 기준, grid보다 위). 조정 단위 범위는 Q8을 따른다. 저장 위치·소유는 설계에 맡긴다.
- footprint 크기의 원본은 계속 배치 설비 Blueprint의 `PlacementFootprint`와 grid 간격 설정 하나다. footprint 표시를 위해 설비별로 크기를 따로 입력하지 않는다.
- 유지: 메시 미리보기 재질·복제 대상, 구역 grid 재질·값, Definition 내용, 설비 Blueprint의 footprint·collision·Navigation, Level 배치.
- 새 표현 asset이 필요하면 기존 Placement 재질 폴더 규칙을 따른다(사전 허용 범위는 S2).

## 7. 실패·취소·복구 계약

- 메시 미리보기가 존재하지 않는 모든 경우(초기화 실패, 세션 종료, 확정 성공) footprint 표시도 없다. footprint 표시만 따로 남는 상태는 없다.
- footprint 표시를 준비하지 못하면(표현 재질 누락 등) 사용자 결과는 설계에서 정한다. 단 배치 판정·확정 결과를 바꾸지 않고, 메시 미리보기 없이 footprint만 보이거나 그 반대로 판정과 다른 표시가 보이는 상태를 만들지 않는다. 실패를 조용히 숨기지 않고 로그로 알린다.
- 확정 실패 시 아이템·미리보기·footprint 표시는 실패 직전과 같다.

## 8. 수직 구현 판단

- 판단 기준상 Content(새 표현 asset 가능성)와 C++가 함께 바뀌고, 모든 설비에 공통 적용되며, 화면 표현이 수용 기준이다.
- 기존 미리보기가 이미 모든 설비 공통 경로이고 설비별 코드·asset이 없으므로, 추천은 한 작업 안에서 공통 구현하고 대표 설비로 PIE를 먼저 보는 방식이다(Q9 A).
- 대표 시나리오: footprint가 메시보다 확실히 넓은 설비 하나로 FPV-001~009. 대표 설비는 Editor 사전 조사(10절 1번)의 footprint−메시 차이로 정한다. 조사 전 후보: Circulator·Boiler·Cooler(공통 sample mesh를 scale한 설비).

## 9. 비목표

- footprint 높이(Z 부피)·3D 상자 표시
- 무효 사유별 영역 강조(겹친 부분만 다른 색, 구역 밖 부분만 강조 등), 무효 사유 문구 변경
- footprint 크기·메시 크기 자체의 수정, 판정 규칙 변경
- 설치된 설비 footprint 표시(Q6 B면 범위 안), Q 회수 중 표시(Q7 B면 범위 안)
- 플레이어가 footprint 표시를 켜고 끄는 입력
- 컴퓨터 욕탕 관리 지도의 욕탕 타일 크기 문제(별도 작업 `BUG-2026-10-02_bath_water_map_grid_scale_mismatch`)
- 배송 상자 개봉 위치, 회수 아이템 크기·물리

## 설계에 맡김

- footprint 표시를 그리는 수단(별도 component, decal, 재질, 메시 등)과 소유자
- 메시 미리보기와 같은 후보 transform·숨김·유효성 갱신을 공유하는 방법
- grid와의 높이 차이, 반투명끼리의 그리기 순서를 지키는 방법(자기 메시 아래에서도 보이기, grid 위에서 깜빡이지 않기)
- 조정값의 저장 위치와 형식
- 표현 준비 실패의 구체 처리
- 자동화 테스트 구성

## 10. Editor 사전 조사 요청 (질문지 전달 전, 읽기 전용)

조사 결과로 2절 근거, 8절 대표 설비, 질문지 기본값과 Q4 근거를 갱신한다. 저장·Compile·resave 금지. `Content/Bathhouse/Blueprints/Facility/BP_ClothesLocker.uasset`은 작업 트리에서 사용자가 수정 중이므로 현재 디스크 상태를 읽기만 하고 그 사실을 보고에 적는다.

1. 활성 16개 Definition의 `PlacedFacilityClass`마다(Unreal 정본 Definition 표):
   - `PlacementFootprint`의 root scale·relative transform 적용 XY full size(cm)와 Actor root 기준 XY 중심 offset, 파생 칸 수
   - 미리보기 대상 static mesh(class default에서 보이는 non-instanced `UStaticMeshComponent`) 전체의 Actor root 기준 XY bounds(cm)와 최저·최고 Z
   - 축별 차이(footprint − 메시)와 메시가 footprint 밖으로 나가는 축·양
   - 사용자 관찰 결과: 어느 설비에서 footprint가 메시보다 눈에 띄게 넓거나 좁은지(대표 설비 선정용)
2. 현재 실제로 쓰이는 미리보기 재질: Project Settings `ValidPreviewMaterial`·`InvalidPreviewMaterial`의 저장값과 Config 값, 참조 asset(`/Game/Material/MI_Preview_*`인지 `/Game/Bathhouse/Materials/Placement/MI_FacilityPreview_*`인지)의 blend·shading·two-sided·색·opacity. Unreal 정본은 두 경로를 함께 적고 있어 실제 사용 asset을 확인한다.
3. 구역 grid 표시값: `BP_FacilityPlacementZone`·`BP_BathhouseSpace` Class Default와 DefaultMap 공간 instance 3개의 `GridZOffsetCm`, `GridLineThicknessCm`, `MajorGridIntervalCells` override 여부·값, Project Settings `Grid Size Cm` 값(footprint 표시 높이·외곽선 두께 제안값을 grid와 구별되게 정하기 위함)
4. 설치 바닥보다 아래로 내려가는 미리보기 메시(최저 Z < 0)가 있는지(바닥 표시를 메시가 덮는지 판단용)
