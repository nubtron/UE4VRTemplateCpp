// Fill out your copyright notice in the Description page of Project Settings.

#include "MotionControllerPawnCpp.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Engine/World.h"
#include "MotionControllerCpp.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"

AMotionControllerPawnCpp::AMotionControllerPawnCpp()
{
	PrimaryActorTick.bCanEverTick = true;

	VROrigin = CreateDefaultSubobject<USceneComponent>("VR Origin");
	SetRootComponent(VROrigin);
} 

void AMotionControllerPawnCpp::BeginPlay()
{
	Super::BeginPlay();

	SetupPlayerHeight();
	SetupMotionControllers();
}

void AMotionControllerPawnCpp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Left Hand Teleport Rotation
	check(IsValid(LeftController));
	if (LeftController->IsTeleporterActive())
	{
		LeftController->SetTeleportRotation(GetRotationFromInput(ThumbLeftInput.Y, ThumbLeftInput.X, LeftController));
	}

	// Right Hand Teleport Rotation
	check(IsValid(RightController));
	if (RightController->IsTeleporterActive())
	{
		RightController->SetTeleportRotation(GetRotationFromInput(ThumbRightInput.Y, ThumbRightInput.X, RightController));
	}
}

void AMotionControllerPawnCpp::SetupPlayerInputComponent(UInputComponent* const PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(IsValid(PlayerInputComponent));

	PlayerInputComponent->BindAction(
	    TEXT("GrabLeft"), IE_Pressed, this, &AMotionControllerPawnCpp::GrabLeft_HandlePressed);
	PlayerInputComponent->BindAction(
	    TEXT("GrabLeft"), IE_Released, this, &AMotionControllerPawnCpp::GrabLeft_HandleReleased);

	PlayerInputComponent->BindAction(
	    TEXT("GrabRight"), IE_Pressed, this, &AMotionControllerPawnCpp::GrabRight_HandlePressed);
	PlayerInputComponent->BindAction(
	    TEXT("GrabRight"), IE_Released, this, &AMotionControllerPawnCpp::GrabRight_HandleReleased);

	PlayerInputComponent->BindAction(
	    TEXT("TeleportLeft"), IE_Pressed, this, &AMotionControllerPawnCpp::TeleportLeft_HandlePressed);
	PlayerInputComponent->BindAction(
	    TEXT("TeleportLeft"), IE_Released, this, &AMotionControllerPawnCpp::TeleportLeft_HandleReleased);

	PlayerInputComponent->BindAction(
	    TEXT("TeleportRight"), IE_Pressed, this, &AMotionControllerPawnCpp::TeleportRight_HandlePressed);
	PlayerInputComponent->BindAction(
	    TEXT("TeleportRight"), IE_Released, this, &AMotionControllerPawnCpp::TeleportRight_HandleReleased);

	
	PlayerInputComponent->BindAxis(TEXT("MotionControllerThumbLeft_X"), this, &ThisClass::MotionControllerThumbLeft_X_HandleAxisInput);
	PlayerInputComponent->BindAxis(TEXT("MotionControllerThumbLeft_Y"), this, &ThisClass::MotionControllerThumbLeft_Y_HandleAxisInput);
	PlayerInputComponent->BindAxis(TEXT("MotionControllerThumbRight_X"), this, &ThisClass::MotionControllerThumbRight_X_HandleAxisInput);
	PlayerInputComponent->BindAxis(TEXT("MotionControllerThumbRight_Y"), this, &ThisClass::MotionControllerThumbRight_Y_HandleAxisInput);
}

void AMotionControllerPawnCpp::SetupPlayerHeight()
{
	// Setup Player Height for various Platforms (PS4, Vive, Oculus)

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
		// Force Enable. PS Move lacks thumbstick input, this option lets user adjust pawn orientation during teleport
		// using controller Roll motion.
		bUseControllerRollToRotate = true;
	}
}

void AMotionControllerPawnCpp::SetupMotionControllers()
{
	// Spawn and attach both motion controllers

	check(LeftController == nullptr);
	LeftController = SetupMotionController(EControllerHand::Left);
	check(RightController == nullptr);
	RightController = SetupMotionController(EControllerHand::Right);
}

AMotionControllerCpp* AMotionControllerPawnCpp::SetupMotionController(const EControllerHand Hand)
{
	UWorld* const World = GetWorld();
	check(IsValid(World));
	check(IsValid(MotionControllerCppClass));

	AMotionControllerCpp* const MotionController =
		World->SpawnActorDeferred<AMotionControllerCpp>(MotionControllerCppClass,
		                                                FTransform::Identity,
		                                                /*Owner*/ this,
		                                                /*Instigator*/ nullptr,
		                                                ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	check(IsValid(MotionController));
	MotionController->Hand = Hand;
	UGameplayStatics::FinishSpawningActor(MotionController, FTransform::Identity);

	check(IsValid(VROrigin));
	MotionController->AttachToComponent(VROrigin, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	return MotionController;
}

void AMotionControllerPawnCpp::ExecuteTeleportation(AMotionControllerCpp* const MotionController)
{
	if (bIsTeleporting) 
	{
		return;
	}

	check(IsValid(MotionController));
	if (!MotionController->IsValidTeleportDestination())
	{
		MotionController->DisableTeleporter();
		return;
	}

	bIsTeleporting                  = true;
	APlayerCameraManager* const PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	check(IsValid(PlayerCameraManager));
	PlayerCameraManager->StartCameraFade(/*FromAlpha*/ 0.f,
	                                     /*ToAlpha*/ 1.f,
	                                     FadeOutDuration,
	                                     TeleportFadeColor,
	                                     /*bShouldFadeAudio*/ false,
	                                     /*bHoldWhenFinished*/ true);

	FTimerHandle   TimerHandle;
	FTimerDelegate TeleportTimerDelegate;
	TeleportTimerDelegate.BindUObject(
	    this, &AMotionControllerPawnCpp::FinishTeleportation, TWeakObjectPtr<AMotionControllerCpp>(MotionController));
	GetWorldTimerManager().SetTimer(TimerHandle, TeleportTimerDelegate, FadeOutDuration, /*bLoop*/ false);
}

void AMotionControllerPawnCpp::FinishTeleportation(const TWeakObjectPtr<AMotionControllerCpp> MotionController)
{
	if (!MotionController.IsValid()) 
	{
		return;
	}
	MotionController->DisableTeleporter();

	FVector  TeleportLocation;
	FRotator TeleportRotation;
	MotionController->GetTeleportDestination(/*out*/ TeleportLocation, /*out*/ TeleportRotation);
	TeleportTo(TeleportLocation, TeleportRotation);

	APlayerCameraManager* const PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	check(IsValid(PlayerCameraManager));
	PlayerCameraManager->StartCameraFade(/*FromAlpha*/ 1.f,
	                                     /*ToAlpha*/ 0.f,
	                                     FadeInDuration,
	                                     TeleportFadeColor,
	                                     /*bShouldFadeAudio*/ false,
	                                     /*bHoldWhenFinished*/ true);

	bIsTeleporting = false;
}

void AMotionControllerPawnCpp::GrabLeft_HandlePressed()
{
	check(IsValid(LeftController));
	LeftController->GrabActor();
}

void AMotionControllerPawnCpp::GrabLeft_HandleReleased()
{
	check(IsValid(LeftController));
	LeftController->ReleaseActor();
}

void AMotionControllerPawnCpp::GrabRight_HandlePressed()
{
	check(IsValid(RightController));
	RightController->GrabActor();
}

void AMotionControllerPawnCpp::GrabRight_HandleReleased()
{
	check(IsValid(RightController));
	RightController->ReleaseActor();
}

void AMotionControllerPawnCpp::TeleportLeft_HandlePressed()
{
	check(IsValid(LeftController));
	LeftController->ActivateTeleporter();
	check(IsValid(RightController));
	RightController->DisableTeleporter();
}

void AMotionControllerPawnCpp::TeleportLeft_HandleReleased()
{
	check(IsValid(LeftController));
	if (!LeftController->IsTeleporterActive()) 
	{
		return;
	}
	ExecuteTeleportation(LeftController);
}

void AMotionControllerPawnCpp::TeleportRight_HandlePressed()
{
	check(IsValid(RightController));
	RightController->ActivateTeleporter();
	check(IsValid(LeftController));
	LeftController->DisableTeleporter();
}

void AMotionControllerPawnCpp::TeleportRight_HandleReleased()
{
	check(IsValid(RightController));
	if (!RightController->IsTeleporterActive()) 
	{
		return;
	}
	ExecuteTeleportation(RightController);
}

FRotator AMotionControllerPawnCpp::GetRotationFromInput(const float                       UpAxis,
                                                        const float                       RightAxis,
                                                        const AMotionControllerCpp* const MotionController) const
{
	check(IsValid(MotionController));
	// Get Roll difference since we initiated the teleport. (Allows wrist to change the pawn orientation when
	// teleporting)
	if (bUseControllerRollToRotate)
	{
		const FTransform InitialRotation{MotionController->GetInitialControllerRotation()};
		const FTransform MotionControllerTransform = MotionController->GetTransform();
		const float ControllerRoll = MotionControllerTransform.GetRelativeTransform(InitialRotation).Rotator().Roll;
		const float YawAdjustment  = ControllerRoll * 3.f; // Multiply to make 180 spins of orientation much easier.
		const float CurrentYaw     = GetActorRotation().Yaw;
		// Add current rotation to the controller adjustment
		// Roll of controller gets converted to Yaw for pawn orientation.
		const float    NewYaw = CurrentYaw + YawAdjustment;
		const FRotator PawnOrientation{0.f, NewYaw, 0.f};
		return PawnOrientation;
	}
	else
	{
		// Check whether thumb is near center (to ignore rotation overrides)
		if (FMath::Abs(UpAxis) + FMath::Abs(RightAxis) >= ThumbDeadzone)
		{
			// Rotate input X + Y to always point forward relative to the current pawn rotation.
			const FVector  Input              = FVector{UpAxis, RightAxis, 0.f}.GetSafeNormal();
			const FRotator CurrentYawRotation = FRotator{0.f, GetActorRotation().Yaw, 0.f};
			return CurrentYawRotation.RotateVector(Input).ToOrientationRotator();
		}
		else
		{
			// Use Default rotation if thumb is near center of the pad
			return GetActorRotation();
		}
	}
}
