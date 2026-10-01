# PROMPT_REVIEW — DOC-TUNING-REFS 코드 상수의 데이터 이전
- 작업 ID: `DOC-TUNING-REFS`
- 단계: 구현
- 상태: 완료

## 1. 기능 계약과 단계

- 입력: `PROMPT_IMPLEMENTATION.md`(기능 명세 생략, 동작 불변 리팩터, 시나리오 ID 없음). 단계 시작 커밋 `fc8e438`. 재개 조건(UNBOX-SPAWN-VIEW main 병합, 사용자 재개 지시)은 마스터 확인으로 충족.
- 모든 신규 property 기본값 = 기존 상수. `Default*` 상수는 `UShopSettings` header가 원본.

## 2. 설계 절 ↔ 코드·테스트

| 절 | 코드 | 테스트 |
|---|---|---|
| 3.1 배송 재시도 간격 | `ShopSettings.h/.cpp`(`DeliveryAttemptIntervalSeconds`, `GetDeliveryAttemptIntervalSeconds`, `DefaultDeliveryAttemptIntervalSeconds`), `ShopOrderSubsystem.cpp` `Tick` | `Shop.OrderDelayWaitingAndFIFO`(fixture 간격·딜레이로 tick 수 계산), `Shop.SettingsDefaults` |
| 3.2 getter fallback 중복 | `ShopSettings.h/.cpp`(`DefaultDeliveryDelaySeconds`, `DefaultDeliveryNoticeSeconds`), `ShopNoticeWidget.cpp` 삼항 제거 | `Shop.SettingsDefaults`(NaN 입력에서 `Default*`와 비교) |
| 3.3 남은 시간 갱신 간격 | `ShopScreenWidget.h/.cpp` `CountdownRefreshIntervalSeconds`, `Max(0.1, 값)` | 없음(UI Tick, 사용자 PIE) |
| 3.4 clearance 띄움 | `CleaningDirectorActor.h`(`SpawnClearanceFloorOffsetCm`, getter 2개), `CleaningSpawnRules.h/.cpp`(`ClearanceFloorOffset`, 입력 검사, struct 기본값 0/중립), 두 zone `FindSpawnTransform` 기본 인자 제거와 인자 추가, director 전달 | `Cleaning.Clearance.*`, `CleaningTowelAutomationTests` 구역 후보, `CleaningLitterSpawnAutomationTests`의 floor query(`FCleaningLitterFloorTest`) |
| 3.5 발밑 정리 Z 허용 오차 | `WaterStainActor.h`, `LitterActor.h`(`PlacementClearHeightToleranceCm`+getter), `Intersects(…, HeightToleranceCm)`, `CleaningWorldSubsystem.cpp` | `Cleaning.Litter.PlacementClear`(허용 오차 안·밖 경계 추가) |
| 3.6 놓기 속도 | `PhysicalCarryable.h` 두 함수 pure virtual, `UtilityShovelActor.h` override 2개, 설비 세 Actor fallback을 `GetDefault<UFacilityPlacementComponent>()` getter로 | `Utility.Labor.ShovelReleaseVelocityUsesProperty`(신규), `PhysicalCarryFixedSlot` 기대 속도를 Basket getter로 |
| 4절 기타 | `HeldTargetUseAutomationTests`의 0.15 리터럴을 component CDO 값 비교로, `ServiceFridge` guard에 새 property 저장·복원 | 해당 테스트 |

## 3. 변경 파일(Source만)

- Shop·UI: `Public/Shop/ShopSettings.h`, `Private/Shop/ShopSettings.cpp`, `Private/Shop/ShopOrderSubsystem.cpp`, `Private/UI/ShopNoticeWidget.cpp`, `Public/UI/ShopScreenWidget.h`, `Private/UI/ShopScreenWidget.cpp`
- Cleaning: `Public/Cleaning/{CleaningDirectorActor,StainSpawnZoneActor,LitterSpawnZoneActor,WaterStainActor,LitterActor}.h`, `Private/Cleaning/{CleaningDirectorActor,CleaningSpawnRules(.h/.cpp),StainSpawnZoneActor,LitterSpawnZoneActor,CleaningWorldSubsystem}.cpp`
- Carry: `Public/Interaction/PhysicalCarryable.h`, `Public/Utility/UtilityShovelActor.h`, `Private/Facility/{BathhouseFacilityActor,BathWaterUtilityFacilityActor}.cpp`, `Private/Towel/TowelProcessingMachineActor.cpp`
- 테스트: `ShopAutomationTests`, `ServiceFridgeAutomationTests`, `CleaningSpawnClearanceAutomationTests`, `CleaningTowelAutomationTests`, `CleaningLitterSpawnAutomationTests`, `PhysicalCarryFixedSlotAutomationTests`, `UtilityLaborRecoveryAutomationTests`(신규 삽 테스트), `HeldTargetUseAutomationTests`
- UNBOX 금지 대상(`ShopUnboxing*`, `TrashBagDropPlacement`, `ALitterTongsActor` Tie 값, `PlayerViewFrontPlacement`) 미수정. Content·Config 미수정.

## 4. 클래스 크기·책임·API 영향

- 각 class에 property 1~2개와 getter만 추가. 독립 책임 추가 없음. `ShopSettings.h` 175줄, `ShopSettings.cpp` 161줄.
- 상태 owner·의존 방향 변화 없음. Core Redirect 불필요(rename·삭제 없음).
- C++ API 변경: `IPhysicalCarryable::GetThrowImpulseStrength/GetUpwardThrowImpulseStrength` pure virtual(Blueprint 비노출). 구현체 15개 중 삽만 override가 없었고 추가함. `AStainSpawnZoneActor/ALitterSpawnZoneActor::FindSpawnTransform`은 기본 인자를 지우고 `ClearanceFloorOffset` 인자 추가(호출자는 director와 테스트뿐). `FCleaningFootprintOverlap::Intersects`에 인자 추가(private header).
- 결함 수정: 삽의 `ThrowImpulseStrength`·`UpwardThrowImpulseStrength` UPROPERTY가 인터페이스 상수에 가려 무시되던 것을 이제 반영한다(BP CDO 값 확인은 `PROMPT_UNREAL.md` 2절).

## 5. 빌드·테스트 결과와 Source 식별값

- 빌드: UE 5.8 `BathhouseSimEditor Win64 Development` 성공(Editor 미실행 확인). 로그 `Saved/Logs/DocTuningBuild.log`.
- Automation: `Automation RunTests BathhouseSim` 전체, 통과 154, 실패 0, `EXIT CODE: 0`. 신규 `Shop.SettingsDefaults`·`Utility.Labor.ShovelReleaseVelocityUsesProperty` 포함. 로그 `Saved/Logs/DocTuningAutomation.log`, 리포트 `Saved/Automation/Reports/20261001/DocTuning`.
- Source 식별값: HEAD `fc8e43825d3c906354883065f937a1460a11e931`, `git diff fc8e438 -- Source Config | sha256sum` = `348f2413840e9cf54cb0240be182359512194b7bfb4dfa569d45bddc0ceec636`(빌드·테스트 이후 Source 수정 없음).
- `git diff --check`는 이 작업 트리의 LF/CRLF 정규화 경고만 낸다(공백 오류 없음).

## 6. 리뷰 중점과 미검증

- 2절 분류에 맞지 않는 새 리터럴이 없는지. 남은 의도적 리터럴: getter clamp(`Max(0.1f, …)` 등 ClampMin과 같은 하한), 테스트 fixture 값.
- `FCleaningFloorSpawnSettings` 중립 기본값(0): zone이 항상 모두 채우므로 동작 불변. 누락 시 `Find`는 높이·반경·띄움 입력 검사로 실패한다(`TraceDistance`·`MaximumSlopeDegrees`·`FloorTolerance`·`Spacing`은 0이어도 입력 검사 대상이 아니라 조용히 엄격해짐. zone 두 곳이 모두 채운다).
- `CleaningLitterSpawnAutomationTests`·`CleaningTowelAutomationTests` 등이 fixture 반경·허용 오차를 로컬 값으로 둠(제품 값 고정 아님).
- 범위 밖 참고: `BathhouseEconomyTests.cpp`의 `StartingMoney == 100000` 등 데이터 기본값 고정 단언(설계 4절 말미).
- 미검증: 상점 화면 남은 시간 갱신 간격과 삽 BP CDO 값은 UI·Editor 의존(사용자 PIE·`PROMPT_UNREAL.md`). Architecture 정본의 "이전 예정" 문구는 설계 7절대로 구현 후 아키텍처·마스터가 갱신하며 구현 단계는 정본을 수정하지 않았다.
