# 통합 검토 결과 — 상점 확장 R1/P3 (2026-09-28)

## 판정

**부분 완료; 통합 승인 보류.** 사용자가 코드 리뷰 물리 gate 통과를 근거로 대기를 해제했다. Catalog와 배송 상자 Blueprint는 MCP authoring, Compile, 개별 Save까지 완료했다. 새 프로세스 reload가 시작 단계에서 멈췄고, WBP hierarchy는 현재 MCP로 조회·편집할 수 없어 완결 상태를 확인하지 못했다.

사용자는 이번 단계에서 PIE와 검증을 제외했다. PIE 및 명시적 Data Validation은 호출하지 않았다.

## MCP 연결과 범위

- 기존 작업용 UE 5.8 Editor PID 30960에 Streamable HTTP MCP로 연결해 실제 asset 조회와 편집을 수행했다. 대상 Editor와 `127.0.0.1:8000` listener PID가 일치했다.
- 도구 목록은 `AssetTools`, `BlueprintTools`, `ObjectTools`, `ActorTools`를 포함했다. Widget tree 전용 toolset은 없었다. Computer Use, Python reflection, commandlet asset 수정, Save All은 사용하지 않았다.
- 시작 전 Catalog와 Blueprint는 clean이었으며 변경 후 package 경로 기준 dirty=false를 확인했다. WBP도 clean이었고 수정·저장하지 않았다.

## 저장한 Editor authoring

### Catalog

- `/Game/Bathhouse/Data/Shop/DA_ShopCatalog`: 기존 Shower 행을 보존하고 `Bath`, `Washer`, `Dryer`, `Boiler`, `Cooler`, `Circulator`를 뒤에 추가했다.
- 새 행은 모두 `bForSale=true`, 가격 `10000`, Icon 없음이다. 표시 이름은 욕조, 세탁기, 건조기, 보일러, 쿨러, 순환기이며 각 `DA_FacilityPlacement_*` Definition 참조를 지정했다. 일곱 Definition의 MCP class 조회 결과는 모두 `FacilityPlacementDefinition`이었다.
- Catalog는 단독 Save 성공을 반환했고 package 저장 로그가 남았다. 저장 후 package 경로의 dirty 상태는 false다.

### 배송 상자

- `/Game/Bathhouse/Blueprints/Shop/BP_ShopDeliveryBox`: Parent `/Script/BathhouseSim.ShopDeliveryBoxActor`, root `BoxMesh`를 유지했다.
- 기존 mesh 참조 `/Game/Bathhouse/Meshes/SM_Facility_sample.SM_Facility_sample`와 상대 위치·회전 0을 보존했다. scale은 `(0.8,0.8,0.8)`, `QueryAndPhysics`, CCD On, Pawn Ignore다. `HeldTransform`은 위치 `(10,50,-60)`, 회전 identity, scale `(1,1,1)`로 설정했다.
- BP Compile은 `warnings_as_errors=true`로 성공 응답을 반환했다. Blueprint와 Catalog를 각각 개별 Save했으며 package 저장 로그가 확인됐다. BP package 경로 기준 dirty=false다.
- mesh asset reference는 유지했다. 이번 요청에서 시각 수용을 제외했으므로 형상이 cube인지 viewport로 판정하지 않았다.

### Project Settings와 WBP

- Shop Settings CDO 및 `Config/DefaultGame.ini`에서 Catalog와 DeliveryBoxClass 참조가 계약 경로와 일치했다. `UnboxOverlapDepthCm=8`도 확인했다. Config는 이미 올바르므로 수정하지 않았다.
- `/Game/Bathhouse/UI/Shop/WBP_ShopScreen`의 parent는 `/Script/BathhouseSim.ShopScreenWidget`이다. `ObjectTools`의 `WidgetTree`, `GeneratedClass`, `ParentClass` 조회는 generated class CDO에서 읽지 못했다. 현재 toolset에는 UMG tree 조회·편집 tool이 없어 `ProductScroll`과 cart/order panel의 형제 관계를 확인하거나 고칠 수 없었다. WBP는 변경하지 않았다.

## Compile, Save, 재로드

- `BP_ShopDeliveryBox` Compile 및 두 allowlist asset의 개별 Save는 성공했다. Save 직후 엔진 로그에 `AssetCheck: ... Validating asset`가 남았지만, 이를 명시적 Data Validation 결과로 취급하지 않는다.
- 새 프로세스 reload를 위해 PID 30960을 종료한 뒤 PID 24300으로 UE 5.8 Editor를 재실행했다. `LogTurnkeySupport`의 SDK 확인 시작 뒤 약 10분 동안 `Saved/Logs/BathhouseSim.log`가 14:33:28에서 갱신되지 않았고, `Intermediate/TurnkeyLog_0.log`와 MCP listener가 생성되지 않았다. 원인은 확정하지 않았다.
- PID 24300은 작업 소유 프로세스임을 확인한 뒤 종료했다. PID 30960 및 24300 모두 소멸했고 port 8000 listener와 UnrealEditor 프로세스가 남지 않았다. 새 프로세스의 asset reload는 미완료다.

## 남은 상태

- 새 Editor가 MCP 초기화까지 정상 진입한 뒤 저장한 Catalog와 BP의 fresh-process reload를 확인해야 한다. 현재 in-memory readback/Save 로그는 fresh-process 확인이 아니다.
- WBP의 `ProductScroll` 안에는 상품 목록만 두고 cart/order panel은 바깥 형제로 남기는 layout 확인·필요한 수정은 MCP 미지원이다. 세부 조작은 `.md/USER_UNREAL.md`에 기록했다.
- 사용자가 제외한 PIE와 Data Validation 결과는 미실행이다. 어떤 통과도 주장하지 않는다.
- fresh-process reload 전이므로 `.md/Unreal/ShopSystem.md`의 확인된 저장 상태는 갱신하지 않았다. `.md/PROMPT_UNREAL.md`에는 실행 결과와 미완료 상태를 표시했다.

## 변경 범위

이번 Editor 작업으로 저장한 Content는 `DA_ShopCatalog.uasset`, `BP_ShopDeliveryBox.uasset` 두 개다. Config, Level, WBP, Source는 수정하지 않았다. 새 Editor와 MCP listener는 모두 종료 상태다.