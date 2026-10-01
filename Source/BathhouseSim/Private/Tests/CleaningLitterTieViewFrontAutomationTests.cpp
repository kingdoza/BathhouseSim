#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/CleaningLitterAutomationTestSupport.h"
#include "Character/FirstPersonCharacter.h"
#include "Cleaning/TrashBagDropPlacement.h"
#include "GameFramework/Character.h"
#include "Misc/DataValidation.h"
using namespace CleaningLitterTest;

// USV-017..019 and the Tie value-source contract. Every expectation is computed from the request that
// ALitterTongsActor::BuildTieDropRequest fills (the runtime's single read point) or from the bag class shape.
namespace
{
constexpr double TieViewFrontTol = 0.2;

struct FTieFixture
{
	UWorld* World = nullptr;
	FPlayer Player;
	ALitterTongsActor* Tongs = nullptr;
	TArray<AActor*> Ignored;

	bool Init(FAutomationTestBase& Test, UWorld* InWorld)
	{
		World = InWorld;
		if (!BuildPlayer(Test, World, Player))
		{
			return false;
		}
		AddCapsule(Player);
		Box(World, FVector(0, 0, -50), FVector(5000, 5000, 50)); // floor, top at z = 0 (feet)
		Tongs = CleaningLitterTest::Tongs(World);
		Ignored = { Player.Pawn, Tongs };
		return true;
	}

	FTrashBagDropRequest Request() const
	{
		FTrashBagDropRequest Out;
		Tongs->BuildTieDropRequest(Context(const_cast<FPlayer&>(Player), Tongs), Out);
		return Out;
	}

	FVector Camera() const { return Player.Camera->GetComponentLocation(); }

	bool Shape(const FTransform& Transform, FVector& OutCenter, FQuat& OutRotation, FCollisionShape& OutShape) const
	{
		const UPrimitiveComponent* Template = nullptr;
		FText Failure;
		return ATrashBagActor::BuildClassCollisionQuery(
			ATrashBagActor::StaticClass(), Transform, OutCenter, OutRotation, OutShape, Template, Failure);
	}

	bool Find(const FTrashBagDropRequest& Request, FTransform& OutTransform, ETrashBagDropStage& OutStage) const
	{
		OutStage = ETrashBagDropStage::None;
		return FTrashBagDropPlacement::Find(
			*World, Player.Pawn, Ignored, ATrashBagActor::StaticClass(), Request, OutTransform, &OutStage);
	}

	// The shape of a bag standing upright at the pawn yaw, used to size the expectations.
	FVector HalfExtent() const
	{
		FVector Center;
		FQuat Rotation;
		FCollisionShape Shape;
		this->Shape(FTransform(FRotator(0, Player.Pawn->GetActorRotation().Yaw, 0), FVector::ZeroVector), Center, Rotation, Shape);
		return Shape.GetExtent().GetAbs();
	}

	AActor* AddPlate(const FVector& Center, const FVector& HalfExtent) const
	{
		return Box(World, Center, HalfExtent)->GetOwner();
	}

	/** View-stage-only blocker: a thin eye-level plate half the minimum view distance in front of the camera. */
	void AddViewStageBlocker(const FTrashBagDropRequest& Request) const
	{
		AddPlate(Camera() + FVector(Request.ViewMinDistanceCm * 0.5f, 0, 0), FVector(0.5f, 3000.0f, 20.0f));
	}
};
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCleaningLitterTieViewFrontTest, "BathhouseSim.Cleaning.Litter.TieViewFront.Placement",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCleaningLitterTieViewFrontTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("TieViewFront"));
	UWorld* World = Scope.Get();
	FTieFixture Fixture;
	if (!World || !Fixture.Init(*this, World))
	{
		return false;
	}
	const FTrashBagDropRequest Request = Fixture.Request();
	const FVector Camera = Fixture.Camera();
	const FVector Half = Fixture.HalfExtent();
	FTransform Transform;
	ETrashBagDropStage Stage = ETrashBagDropStage::None;

	// USV-017: level view, bag nearest part at the view distance, centered on the view ray, yaw = player yaw.
	TestTrue(TEXT("USV-017: level view places"), Fixture.Find(Request, Transform, Stage));
	TestTrue(TEXT("USV-017: level view uses the view-front stage"), Stage == ETrashBagDropStage::ViewFront);
	FVector Center;
	FQuat Rotation;
	FCollisionShape Shape;
	if (TestTrue(TEXT("Result resolves a collision shape"), Fixture.Shape(Transform, Center, Rotation, Shape)))
	{
		const FVector Expected = Camera + FVector::ForwardVector * (Request.ViewDistanceCm + Half.X);
		TestTrue(TEXT("USV-017: bag center = camera + view * (distance + half depth)"),
			(Center - Expected).Size() <= TieViewFrontTol);
		TestTrue(TEXT("USV-017: bag yaw is the player yaw"),
			FMath::Abs(FMath::FindDeltaAngleDegrees(Transform.Rotator().Yaw, Fixture.Player.Pawn->GetActorRotation().Yaw)) <= 0.5f);
	}

	// USV-018: downward view, litter and a stain at the candidate do not block and the bag is above the floor.
	Fixture.Player.Camera->SetWorldRotation(FRotator(-45, 0, 0));
	const FTrashBagDropRequest Pitched = Fixture.Request();
	FTransform Free;
	TestTrue(TEXT("USV-018: pitched view places"), Fixture.Find(Pitched, Free, Stage));
	TestTrue(TEXT("USV-018: pitched view uses the view-front stage"), Stage == ETrashBagDropStage::ViewFront);
	FVector FreeCenter;
	if (Fixture.Shape(Free, FreeCenter, Rotation, Shape))
	{
		TestTrue(TEXT("USV-018: bag bottom is above the floor"), FreeCenter.Z - Shape.GetExtent().Z > 0.0);
		Litter(World, FreeCenter);
		AActor* Stain = World->SpawnActor<AWaterStainActor>(AWaterStainActor::StaticClass(), FTransform(FreeCenter));
		TestNotNull(TEXT("Stain is spawned at the candidate"), Stain);
		FTransform WithClutter;
		TestTrue(TEXT("USV-018: litter and a stain do not block"), Fixture.Find(Pitched, WithClutter, Stage));
		TestTrue(TEXT("USV-018: clutter keeps the view-front stage and position"),
			Stage == ETrashBagDropStage::ViewFront && WithClutter.GetLocation().Equals(Free.GetLocation(), 0.1f));
		if (Stain)
		{
			Stain->Destroy();
		}
	}

	// A guest next to the candidate does not block either (bags never check Pawn).
	if (ACharacter* Guest = World->SpawnActor<ACharacter>(ACharacter::StaticClass(), FreeCenter, FRotator::ZeroRotator))
	{
		FTransform WithGuest;
		TestTrue(TEXT("A guest at the candidate does not block the bag"), Fixture.Find(Pitched, WithGuest, Stage));
		TestTrue(TEXT("A guest does not change the stage"), Stage == ETrashBagDropStage::ViewFront);
		Guest->Destroy();
	}

	// Floor stage: the eye-level plate blocks only the view stage.
	Fixture.Player.Camera->SetWorldRotation(FRotator::ZeroRotator);
	const FTrashBagDropRequest Level = Fixture.Request();
	Fixture.AddViewStageBlocker(Level);
	FTransform Floor;
	TestTrue(TEXT("Blocked view stage falls back to the floor"), Fixture.Find(Level, Floor, Stage));
	TestTrue(TEXT("Blocked view stage uses the floor-front stage"), Stage == ETrashBagDropStage::FloorFront);
	if (Fixture.Shape(Floor, Center, Rotation, Shape))
	{
		const FVector Feet(0, 0, 0);
		const FVector Expected = Feet + FVector::ForwardVector * Level.FloorForwardDistanceCm
			+ FVector::UpVector * (Shape.GetExtent().Z + Level.FloorClearanceCm);
		TestTrue(TEXT("Floor-front center = feet + forward * distance + (half height + clearance)"),
			(Center - Expected).Size() <= TieViewFrontTol);
	}

	// USV-019: a full-height wall inside the minimum distance fails, execution keeps the count.
	AActor* Wall = Fixture.AddPlate(Camera + FVector(Level.ViewMinDistanceCm * 0.5f + 5.0f, 0, 0), FVector(0.5f, 3000.0f, 3000.0f));
	FText Failure;
	TestTrue(TEXT("Tongs are held"), Fixture.Player.Carry->TryTakePhysicalObject(Fixture.Tongs, Failure));
	Set<FIntProperty>(Fixture.Tongs, TEXT("BagCount"), 5);
	int32 BagsBefore = 0;
	for (TActorIterator<ATrashBagActor> It(World); It; ++It) { ++BagsBefore; }
	TestFalse(TEXT("USV-019: blocked drop fails"), Fixture.Find(Level, Floor, Stage));
	const FHeldEquipmentUseResult Result = Fixture.Tongs->ExecuteSecondaryEquipmentUse(Context(Fixture.Player, Fixture.Tongs));
	TestFalse(TEXT("USV-019: execution fails"), Result.bSucceeded);
	TestEqual(TEXT("USV-019: failure reason"), Result.FailureReason.ToString(), FString(TEXT("봉투를 놓을 공간이 없음")));
	TestEqual(TEXT("USV-019: bag count is unchanged"), Fixture.Tongs->GetBagCount(), 5);
	int32 BagsAfter = 0;
	for (TActorIterator<ATrashBagActor> It(World); It; ++It) { ++BagsAfter; }
	TestEqual(TEXT("USV-019: no bag was spawned"), BagsAfter, BagsBefore);
	Wall->Destroy();

	// Straight down: view-front works, the floor stage has no horizontal direction and is not attempted.
	Fixture.Player.Camera->SetWorldRotation(FRotator(-90, 0, 0));
	const FTrashBagDropRequest Down = Fixture.Request();
	FTransform DownTransform;
	TestTrue(TEXT("Vertical view places in front of the camera"), Fixture.Find(Down, DownTransform, Stage));
	TestTrue(TEXT("Vertical view uses the view-front stage"), Stage == ETrashBagDropStage::ViewFront);
	{
		Fixture.AddPlate(Camera - FVector(0, 0, Down.ViewMinDistanceCm * 0.5f), FVector(3000.0f, 3000.0f, 0.5f));
		TestFalse(TEXT("Blocked vertical view has no floor stage"), Fixture.Find(Down, DownTransform, Stage));
		TestTrue(TEXT("Blocked vertical view reports no stage"), Stage == ETrashBagDropStage::None);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCleaningLitterTieTuningSourceTest, "BathhouseSim.Cleaning.Litter.TieViewFront.TuningSource",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCleaningLitterTieTuningSourceTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("TieTuningSource"));
	UWorld* World = Scope.Get();
	FTieFixture Fixture;
	if (!World || !Fixture.Init(*this, World))
	{
		return false;
	}
	ALitterTongsActor* T = Fixture.Tongs;
	Set<FFloatProperty>(T, TEXT("TieViewDistanceCm"), 83.0f);
	Set<FFloatProperty>(T, TEXT("TieViewMinDistanceCm"), 41.0f);
	Set<FFloatProperty>(T, TEXT("TieViewPullStepCm"), 7.0f);
	Set<FFloatProperty>(T, TEXT("TieForwardDistanceCm"), 77.0f);
	Set<FFloatProperty>(T, TEXT("TieMinForwardDistanceCm"), 33.0f);
	Set<FFloatProperty>(T, TEXT("TieForwardPullStepCm"), 9.0f);
	Set<FFloatProperty>(T, TEXT("TieFloorClearanceCm"), 11.0f);
	Set<FFloatProperty>(T, TEXT("TieCameraClearanceCm"), 12.5f);
	const FTrashBagDropRequest Request = Fixture.Request();
	TestEqual(TEXT("View distance"), Request.ViewDistanceCm, 83.0f);
	TestEqual(TEXT("View minimum"), Request.ViewMinDistanceCm, 41.0f);
	TestEqual(TEXT("View step"), Request.ViewPullStepCm, 7.0f);
	TestEqual(TEXT("Floor distance"), Request.FloorForwardDistanceCm, 77.0f);
	TestEqual(TEXT("Floor minimum"), Request.FloorMinForwardDistanceCm, 33.0f);
	TestEqual(TEXT("Floor step"), Request.FloorPullStepCm, 9.0f);
	TestEqual(TEXT("Floor clearance"), Request.FloorClearanceCm, 11.0f);
	TestEqual(TEXT("Camera clearance"), Request.CameraClearanceCm, 12.5f);
	TestTrue(TEXT("Camera origin comes from the use context"), Request.CameraOrigin.Equals(Fixture.Camera(), 0.01f));

	// The placement follows the request: view distance and floor values.
	FTransform Transform;
	ETrashBagDropStage Stage = ETrashBagDropStage::None;
	FVector Center;
	FQuat Rotation;
	FCollisionShape Shape;
	TestTrue(TEXT("Changed view distance places"), Fixture.Find(Request, Transform, Stage));
	if (Fixture.Shape(Transform, Center, Rotation, Shape))
	{
		TestTrue(TEXT("View-front follows the changed distance"),
			Stage == ETrashBagDropStage::ViewFront
			&& FMath::Abs(Center.X - (Fixture.Camera().X + Request.ViewDistanceCm + Shape.GetExtent().X)) <= TieViewFrontTol);
	}
	Fixture.AddViewStageBlocker(Request);
	TestTrue(TEXT("Changed floor values place"), Fixture.Find(Request, Transform, Stage));
	if (Fixture.Shape(Transform, Center, Rotation, Shape))
	{
		TestTrue(TEXT("Floor-front follows the changed distance and clearance"),
			Stage == ETrashBagDropStage::FloorFront
			&& FMath::Abs(Center.X - Request.FloorForwardDistanceCm) <= TieViewFrontTol
			&& FMath::Abs(Center.Z - (Shape.GetExtent().Z + Request.FloorClearanceCm)) <= TieViewFrontTol);
	}

	// Invalid stage values skip only that stage.
	FTrashBagDropRequest NoView = Fixture.Request();
	NoView.ViewPullStepCm = 0.0f;
	FTrashBagDropRequest NoFloor = Fixture.Request();
	NoFloor.FloorMinForwardDistanceCm = 0.0f;
	TestTrue(TEXT("A zero view step skips the view stage"), Fixture.Find(NoView, Transform, Stage));
	TestTrue(TEXT("A zero view step lands on the floor stage"), Stage == ETrashBagDropStage::FloorFront);
	TestFalse(TEXT("A zero floor minimum skips the floor stage (view blocked)"), Fixture.Find(NoFloor, Transform, Stage));
	TestTrue(TEXT("Skipped stages report no stage"), Stage == ETrashBagDropStage::None);

#if WITH_EDITOR
	const auto Validate = [&](const TCHAR* Name, const float Value)
	{
		FDataValidationContext Context;
		const float Saved = FindFProperty<FFloatProperty>(T->GetClass(), Name)->GetPropertyValue_InContainer(T);
		Set<FFloatProperty>(T, Name, Value);
		const EDataValidationResult Result = T->IsDataValid(Context);
		Set<FFloatProperty>(T, Name, Saved);
		return Result;
	};
	TestTrue(TEXT("Authored values validate"), Validate(TEXT("TieViewPullStepCm"), 7.0f) == EDataValidationResult::Valid);
	TestTrue(TEXT("Non-positive view step is invalid"), Validate(TEXT("TieViewPullStepCm"), 0.0f) == EDataValidationResult::Invalid);
	TestTrue(TEXT("View distance below the minimum is invalid"), Validate(TEXT("TieViewDistanceCm"), 40.0f) == EDataValidationResult::Invalid);
	TestTrue(TEXT("Non-positive view minimum is invalid"), Validate(TEXT("TieViewMinDistanceCm"), 0.0f) == EDataValidationResult::Invalid);
	TestTrue(TEXT("Negative camera clearance is invalid"), Validate(TEXT("TieCameraClearanceCm"), -1.0f) == EDataValidationResult::Invalid);
#endif
	return true;
}
#endif
