#include "Tests/BathhouseEconomyTestProbe.h"

#include "Economy/BathhouseCashPaymentActor.h"
#include "Economy/BathhousePlayerState.h"
#include "Economy/PlayerWalletComponent.h"
#include "GameFramework/Pawn.h"
#include "UObject/UnrealType.h"

void UBathhouseCashReentryTestProbe::Initialize(
	ABathhouseCashPaymentActor* InCashActor,
	const FPlayerInteractionContext& InContext)
{
	CashActor = InCashActor;
	InteractionContext = InContext;
}

void UBathhouseCashReentryTestProbe::BindToWallet(UPlayerWalletComponent* InWallet)
{
	UnbindFromWallet();
	Wallet = InWallet;
	if (Wallet)
	{
		Wallet->OnMoneyChanged.AddDynamic(this, &UBathhouseCashReentryTestProbe::HandleMoneyChanged);
	}
}

void UBathhouseCashReentryTestProbe::UnbindFromWallet()
{
	if (Wallet)
	{
		Wallet->OnMoneyChanged.RemoveDynamic(this, &UBathhouseCashReentryTestProbe::HandleMoneyChanged);
	}
	Wallet = nullptr;
}

void UBathhouseCashReentryTestProbe::HandleMoneyChanged(const int32 PreviousMoney, const int32 CurrentMoney)
{
	if (bDidAttemptReentry || !CashActor)
	{
		return;
	}
	bDidAttemptReentry = true;
	++ReentryAttemptCount;
	ReentryResult = CashActor->ExecuteInteraction(InteractionContext);
}

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
struct FScopedEconomyAutomationWorld
{
	FWorldContext* Context = nullptr;
	UWorld* World = nullptr;

	bool Initialize(FAutomationTestBase& Test)
	{
		if (!GEngine)
		{
			Test.AddError(TEXT("GEngine is required for Economy automation."));
			return false;
		}
		const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("EconomyAutomationWorld"));
		Context = &GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		if (!World)
		{
			GEngine->DestroyWorldContext(World);
			Context = nullptr;
			Test.AddError(TEXT("Failed to create an Economy automation world."));
			return false;
		}
		World->AddToRoot();
		Context->SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return true;
	}

	void Destroy()
	{
		if (World)
		{
			World->DestroyWorld(false);
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
			World->RemoveFromRoot();
			World = nullptr;
		}
		Context = nullptr;
	}

	~FScopedEconomyAutomationWorld()
	{
		Destroy();
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseWalletIdempotenceTest,
	"BathhouseSim.Economy.WalletValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseWalletIdempotenceTest::RunTest(const FString& Parameters)
{
	FScopedEconomyAutomationWorld TestWorld;
	if (!TestWorld.Initialize(*this))
	{
		return false;
	}

	ABathhousePlayerState* PlayerState = TestWorld.World->SpawnActor<ABathhousePlayerState>(
		ABathhousePlayerState::StaticClass(), FTransform::Identity);
	TestNotNull(TEXT("Player state is spawned for wallet initialization"), PlayerState);
	UPlayerWalletComponent* Wallet = PlayerState ? PlayerState->GetWallet() : nullptr;
	TestNotNull(TEXT("Wallet component is created"), Wallet);
	const FIntProperty* StartingMoneyProperty = FindFProperty<FIntProperty>(
		UPlayerWalletComponent::StaticClass(), TEXT("StartingMoney"));
	TestNotNull(TEXT("Wallet StartingMoney property exists"), StartingMoneyProperty);
	if (!Wallet || !StartingMoneyProperty)
	{
		return false;
	}

	const int32 StartingMoney = StartingMoneyProperty->GetPropertyValue_InContainer(Wallet);
	TestEqual(TEXT("Wallet uses the configured starting balance"), StartingMoney, 100000);
	TestEqual(TEXT("Wallet initialization applies the starting balance"), Wallet->GetCurrentMoney(), StartingMoney);
	TestFalse(TEXT("Zero is rejected"), Wallet->TryAddMoney(0));
	TestFalse(TEXT("Negative values are rejected"), Wallet->TryAddMoney(-1));
	TestTrue(TEXT("Positive payment succeeds"), Wallet->TryAddMoney(10000));
	TestEqual(TEXT("Payment is applied exactly once per successful call"), Wallet->GetCurrentMoney(), StartingMoney + 10000);
	TestFalse(TEXT("Overflow is rejected"), Wallet->TryAddMoney(MAX_int32));
	TestEqual(TEXT("Rejected overflow leaves the balance unchanged"), Wallet->GetCurrentMoney(), StartingMoney + 10000);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseCashReentrancyTest,
	"BathhouseSim.Economy.CashReentrancy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseCashReentrancyTest::RunTest(const FString& Parameters)
{
	FScopedEconomyAutomationWorld TestWorld;
	if (!TestWorld.Initialize(*this))
	{
		return false;
	}

	ABathhousePlayerState* PlayerState = TestWorld.World->SpawnActor<ABathhousePlayerState>(
		ABathhousePlayerState::StaticClass(), FTransform::Identity);
	APawn* Pawn = TestWorld.World->SpawnActor<APawn>(APawn::StaticClass(), FTransform::Identity);
	ABathhouseCashPaymentActor* CashActor = TestWorld.World->SpawnActor<ABathhouseCashPaymentActor>(
		ABathhouseCashPaymentActor::StaticClass(), FTransform::Identity);
	UBathhouseCashReentryTestProbe* Probe = NewObject<UBathhouseCashReentryTestProbe>();
	TestNotNull(TEXT("Player state is created"), PlayerState);
	TestNotNull(TEXT("Pawn is created"), Pawn);
	TestNotNull(TEXT("Cash actor is created"), CashActor);
	TestNotNull(TEXT("Reentry probe is created"), Probe);
	if (!PlayerState || !Pawn || !CashActor || !Probe)
	{
		return false;
	}

	Pawn->SetPlayerState(PlayerState);
	UPlayerWalletComponent* Wallet = PlayerState->GetWallet();
	TestNotNull(TEXT("Player wallet is available"), Wallet);
	if (!Wallet)
	{
		return false;
	}
	const int32 StartingMoney = Wallet->GetCurrentMoney();

	FPlayerInteractionContext Context;
	Context.Interactor = Pawn;
	Context.HitActor = CashActor;
	Probe->Initialize(CashActor, Context);
	Probe->BindToWallet(Wallet);

	const FPlayerInteractionResult Result = CashActor->ExecuteInteraction(Context);
	TestTrue(TEXT("The original cash claim succeeds"), Result.bSucceeded);
	TestEqual(TEXT("The money-change listener attempts exactly one reentry"), Probe->GetReentryAttemptCount(), 1);
	TestFalse(TEXT("The reentrant cash claim is rejected"), Probe->GetReentryResult().bSucceeded);
	TestEqual(TEXT("One cash actor adds exactly one payment"), Wallet->GetCurrentMoney(), StartingMoney + 10000);
	TestTrue(TEXT("The cash actor remains committed as claimed"), CashActor->IsClaimed());

	Probe->UnbindFromWallet();
	return true;
}
#endif
