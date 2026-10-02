#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/ServiceFacilityAutomationTestSupport.h"
#include "Service/DisplayFacilityTakeSelection.h"
#include "Service/ServiceDisplaySettings.h"
#include "Components/StaticMeshComponent.h"

using namespace ServiceFacilityTest;

namespace
{
FDisplayFacilityTakeCandidate Candidate(const int32 Index, const int32 Count, const int32 Takeable,
										const TArray<FVector>& Locations)
{
	FDisplayFacilityTakeCandidate Result;
	Result.SpaceIndex = Index;
	Result.Count = Count;
	Result.TakeableCount = Takeable;
	Result.ItemLocations = Locations;
	return Result;
}

/** Builds a world, a placed vanity or shower, and an empty box in the player's hand. */
template <typename BodyType>
bool WithEmptyBoxFacility(FAutomationTestBase& Test, const TCHAR* WorldName, const bool bShower, BodyType Body)
{
	FScopedUtilityLaborWorld Scope(WorldName);
	ServiceFacilityTest::FFixture Fixture(Scope.Get());
	if (!Fixture.Install(Test, bShower))
	{
		Test.AddError(FString::Printf(TEXT("%s: facility install failed"), WorldName));
		return false;
	}
	AItemBoxActor* Box = SpawnBox(Fixture.World, nullptr, 0);
	BeginActorPlayIfNeeded(Box);
	FText Failure;
	if (!Box || !Fixture.Player.Carry->TryTakePhysicalObject(Box, Failure))
	{
		Test.AddError(FString::Printf(TEXT("%s: empty box not held"), WorldName));
		return false;
	}
	Body(Fixture, Box);
	return true;
}

void Rmb(ServiceFacilityTest::FFixture& Fixture)
{
	Fixture.Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	Fixture.Player.HeldUse->EndUse();
}

/** HUD query and highlight (only the key group, on its last filled slot) in one place. */
void ExpectTarget(FAutomationTestBase& Test, ServiceFacilityTest::FFixture& Fixture, const FString& Label, const int32 Key,
				  const bool bHighlight)
{
	const FPlayerInteractionQuery Query = Fixture.Player.Interaction->GetCurrentInteractionQuery();
	Test.TestEqual(FString::Printf(TEXT("%s: key"), *Label), Query.HeldUseTargetKey, Key);
	Test.TestTrue(FString::Printf(TEXT("%s: take row visible"), *Label), Query.bHeldTakeVisible);
	Test.TestEqual(FString::Printf(TEXT("%s: take possible"), *Label), Query.bCanHeldTake, bHighlight);
	for (const UDisplaySpaceComponent* Space : Fixture.Facility->GetSpaces())
	{
		const bool bShouldShow = bHighlight && Space->GetSpaceIndex() == Key;
		Test.TestEqual(FString::Printf(TEXT("%s: highlight of group %d"), *Label, Space->GetSpaceIndex()),
					   Space->GetTakeHighlightProxy()->IsVisible(), bShouldShow);
		if (bShouldShow)
		{
			TArray<FVector> Items;
			Space->GetVisibleItemWorldLocations(Items);
			Test.TestTrue(FString::Printf(TEXT("%s: highlight is on the last filled slot"), *Label),
						  !Items.IsEmpty() &&
							  Space->GetTakeHighlightProxy()->GetComponentLocation().Equals(Items.Last(), 1e-3));
		}
	}
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseEmptyBoxTakeSelectionRulesTest,
								 "BathhouseSim.Service.Facility.EmptyBoxTake.SelectionRules",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseEmptyBoxTakeSelectionRulesTest::RunTest(const FString& Parameters)
{
	const FVector Start = FVector::ZeroVector;
	const FVector End(1000, 0, 0);
	using Rules = FDisplayFacilityTakeSelection;
	TestNearlyEqual(TEXT("Point on the axis has zero angle"), Rules::GetAimAngleRadians(Start, End, FVector(100, 0, 0)),
					0.0, 1e-9);
	TestNearlyEqual(TEXT("45 degree point"), Rules::GetAimAngleRadians(Start, End, FVector(100, 100, 0)),
					UE_DOUBLE_PI / 4, 1e-9);
	TestTrue(TEXT("Point behind the camera is past 90 degrees"),
			 Rules::GetAimAngleRadians(Start, End, FVector(-100, 10, 0)) > UE_DOUBLE_PI / 2);
	TestNearlyEqual(TEXT("Angle ignores depth"), Rules::GetAimAngleRadians(Start, End, FVector(100, 10, 0)),
					Rules::GetAimAngleRadians(Start, End, FVector(1000, 100, 0)), 1e-9);
	TestEqual(TEXT("Zero direction gives zero"), Rules::GetAimAngleRadians(Start, Start, FVector(5, 5, 5)), 0.0);

	// EBT-002 / P2: the perpendicular distance is smaller for A (10 < 20) but the angle is smaller for B, so the
	// removed trace-line rule would have chosen A.
	{
		const TArray<FDisplayFacilityTakeCandidate> Spaces = {Candidate(0, 1, 1, {FVector(100, 10, 0)}),
															  Candidate(1, 1, 1, {FVector(300, 20, 0)})};
		TestEqual(TEXT("EBT-002: nearest by angle wins irrespective of depth"),
				  Rules::SelectSpaceIndex(Spaces, Start, End), 1);
	}
	// EBT-003: a group is as near as its nearest visible item.
	{
		const TArray<FDisplayFacilityTakeCandidate> Spaces = {
			Candidate(0, 3, 3, {FVector(100, 90, 0), FVector(100, 1, 0), FVector(100, 50, 0)}),
			Candidate(1, 1, 1, {FVector(100, 10, 0)})};
		TestEqual(TEXT("EBT-003: group distance is its nearest item"), Rules::SelectSpaceIndex(Spaces, Start, End), 0);
	}
	// P8 / EBT-001, 004: groups without takeable stock are never candidates.
	{
		const TArray<FDisplayFacilityTakeCandidate> Spaces = {Candidate(0, 1, 0, {FVector(100, 0, 0)}),
															  Candidate(1, 1, 1, {FVector(300, 200, 0)})};
		TestEqual(TEXT("EBT-004: in-use-only group is excluded"), Rules::SelectSpaceIndex(Spaces, Start, End), 1);
	}
	{
		const TArray<FDisplayFacilityTakeCandidate> Spaces = {Candidate(0, 0, 0, {}),
															  Candidate(1, 1, 1, {FVector(300, 200, 0)})};
		TestEqual(TEXT("EBT-001: empty group is excluded"), Rules::SelectSpaceIndex(Spaces, Start, End), 1);
	}
	{
		const TArray<FDisplayFacilityTakeCandidate> Spaces = {Candidate(0, 1, 1, {}),
															  Candidate(1, 1, 1, {FVector(300, 200, 0)})};
		TestEqual(TEXT("Takeable stock without visible items is not a candidate"),
				  Rules::SelectSpaceIndex(Spaces, Start, End), 1);
	}
	// P4: exact tie, lower SpaceIndex wins whatever the input order.
	{
		const TArray<FDisplayFacilityTakeCandidate> Spaces = {Candidate(3, 1, 1, {FVector(100, 10, 0)}),
															  Candidate(1, 1, 1, {FVector(100, -10, 0)})};
		TestEqual(TEXT("Tie selects the lower SpaceIndex"), Rules::SelectSpaceIndex(Spaces, Start, End), 1);
	}
	// P7: no candidate, the substitute group lets the router report the reason.
	{
		const TArray<FDisplayFacilityTakeCandidate> Spaces = {Candidate(0, 0, 0, {}),
															  Candidate(2, 1, 0, {FVector(100, 0, 0)})};
		TestEqual(TEXT("EBT-006: stocked group stands in for reasons"), Rules::SelectSpaceIndex(Spaces, Start, End), 2);
	}
	{
		const TArray<FDisplayFacilityTakeCandidate> Spaces = {Candidate(3, 0, 0, {}), Candidate(1, 0, 0, {}),
															  Candidate(2, 0, 0, {})};
		TestEqual(TEXT("EBT-005: all empty selects the lowest SpaceIndex"),
				  Rules::SelectSpaceIndex(Spaces, Start, End), 1);
	}
	TestEqual(TEXT("No groups selects nothing"), Rules::SelectSpaceIndex({}, Start, End),
			  static_cast<int32>(INDEX_NONE));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseEmptyBoxTakeVanityTest,
								 "BathhouseSim.Service.Facility.EmptyBoxTake.VanityTraceQueryCueExecute",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseEmptyBoxTakeVanityTest::RunTest(const FString& Parameters)
{
	FScopedDisplaySettings SettingsGuard;
	SettingsGuard.Settings->bShowTakeHighlight = true;
	FText Failure;

	WithEmptyBoxFacility(*this, TEXT("EmptyBoxEbt001"), false,
						 [&](ServiceFacilityTest::FFixture& Fixture, AItemBoxActor* Box)
						 {
							 auto Spaces = Fixture.Facility->GetSpaces();
							 Spaces[3]->ImportStock(Fixture.Kinds[3], 3, Failure);
							 Fixture.AimAt(0);
							 ExpectTarget(*this, Fixture, TEXT("EBT-001"), 3, true);
							 Rmb(Fixture);
							 TestTrue(TEXT("EBT-001: box holds the comb kind"),
									  Box->GetContents().Kind == Fixture.Kinds[3]);
							 TestEqual(TEXT("EBT-001: one comb taken"), Box->GetContents().Count, 1);
							 TestEqual(TEXT("EBT-001: comb group lost one"), Spaces[3]->GetStock().Count, 2);
						 });

	WithEmptyBoxFacility(*this, TEXT("EmptyBoxEbt002"), false,
						 [&](ServiceFacilityTest::FFixture& Fixture, AItemBoxActor*)
						 {
							 auto Spaces = Fixture.Facility->GetSpaces();
							 Spaces[0]->ImportStock(Fixture.Kinds[0], 2, Failure);
							 Spaces[3]->ImportStock(Fixture.Kinds[3], 3, Failure);
							 const int32 Expected[] = {0, 0, 3, 3};
							 for (int32 Aim = 0; Aim < 4; ++Aim)
							 {
								 Fixture.AimAt(Aim);
								 ExpectTarget(*this, Fixture, FString::Printf(TEXT("EBT-002 aim %d"), Aim),
											  Expected[Aim], true);
							 }
						 });

	WithEmptyBoxFacility(
		*this, TEXT("EmptyBoxEbt003"), false,
		[&](ServiceFacilityTest::FFixture& Fixture, AItemBoxActor*)
		{
			auto Spaces = Fixture.Facility->GetSpaces();
			Spaces[1]->ImportStock(Fixture.Kinds[1], 1, Failure);
			Spaces[2]->ImportStock(Fixture.Kinds[2], 4, Failure);
			TArray<FVector> LotionItems;
			TArray<FVector> SwabItems;
			Spaces[1]->GetVisibleItemWorldLocations(LotionItems);
			Spaces[2]->GetVisibleItemWorldLocations(SwabItems);
			if (!TestTrue(TEXT("EBT-003: items are visible"), LotionItems.Num() == 1 && SwabItems.Num() == 4))
			{
				return;
			}
			Fixture.AimAtPoint(LotionItems[0]);
			ExpectTarget(*this, Fixture, TEXT("EBT-003 on the lotion item"), 1, true);
			// Between the lotion item and the swab items, nearer the swabs. The empty lotion slots still pull the
			// group center toward the lotion, which is what the removed center rule would have used.
			FVector Between = LotionItems[0];
			Between.Y = (LotionItems[0].Y + SwabItems[0].Y) * 0.5 + 2.0;
			const double ToLotionItem = FVector::Dist(Between, LotionItems[0]);
			const double ToSwabItem = FVector::Dist(Between, SwabItems[0]);
			const double ToLotionCenter = FVector::Dist(Between, Spaces[1]->GetSlotsWorldCenter());
			if (!TestTrue(TEXT("EBT-003: swab item nearer than lotion item"), ToSwabItem < ToLotionItem) ||
				!TestTrue(TEXT("EBT-003: lotion slot center nearer than swab item"), ToLotionCenter < ToSwabItem))
			{
				return;
			}
			Fixture.AimAtPoint(Between);
			ExpectTarget(*this, Fixture, TEXT("EBT-003 between"), 2, true);
		});

	WithEmptyBoxFacility(*this, TEXT("EmptyBoxEbt004"), false,
						 [&](ServiceFacilityTest::FFixture& Fixture, AItemBoxActor* Box)
						 {
							 auto Spaces = Fixture.Facility->GetSpaces();
							 Spaces[1]->ImportStock(Fixture.Kinds[1], 1, Failure, 9);
							 Spaces[3]->ImportStock(Fixture.Kinds[3], 2, Failure);
							 TArray<FVector> LotionItems;
							 Spaces[1]->GetVisibleItemWorldLocations(LotionItems);
							 if (!TestEqual(TEXT("EBT-004: in-use item is visible"), LotionItems.Num(), 1))
							 {
								 return;
							 }
							 Fixture.AimAtPoint(LotionItems[0]);
							 ExpectTarget(*this, Fixture, TEXT("EBT-004"), 3, true);
							 Rmb(Fixture);
							 TestTrue(TEXT("EBT-004: comb was taken"), Box->GetContents().Kind == Fixture.Kinds[3]);
							 TestEqual(TEXT("EBT-004: comb group lost one"), Spaces[3]->GetStock().Count, 1);
							 TestEqual(TEXT("EBT-004: lotion count untouched"), Spaces[1]->GetStock().Count, 1);
							 TestEqual(TEXT("EBT-004: in-use remaining untouched"),
									   Spaces[1]->GetStock().InUseRemaining, 9);
						 });

	WithEmptyBoxFacility(
		*this, TEXT("EmptyBoxEbt005"), false,
		[&](ServiceFacilityTest::FFixture& Fixture, AItemBoxActor* Box)
		{
			TArray<FPlayerInteractionResult> Reports;
			Fixture.Player.Interaction->OnInteractionAttemptFinishedNative.AddLambda(
				[&Reports](const FPlayerInteractionResult& Result) { Reports.Add(Result); });
			Fixture.AimAt(1);
			ExpectTarget(*this, Fixture, TEXT("EBT-005"), 0, false);
			const FPlayerInteractionQuery Query = Fixture.Player.Interaction->GetCurrentInteractionQuery();
			TestEqual(TEXT("EBT-005: take reason"), Query.HeldTakeFailureReason.ToString(),
					  FString(TEXT("꺼낼 물건 없음")));
			TestEqual(TEXT("EBT-005: apply reason"), Query.HeldApplyFailureReason.ToString(),
					  FString(TEXT("박스가 비어 있음")));
			Reports.Reset();
			Rmb(Fixture);
			if (TestEqual(TEXT("EBT-005: one failure report"), Reports.Num(), 1))
			{
				TestFalse(TEXT("EBT-005: report is a failure"), Reports[0].bSucceeded);
				TestEqual(TEXT("EBT-005: report reason"), Reports[0].FailureReason.ToString(),
						  FString(TEXT("꺼낼 물건 없음")));
			}
			TestEqual(TEXT("EBT-005: box stays empty"), Box->GetContents().Count, 0);
			TestEqual(TEXT("EBT-005: no stock appears"), Fixture.Facility->GetManager()->GetTotalStock(), 0);
		});

	WithEmptyBoxFacility(*this, TEXT("EmptyBoxEbt006"), false,
						 [&](ServiceFacilityTest::FFixture& Fixture, AItemBoxActor* Box)
						 {
							 auto Spaces = Fixture.Facility->GetSpaces();
							 Spaces[1]->ImportStock(Fixture.Kinds[1], 1, Failure, 9);
							 Fixture.AimAt(3);
							 ExpectTarget(*this, Fixture, TEXT("EBT-006"), 1, false);
							 TestEqual(TEXT("EBT-006: take reason"),
									   Fixture.Player.Interaction->GetCurrentInteractionQuery()
										   .HeldTakeFailureReason.ToString(),
									   FString(TEXT("사용 중인 것은 꺼낼 수 없음")));
							 Rmb(Fixture);
							 TestEqual(TEXT("EBT-006: box stays empty"), Box->GetContents().Count, 0);
							 TestEqual(TEXT("EBT-006: stock untouched"), Spaces[1]->GetStock().Count, 1);
							 TestEqual(TEXT("EBT-006: in-use untouched"), Spaces[1]->GetStock().InUseRemaining, 9);
						 });

	WithEmptyBoxFacility(*this, TEXT("EmptyBoxEbt012"), false,
						 [&](ServiceFacilityTest::FFixture& Fixture, AItemBoxActor* Box)
						 {
							 auto Spaces = Fixture.Facility->GetSpaces();
							 Spaces[1]->ImportStock(Fixture.Kinds[1], 1, Failure);
							 Spaces[3]->ImportStock(Fixture.Kinds[3], 2, Failure);
							 TArray<FVector> LotionItems;
							 Spaces[1]->GetVisibleItemWorldLocations(LotionItems);
							 if (!TestEqual(TEXT("EBT-012: lotion item is visible"), LotionItems.Num(), 1))
							 {
								 return;
							 }
							 Fixture.AimAtPoint(LotionItems[0]);
							 ExpectTarget(*this, Fixture, TEXT("EBT-012 before"), 1, true);
							 bool bDepleted = false;
							 TestTrue(TEXT("EBT-012: customer starts the fresh bottle"),
									  Spaces[1]->ConsumeOneUse(bDepleted));
							 Fixture.Player.Interaction->RefreshInteractionQuery();
							 ExpectTarget(*this, Fixture, TEXT("EBT-012 after"), 3, true);
							 Rmb(Fixture);
							 TestTrue(TEXT("EBT-012: comb was taken"), Box->GetContents().Kind == Fixture.Kinds[3]);
							 TestEqual(TEXT("EBT-012: lotion untouched"), Spaces[1]->GetStock().Count, 1);
						 });

	SettingsGuard.Settings->bShowTakeHighlight = false;
	WithEmptyBoxFacility(*this, TEXT("EmptyBoxEbt013"), false,
						 [&](ServiceFacilityTest::FFixture& Fixture, AItemBoxActor* Box)
						 {
							 auto Spaces = Fixture.Facility->GetSpaces();
							 Spaces[3]->ImportStock(Fixture.Kinds[3], 3, Failure);
							 Fixture.AimAt(0);
							 const FPlayerInteractionQuery Query =
								 Fixture.Player.Interaction->GetCurrentInteractionQuery();
							 TestEqual(TEXT("EBT-013: key"), Query.HeldUseTargetKey, 3);
							 TestTrue(TEXT("EBT-013: take possible"), Query.bCanHeldTake);
							 for (const UDisplaySpaceComponent* Space : Spaces)
							 {
								 TestFalse(TEXT("EBT-013: no highlight"), Space->GetTakeHighlightProxy()->IsVisible());
							 }
							 Rmb(Fixture);
							 TestTrue(TEXT("EBT-013: comb was taken"), Box->GetContents().Kind == Fixture.Kinds[3]);
						 });
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseEmptyBoxTakeUnchangedTest,
								 "BathhouseSim.Service.Facility.EmptyBoxTake.ShowerAndFilledBoxUnchanged",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseEmptyBoxTakeUnchangedTest::RunTest(const FString& Parameters)
{
	FScopedDisplaySettings SettingsGuard;
	SettingsGuard.Settings->bShowTakeHighlight = true;
	SettingsGuard.Settings->InsertPreviewMaterial = TSoftObjectPtr<UMaterialInterface>(
		LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
	FText Failure;

	WithEmptyBoxFacility(*this, TEXT("EmptyBoxEbt010"), true,
						 [&](ServiceFacilityTest::FFixture& Fixture, AItemBoxActor* Box)
						 {
							 auto Spaces = Fixture.Facility->GetSpaces();
							 Spaces[1]->ImportStock(Fixture.Kinds[1], 2, Failure);
							 Fixture.AimAt(0);
							 ExpectTarget(*this, Fixture, TEXT("EBT-010"), 1, true);
							 Rmb(Fixture);
							 TestTrue(TEXT("EBT-010: body wash was taken"), Box->GetContents().Kind == Fixture.Kinds[1]);
							 TestEqual(TEXT("EBT-010: one taken"), Box->GetContents().Count, 1);
							 TestEqual(TEXT("EBT-010: shampoo group untouched"), Spaces[0]->GetStock().Count, 0);
						 });

	// EBT-011 and EBT-008: a box with a kind keeps selecting its kind group wherever the aim is.
	FScopedUtilityLaborWorld Scope(TEXT("EmptyBoxFilledWorld"));
	ServiceFacilityTest::FFixture Fixture(Scope.Get());
	if (!Fixture.Install(*this))
	{
		return false;
	}
	auto Spaces = Fixture.Facility->GetSpaces();
	Spaces[1]->ImportStock(Fixture.Kinds[1], 1, Failure);
	Spaces[3]->ImportStock(Fixture.Kinds[3], 3, Failure);
	AItemBoxActor* LotionBox = SpawnBox(Fixture.World, Fixture.Kinds[1], 2);
	if (!TestTrue(TEXT("Lotion box is held"), Fixture.Player.Carry->TryTakePhysicalObject(LotionBox, Failure)))
	{
		return false;
	}
	Fixture.AimAt(3);
	ExpectTarget(*this, Fixture, TEXT("EBT-011"), 1, true);
	TestTrue(TEXT("EBT-011: insert preview on the box kind group"), Spaces[1]->GetInsertPreview()->IsVisible());
	for (const UDisplaySpaceComponent* Space : Spaces)
	{
		if (Space->GetSpaceIndex() != 1)
		{
			TestFalse(TEXT("EBT-011: other group has no insert preview"), Space->GetInsertPreview()->IsVisible());
		}
	}
	StowBox(Fixture.Player, LotionBox);

	Spaces[0]->ImportStock(Fixture.Kinds[0], 1, Failure);
	AItemBoxActor* DryerBox = SpawnBox(Fixture.World, Fixture.Kinds[0], 1);
	if (!TestTrue(TEXT("Dryer box is held"), Fixture.Player.Carry->TryTakePhysicalObject(DryerBox, Failure)))
	{
		return false;
	}
	Fixture.AimAt(3);
	Rmb(Fixture);
	TestEqual(TEXT("EBT-008: dryer box gained one"), DryerBox->GetContents().Count, 2);
	TestEqual(TEXT("EBT-008: dryer group emptied"), Spaces[0]->GetStock().Count, 0);
	TestEqual(TEXT("EBT-008: comb group untouched"), Spaces[3]->GetStock().Count, 3);
	return true;
}
#endif
