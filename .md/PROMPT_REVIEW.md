# 코드 리뷰 결과 — 상점 확장: 무리 개봉·7종 준비·배송 상자 scale

## 단계와 범위

- 현재 단계: R1과 P3 Source 구현 및 검증 완료. UE 5.8 빌드, Shop 11/11, 전체 BathhouseSim 69/69 완료(실패 0). Editor authoring/PIE는 아직 수행하지 않았다.
- 승인 및 QNA: 기존 승인 상태를 유지하며 QNA 미해결 사항은 없다.
- 기존 Source 변경을 보존하고 R1/P3의 collision shape, definition scale, 실제 spawn scale 수정과 자동화 fixture 보정을 반영했다.
- 최종 Content/Config/Level 변경은 없다. copy-first gate의 임시 BP 사본은 로드 확인 뒤 삭제했고, 원본 SHA와 Content git status가 그대로임을 확인했다.

## 시나리오 추적

| 시나리오 | 코드 경로 | 자동화 | 실행 결과 |
|---|---|---|---|
| SHOP-018~020, 038~040 | FShopUnboxingCluster 5축 SAT 무리 계산, FShopUnboxingPlacement shape-center 배치와 정면·상부 재시도 | UnboxingClusterLayout, FreshInstallTrashAndUnboxing, UnboxingDepthPlacementObservation | PASS |
| SHOP-019, 041~042 | padded environment overlap, 별도 ECC_Pawn 검사, shape-center LOS | UnboxingEnvironmentClearance, UnboxingPawnAvoidance, FreshInstallTrashAndUnboxing | PASS |
| SHOP-043, 045 | transaction overlap depth와 물리 반발; 닫힌 방 정착 | UnboxingPhysics | 코드 gate PASS(D20 effective 15cm가 D2보다 큰 분리 속도); D8 중간값은 D20보다 커 단조 증가는 미확인 |
| SHOP-035, 045 사전 | 7개 Placement Definition collision query, SpawnFreshItem, free-world 활성 | SevenDefinitionSpawn; Boiler 신규 설치 잔량 회귀 | PASS |
| 배송 상자 scale | root scale, half extent 단일 배율, HeldTransform, pickup/drop/recovery, validation | DeliveryBoxScale, BlueprintLoad | PASS |
| SHOP-032, 034~035, 044 | catalog 판매 상태, Blueprint와 UI 연결 | Editor/PIE 인계 | 미실행 |

## P1/P3 — 빌드, 자동화 및 물리 측정

### 백업과 빌드

- BP_ShopDeliveryBox 백업: Saved/MigrationBackup/20260928_shop_ext/BP_ShopDeliveryBox.uasset
- SHA-256: 193B9701268E607C9574F4E12E0BA1B3F173C967F052A84C42CF4DE4561047D7
- UE 5.8 BathhouseSimEditor Win64 Development Build.bat: 성공. 이번 R1/P3 테스트 fixture 수정 후 재빌드도 통과했다.

### Copy-first Blueprint load gate

| 순서 | 맵 / 패키지 | 결과 |
|---|---|---|
| 1. 임시 사본 | Template_Default /Game/Developers/MigrationCheck/BP_ShopDeliveryBox | 1 성공, 0 경고, 0 실패 |
| 2. 사본 정리 | 임시 사본과 Content git status | 사본 없음, 원본 SHA 동일, Content 변경 없음 |
| 3. 원본 Template | Template_Default /Game/Bathhouse/Blueprints/Shop/BP_ShopDeliveryBox | 1 성공, 0 경고, 0 실패 |
| 4. 원본 DefaultMap | /Game/Maps/DefaultMap /Game/Bathhouse/Blueprints/Shop/BP_ShopDeliveryBox | 1 성공, 0 경고, 0 실패 |

세 load gate는 이전 Source 구현 때 완료한 결과다. 이후 변경은 C++/자동화뿐이며 Blueprint와 Content는 바꾸지 않았다.

### 집중 및 전체 자동화

- 집중 결과: Saved/Automation/Reports/2026-09-28/Shop_R1_P3_Final/index.json — 11개 성공, 경고 0, 실패 0, 미실행 0.
- 전체 결과: Saved/Automation/Reports/2026-09-28/BathhouseSim_R1_P3_Final/index.json — 총 69개, 성공 62개, 경고가 있는 성공 7개, 실패 0, 미실행 0.
- 전체 로그: Saved/Logs/BathhouseSim.log.

### 물리 측정

sanity 자유낙하는 중력 −980cm/s²에서 0.5초 후 하강 속도 488.736cm/s(기대 490), 낙하 126.359cm(기대 122.500)로 통과했다.

| 요청 D | 정본의 유효 목표 Dij | 최초 겹침 | 0.5초 뒤 signed penetration | 최고 전체 선속도 | 최고 분리 방향 상대속도 | 첫 3 step 최고 분리 속도 |
|---:|---:|---:|---:|---:|---:|---:|
| 2cm | 2cm | 2.000cm | −8.728cm | 491.280cm/s | 9.319cm/s | 9.319cm/s |
| 8cm | 8cm | 8.000cm | −26.884cm | 509.518cm/s | 40.096cm/s | 38.550cm/s |
| 20cm | 15cm | 15.000cm | −11.382cm | 495.161cm/s | 22.022cm/s | 22.022cm/s |

아키텍처의 쌍별 목표는 `Dij = min(D, 0.5 × 더 작은 half extent)`다. Shower root scale 0.6에서 D=20의 유효 깊이는 15cm이므로 자동화는 D±0.5가 아니라 정본의 유효 깊이를 단언한다. 이는 테스트 기대값을 정본에 맞춘 것이며 cluster 제품 코드는 바꾸지 않았다.

sanity gate와 UnboxingPhysics가 통과했고 현재 자동화의 endpoint 조건(요청 D=20의 분리 속도 22.022 > D=2의 9.319cm/s)은 성립한다. 다만 D=8은 40.096cm/s로 D=20보다 높아, 2→8→20 전 구간의 단조 증가는 확인되지 않았다. depenetration 설정과 impulse는 변경하지 않았으며 PIE에서 세 값을 계속 비교하도록 Unreal 인계 문서에 남겼다.

닫힌 방에서 혼합 10개 물품은 모두 3초 동안 중력에 반응하고 벽·천장 안에 남았으며, 가장 낮은 collision 바닥면은 −0.00cm(바닥 top 0cm)였다.

## P2 — 비에디터 타깃 가드

ShopUnboxingScatterAutomationTests.cpp의 Misc/DataValidation.h include와 IsDataValid/FDataValidationContext 단언 블록을 WITH_EDITOR로 감쌌다. WITH_DEV_AUTOMATION_TESTS만으로 editor 전용 API를 노출하지 않는다. 데이터 검증은 움직인 runtime actor 대신 transient scale fixture의 CDO에서 수행한다.

## R1 — 환경 여유 방향과 생성 scale 검증

- 여유 shape는 `(Ex + Dc, Ey + Dc, Ez + Dc/2)`, 중심은 candidate 중심 + world Z `Dc/2`를 사용한다. LOS 끝점은 원래 shape 중심이다. 아래 방향 부풀림, 바닥 높이, Dc 범위는 바꾸지 않았다.
- `UnboxingDepthPlacementObservation`: ForwardDistance 100/500cm × D 0/8/20/30/50cm의 10개 경우 모두 정면 단계, 최저 바닥면 932cm = 발바닥 912cm + 20cm로 통과했다.
- `UnboxingEnvironmentClearance`: item shape 밖 수평 Dc 안쪽 벽 및 위쪽 Dc 안쪽 천장을 감지했고 안전 위치 재배치를 통과했다. `UnboxingPawnAvoidance`의 capsule 회피와 기존 `FreshInstallTrashAndUnboxing`도 통과했다.
- `SpawnFreshItem`은 transform scale을 root 최종 scale로 override한다. Definition scale helper가 CDO root relative scale을 반환하며, 생성 actor와 template scale이 `(0.6,0.6,0.6)`으로 일치했다. 생성 actor collision bounds 기반 바닥 +20cm 검증도 통과했다.
- P3의 scale 검사, R1의 환경 여유 및 높이 검증을 함께 적용했다. 기존 held object의 floor/clearance 규칙을 변경하지 않았다.

- `AddShopRoomBlocker`는 root 등록 뒤 owner transform을 적용하도록 고쳤다. 기존 helper는 actor spawn 후 root를 등록해 floor/wall/ceiling이 지정 좌표와 어긋났고, 이 fixture 결함이 환경 여유와 물리 정착 단언을 깨뜨렸다.
## 구현 요약 및 영향

- `UShopSettings`는 기본 8cm, 0~50cm clamp, non-finite fallback 8cm를 사용한다. `FShopUnboxingCluster`는 seeded layout과 쌍별 유효 깊이를 계산하고 `FShopUnboxingPlacement`는 Definition collision query와 환경 여유 shape를 사용한다.
- `APlaceableFacilityItemActor::GetDefinitionItemScale`가 CDO root scale을 제공하고 `SpawnFreshItem`의 두 spawn 단계가 `OverrideRootScale`을 쓴다. 상점 배치와 테스트는 같은 scale 원천을 사용한다.
- Transaction의 생성·rollback 순서와 물리 impulse, Project/Body depenetration 설정은 바꾸지 않았다. Content, Config, Level도 수정하지 않았다.
- C++ 빌드, 집중 Shop 자동화, 전체 BathhouseSim 회귀가 통과했다. 물리 테스트의 D20>D2 endpoint 비교는 통과했으나 D=8 중간 측정이 더 커 PIE 추세 확인은 남아 있다.

## 변경 파일과 크기

| 파일 | HEAD → 현재 줄 수 |
|---|---:|
| Public/Placement/PlaceableFacilityItemActor.h | 114 → 118 |
| Private/Placement/PlaceableFacilityItemActor.cpp | 417 → 422 |
| Private/Placement/PlaceableFacilityItemCollision.cpp | 100 → 133 |
| Private/Shop/ShopUnboxingPlacement.cpp | 264 → 394 |
| Private/Shop/ShopUnboxingCluster.cpp | 신규 233 |
| Private/Shop/ShopUnboxingCluster.h | 신규 27 |
| Private/Tests/ShopAutomationTests.cpp | 1166 → 1262 |
| Private/Tests/ShopUnboxingScatterAutomationTests.cpp | 신규 1373 |

Build.cs dependency는 추가하지 않았다. 기존 reflected 이름·enum 값·serialization 변경이나 Core Redirect는 없다.
