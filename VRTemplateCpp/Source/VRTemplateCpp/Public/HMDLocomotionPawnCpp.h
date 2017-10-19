// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "HMDLocomotionPawnCpp.generated.h"

// ResetOrientationAndPosition

class UCameraComponent;
class UArrowComponent;

UCLASS()
class VRTEMPLATECPP_API AHMDLocomotionPawnCpp : public APawn
{
	GENERATED_BODY()

   public:
	AHMDLocomotionPawnCpp();

   protected:
	UPROPERTY(EditAnywhere)
	float DefaultPlayerHeight = 180.f;

	UPROPERTY(EditAnywhere)
	float RotationLengthThreshold = 100.f;

	UPROPERTY(EditAnywhere)
	float FadeOutDuration = 0.1f;

	UPROPERTY(EditAnywhere)
	float FadeInDuration = 0.2f;

	UPROPERTY(EditAnywhere)
	FLinearColor TeleportFadeColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere)
	TEnumAsByte<EObjectTypeQuery> TeleportTraceObjectType;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* TeleportPin;

	UPROPERTY(VisibleAnywhere)
	UArrowComponent* TraceDirection;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Arrow;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Ring;

	virtual void BeginPlay() override;
	virtual void Tick(const float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* const PlayerInputComponent) override;
	void         EndPlay(const EEndPlayReason::Type EndPlayReason) override;

   private:
	UPROPERTY()
	USceneComponent* VROrigin;

	UPROPERTY()
	UMaterialInstanceDynamic* RingGlowMatInst;

	FVector  CurrentLookAtLocation = FVector::ZeroVector;
	FVector  PinnedLocation        = FVector::ZeroVector;
	FRotator PinnedRotation        = FRotator::ZeroRotator;
	bool     bLocationPinned       = false;
	bool     bUseGamepad           = true;
	bool     bCurrentLocationValid = false;
	bool     bLocationFound        = false;

	float TeleportUpAxisValue    = 0.f;
	float TeleportRightAxisValue = 0.f;

	void ResetOrientationAndPosition();
	void SetupHMDCameraHeight();
	void HandleTeleportation();
	void UpdateTeleportDirection();

	void HandleTeleportDirectionUpInput(const float Value) { TeleportUpAxisValue = Value; }
	void HandleTeleportDirectionRightInput(const float Value) { TeleportRightAxisValue = Value; }
	void HandleHMDTeleportPressed();
	void HandleHMDTeleportReleased();
	void FinishTeleport();

	// Create MID to give activation feedback during teleportation.
	void CreateTeleportationMID();

	bool    GetTeleportDestination(FVector& OutLocation, FVector& OutHitNormal) const;
	FVector GetThumstickFacingDirection() const;
	bool    ShouldUpdateFacingDirection() const;
};
