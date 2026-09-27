# 위젯 UI 워크플로 제안 — 웹 프로토타입 협의와 Unreal Python 구성

## 상태

- **미도입 시안.** 본격적인 위젯 작업을 시작할 때 도입 여부를 결정한다.
- 도입 전까지는 [AGENT_WORKFLOW.md](AGENT_WORKFLOW.md)와 기존 `AGENT_*.md` 규칙을 그대로 따른다. 이 문서는 어떤 단계에도 권한이나 결과물을 추가하지 않는다.
- 작성: 2026-09-27, 기능 명세 에이전트와 사용자 협의 결과.

## 배경

현재 워크플로에서 위젯 레이아웃 결정은 소유자가 없다.

- 기능 명세는 "무엇이 읽혀야 하는지"까지만 쓰고 세부 배치는 "Editor 확인 후 결정"으로 넘겨 왔다.
- 아키텍처는 native Widget과 WBP 경계, `BindWidget` 이름·타입만 정하고 layout/style은 WBP 소유로 둔다.
- Unreal MCP toolset에는 WidgetTree layout 편집 기능이 없다. 레이아웃은 별도 승인된 Editor Python API 작업이나 사용자 수작업으로 처리됐다.
- 이전 Codex 중심 작업은 이미지 생성으로 시안을 잡고 구현 목표로 삼았다. Claude는 이미지를 생성할 수 없고, 이미지에서 픽셀 단위 수치를 정확히 뽑아내는 일도 약하다.

결과적으로 레이아웃이 Editor 단계나 사용자에게 암묵적으로 떨어지고, 구현 목표가 수치로 고정되지 않는다.

## 핵심 방식

레이아웃과 화면 동작을 **이미지가 아니라 코드와 수치로** 합의한다.

```text
기능 명세: 웹 프로토타입(HTML) + 레이아웃 데이터(JSON) → 사용자 협의·승인
→ 명세에 수치표와 UI 시나리오 기록
→ 아키텍처: native/WBP 경계, BindWidget 이름·타입
→ C++ 구현
→ 코드 리뷰
→ Unreal Python API로 WBP 구성 (수치는 layout.json에서 읽음)
→ 수치 대조 검증 + PIE 사용자 시각 판정
→ 통합 리뷰
```

## 단계별 역할 변화

| 단계 | 추가되는 역할 | 하지 않는 것 |
|---|---|---|
| 기능 명세 | 프로토타입·`layout.json` 작성, 사용자와 레이아웃·상호작용 협의, 명세에 수치표와 UI 시나리오 기록 | native/WBP 책임 분리, `BindWidget` 이름, 데이터 구조 결정 |
| 아키텍처 | 레이아웃 요소 ID를 `BindWidget`·native 책임에 대응, WBP가 소유할 hierarchy 범위 결정 | 승인된 배치·크기·색 변경. 바꿔야 하면 기능 명세로 복귀 |
| 구현 | native Widget과 `PROMPT_UNREAL.md`에 요소 ID 대응표 전달 | 레이아웃 수치를 C++에 하드코딩 |
| Unreal 작업 | Unreal Python 스크립트로 WBP hierarchy·slot·style 구성, 저장·재로드 | Save All, allowlist 밖 asset 수정, 승인되지 않은 수치 임의 조정 |
| 통합 리뷰 | WBP 실제 slot 값과 `layout.json` 수치 대조, BindWidget 일치 확인 | 시각 판정을 모델 비전만으로 승인 |

## 프로토타입 규칙

### 프레임

- 화면별 고정 기준 해상도에서 그린다. 예: 컴퓨터 world screen은 1024×576.
- 화면 HUD처럼 해상도에 따라 늘어나는 위젯은 기준 해상도와 anchor를 함께 정한다. 기준 해상도와 DPI Scale 규칙은 도입 시 결정한다.
- 게임 상태를 바꾸는 **상태 전환 패널**은 프레임 밖에 둔다. 예: 가동 부족, 욕탕 미선택, 설비 0대, 긴 이름.

### UMG 대응 요소만 사용

| 프로토타입 | UMG |
|---|---|
| 절대 배치 영역 | Canvas Panel slot (anchor, offset, size, alignment) |
| 가로·세로 나열 | Horizontal/Vertical Box (padding, fill) |
| 겹치기 | Overlay |
| 배경·테두리 | Border, Image (단색·9-slice) |
| 버튼·슬라이더·진행 막대·텍스트 | Button, Slider, Progress Bar, Text Block |
| 격자 | Uniform Grid / Grid Panel |
| 스크롤 | Scroll Box |

- 사용 금지: box-shadow, blur·backdrop-filter, 복잡한 그라데이션, flex-wrap 자동 줄바꿈 배치, UMG Render Transform으로 표현할 수 없는 CSS transform, 웹 전용 글꼴 효과.
- 글꼴은 프로젝트에서 실제로 쓸 한글 글꼴로 맞춘다. UMG 글꼴 크기와 CSS px의 환산은 도입 시 실제 WBP 하나로 교정해 규칙을 고정한다. 줄바꿈 위치는 다를 수 있으므로 최종 판정은 PIE 화면이다.

### 동작

- 호버, 클릭, 드래그, 선택, 비활성, 실패 안내, 진행 표시처럼 **플레이어가 관찰하는 결과**만 흉내 낸다.
- 도메인 계산은 상태 전환 패널의 가짜 값으로 대신하고 실제 게임 규칙을 재현하지 않는다.
- 프로토타입 코드의 상태 처리 방식은 설계 입력이 아니다.

### 단일 수치 원본

- 레이아웃 수치는 `layout.json` 하나에 둔다. HTML은 이 JSON을 읽어 그린다.
- 각 요소는 안정적인 요소 ID를 갖는다. 요소 ID는 레이아웃 식별자이며 `BindWidget` 이름이 아니다. 대응은 아키텍처 단계가 정한다.
- 색·간격·글꼴 크기는 가능하면 공통 토큰으로 두고 요소는 토큰을 참조한다.

## 결과물과 정본 우선순위

| 결과물 | 위치(안) | 작성 단계 |
|---|---|---|
| 프로토타입 | `.md/UI/<ScreenName>/prototype.html` | 기능 명세 |
| 레이아웃 데이터 | `.md/UI/<ScreenName>/layout.json` | 기능 명세 |
| 수치표·UI 시나리오 | `.md/PROMPT_ARCHITECTURE.md` 안 | 기능 명세 |
| Unreal Python 스크립트 | 도입 시 결정 (예: `Scripts/Unreal/UI/<ScreenName>.py`) | Unreal 작업 |
| WBP 현재 구조 | `.md/Unreal/InteractionUISystem.md` 등 | Unreal 작업 |

- 충돌 시 우선순위: 명세 문장 > `layout.json` > HTML 렌더링.
- UI 시나리오는 기존 Given/When/Then 형식을 쓰고 ID 접두사는 `UI-`를 제안한다.
- 프로토타입은 사용자 확인용으로 아티팩트 링크로 공유할 수 있다. 저장소의 파일이 정본이다.

## Unreal Python API 작업 규칙(안)

현재 [AGENT_UNREAL_MCP.md](AGENT_UNREAL_MCP.md)는 MCP에 없는 Widget 편집 기능을 "Python reflection이나 asset serialization 우회로 만들어내지 않는다"고 정한다. 도입하려면 이 규칙과 구분되는 **공식 Editor Python API 사용 범위**를 명시해야 한다.

- 대상: 프롬프트 allowlist에 있는 WBP와 UI asset만.
- 방법: Unreal Editor 공식 Python API로 WidgetTree hierarchy, slot 속성, style을 설정한다. asset 파일 직접 편집이나 비공개 serialization 조작은 하지 않는다.
- 스크립트는 저장소에 남기고 여러 번 실행해도 같은 결과가 되게 만든다(idempotent). 수치는 `layout.json`에서 읽고 스크립트에 하드코딩하지 않는다.
- 필수 확인: native parent, `BindWidget` 이름·타입, Compile `BS_UP_TO_DATE`·오류/경고, 개별 Save, 새 프로세스 재로드.
- Save All은 쓰지 않는다. 예상 밖 dirty package는 보고한다.
- 적용 결과를 WBP에서 다시 읽어 `layout.json`과 자동 대조하고 차이를 보고한다.

## 검증

- **수치 검증(자동):** WBP slot의 anchor·offset·size·padding, 글꼴 크기, 색 토큰을 `layout.json`과 대조한다.
- **구조 검증(자동):** `BindWidget` 이름·타입, native parent, Event Graph에 도메인 로직이 없는지.
- **시각 검증(사용자):** PIE 화면을 프로토타입과 나란히 보고 판정한다. 모델 비전은 큰 누락·잘림 같은 거친 확인에만 쓰고 승인 근거로 삼지 않는다.

## 도입 시 수정할 문서

- `AGENT_WORKFLOW.md`: 정기 결과물 표, UI 작업 경로, 문서 크기 정책의 UI 결과물 항목
- `AGENT_FEATURE_SPEC.md`: UI 변경 시 프로토타입·수치표·UI 시나리오 필수 항목
- `AGENT_ARCHITECTURE.md`: 요소 ID → `BindWidget` 대응 책임
- `AGENT_UNREAL_MCP.md` 또는 신규 `AGENT_UNREAL_PYTHON.md`: Editor Python API 허용 범위와 절차
- `AGENT_INTEGRATION_REVIEW.md`: 수치 대조 검증
- `Architecture/UISystem.md`, `Unreal/0_UNREAL.md`: UI 결과물 라우팅

## 도입 시 결정할 사항

- HUD 기준 해상도, DPI Scale 규칙, anchor 정책
- 프로젝트 한글 글꼴, UMG 글꼴 크기 환산 규칙
- 공통 스타일 토큰(색·간격·글꼴 크기)
- Unreal Python 작업을 MCP 에이전트의 모드로 둘지, 별도 에이전트로 분리할지
- Python 스크립트 저장 위치와 실행 방법(Editor 내 실행, commandlet 등)
- 프로토타입 공유 방식(아티팩트 링크, 로컬 파일)

## 시범 대상 제안

도입을 결정하면 먼저 욕탕 관리 화면(지도, 상세 패널, 슬라이더, 용량 요약) 하나로 전 과정을 시범 운영한다. 프로토타입 협의 → Python 구성 → 수치 대조 → PIE 판정까지 한 번 거친 뒤 규칙을 확정한다.
