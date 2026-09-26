#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Tests/PlayerInteractionFocusObserverAutomationTestProbe.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionFocusObserverAutomationTest,
	"BathhouseSim.Interaction.FocusObserverLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerInteractionFocusObserverAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	if (!TestNotNull(TEXT("Automation engine exists"), GEngine))
	{
		return false;
	}

	const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("InteractionFocusObserverWorld"));
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		GEngine->DestroyWorldContext(World);
		return false;
	}
	World->AddToRoot();
	WorldContext.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();

	APawn* User = World->SpawnActor<APawn>();
	UCameraComponent* Camera = User ? NewObject<UCameraComponent>(User, TEXT("ObserverTestCamera")) : nullptr;
	UPlayerInteractionComponent* Interaction = User
		? NewObject<UPlayerInteractionComponent>(User, TEXT("ObserverTestInteraction")) : nullptr;
	if (!TestNotNull(TEXT("Observer test pawn exists"), User)
		|| !TestNotNull(TEXT("Observer test camera exists"), Camera)
		|| !TestNotNull(TEXT("Observer test interaction exists"), Interaction))
	{
		World->DestroyWorld(false);
		World->RemoveFromRoot();
		GEngine->DestroyWorldContext(World);
		return false;
	}
	User->SetRootComponent(Camera);
	User->AddInstanceComponent(Camera);
	User->AddInstanceComponent(Interaction);
	Camera->RegisterComponent();
	Interaction->RegisterComponent();
	Interaction->Configure(Camera, nullptr);
	if (!Interaction->HasBegunPlay())
	{
		Interaction->BeginPlay();
	}
	TestTrue(TEXT("Observer interaction component entered play before lifecycle assertions"), Interaction->HasBegunPlay());

	TArray<FString> Events;
	APlayerInteractionFocusObserverAutomationTarget* First = World->SpawnActor<APlayerInteractionFocusObserverAutomationTarget>(
		FVector(100.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	APlayerInteractionFocusObserverAutomationTarget* Second = World->SpawnActor<APlayerInteractionFocusObserverAutomationTarget>(
		FVector(200.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("First observer target exists"), First)
		|| !TestNotNull(TEXT("Second observer target exists"), Second))
	{
		World->DestroyWorld(false);
		World->RemoveFromRoot();
		GEngine->DestroyWorldContext(World);
		return false;
	}
	First->ConfigureEventLog(&Events, TEXT("First"));
	Second->ConfigureEventLog(&Events, TEXT("Second"));

	Interaction->RefreshInteractionQuery();
	TestEqual(TEXT("First focus commit sends one change"), First->ChangedCount, 1);
	Interaction->RefreshInteractionQuery();
	TestEqual(TEXT("An unchanged query does not notify again"), First->ChangedCount, 1);
	First->SetQueryRevision(1);
	Interaction->RefreshInteractionQuery();
	TestEqual(TEXT("A changed query sends one new notification"), First->ChangedCount, 2);

	First->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
	Second->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	const int32 EventCountBeforeReplacement = Events.Num();
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Target replacement notifies the previous observer before the new one"),
		Events.Num() == EventCountBeforeReplacement + 2
		&& Events[EventCountBeforeReplacement] == TEXT("First.Ended")
		&& Events[EventCountBeforeReplacement + 1] == TEXT("Second.Changed"));
	TestEqual(TEXT("Target replacement ends the old focus once"), First->EndedCount, 1);

	Interaction->ClearInteractionQuery();
	TestEqual(TEXT("Explicit clear ends the current observer once"), Second->EndedCount, 1);
	Interaction->ClearInteractionQuery();
	TestEqual(TEXT("Repeated clear does not send a second end notification"), Second->EndedCount, 1);

	Second->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	Interaction->SetInteractionSuppressed(true);
	TestEqual(TEXT("Suppression ends the current focus"), Second->EndedCount, 2);
	Interaction->SetInteractionSuppressed(true);
	TestEqual(TEXT("Repeated suppression does not repeat focus end"), Second->EndedCount, 2);
	Interaction->SetInteractionSuppressed(false);
	Interaction->RefreshInteractionQuery();
	TestEqual(TEXT("Unsuppression refreshes and notifies the current target"), Second->ChangedCount, 3);

	Second->SetQueryRevision(1);
	Second->ClearInteractionDuringNextNotification();
	const int32 EventCountBeforeReentry = Events.Num();
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Reentrant clear drains the changed notification then one end notification"),
		Events.Num() == EventCountBeforeReentry + 2
		&& Events[EventCountBeforeReentry] == TEXT("Second.Changed")
		&& Events[EventCountBeforeReentry + 1] == TEXT("Second.Ended"));
	TestEqual(TEXT("Reentrant focus end is delivered exactly once"), Second->EndedCount, 3);

	Second->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
	APlayerInteractionFocusObserverAutomationTarget* DestroyedTarget = World->SpawnActor<APlayerInteractionFocusObserverAutomationTarget>(
		FVector(100.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	if (TestNotNull(TEXT("Target for destruction safety exists"), DestroyedTarget))
	{
		DestroyedTarget->ConfigureEventLog(&Events, TEXT("Destroyed"));
		Interaction->RefreshInteractionQuery();
		DestroyedTarget->Destroy();
		Interaction->RefreshInteractionQuery();
		TestFalse(TEXT("Destroyed target is not called during focus cleanup"), Events.Contains(TEXT("Destroyed.Ended")));
	}

	Second->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	Interaction->DestroyComponent();
	TestEqual(TEXT("EndPlay ends its current observer before clearing delegates"), Second->EndedCount, 4);
	World->DestroyWorld(false);
	World->RemoveFromRoot();
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
