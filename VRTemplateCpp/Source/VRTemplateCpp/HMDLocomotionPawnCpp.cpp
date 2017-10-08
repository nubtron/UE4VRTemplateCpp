// Fill out your copyright notice in the Description page of Project Settings.

#include "HMDLocomotionPawnCpp.h"

#include "Components/InputComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/ArrowComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AI/Navigation/NavigationSystem.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"

AHMDLocomotionPawnCpp::AHMDLocomotionPawnCpp()
{
	PrimaryActorTick.bCanEverTick = true;
	VROrigin                      = CreateDefaultSubobject<USceneComponent>("VR Origin");
	TeleportPin                   = CreateDefaultSubobject<UStaticMeshComponent>("Teleport Pin");
	TraceDirection                = CreateDefaultSubobject<UArrowComponent>("Trace Direction");
	Arrow                         = CreateDefaultSubobject<UStaticMeshComponent>("Arrow");
}

void AHMDLocomotionPawnCpp::BeginPlay()
{
	Super::BeginPlay();

	SetupHMDCameraHeight();
	CreateTeleportationMID();
}

void AHMDLocomotionPawnCpp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	HandleTeleportation();
}

void AHMDLocomotionPawnCpp::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(IsValid(PlayerInputComponent));
	PlayerInputComponent->BindAction(
	    TEXT("ResetOrientationAndPosition"), IE_Pressed, this, &ThisClass::ResetOrientationAndPosition);
	PlayerInputComponent->BindAction(TEXT("HMDTeleport"), IE_Pressed, this, &ThisClass::HandleHMDTeleportPressed);
	PlayerInputComponent->BindAction(TEXT("HMDTeleport"), IE_Released, this, &ThisClass::HandleHMDTeleportReleased);
}

void AHMDLocomotionPawnCpp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
}

void AHMDLocomotionPawnCpp::ResetOrientationAndPosition()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AHMDLocomotionPawnCpp::SetupHMDCameraHeight()
{
	const FName HMDDeviceName = UHeadMountedDisplayFunctionLibrary::GetHMDDeviceName();
	if (HMDDeviceName == TEXT("Oculus Rift") || HMDDeviceName == TEXT("Steam VR"))
	{
		// Windows (Vive/Oculus)
		UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);
	}
	else if (HMDDeviceName == TEXT("PSVR"))
	{
		// PS4
		UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Eye);
		check(IsValid(VROrigin));
		const FVector DeltaLocation{0.f, 0.f, DefaultPlayerHeight};
		VROrigin->AddLocalOffset(DeltaLocation);
	}
}

void AHMDLocomotionPawnCpp::HandleTeleportation()
{
	FVector HitNormal;
	bCurrentLocationValid = GetTeleportDestination(/*out*/ CurrentLookAtLocation, /*out*/ HitNormal);
	if (bLocationPinned)
	{
		UpdateTeleportDirection();
	}
	else
	{
		// Find and show a potential spot to teleport to
		check(IsValid(TeleportPin));
		TeleportPin->SetWorldLocation(
		    CurrentLookAtLocation, /*bSweep*/ false, /*OutSweepHitResult*/ nullptr, ETeleportType::TeleportPhysics);

		TeleportPin->SetVisibility(bCurrentLocationValid, /*bPropagatetoChildren*/ true);
	}

	/***  Update teleport meshes position and orientations ***/

	check(IsValid(Arrow));
	const auto bShouldArrowBeVisible = ShouldUpdateFacingDirection() && bLocationPinned;
	Arrow->SetVisibility(bShouldArrowBeVisible);

	// Adjust fall-off of the glowing cylinder.
	check(IsValid(RingGlowMatInst));
	const float HeightScale = bLocationPinned ? 1.f : 0.35f; // Adjust the height of cylinder fall-off
	RingGlowMatInst->SetScalarParameterValue(TEXT("HeightScale"), HeightScale);

	if (bUseGamepad)
	{
		Arrow->SetWorldRotation(PinnedRotation);
	}
	else
	{
		FRotator DeviceOrientation;
		FVector  DevicePosition;
		UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(/*out*/ DeviceOrientation,
		                                                              /*out*/ DevicePosition);
		const FRotator DeviceYaw{0.f, DeviceOrientation.Yaw, 0.f};
		const auto     NewArrowRotation = UKismetMathLibrary::ComposeRotators(PinnedRotation, DeviceYaw);
		Arrow->SetWorldRotation(NewArrowRotation);
	}
}

void AHMDLocomotionPawnCpp::UpdateTeleportDirection()
{
	if (bUseGamepad)
	{
		PinnedRotation = GetThumstickFacingDirection().ToOrientationRotator();
	}
	else
	{
		// Create "Look-at" direction from teleport pin to what player (HMD) is currently looking at
		const FRotator PinToLookAtRotation = (CurrentLookAtLocation - PinnedLocation).ToOrientationRotator();
		// Only retain Yaw for new facing direction.
		PinnedRotation = FRotator{0.f, PinToLookAtRotation.Yaw, 0.f};
	}
}

void AHMDLocomotionPawnCpp::HandleHMDTeleportPressed()
{
	// Pin the current location we look at
	if (!bCurrentLocationValid) return;

	bLocationPinned = true;
	bLocationFound  = true;
	PinnedLocation  = CurrentLookAtLocation;
}

void AHMDLocomotionPawnCpp::HandleHMDTeleportReleased()
{
	if (!bLocationPinned) return;
	bLocationPinned = false;

	if (!bLocationFound) return;

	auto* const PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	check(IsValid(PlayerCameraManager));

	PlayerCameraManager->StartCameraFade(/*FromAlpha*/ 0.f,
	                                     /*ToAlpha*/ 1.f,
	                                     FadeOutDuration,
	                                     TeleportFadeColor,
	                                     /*bShouldFadeAudio*/ false,
	                                     /*bHoldWhenFinished*/ true);

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ThisClass::FinishTeleport, FadeOutDuration, /*bLoop*/ false);
}

void AHMDLocomotionPawnCpp::FinishTeleport()
{

	FRotator DeviceRotation;
	FVector  DevicePosition; // Relative HMD location from Origin
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(DeviceRotation, DevicePosition);

	const FVector Position2D{DevicePosition.X, DevicePosition.Y, 0.f}; // Ignore relative Height difference

	// Update rotation or keep the existing rotation.
	const FRotator DestRotation = ShouldUpdateFacingDirection() ? PinnedRotation : GetActorRotation();

	// Substract HMD origin(Camera) to get correct Pawn destination for teleportation.
	const FVector DestLocation = PinnedLocation - DestRotation.RotateVector(Position2D);

	TeleportTo(DestLocation, DestRotation);

	auto* const PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	check(IsValid(PlayerCameraManager));

	PlayerCameraManager->StartCameraFade(/*FromAlpha*/ 1.f, /*ToAlpha*/ 0.f, FadeInDuration, TeleportFadeColor);
}

void AHMDLocomotionPawnCpp::CreateTeleportationMID()
{
	check(IsValid(TeleportPin));
	RingGlowMatInst = TeleportPin->CreateDynamicMaterialInstance(/*ElementIndex*/ 0);
	check(IsValid(RingGlowMatInst));
}

bool AHMDLocomotionPawnCpp::GetTeleportDestination(FVector& OutLocation, FVector& OutHitNormal) const
{
	// Use Arrow component to set up trace origin and direction
	check(IsValid(TraceDirection));
	const auto StartPos       = TraceDirection->GetComponentLocation();
	const auto LaunchVelocity = TraceDirection->GetComponentLocation() + TraceDirection->GetForwardVector() * 1000.f;

	// Simulate throwing a projectile (including gravity) to find a teleport location.
	const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = {TeleportTraceObjectType};
	const TArray<AActor*>                       ActorsToIgnore;
	FHitResult                                  HitResult;
	TArray<FVector>                             PathPositions;
	FVector                                     LastTraceDestination;
	UGameplayStatics::Blueprint_PredictProjectilePath_ByObjectType(this,
	                                                               /*out*/ HitResult,
	                                                               /*out*/ PathPositions,
	                                                               LastTraceDestination,
	                                                               StartPos,
	                                                               LaunchVelocity,
	                                                               /*bTracePath*/ true,
	                                                               /*ProjectileRadius*/ 0.f,
	                                                               ObjectTypes,
	                                                               /*bTraceComplex*/ false,
	                                                               ActorsToIgnore,
	                                                               EDrawDebugTrace::None,
	                                                               /*SimFrequency*/ 15.f);
	OutHitNormal                  = HitResult.Normal;
	const float ProjectNavExtents = 500.f;
	// Project our hit against the NavMesh to find a place for player to stand.
	const bool bCanNavigate = UNavigationSystem::K2_ProjectPointToNavigation(GetWorld(),
	                                                                         HitResult.Location,
	                                                                         /*out*/ OutLocation,
	                                                                         /*NavData*/ nullptr,
	                                                                         /*FilterClass*/ nullptr,
	                                                                         FVector{ProjectNavExtents});
	return bCanNavigate;
}

FVector AHMDLocomotionPawnCpp::GetThumstickFacingDirection() const
{
	// Create a look-at vector based on gamepad input rotated by the camera facing direction

	// FIXME
	// const auto* const PlayerController = CastChecked<APlayerController>(GetController());
	// const auto* const InputComponent   = PlayerController->InputComponent;
	// check(IsValid(InputComponent));

	const float   TeleportUpAxisValue    = GetInputAxisValue(TEXT("TeleportDirectionUp"));
	const float   TeleportRightAxisValue = GetInputAxisValue(TEXT("TeleportDirectionRight"));
	const FVector TeleportDirection      = FVector{TeleportUpAxisValue, TeleportRightAxisValue, 0.f}.GetSafeNormal();

	check(IsValid(Camera));
	const FVector  PinToCamera         = Camera->GetComponentLocation() - PinnedLocation;
	const FRotator PinToCameraRotation = UKismetMathLibrary::MakeRotFromX(PinToCamera);
	const FRotator PinToCameraYaw{0.f, PinToCameraRotation.Yaw, 0.f};

	return PinToCameraYaw.RotateVector(TeleportDirection) * 400.f;
}

bool AHMDLocomotionPawnCpp::ShouldUpdateFacingDirection() const
{
	if (bUseGamepad)
	{
		// Is player directing using the thumbstick input?
		return GetInputAxisValue(TEXT("TeleportDirectionUp")) != 0.f ||
		       GetInputAxisValue(TEXT("TeleportDirectionRight")) != 0.f;
	}
	else
	{
		// Is our HMD pointing at a direction in the world (other than out pinned location)
		return FVector::Dist(CurrentLookAtLocation, PinnedLocation) > RotationLengthThreshold;
	}
}
