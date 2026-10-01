# Editor 사전 조사 — UNBOX-SPAWN-VIEW 플레이어 앞 생성 위치를 카메라 정면 기준으로

- 작업 ID: `UNBOX-SPAWN-VIEW`
- 단계: Editor 사전 조사
- 상태: 완료

(Editor 워커의 Write가 하네스에 거부되어 완료 보고 전문을 마스터가 사용자 승인 후 저장함, 2026-10-01)

기능 명세는 조사 불필요(P8)로 봤지만 사용자가 조사 진행을 결정했다. 조사 시점 사용자 답변은 Q1 B, Q2 A, Q3 A, Q4 A, Q5 B, Q6 B다. (이 조사 결과 1번을 근거로 사용자가 2026-10-01 Q3를 B로 변경했다.)

## 명세 가정과 다른 사실·사용자 선택에 영향을 줄 사실

1. **Q3 A(카메라→무리 중심 거리 100cm)에서는 큰 무리가 카메라와 플레이어 몸을 덮는다.**
   - 개봉 배치 검사는 플레이어를 무시한다(`ShopUnboxingPlacement.cpp` 127·147·163행 `AddIgnoredActor(&Player)`). 무리가 카메라·capsule과 겹쳐도 막히지 않는다.
   - Source `BuildLayout`을 그대로 옮긴 모의 계산(각 4000회, 겹침 깊이 20, 수평 시선, 무리 bounds 중심을 시선 100cm에 둠) 결과:
     - 설비 1개(60cm 정육면체): 가장 가까운 면이 약 58~70cm 앞이다. 덮지 않는다.
     - 대표 SHOP-033(설비 4개): 무리 크기 중앙값 156×155×131cm. 가장 가까운 면 중앙값 22cm 앞, **7.8%는 카메라 뒤까지**, 21%는 10cm 이내.
     - 설비 10개(카트 최대): 무리 크기 중앙값 237×237×201cm, 95백분위 약 3.2m. **82%가 카메라 뒤까지** 뻗는다.
     - 품목 박스 10개: 중앙값 90×90×70cm. 가장 가까운 면 최소 19cm로 덮지 않는다.
   - 이 상태로는 USV-002의 "무리가 카메라를 덮지 않음"과 Q3 연쇄 메모의 USV-011 문장이 성립하지 않는다.
2. **"욕탕 포함 최대 무리"의 shape는 큰 욕탕 크기가 아니다.**
   - 개봉으로 생기는 설비 아이템은 카탈로그 설비 13종(욕탕 포함) 모두 `BP_PlaceableFacilityItem_C`다. `RecoveryItemMesh`가 비어 있어 Cube fallback이고, root scale은 0.6이다. 그래서 **모두 60×60×60cm(반 30)** 정육면체다.
   - 품목 박스(7종)는 `BP_ItemBox` `BoxMesh` scale (0.4,0.3,0.1), 즉 **40×30×10cm**다.
3. **눈높이는 명세 추정 158cm가 아니라 166cm다.**
   - capsule 반높이는 96(엔진 기본 88 아님), 반지름은 30이다.
   - 카메라 `FirstPersonCamera`는 capsule 중심 기준 (0,0,70)이다. Source 생성자의 X −10을 BP가 0으로 덮어쓴다. 카메라 XY는 capsule 축과 같다.
4. 실제값으로 다시 계산한 현재 각도(시선 수평, FOV 90 기준). 화면 세로 반각은 16:9에서 29.4°, 16:10에서 32.0°다.
   - 개봉 1개
     - 무리 밑면(바닥 위 20, 100cm 앞) **55.6°** 아래(명세 약 54°)
     - 중심 49.2°
     - 가장 위로 보이는 윗면 먼 모서리도 약 31° 아래라서 화면 밖이다.
   - 봉투
     - 중심(바닥 위 25, 60cm 앞) **66.9°** 아래(명세 약 65°)
     - 윗면 먼 모서리 58.2°
   - 결론("둘 다 화면 아래 밖")은 같다.
5. Q2 A·Q3 A 아래로 볼 때 참고 사실:
   - 시선 pitch가 약 −72.5°보다 낮으면 100cm 지점이 capsule 반지름(30) 안의 수평 위치가 된다. 봉투 60cm는 −60°부터다.
   - pitch 한계는 ±80°다(`BP_FirstPersonCameraManager`). −80°에서 100cm 지점은 바닥 위 67.5cm, 수평 17cm 앞이라 바닥 아래로는 가지 않는다.

## 범위와 방법

- 실행 방식: 읽기 전용 Python(숨김 Editor `-ExecCmds`, DefaultMap 로드), 전후 dirty 0개. mutation·Compile·Save·PIE는 하지 않았다.
- 근거 파일: `Saved/Claude/Discovery3/disc_01_probe.py`, `probe.json`(character·unbox_values 절), `probe_run3.log`.
- 모의 계산: `Saved/Claude/Discovery3/disc_02_cluster_sim.py`, 결과 `cluster_sim.txt`. 시스템 Python으로 옮긴 것이라 난수열은 엔진과 다르고 분포만 근거다.
- 결과에 들어간 근거: Source `ShopUnboxingPlacement.cpp`·`ShopUnboxItemShape.cpp`·`PlaceableFacilityItemCollision.cpp`.

## 1. DefaultMap 플레이어 pawn

- 사용 경로: World Settings GameMode override `BP_FirstPersonGameMode_C` → DefaultPawn `BP_FirstPersonCharacter_C`, Controller `BP_FirstPersonController_C`, CameraManager `BP_FirstPersonCameraManager_C`.
- 레벨 배치: PlayerStart (−200,0,92), yaw 180. 레벨에 배치된 pawn은 없다.

| 항목 | 값 |
|---|---|
| Capsule half height / radius | 96 / 30 |
| FirstPersonCamera (parent CollisionCylinder) | relative (0,0,70), 회전 0, UsePawnControlRotation true |
| 카메라 FOV / AspectRatio / Constrain | 90 / 1.778 / false (뷰포트 비율 사용, 엔진 기본 MaintainXFOV, Config 변경 없음) |
| CameraManager DefaultFOV / ViewPitch | 90 / −80 ~ +80 |
| 눈높이(발바닥 기준) | 96 + 70 = **166cm** |
| BaseEyeHeight / CrouchedHalfHeight / MaxStepHeight | 64 / 40 / 45 (카메라 위치에는 쓰이지 않음) |

## 2. 생성 거리·조정값 실제값

| 값 | 위치 | 실제값 |
|---|---|---|
| `UnboxForwardDistanceCm` | `ShopSettings` CDO | 100 (`Config/DefaultGame.ini`에 키 없음, C++ 기본값) |
| `UnboxOverlapDepthCm` | `ShopSettings` CDO | 20 (`DefaultGame.ini` 23행; C++ 기본 8) |
| `CartTotalQuantityLimit` | `ShopSettings` | 10 (한 상자 최대 10개) |
| `TieForwardDistanceCm` / `TieMinForwardDistanceCm` | `/Game/Bathhouse/Blueprints/Cleaning/BP_LitterTongs` CDO | 60 / 30 |
| `TiedBagClass` | 같은 BP | `BP_TrashBag_C` |
| 봉투 shape | `BP_TrashBag` root `BagMesh` | Cube, scale (0.3,0.3,0.4) = 30×30×40cm (반높이 20) |
| 바닥 여유(개봉) | Source 상수 `FloorClearanceCm` | 20 |

- 짝마다 실제 겹침 깊이: 설비 두 개는 min(20, 0.5×30) = 15cm, 박스가 낀 짝은 min(20, 0.5×5) = 2.5cm다.

## 3. 카탈로그 상품 shape (개봉 배치 계산에 쓰이는 반 extent)

| 종류 | 상품 | 반 extent (cm) |
|---|---|---|
| 설비 아이템 | Shower, Bath, Washer, Dryer, Boiler, Cooler, Circulator, DrinkFridge, Vanity, MassageChair, RestBench, Television, ScrubTable | 30,30,30 (모두 동일) |
| 품목 박스 | BananaMilkBox, HairDryerBox, SkinLotionBox, CottonSwabBox, CombBox, ShampooBox, BodyWashBox | 20,15,5 |

- 계산 근거: 설비는 정의마다 `RecoveryItemClass=BP_PlaceableFacilityItem_C`이고, `RecoveryItemMesh`가 없어 Cube를 쓰며 root scale이 0.6이다(mesh bounds 50 × 0.6).

## 4. 무리 크기 모의 계산 요약 (`cluster_sim.txt`)

| 구성 | 크기 중앙값 X×Y×Z | 95백분위 X | Q3 A 100cm에서 가장 가까운 면 중앙값 | 카메라 뒤까지 |
|---|---|---|---|---|
| 설비 1 | 78×78×60 | 85 | 61 | 0% |
| 설비 4 (SHOP-033) | 156×155×131 | 209 | 22 | 7.8% |
| 설비 10 | 237×237×201 | 324 | −18 | 81.9% |
| 박스 10 | 90×90×70 | 122 | 55 | 0% |
| 설비 5 + 박스 5 | 199×200×159 | 268 | 0 | 49.0% |

- 같은 모의에서 무리가 capsule 기둥(반지름 30, 카메라 위 26 ~ 아래 166)과 겹친 비율: 설비 4개 66%, 설비 10개 91%.
- 이 값은 바닥·벽·천장 충돌 검사와 당김 단계 없이 시선 앞 트인 곳만 가정한 기하 추정이다.

## 미확정

- 실제 뷰포트 비율(16:9 가정)과 화면 안 보이는 범위는 PIE에서 확인할 대상이다.
- 천장 높이와 무리 높이(설비 10개 중앙값 약 2m)의 관계는 조사하지 않았다.

## 기준선과 종료

- 시작 커밋 `2c24374`. Content 변경 없음, dirty 0개다.
- 작업용 Editor 4개는 모두 종료됐고 저장한 것은 없다.
