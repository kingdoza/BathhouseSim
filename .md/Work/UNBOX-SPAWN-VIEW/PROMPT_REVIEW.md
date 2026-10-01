# PROMPT_REVIEW — UNBOX-SPAWN-VIEW 플레이어 앞 생성 위치를 카메라 시선 기준으로

- 작업 ID: `UNBOX-SPAWN-VIEW`
- 단계: 구현
- 상태: 완료

## 1. 기능 계약과 단계

- 계약: 같은 폴더 `PROMPT_ARCHITECTURE.md`(USV-001~021), 설계 `PROMPT_IMPLEMENTATION.md`(아키텍처 재작업 `98db7d6` 반영본). 수직 구현 없음, 개봉과 봉투를 한 번에 구현.
- 실행 위치: 메인 작업 트리, 브랜치 `work/UNBOX-SPAWN-VIEW`, 구현 시작 커밋 `bd95749`(마스터 결정으로 PROMPT 11절의 worktree 경로 대신 메인 트리 사용).
- Content·Config 변경 없음(`git status`로 확인). 커밋 없음.

## 2. 시나리오 ID별 코드·테스트 연결

| 시나리오 | 구현 | 테스트(`BathhouseSim.` 이하) |
|---|---|---|
| USV-001, 002, 015 | `ShopUnboxingPlacement.cpp` 1단계 ViewFront, `PlayerViewFrontPlacement::ComputeViewFrontTranslation` | `Shop.UnboxViewFront.OpenAndRepresentative`(샤워기 1개·대표 4개·혼합 5개·최대 수량, seed 1~20), `Interaction.ViewFrontPlacement.Geometry` |
| USV-003~006 | 같은 경로, pitch −45·−80·+45 | `Shop.UnboxViewFront.PitchAndFloor` |
| USV-007~010 | 시선 당김(`BuildPullDistances`), 2단계 FloorFront, 3단계 Overhead, 4단계 FinalStack | `Shop.UnboxViewFront.BlockedStages`(낮은 천장·벽 당김·가까운 벽·cage) |
| USV-011, 014 | `IsCandidateSafe`의 손님(`ECC_Pawn`) 검사 재사용, seed | `Shop.UnboxViewFront.GuestAndSeed` |
| USV-012, 013, P10 | `GetCameraClearancePushCm`(FloorFront translation 뒤) | `Shop.UnboxViewFront.CameraClearance`(급한 아래 시선에서 push 경로 19/20 seed 실행 확인) |
| USV-016, 020 | 생성 뒤 처리·G/Q/배송 경로 미변경 | 기존 `Shop.FreshInstallTrashAndUnboxing`, `Service.Shop.*`, `Service.Amenity.Shop.*`, `Interaction.Equipment.*` 회귀 |
| USV-017~019 | `TrashBagDropPlacement.cpp`(ViewFront·FloorFront), `ALitterTongsActor::BuildTieDropRequest` | `Cleaning.Litter.TieViewFront.Placement`, `Cleaning.Litter.TongsAndBag`, `.BagScaleAndFrontDrop` |
| USV-021 | 위 시선 생성 + 물리 | `Shop.UnboxViewFront.RoomPhysics`, 기존 `Shop.UnboxingPhysics` |
| 값 원본 | `FShopUnboxingTuning::FromSettings`, `BuildTieDropRequest` | `Shop.UnboxViewFront.TuningSource`, `Cleaning.Litter.TieViewFront.TuningSource`, 기존 `Shop.UnboxingClusterLayout` |
| 2단계 보존 | 바닥 정면 규칙 유지 | `Shop.UnboxViewFront.FloorFrontUnchanged`(D를 `UnboxOverlapDepthCm` ClampMin·ClampMax 메타데이터와 중간 값으로 순회), `Shop.UnboxingDepthPlacementObservation` |

## 3. 변경 파일과 구현 요약

신규(Source)

- `Private/Interaction/PlayerViewFrontPlacement.h/.cpp`: 순수 기하 4개 함수. 조정값·기본값 없음.
- `Private/Shop/ShopUnboxingTuning.h/.cpp`: `FShopUnboxingTuning`(`FShopUnboxClusterTuning` 포함)과 `FromSettings`. `UShopSettings` getter를 읽는 유일한 개봉 위치 계산 지점.

수정(Source)

- `Public/Shop/ShopSettings.h`, `Private/Shop/ShopSettings.cpp`: `Shop|Unboxing` 신규 property 18개(`UPROPERTY(Config, EditAnywhere)` + ClampMin/ClampMax)와 getter, `Default*` 상수(기본값 정본, 기존 `UnboxForwardDistanceCm`·`UnboxOverlapDepthCm` fallback도 이 상수 사용), `MaxUnboxOverlapDepthCm`·`UnboxMinStepCm`. 기존 property 이름·의미·기본값 불변.
- `Private/Shop/ShopUnboxingPlacement.h/.cpp`: `EShopUnboxPlacementStage`, `FShopUnboxingPlacementRequest`, 새 `FindSpawnTransforms`(구 시그니처 삭제). 단계 orchestration(`TryMakeLayout`은 translation 콜백만 바꿔 끼움, 안전 검사 복제 없음). 파일 상단 상수와 loop 리터럴 삭제.
- `Private/Shop/ShopUnboxingCluster.h/.cpp`: `BuildLayout`이 `FShopUnboxClusterTuning`을 받음. 상수 4개와 비율 리터럴, `Clamp(DepthCm, 0, 50)` 삭제(범위 정리는 `UShopSettings::GetUnboxOverlapDepthCm`). 알고리즘·난수 소비 순서 불변.
- `Private/Shop/ShopUnboxingTransaction.cpp`: context 카메라 값으로 request를 채우고 `FromSettings`로 tuning 생성. `CameraOrigin` NaN이면 `MissingUnboxContext`.
- `Private/Cleaning/TrashBagDropPlacement.h/.cpp`: `ETrashBagDropStage`, `FTrashBagDropRequest`(멤버 초기값 0), 새 `Find`. `UCameraComponent` 조회 제거. 후보 검사 함수 하나를 두 단계가 공유.
- `Public/Cleaning/LitterTongsActor.h`, `Private/Cleaning/LitterTongsActor.cpp`: `Tie*` property 6개 추가(기존 `TieForwardDistanceCm`·`TieMinForwardDistanceCm` 불변), `BuildTieDropRequest`(Tie 값을 읽는 유일한 지점), `IsDataValid`에 새 값 무효 조합 오류 추가.

테스트

- 신규: `Tests/PlayerViewFrontPlacementAutomationTests.cpp`, `Tests/ShopUnboxViewFrontAutomationTests.cpp`, `Tests/CleaningLitterTieViewFrontAutomationTests.cpp`.
- 갱신: `ShopUnboxShapeTestSupport.h`(`MakeTuning`·`MakeRequest`·`MakeFloorStageTuning`), `ShopAutomationTests.cpp`(`OpenContext.CameraOrigin`, 열린 바닥 ViewFront 단언, 바닥 정면 기대값을 tuning에서 읽기), `ShopUnboxingScatterAutomationTests.cpp`(tuning 기반 `BuildLayout`·fallback 비교는 `UShopSettings::Default*`, D 순회는 메타데이터 범위), `CleaningLitterAutomationTestSupport.h`(`Context()`에 카메라 값, `TieRequest`·`TieFloorOnlyRequest`), `CleaningLitterToolAutomationTests.cpp`(전 높이 벽, 수직 시선 단언을 ViewFront 성공으로).

기존 바닥 정면·머리 위·최후 단계 검증 테스트는 `MakeFloorStageTuning`/`TieFloorOnlyRequest`로 시선 단계를 건너뛰어(무효 간격) 단계 2~4만 검증한다. 설계 10.2의 "눈높이 판으로 막음" 방식은 새 테스트(`FloorFrontUnchanged`, `TieViewFront.*`)가 쓴다.

## 4. 클래스 크기·책임 변화

| 파일 | 변경 전 → 후(줄) | 판단 |
|---|---|---|
| `ShopUnboxingPlacement.cpp` | 391 → 490 | 500 이하. 단계 orchestration만 추가, 안전 검사 단계별 복제 없음 |
| `LitterTongsActor.cpp` / `.h` | 397 → 437 / 143 → 168 | 설계 허용(약 30줄)을 넘어 40줄. 늘어난 것은 `BuildTieDropRequest`와 `IsDataValid` 검사(값 원본 지점·authoring 검증) |
| `TrashBagDropPlacement.cpp` | 76 → 142 | 두 단계와 공유 후보 검사 |
| `ShopSettings.h/.cpp` | 53 → 167 / 42 → 153 | 18개 조정값의 선언·getter(원본 이전의 직접 결과) |

책임 경계는 설계 5.4와 같다. 순수 기하만 Interaction private helper로 공유하고 단계 순서·충돌·손님 검사는 각 시스템이 소유한다. Shop → Interaction, Cleaning → Interaction 의존만 추가(새 module 없음).

## 5. Blueprint·API·Core Redirect 영향

- reflected 추가만(`UShopSettings` 18개, `ALitterTongsActor` 6개). rename·삭제 없음, Core Redirect 불필요.
- `FindSpawnTransforms`·`FTrashBagDropPlacement::Find`·`FShopUnboxingCluster::BuildLayout` 시그니처 변경: private 헤더이며 호출자는 transaction·tongs·테스트뿐(모두 갱신).
- `Config/DefaultGame.ini` 변경 없음(키가 없으면 header 기본값 사용). `BP_LitterTongs` resave 없음.

## 6. 검증 결과

- 빌드: UE 5.8 `BathhouseSimEditor Win64 Development`, `Result: Succeeded`. 경고 0(UBT 출력의 `warning` 없음). 최종 빌드 로그 `Saved/Logs/usv_build_final.log`(증분이라 컴파일 출력은 없음). 중간 빌드 로그는 Automation 실행이 `Saved/Automation/Logs`를 정리해 남지 않았다. 컴파일 오류 수정 3회 뒤 성공했고 오류 로그는 폐기됐다.
- 빌드 시점 Source 식별값: HEAD `bd957496aa0c718a882b8eb9e0c2730c9b5e2769`, `git diff HEAD -- Source Config` + 신규 untracked 7개 파일(`### 경로` 구분, 경로 오름차순) 내용 연결본의 SHA-256 `5D815EEA8386DEDD9A07599A728B80FE3342C3934135F750CA0B8CE098AE44A7`(`git diff HEAD -- Source Config`만의 SHA-256은 `558FFDB6E9DCB3B063FCD159445109725D7A03E51AF51F45F9DEFCF2DB3FA529`). 최종 빌드는 이 상태에서 수행했다. 마지막 Automation 이후 Source 수정 없음.
- Automation(`UnrealEditor-Cmd`, `-DDC-ForceMemoryCache`, 한 번에 `+`로 이은 필터): `BathhouseSim.Interaction.ViewFrontPlacement`, `BathhouseSim.Shop`, `BathhouseSim.Cleaning`, `BathhouseSim.Service.Shop`, `BathhouseSim.Service.Amenity.Shop`, `BathhouseSim.Interaction.Equipment` 총 46개 전부 통과. 로그: `Saved/Automation/Logs/usv_auto3_editor.log`(Editor 로그 사본, 현재 남아 있음). 이전 두 회차에서는 새 시나리오 테스트의 fixture 오류(벽 거리·회전 미반영·천장 높이)를 고쳤다. 제품 코드의 검사를 완화한 수정은 없다.
- 관찰 지표: `OpenAndRepresentative`에서 샤워기 1개·대표 4개·혼합 5개·최대 수량 모두 ViewFront 20/20, `CameraClearance`는 push 경로 19/20 seed.
- 정적 검사: `git diff --check`는 공백 오류 없음(LF/CRLF 변환 경고만). 4절 값 리터럴 점검: `ShopUnboxingPlacement.cpp`·`ShopUnboxingCluster.cpp`·`TrashBagDropPlacement.cpp`·`PlayerViewFrontPlacement.cpp`에 남은 수는 0 판정·`0.5`(중앙·여유 shape 기하)·`360`(yaw 정의역)·`AxisEpsilon`·`PairLimit + 0.01f`(기존 쌍 깊이 비교의 수치 안정 허용 오차)뿐이다. 마지막 항목은 설계 4.4에 명시되지 않은 기존 리터럴이라 리뷰어 판단을 요청한다(동작 조정 값이 아니라 float 비교 오차).
- Content·Config 변경 없음: `git status --short -- Content Config` 출력 없음.

## 7. 리뷰 중점, 전역 영향, 미검증

- 설계 12절 기준(원본 밖 수치 없음, 읽는 지점 둘, 기본값 불변, "가장 가까운 부분" 정의 한 곳, 시야 기준점, 봉투 Pawn 검사 없음, Placement 500줄 이하)을 그대로 확인할 것.
- FinalStack은 설계대로 규칙을 바꾸지 않았다. 천장이 낮아 무리가 들어갈 단이 없으면 플레이어 자리 쌓기에서 카메라가 물품 안에 있을 수 있다(설계 PROMPT 10.1 `.CameraClearance`(b)의 "단계와 무관하게 카메라가 box 안에 없음"은 FinalStack을 제외해 테스트했다). 사용자 PIE에서 매우 낮은 천장 최대 수량 개봉 시 확인 대상이다.
- `FloorFrontUnchanged` 등 일부 테스트는 시선 단계를 얇은 판으로 막아 FloorFront를 검증하고, 기존 회귀 테스트는 무효 간격으로 시선 단계를 건너뛴다.
- 개봉 `OverlapDepthCm` getter fallback·범위는 기존 동작과 같다(`MaxUnboxOverlapDepthCm` 상수화).
- 미검증: 실제 Editor·PIE의 카메라 입력(`UPlayerEquipmentUseComponent::BuildContext`)과 Project Settings UI 표시. `Interaction.Equipment`·`Service.*Shop` 회귀 테스트가 실제 컴포넌트 경로의 개봉 성공을 통과했다.
- Architecture 정본(`ShopSystem.md`, `CleaningLitterSystem.md`, `InteractionSystem.md`, `0_ARCHITECTURE.md`)은 아키텍처 단계에서 이미 현재 구조로 반영돼 있어 구현 중 갱신하지 않았다. 구현이 추가한 `UShopSettings::MaxUnboxOverlapDepthCm`·`UnboxMinStepCm` 같은 보조 상수와 `FShopUnboxClusterTuning` 위치는 정본의 책임 기술과 충돌하지 않는다.
