// Fill out your copyright notice in the Description page of Project Settings.

#include "MotionControllerCpp.h"

#include "SteamVRChaperoneComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "PickupActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "HandAnimInstance.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Components/SplineMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/ArrowComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AI/Navigation/NavigationSystem.h"
#include "Engine/StaticMesh.h"
#include "MotionControllerComponent.h"

AMotionControllerCpp::AMotionControllerCpp()
{
	PrimaryActorTick.bCanEverTick = true;

	auto* const SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	SetRootComponent(SceneRoot);
	{
		MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("Motion Controller"));
		MotionController->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
		{
			HandMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Hand Mesh"));
			HandMesh->AttachToComponent(MotionController, FAttachmentTransformRules::KeepRelativeTransform);
			{
				ArcDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("Arc Direction"));
				ArcDirection->AttachToComponent(HandMesh, FAttachmentTransformRules::KeepRelativeTransform);

				ArcSpline = CreateDefaultSubobject<USplineComponent>(TEXT("ArcSpline"));
				ArcSpline->AttachToComponent(HandMesh, FAttachmentTransformRules::KeepRelativeTransform);

				GrapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Grab Sphere"));
				GrapSphere->AttachToComponent(HandMesh, FAttachmentTransformRules::KeepRelativeTransform);
			}
			ArcEndPoint = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Arc End Point"));
			ArcEndPoint->AttachToComponent(MotionController, FAttachmentTransformRules::KeepRelativeTransform);
		}
		TeleportCylinder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Teleport Cylinder"));
		TeleportCylinder->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
		{
			Ring = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));
			Ring->AttachToComponent(TeleportCylinder, FAttachmentTransformRules::KeepRelativeTransform);

			Arrow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Arrow"));
			Arrow->AttachToComponent(TeleportCylinder, FAttachmentTransformRules::KeepRelativeTransform);
			{
				RoomScaleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Room Scale Mesh"));
				RoomScaleMesh->AttachToComponent(Arrow, FAttachmentTransformRules::KeepRelativeTransform);
			}
		}
	}

	SteamVRChaperone = CreateDefaultSubobject<USteamVRChaperoneComponent>(TEXT("Chaperone"));
	BeamMesh         = CreateDefaultSubobject<UStaticMesh>(TEXT("Beam Mesh"));
}

void AMotionControllerCpp::DisableTeleporter()
{
	if (!bIsTeleporterActive) return;
	bIsTeleporterActive = false;

	check(IsValid(TeleportCylinder));
	TeleportCylinder->SetVisibility(/*bNewVisiblility*/ false, /*bPropagateToChildren*/ true);

	check(IsValid(ArcEndPoint));
	ArcEndPoint->SetVisibility(/*bNewVisiblility*/ false, /*bPropagateToChildren*/ false);

	check(IsValid(RoomScaleMesh));
	RoomScaleMesh->SetVisibility(/*bNewVisiblility*/ false, /*bPropagateToChildren*/ false);
}

void AMotionControllerCpp::GetTeleportDestination(FVector& OutPosition, FRotator& OutRotation) const
{
	OutRotation = TeleportRotation;

	FRotator DeviceRotation;
	FVector  DeviceLocation; // Relative HMD location from Origin
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(DeviceRotation, DeviceLocation);
	const FVector DeviceLocation2D{DeviceLocation.X, DeviceLocation.Y, 0.f};

	check(IsValid(TeleportCylinder));

	// Substract HMD origin(Camera) to get correct Pawn destination for teleportation.
	OutPosition = TeleportCylinder->GetComponentLocation() - TeleportRotation.RotateVector(DeviceLocation2D);
}

void AMotionControllerCpp::GrabActor()
{
	bWantsToGrip            = true;
	auto* const NearestMesh = GetActorNearHand();
	if (!IsValid(NearestMesh)) return;
	AttachedActor = NearestMesh;
	if (auto* const PickupActor = Cast<IPickupActor>(AttachedActor))
	{
		check(IsValid(MotionController));
		PickupActor->AttachTo(MotionController);
		RumbleController(/*Intensity*/ 0.7f);
	}
}

void AMotionControllerCpp::ReleaseActor()
{
	bWantsToGrip = false;
	if (!IsValid(AttachedActor)) return;

	const auto* AttachedActorRoot = AttachedActor->GetRootComponent();
	check(IsValid(AttachedActorRoot));
	check(IsValid(MotionController));

	// Make sure this hand is still holding the Actor (May have been taken by another hand / event)
	if (AttachedActorRoot->GetAttachParent() == MotionController)
	{
		if (auto* const PickupActor = Cast<IPickupActor>(AttachedActor))
		{
			PickupActor->Drop();
			RumbleController(/*Intensity*/ 0.2f);
		}
	}

	AttachedActor = nullptr;
}

void AMotionControllerCpp::ActivateTeleporter()
{
	// Set the flag, rest of teleportation is handled on EventGraph during Tick
	bIsTeleporterActive = true;

	check(IsValid(TeleportCylinder));
	TeleportCylinder->SetVisibility(/*bNewVisibility*/ true, /*bPropagateToChildren*/ true);

	// Only show during Teleport if room-scale is available.
	check(IsValid(RoomScaleMesh));
	RoomScaleMesh->SetVisibility(bIsRoomScale);

	// Store Rotation to later compare roll value to support wrist-based orientation of the teleporter.
	check(IsValid(MotionController));
	InitialControllerRotation = MotionController->GetComponentRotation();
}

void AMotionControllerCpp::BeginPlay()
{
	Super::BeginPlay();

	SetupRoomscaleOutline();

	// Hide until activation of teleporter
	{
		check(IsValid(TeleportCylinder));
		TeleportCylinder->SetVisibility(false, /*bPropagateToChildren*/ true);

		check(IsValid(RoomScaleMesh));
		RoomScaleMesh->SetVisibility(false, /*bPropagateToChildren*/ false);
	}

	check(IsValid(MotionController));
	MotionController->Hand = Hand;

	// Invert scale on hand mesh to create left-hand
	if (Hand == EControllerHand::Left)
	{
		check(IsValid(HandMesh));
		HandMesh->SetWorldScale3D({1.f, 1.f, -1.f});
	}

	// Register Events
	check(IsValid(GrapSphere));
	GrapSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleBeginOverlap_GrabSphere);
	GrapSphere->OnComponentHit.AddDynamic(this, &ThisClass::HandleComponentHit_ControllerMesh);
}

void AMotionControllerCpp::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateAnimationOfHand();
	UpdateRoomScaleOutline();

	// Only let hand collide with environment while gripping
	const auto HandCollisionMode =
	    GripState == EGrip::Grab ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision;
	check(IsValid(HandMesh));
	HandMesh->SetCollisionEnabled(HandCollisionMode);

	HandleTeleportArc();
}

void AMotionControllerCpp::SetupRoomscaleOutline()
{
	check(IsValid(SteamVRChaperone));
	const auto ChaperoneBounds = SteamVRChaperone->GetBounds();
	FVector    ChaperoneCenter;
	FRotator   ChaperoneRotation;
	FVector2D  ChaperoneSize;
	UKismetMathLibrary::MinimumAreaRectangle(this,
	                                         ChaperoneBounds,
	                                         FVector::UpVector,
	                                         /*out*/ ChaperoneCenter,
	                                         /*out*/ ChaperoneRotation,
	                                         /*out*/ ChaperoneSize.X,
	                                         /*out*/ ChaperoneSize.Y);

	// Measure Chaperone (Defaults to 100x100 if  roomscale isn't used)

	const bool bIsRoomScale =
	    !FMath::IsNearlyEqual(ChaperoneSize.X, 100.f) || !FMath::IsNearlyEqual(ChaperoneSize.Y, 100.f);

	if (!bIsRoomScale) return;

	check(IsValid(RoomScaleMesh));
	const float ChaperoneMeshHeight = 70.f;
	RoomScaleMesh->SetWorldScale3D({ChaperoneSize.X, ChaperoneSize.Y, ChaperoneMeshHeight});
	RoomScaleMesh->SetRelativeRotation(ChaperoneRotation);
}

void AMotionControllerCpp::UpdateAnimationOfHand()
{
	if (IsValid(AttachedActor) || bWantsToGrip)
	{
		GripState = EGrip::Grab;
		return;
	}

	const auto* const ActorNearHand = GetActorNearHand();
	if (!IsValid(ActorNearHand))
	{
		GripState = EGrip::CanGrab;
	}
	else
	{
		if (bWantsToGrip)
		{
			GripState = EGrip::Grab;
		}
		else
		{
			GripState = EGrip::Open;
		}
	}

	UpdateHandMeshAnimation();
}

void AMotionControllerCpp::UpdateHandMeshAnimation()
{
	check(IsValid(HandMesh));
	auto* const HandAnimInstance = CastChecked<UHandAnimInstance>(HandMesh->GetAnimInstance());
	HandAnimInstance->SetGripState(GripState);
}

void AMotionControllerCpp::UpdateRoomScaleOutline()
{
	check(IsValid(RoomScaleMesh));
	if (!RoomScaleMesh->IsVisible()) return;

	// Update Room-scale outline location relative to Teleport target
	FRotator DeviceRotation;
	FVector  DevicePosition;
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(/*out*/ DeviceRotation, /*out*/ DevicePosition);

	const FRotator DeviceYaw = FRotator{0.f, DeviceRotation.Yaw, 0.f};
	const FVector  DevicePosition2D{DevicePosition.X, DevicePosition.Y, 0.f};
	const FVector  NewLocation = DeviceYaw.UnrotateVector(-DevicePosition2D);

	RoomScaleMesh->SetRelativeLocation(NewLocation);
}

void AMotionControllerCpp::HandleTeleportArc()
{
	ClearArc();
	if (!bIsTeleporterActive) return;

	TArray<FVector> TracePoints;
	FVector         NavMeshLocation;
	FVector         TraceLocation;
	bIsValidTeleportDestination = TraceTeleportDestination(TracePoints, NavMeshLocation, TraceLocation);

	check(IsValid(TeleportCylinder));
	TeleportCylinder->SetVisibility(bIsValidTeleportDestination, /*bPropagateToChildren*/ true);

	// Create downward vector
	const FVector DownwardVector = NavMeshLocation + FVector{0.f, 0.f, -200.f};

	// Trace down to find a valid location for player to stand at (original NavMesh location is offset upwards, so we
	// must find the actual floor)
	const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = {TeleportTraceQuery};
	FHitResult                                  TraceHitResult;
	UKismetSystemLibrary::LineTraceSingleForObjects(this,
	                                                NavMeshLocation,
	                                                DownwardVector,
	                                                ObjectTypes,
	                                                /*bTraceComplex*/ false,
	                                                /*ActorsToIgnore*/ {},
	                                                EDrawDebugTrace::None,
	                                                TraceHitResult,
	                                                /*bIgnoreSelf*/ true);
	const FVector TeleportLocation = TraceHitResult.bBlockingHit ? TraceHitResult.ImpactPoint : NavMeshLocation;

	FHitResult SweepHitResult;
	TeleportCylinder->SetWorldLocation(
	    TeleportLocation, /*bSweep*/ false, /*OutSweepHitResult*/ nullptr, ETeleportType::TeleportPhysics);

	// Rumble controller when a valid teleport location was found (T/N: or lost)
	if (bIsValidTeleportDestination != bLastFrameValidDestination)
	{
		RumbleController(/*Intensity*/ 0.3f);
	}

	bLastFrameValidDestination = bIsValidTeleportDestination;

	UpdateArcSpline(bIsValidTeleportDestination, TracePoints);
	UpdateArcEndpoint(TraceLocation, bIsValidTeleportDestination);
}

void AMotionControllerCpp::ClearArc()
{
	for (auto* const SplineMesh : SplineMeshes)
	{
		if (!IsValid(SplineMesh)) continue;
		SplineMesh->DestroyComponent();
	}
	SplineMeshes.Empty();
	check(IsValid(ArcSpline));
	ArcSpline->ClearSplinePoints(/*bUpdateSpline*/ true);
}

bool AMotionControllerCpp::TraceTeleportDestination(TArray<FVector>& OutTracePoints,
                                                    FVector&         OutNavMeshLocation,
                                                    FVector&         OutTraceLocation) const
{
	check(IsValid(ArcDirection));
	const FVector StartPos                                  = ArcDirection->GetComponentLocation();
	const FVector LaunchVelocity                            = ArcDirection->GetForwardVector() * TeleportLaunchVelocity;
	const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = {TeleportTraceQuery};
	const TArray<AActor*>                       ActorsToIgnore;

	FHitResult HitResult;
	FVector    LastTraceDestination;

	const bool bHasHit            = UGameplayStatics::Blueprint_PredictProjectilePath_ByObjectType(this,
                                                                                        /*out*/ HitResult,
                                                                                        /*out*/ OutTracePoints,
                                                                                        /*out*/ LastTraceDestination,
                                                                                        StartPos,
                                                                                        LaunchVelocity,
                                                                                        /*bTracePath*/ true,
                                                                                        /*ProjectileRadius*/ 0.f,
                                                                                        ObjectTypes,
                                                                                        /*bTraceComplex*/ false,
                                                                                        ActorsToIgnore,
                                                                                        EDrawDebugTrace::None,
                                                                                        /*DrawDebugTime*/ 0.f);
	OutTraceLocation              = HitResult.Location;
	const float ProjectNavExtents = 500.f;
	const bool  bCanNavigate      = UNavigationSystem::K2_ProjectPointToNavigation(GetWorld(),
                                                                             HitResult.Location,
                                                                             /*out*/ OutNavMeshLocation,
                                                                             /*NavData*/ nullptr,
                                                                             /*FilterClass*/ nullptr,
                                                                             FVector{ProjectNavExtents});
	const bool  bSuccess          = bHasHit && bCanNavigate;
	return bSuccess;
}

void AMotionControllerCpp::RumbleController(const float Intensity)
{
	auto* const LocalPlayerController = UGameplayStatics::GetPlayerController(this, 0);
	check(IsValid(LocalPlayerController));
	LocalPlayerController->PlayHapticEffect(RumbleHaptics, Hand, Intensity);
}

void AMotionControllerCpp::UpdateArcSpline(const bool bFoundValidLocation, const TArray<FVector>& SplinePoints)
{

	if (!bFoundValidLocation)
	{
		// Create Small Stub line when we failed to find a teleport location
		check(IsValid(ArcDirection));
		const TArray<FVector> StubPoints = {
		    ArcDirection->GetComponentLocation(),
		    ArcDirection->GetComponentLocation() + ArcDirection->GetForwardVector() * 20.f};
		UpdateArcSpline(/*bFoundValidLocation*/ true, StubPoints);
		return;
	}

	check(IsValid(ArcSpline));
	for (const auto& SplinePoint : SplinePoints)
	{
		// Build a spline from all trace points. This generates tangets we can use to build a smoothly curved spline
		// mesh
		ArcSpline->AddSplinePoint(SplinePoint, ESplineCoordinateSpace::Local, /*bUpdateSpline*/ true);
	}
	// Update the point type to create the curve
	ArcSpline->SetSplinePointType(SplinePoints.Num() - 1, ESplinePointType::CurveClamped, /*bUpdateSpline*/ true);

	check(IsValid(BeamMesh));

	// T/N: We subtract 1 from the number of spline points because there are line N-1 lines connecting a curve with N
	// points.
	const int32 LargestSplinePointIndex = ArcSpline->GetNumberOfSplinePoints() - 1;
	for (int32 i = 0; i < LargestSplinePointIndex; ++i)
	{
		// Add new cylinder mesh
		auto* const NewSplineMesh = NewObject<USplineMeshComponent>(this);
		check(IsValid(NewSplineMesh));
		NewSplineMesh->SetStaticMesh(BeamMesh);
		check(IsValid(BeamMaterial));
		NewSplineMesh->SetMaterial(/*ElementIndex*/ 0, BeamMaterial);
		NewSplineMesh->SetMobility(EComponentMobility::Movable);
		NewSplineMesh->SetStartScale({ 4.f, 4.f });
		NewSplineMesh->SetEndScale({ 4.f, 4.f });
		NewSplineMesh->SetBoundaryMax(1.f);
		NewSplineMesh->RegisterComponent();
		SplineMeshes.Add(NewSplineMesh);

		// Set the tangents and position to build slightly bend the cylinder. All cylinders combined create a smooth
		// arc.
		const FVector& StartPos     = SplinePoints[i];
		const FVector& StartTangent = ArcSpline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local);
		const FVector& EndPos       = SplinePoints[i + 1];
		const FVector& EndTangent   = ArcSpline->GetTangentAtSplinePoint(i + 1, ESplineCoordinateSpace::Local);
		NewSplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent, /*bUpdateMesh*/ true);
	}
}

void AMotionControllerCpp::UpdateArcEndpoint(const FVector& NewLocation, const bool bValidLocationFound)
{
	check(IsValid(ArcEndPoint));
	const bool bIsArcEndpointVisible = bValidLocationFound && bIsTeleporterActive;
	ArcEndPoint->SetVisibility(bIsArcEndpointVisible);
	FHitResult SweepResult;
	ArcEndPoint->K2_SetWorldLocation(NewLocation, /*bSweep*/ false, SweepResult, /*bTeleport*/ true);

	check(IsValid(Arrow));
	FRotator DeviceRotation;
	FVector  DevicePosition;
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(DeviceRotation, DevicePosition);
	const FRotator DeviceYaw{0.f, DeviceRotation.Yaw, 0.f};
	// Combine the two rotations to get an accurate preview of where player will look to after a teleport.
	const auto ArrowRotation = UKismetMathLibrary::ComposeRotators(TeleportRotation, DeviceYaw);
	Arrow->SetWorldRotation(ArrowRotation);

	check(IsValid(RoomScaleMesh));
	RoomScaleMesh->SetWorldRotation(TeleportRotation);
}

void AMotionControllerCpp::HandleBeginOverlap_GrabSphere(UPrimitiveComponent* const OverlappedComponent,
                                                         AActor* const              OtherActor,
                                                         UPrimitiveComponent* const OtherComp,
                                                         const int32                OtherBodyIndex,
                                                         const bool                 bFromSweep,
                                                         const FHitResult&          SweepResult)
{
	// Rumble Controller when overlapping valid StaticMesh

	auto* const OtherStaticMesh = Cast<UStaticMeshComponent>(OtherComp);
	if (!IsValid(OtherStaticMesh)) return;

	if (OtherStaticMesh->IsSimulatingPhysics())
	{
		RumbleController(/*Intensity*/ 0.8f);
	}
}

void AMotionControllerCpp::HandleComponentHit_ControllerMesh(UPrimitiveComponent* const HitComponent,
                                                             AActor* const              OtherActor,
                                                             UPrimitiveComponent* const OtherComp,
                                                             const FVector              NormalImpulse,
                                                             const FHitResult&          Hit)
{
	const float Intensity = UKismetMathLibrary::MapRangeClamped(
	    NormalImpulse.Size(), /*In Range A*/ 0.f, /*In Range B*/ 1500.f, /*Out Range A*/ 0.f, /*Out Range B*/ 0.8f);
	RumbleController(Intensity);
}

AActor* AMotionControllerCpp::GetActorNearHand() const
{
	check(IsValid(GrapSphere));
	TArray<AActor*> OverlappingActors;
	GrapSphere->GetOverlappingActors(/*out*/ OverlappingActors);

	AActor* NearestOverlappingActor = nullptr;
	float   NearestOverlap          = TNumericLimits<float>::Max();
	for (auto* const Actor : OverlappingActors)
	{
		if (!IsValid(Actor)) continue;

		// Filter to Actors that implement our interface for pickup / dropping
		// We want to only grab simulated meshes.
		if (!Cast<IPickupActor>(Actor)) continue;

		const float DistanceToActor = FVector::Distance(GrapSphere->GetComponentLocation(), Actor->GetActorLocation());
		if (DistanceToActor < NearestOverlap)
		{
			NearestOverlap          = DistanceToActor;
			NearestOverlappingActor = Actor;
		}
	}
	return NearestOverlappingActor;
}
