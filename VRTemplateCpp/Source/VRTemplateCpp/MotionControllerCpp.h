// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grip.h"
#include "MotionControllerCpp.generated.h"

class UMotionControllerComponent;
class UHapticFeedbackEffect_Base;
class UArrowComponent;
class USplineComponent;
class USplineMeshComponent;
class UHandAnimInstance;
class USphereComponent;
class USteamVRChaperoneComponent;

// For Teleport Handling: See MotionControllerPawn

UCLASS()
class VRTEMPLATECPP_API AMotionControllerCpp : public AActor
{
	GENERATED_BODY()

#pragma region Teleportation

   public:
	void ActivateTeleporter();
	void DisableTeleporter();
	void GetTeleportDestination(FVector& OutPosition, FRotator& OutRotation) const;
	bool IsTeleporterActive() const { return bIsTeleporterActive; }
	bool IsValidTeleportDestination() const { return bIsValidTeleportDestination; }

   protected:
	UPROPERTY(EditAnywhere) float TeleportLaunchVelocity           = 900.f;
	UPROPERTY(EditAnywhere) UStaticMeshComponent* TeleportCylinder = nullptr;
	UPROPERTY(EditAnywhere) UStaticMeshComponent* ArcEndPoint      = nullptr;
	UPROPERTY(EditAnywhere) UStaticMesh* BeamMesh                  = nullptr;
	UPROPERTY(EditAnywhere) UArrowComponent* ArcDirection          = nullptr;
	UPROPERTY(EditAnywhere) TEnumAsByte<EObjectTypeQuery> TeleportTraceQuery;

   private:
	UPROPERTY() USplineComponent* ArcSpline = nullptr;
	UPROPERTY() TArray<USplineMeshComponent*> SplineMeshes;

	bool     bIsTeleporterActive         = false;
	bool     bLastFrameValidDestination  = false;
	bool     bIsValidTeleportDestination = false;
	FRotator TeleportRotation            = FRotator::ZeroRotator;

	void HandleTeleportArc();
	void ClearArc();
	bool TraceTeleportDestination(TArray<FVector>& OutTracePoints,
	                              FVector&         OutNavMeshLocation,
	                              FVector&         OutTraceLocation) const;
	void RumbleController(float Intensity);
	void UpdateArcSpline(bool bFoundValidLocation, const TArray<FVector>& SplinePoints);
	void UpdateArcEndpoint(const FVector& NewLocation, bool bValidLocationFound);

#pragma endregion

#pragma region Grabbing

   public:
	void GrabActor();
	void ReleaseActor();

   protected:
	UPROPERTY(EditAnywhere) USphereComponent* GrapSphere = nullptr;

   private:
	bool  bWantsToGrip = false;
	EGrip GripState    = EGrip::Open;
	bool  bIsRoomScale = false;

	UFUNCTION()
	void HandleBeginOverlap_GrabSphere(UPrimitiveComponent* OverlappedComponent,
	                                   AActor*              OtherActor,
	                                   UPrimitiveComponent* OtherComp,
	                                   int32                OtherBodyIndex,
	                                   bool                 bFromSweep,
	                                   const FHitResult&    SweepResult);
	UFUNCTION()
	void    HandleComponentHit_ControllerMesh(UPrimitiveComponent* HitComponent,
	                                          AActor*              OtherActor,
	                                          UPrimitiveComponent* OtherComp,
	                                          FVector              NormalImpulse,
	                                          const FHitResult&    Hit);
	AActor* GetActorNearHand() const;
#pragma endregion

#pragma region Room Scale
   protected:
	UPROPERTY(EditAnywhere) UStaticMeshComponent* RoomScaleMesh = nullptr;

   private:
	UPROPERTY() USteamVRChaperoneComponent* SteamVRChaperone = nullptr;
	UPROPERTY() AActor* AttachedActor                        = nullptr;

	void SetupRoomscaleOutline();
	void UpdateRoomScaleOutline();
#pragma endregion

   public:
	AMotionControllerCpp();
	EControllerHand Hand = EControllerHand::Right;

	FRotator GetInitialControllerRotation() const { return InitialControllerRotation; }

   protected:
	UPROPERTY(EditAnywhere) UStaticMeshComponent* Arrow                  = nullptr;
	UPROPERTY(EditAnywhere) USkeletalMeshComponent* HandMesh             = nullptr;
	UPROPERTY(EditAnywhere) UMotionControllerComponent* MotionController = nullptr;
	UPROPERTY(EditAnywhere) UHapticFeedbackEffect_Base* RumbleHaptics    = nullptr;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

   private:
	FRotator InitialControllerRotation = FRotator::ZeroRotator;
	void     UpdateAnimationOfHand();
	void     UpdateHandMeshAnimation();
};
