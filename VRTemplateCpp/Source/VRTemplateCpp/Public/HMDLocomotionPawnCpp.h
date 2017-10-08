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
	UPROPERTY(EditAnywhere) float DefaultPlayerHeight      = 180.f;
	UPROPERTY(EditAnywhere) float RotationLengthThreshold  = 100.f;
	UPROPERTY(EditAnywhere) float FadeOutDuration          = 0.1f;
	UPROPERTY(EditAnywhere) float FadeInDuration           = 0.2f;
	UPROPERTY(EditAnywhere) FLinearColor TeleportFadeColor = FLinearColor::Black;
	UPROPERTY(EditAnywhere) TEnumAsByte<EObjectTypeQuery> TeleportTraceObjectType;

	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* TeleportPin = nullptr;
	UPROPERTY(VisibleAnywhere) UArrowComponent* TraceDirection   = nullptr;
	UPROPERTY(VisibleAnywhere) UCameraComponent* Camera          = nullptr;
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Arrow       = nullptr;
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Ring       = nullptr;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void         EndPlay(const EEndPlayReason::Type EndPlayReason) override;

   private:
	UPROPERTY() USceneComponent* VROrigin                 = nullptr;
	UPROPERTY() UMaterialInstanceDynamic* RingGlowMatInst = nullptr;

	FVector  CurrentLookAtLocation = FVector::ZeroVector;
	FVector  PinnedLocation        = FVector::ZeroVector;
	FRotator PinnedRotation        = FRotator::ZeroRotator;
	bool     bLocationPinned       = false;
	bool     bUseGamepad           = true;
	bool     bCurrentLocationValid = false;
	bool     bLocationFound        = false;

	void ResetOrientationAndPosition();
	void SetupHMDCameraHeight();
	void HandleTeleportation();
	void UpdateTeleportDirection();

	void HandleHMDTeleportPressed();
	void HandleHMDTeleportReleased();
	void FinishTeleport();

	// Create MID to give activation feedback during teleportation.
	void CreateTeleportationMID();

	bool    GetTeleportDestination(FVector& OutLocation, FVector& OutHitNormal) const;
	FVector GetThumstickFacingDirection() const;
	bool    ShouldUpdateFacingDirection() const;
};
