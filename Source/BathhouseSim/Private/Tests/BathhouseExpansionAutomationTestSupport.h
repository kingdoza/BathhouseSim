#pragma once

#include "CoreMinimal.h"
#include "Building/BathhouseExpansionPurchaseSubsystem.h"
#include "Building/BathhouseSpaceActor.h"
#include "Economy/BathhousePlayerState.h"
#include "Economy/PlayerWalletComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Facility/BathhouseExpansionAuthority.h"
#include "Facility/BathhouseExpansionDefinition.h"
#include "GameplayTagContainer.h"
#include "Interaction/BathhouseKeyRackActor.h"
#include "Misc/AutomationTest.h"
#include "UI/ExpansionScreenWidget.h"
#include "UI/ExpansionSpaceOptionWidget.h"
#include "BathhouseExpansionAutomationTestSupport.generated.h"

/** 지출 callback 안에서 구입 subsystem을 다시 부르는 확장 테스트 probe. */
UCLASS(Transient, NotBlueprintable)
class UBathhouseExpansionMoneyProbe final : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleMoneyChanged(int32 PreviousMoney, int32 CurrentMoney)
	{
		(void)PreviousMoney;
		++ChangeCount;
		LastMoney = CurrentMoney;
		if (bReenter && Subsystem.IsValid() && !bReentered)
		{
			bReentered = true;
			const FBathhouseExpansionView View = Subsystem->BuildView(Buyer.Get());
			ViewBusyInCallback = View.bBusy;
			ReentryResult = Subsystem->TryPurchase(Buyer.Get(), EBathhouseSpaceKind::Bath, View.Options[1].AppliedCount);
		}
	}

	int32 ChangeCount = 0;
	int32 LastMoney = 0;
	bool bReenter = false;
	bool bReentered = false;
	bool ViewBusyInCallback = false;
	EBathhouseExpansionFailure ReentryResult = EBathhouseExpansionFailure::None;
	TWeakObjectPtr<UBathhouseExpansionPurchaseSubsystem> Subsystem;
	TWeakObjectPtr<APlayerState> Buyer;
};

/** BindWidget 없이 native 로직만 시험하는 확장 화면(Abstract 해제). */
UCLASS(Transient, NotBlueprintable)
class UExpansionScreenTestWidget final : public UExpansionScreenWidget
{
	GENERATED_BODY()
};

/** BindWidget 없이 ApplyModel의 null 안전성을 시험하는 선택지 카드(Abstract 해제). */
UCLASS(Transient, NotBlueprintable)
class UExpansionOptionTestWidget final : public UExpansionSpaceOptionWidget
{
	GENERATED_BODY()
};

/** 테스트가 확장 관련 protected 값과 private hook에 접근하는 통로. */
class FBathhouseExpansionAutomationAccess
{
public:
	struct FSpec
	{
		EBathhouseSpaceKind Kind = EBathhouseSpaceKind::Hall;
		FVector2D Center = FVector2D::ZeroVector;
		FVector2D Size = FVector2D::ZeroVector;
		double FloorZ = 0.0;
		double Ceiling = 350.0;
		EBathhouseCleaningChunkKind Chunk = EBathhouseCleaningChunkKind::None;
	};

	static ABathhouseSpaceActor* SpawnSpace(UWorld& World, const FSpec& Spec, const TCHAR* Name)
	{
		FActorSpawnParameters Params;
		Params.Name = Name;
		const FTransform Transform(FVector(Spec.Center.X, Spec.Center.Y, Spec.FloorZ));
		ABathhouseSpaceActor* Actor = World.SpawnActorDeferred<ABathhouseSpaceActor>(
			ABathhouseSpaceActor::StaticClass(), Transform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Actor)
		{
			Actor->SpaceKind = Spec.Kind;
			Actor->FloorSizeCm = Spec.Size;
			Actor->CeilingHeightCm = static_cast<float>(Spec.Ceiling);
			Actor->CleaningChunkKind = Spec.Chunk;
			Actor->Lighting.SpacingCm = 600.0f;
			Actor->Lighting.CeilingOffsetCm = 20.0f;
			Actor->Lighting.IntensityCandela = 600.0f;
			Actor->Lighting.AttenuationRadiusCm = 600.0f;
		}
		return Actor;
	}

	/** 넓힘 한 줄을 더한다. 벽 항목은 같은 양이고 순서대로 들어간다. */
	static void AddStep(ABathhouseSpaceActor& Space, const int32 Price, const float AmountCm, std::initializer_list<EBathhouseSpaceSide> Sides)
	{
		FBathhouseSpaceExpansionStep& Step = Space.ExpansionSteps.AddDefaulted_GetRef();
		Step.Price = Price;
		for (const EBathhouseSpaceSide Side : Sides)
		{
			FBathhouseSpaceExpansionSide& Wall = Step.Sides.AddDefaulted_GetRef();
			Wall.Side = Side;
			Wall.AmountCm = AmountCm;
		}
	}
	static void SetStepAmount(ABathhouseSpaceActor& Space, const int32 Index, const int32 SideIndex, const float AmountCm) { Space.ExpansionSteps[Index].Sides[SideIndex].AmountCm = AmountCm; }
	static void SetStepPrice(ABathhouseSpaceActor& Space, const int32 Index, const int32 Price) { Space.ExpansionSteps[Index].Price = Price; }
	static void SetApplied(ABathhouseSpaceActor& Space, const int32 Count) { Space.AppliedExpansionCount = Count; }
	static void AddAllowedTag(ABathhouseSpaceActor& Space, const TCHAR* Tag) { Space.AllowedFacilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName(Tag))); }
	static void AddEntrance(ABathhouseSpaceActor& Space, const EBathhouseSpaceSide Side, const float Width, const float Height, ABathhouseSpaceActor* Connected)
	{
		FBathhouseSpaceOpening& Opening = Space.Openings.AddDefaulted_GetRef();
		Opening.Side = Side;
		Opening.CenterOffsetCm = 0.0f;
		Opening.WidthCm = Width;
		Opening.HeightCm = Height;
		Opening.ConnectedSpace = Connected;
	}
	static void AddStair(ABathhouseSpaceActor& Space, ABathhouseSpaceActor* Lower)
	{
		FBathhouseStairSpec& Stair = Space.Stairs.AddDefaulted_GetRef();
		Stair.LowerSpace = Lower;
		Stair.TopEdgeCenterOffsetCm = FVector2D(-300.0, 0.0);
		Stair.DownSide = EBathhouseSpaceSide::East;
		Stair.WidthCm = 120.0f;
		Stair.RunCm = 600.0f;
		Stair.StepCount = 10;
		Stair.GuardHeightCm = 100.0f;
	}
	static TArray<FBathhouseSpaceExpansionStep>& Steps(ABathhouseSpaceActor& Space) { return Space.ExpansionSteps; }

	static void ConfigureAuthority(ABathhouseExpansionAuthority& Authority, UBathhouseExpansionDefinition* Definition, const int32 InitialTier)
	{
		Authority.ExpansionDefinition = Definition;
		Authority.InitialTierIndex = InitialTier;
	}
	static void SetAuthorityDefinition(ABathhouseExpansionAuthority& Authority, UBathhouseExpansionDefinition* Definition) { Authority.ExpansionDefinition = Definition; }

	static void AddRackPairs(ABathhouseKeyRackActor& Rack, const int32 Count)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Rack.PairTransforms.Add(FTransform(FVector(0.0f, Index * 30.0f, 100.0f)));
		}
	}

	static void InjectFailure(UBathhouseExpansionPurchaseSubsystem& Subsystem, const int32 Step)
	{
#if WITH_DEV_AUTOMATION_TESTS
		Subsystem.InjectedFailureStep = Step;
#endif
	}
};

/** 홀·목욕공간·작업공간 + 확장 관리자 + 열쇠걸이 + 구입자를 가진 game world fixture. */
struct FBathhouseExpansionTestWorld
{
	UWorld* World = nullptr;
	ABathhouseSpaceActor* Hall = nullptr;
	ABathhouseSpaceActor* Bath = nullptr;
	ABathhouseSpaceActor* Work = nullptr;
	ABathhouseExpansionAuthority* Authority = nullptr;
	ABathhouseKeyRackActor* Rack = nullptr;
	ABathhousePlayerState* Player = nullptr;
	UBathhouseExpansionDefinition* Definition = nullptr;
	UBathhouseExpansionPurchaseSubsystem* Purchase = nullptr;

	// fixture 입력. 기대값은 항상 이 값에서 계산한다.
	static constexpr int32 FixtureRackPairs = 6;
	/** 공간별 넓힘 줄 수(공간별 상한)와 벽당 양. 가격은 공간 줄마다 다르다(아래 Price 표). */
	static constexpr int32 FixtureStepCount = 2;
	static constexpr float FixtureAmountCm = 200.0f;

	static UBathhouseExpansionDefinition* MakeDefinition()
	{
		UBathhouseExpansionDefinition* Definition = NewObject<UBathhouseExpansionDefinition>();
		// 홀 넓힘 줄 수 + 1줄.
		Definition->Tiers = { { 3, 2 }, { 4, 4 }, { 6, 6 } };
		return Definition;
	}

	bool Create(FAutomationTestBase& Test, const TCHAR* BaseName, const bool bSpawnAuthority = true)
	{
		using Access = FBathhouseExpansionAutomationAccess;
		if (!GEngine)
		{
			Test.AddError(TEXT("GEngine is required."));
			return false;
		}
		// Nav 범위 volume이 없는 test world이므로 손님 길 범위 오류만 예상한다.
		Test.AddExpectedError(TEXT("손님 길 범위"), EAutomationExpectedErrorFlags::Contains, 0);
		const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), BaseName);
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		if (!World)
		{
			Test.AddError(TEXT("Failed to create the expansion world."));
			return false;
		}
		World->AddToRoot();
		Context.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
		Purchase = World->GetSubsystem<UBathhouseExpansionPurchaseSubsystem>();

		Definition = MakeDefinition();
		if (bSpawnAuthority)
		{
			Authority = World->SpawnActorDeferred<ABathhouseExpansionAuthority>(
				ABathhouseExpansionAuthority::StaticClass(), FTransform(FVector(0.0f, 5000.0f, 0.0f)));
			Access::ConfigureAuthority(*Authority, Definition, 0);
			Authority->FinishSpawning(FTransform(FVector(0.0f, 5000.0f, 0.0f)));
			if (!Authority->HasActorBegunPlay())
			{
				Authority->DispatchBeginPlay();
			}
			Rack = World->SpawnActorDeferred<ABathhouseKeyRackActor>(
				ABathhouseKeyRackActor::StaticClass(), FTransform(FVector(0.0f, 6000.0f, 0.0f)));
			Access::AddRackPairs(*Rack, FixtureRackPairs);
			Rack->FinishSpawning(FTransform(FVector(0.0f, 6000.0f, 0.0f)));
			if (!Rack->HasActorBegunPlay())
			{
				Rack->DispatchBeginPlay();
			}
		}

		FBathhouseExpansionAutomationAccess::FSpec HallSpec;
		HallSpec.Kind = EBathhouseSpaceKind::Hall;
		HallSpec.Center = FVector2D(0, 0);
		HallSpec.Size = FVector2D(2000, 1400);
		HallSpec.FloorZ = 50.0;
		HallSpec.Ceiling = 350.0;
		HallSpec.Chunk = EBathhouseCleaningChunkKind::Litter;
		FBathhouseExpansionAutomationAccess::FSpec BathSpec = HallSpec;
		BathSpec.Kind = EBathhouseSpaceKind::Bath;
		BathSpec.Center = FVector2D(1540, 0);
		BathSpec.Size = FVector2D(1000, 1300);
		BathSpec.Chunk = EBathhouseCleaningChunkKind::Stain;
		FBathhouseExpansionAutomationAccess::FSpec WorkSpec = HallSpec;
		WorkSpec.Kind = EBathhouseSpaceKind::Work;
		WorkSpec.Center = FVector2D(-200, 0);
		WorkSpec.Size = FVector2D(1400, 900);
		WorkSpec.FloorZ = 50.0 - 400.0;
		WorkSpec.Ceiling = 300.0;
		WorkSpec.Chunk = EBathhouseCleaningChunkKind::None;
		Hall = Access::SpawnSpace(*World, HallSpec, TEXT("Space_Hall"));
		Bath = Access::SpawnSpace(*World, BathSpec, TEXT("Space_Bath"));
		Work = Access::SpawnSpace(*World, WorkSpec, TEXT("Space_Work"));
		if (!Hall || !Bath || !Work)
		{
			Test.AddError(TEXT("Failed to spawn a space actor."));
			return false;
		}
		Access::AddEntrance(*Hall, EBathhouseSpaceSide::West, 200.0f, 260.0f, nullptr);
		Access::AddEntrance(*Hall, EBathhouseSpaceSide::East, 200.0f, 260.0f, Bath);
		Access::AddStair(*Hall, Work);
		// 가격은 공간마다 서로 다르다(D2). 방향은 D3 기본 모습(홀 남·북, 목욕공간 남·북·동, 작업공간 동·북).
		Access::AddStep(*Hall, 1000, FixtureAmountCm, { EBathhouseSpaceSide::South, EBathhouseSpaceSide::North });
		Access::AddStep(*Hall, 2000, FixtureAmountCm, { EBathhouseSpaceSide::South, EBathhouseSpaceSide::North });
		Access::AddStep(*Bath, 1500, FixtureAmountCm, { EBathhouseSpaceSide::South, EBathhouseSpaceSide::North, EBathhouseSpaceSide::East });
		Access::AddStep(*Bath, 2500, FixtureAmountCm, { EBathhouseSpaceSide::South, EBathhouseSpaceSide::North, EBathhouseSpaceSide::East });
		Access::AddStep(*Work, 1200, FixtureAmountCm, { EBathhouseSpaceSide::East, EBathhouseSpaceSide::North });
		Access::AddStep(*Work, 2200, FixtureAmountCm, { EBathhouseSpaceSide::East, EBathhouseSpaceSide::North });
		const TCHAR* HallTags[] = { TEXT("Facility.Type.ClothesLocker"), TEXT("Facility.Type.DrinkFridge"), TEXT("Facility.Type.MassageChair"),
			TEXT("Facility.Type.RestBench"), TEXT("Facility.Type.Television"), TEXT("Facility.Type.Vanity") };
		const TCHAR* BathTags[] = { TEXT("Facility.Type.Bath"), TEXT("Facility.Type.Shower"), TEXT("Facility.Type.ScrubTable") };
		const TCHAR* WorkTags[] = { TEXT("Facility.Type.Boiler"), TEXT("Facility.Type.Cooler"), TEXT("Facility.Type.Circulator"),
			TEXT("Facility.Type.Washer"), TEXT("Facility.Type.Dryer") };
		for (const TCHAR* Tag : HallTags) { Access::AddAllowedTag(*Hall, Tag); }
		for (const TCHAR* Tag : BathTags) { Access::AddAllowedTag(*Bath, Tag); }
		for (const TCHAR* Tag : WorkTags) { Access::AddAllowedTag(*Work, Tag); }
		for (ABathhouseSpaceActor* Space : { Hall, Bath, Work })
		{
			Space->FinishSpawning(Space->GetActorTransform());
		}
		for (ABathhouseSpaceActor* Space : { Hall, Bath, Work })
		{
			if (!Space->HasActorBegunPlay())
			{
				Space->DispatchBeginPlay();
			}
		}
		Player = World->SpawnActor<ABathhousePlayerState>(ABathhousePlayerState::StaticClass(), FTransform::Identity);
		return Player != nullptr;
	}

	ABathhouseSpaceActor* Space(const EBathhouseSpaceKind Kind) const
	{
		return Kind == EBathhouseSpaceKind::Hall ? Hall : (Kind == EBathhouseSpaceKind::Bath ? Bath : Work);
	}

	UPlayerWalletComponent* Wallet() const { return Player ? Player->GetWallet() : nullptr; }

	void Destroy()
	{
		if (World)
		{
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
			World = nullptr;
		}
	}
};
