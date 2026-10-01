# PROMPT_IMPLEMENTATION_R — DOC-TUNING-REFS 코드 리뷰 1회차 수정
- 작업 ID: `DOC-TUNING-REFS`
- 단계: 코드 리뷰
- 상태: 완료
- 출처: 코드 리뷰 1회차

## 리뷰 대상과 결론

- 범위: `git diff fc8e438 1e0bc27`(Source 식별값 `348f2413…c636`, 현재 작업 트리와 같음 확인).
- 제품 코드는 승인 수준이다. 모든 신규 property 기본값 = 기존 상수(120/15, 0.25, 10, 3, 1.0, 1.0, 5.0), `IPhysicalCarryable` 구현체 14개 전부 두 getter override, 두 zone이 `FCleaningFloorSpawnSettings` 전 필드를 채움, 테스트 기하 변환(`FloorOffset - 10`, `FloorOffset + 0.5 - 10`, `FootprintTopZ ± 1`)이 이전 수치와 동일함을 확인했다.
- 테스트 전역 상태 누수 1건(F1)만 수정이 필요하다.

## F1 (중요) `Shop.SettingsDefaults`가 `UShopSettings` CDO의 `DeliveryNoticeSeconds`를 NaN으로 남긴다

- 위치: `Source/BathhouseSim/Private/Tests/ShopAutomationTests.cpp` `FShopSettingsDefaultsAutomationTest::RunTest`가 `Settings.DeliveryNoticeSeconds = NaN`을 쓰지만, `FScopedShopSettingsOverride`(같은 파일 64~84행)는 `DeliveryNoticeSeconds`를 저장·복원하지 않는다.
- 영향: 테스트 뒤 `GetMutableDefault<UShopSettings>()`의 값이 NaN으로 남는다. 런타임 getter는 fallback으로 가려 동작은 같지만, 사용자가 Editor 세션에서 이 테스트를 돌린 뒤 Project Settings > Bathhouse Shop을 편집·저장하면 NaN이 `Config/DefaultGame.ini`에 기록될 수 있다. 같은 세션의 이후 테스트도 오염된 Settings를 본다.
- 수정 방향: `FScopedShopSettingsOverride`에 `DeliveryNoticeSeconds`를 저장 멤버와 소멸자 복원으로 추가한다(다른 필드와 같은 방식). 다른 테스트 로직은 바꾸지 않는다.
- 재검증 조건: 빌드 성공, `Automation RunTests BathhouseSim.Shop` 통과(전체 재실행은 불필요. 테스트 파일 한 곳 변경), `PROMPT_REVIEW.md` 5절의 Source 식별값 갱신.

## F2 (사소, 같이 고치면 좋음) `FCleaningFloorSpawnSettings` 주석이 실제 동작과 다르다

- 위치: `Source/BathhouseSim/Private/Cleaning/CleaningSpawnRules.h`의 "중립 기본값은 누락 시 Find가 실패하도록 한다".
- 실제로 입력 검사로 실패하는 것은 `Extent`·`Radius`·`ClearanceHeight`(0)뿐이다. `TraceDistance`·`MaximumSlopeDegrees`·`FloorTolerance` 0은 조용히 엄격해지고 `Spacing` 0은 오히려 간격 검사를 느슨하게 만든다. 현재는 두 zone이 전 필드를 채워 동작 영향이 없다.
- 수정 방향: 주석을 "호출자(구역)가 모든 필드를 채운다. 0 기본값은 조정값 복제를 피하기 위한 것이며 Radius·ClearanceHeight 누락만 입력 검사로 실패한다" 취지로 사실대로 고친다. 코드 동작은 바꾸지 않는다.

## 수정 금지

- 제품 코드 동작, 기본값, 다른 테스트, Content·Config, Architecture 정본(정본 "이전 예정" 문구 갱신은 설계 7절대로 마스터·아키텍처 몫).
