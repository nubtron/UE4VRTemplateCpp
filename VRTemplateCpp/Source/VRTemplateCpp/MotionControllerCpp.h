// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grip.h"
#include "MotionControllerCpp.generated.h"

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

   public:
	AMotionControllerCpp();

   protected:
	UPROPERTY(EditAnywhere) float TeleportLaunchVelocity = 900.f;

	UPROPERTY(EditAnywhere) UStaticMeshComponent* RoomScaleMesh    = nullptr;
	UPROPERTY(EditAnywhere) UStaticMeshComponent* TeleportCylinder = nullptr;
	UPROPERTY(EditAnywhere) UStaticMeshComponent* ArcEndPoint      = nullptr;
	UPROPERTY(EditAnywhere) UStaticMeshComponent* Arrow            = nullptr;
	UPROPERTY(EditAnywhere) UStaticMesh* BeamMesh                  = nullptr;
	UPROPERTY(EditAnywhere) USkeletalMeshComponent* HandMesh       = nullptr;
	UPROPERTY(EditAnywhere) USphereComponent* GrapSphere           = nullptr;
	UPROPERTY(EditAnywhere) UArrowComponent* ArcDirection          = nullptr;

	UPROPERTY(EditAnywhere) UHapticFeedbackEffect_Base* RumbleHaptics = nullptr;
	UPROPERTY(EditAnywhere) TEnumAsByte<EObjectTypeQuery> TeleportTraceQuery;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

   private:
	UPROPERTY() USteamVRChaperoneComponent* SteamVRChaperone = nullptr;
	UPROPERTY() AActor* AttachedActor                        = nullptr;
	UPROPERTY() USplineComponent* ArcSpline                  = nullptr;
	UPROPERTY() TArray<USplineMeshComponent*> SplineMeshes;

	EControllerHand Hand                       = EControllerHand::Right;
	bool            bIsRoomScale               = false;
	bool            bWantsToGrip               = false;
	EGrip           GripState                  = EGrip::Open;
	bool            bIsTeleporterActive        = false;
	bool            bLastFrameValidDestination = false;
	FRotator        TeleportRotation           = FRotator::ZeroRotator;

	void SetupRoomscaleOutline();
	void UpdateAnimationOfHand();
	void UpdateHandMeshAnimation();
	void UpdateRoomScaleOutline();
	void HandleTeleportArc();
	void ClearArc();
	bool TraceTeleportDestination(TArray<FVector>& OutTracePoints,
	                              FVector&         OutNavMeshLocation,
	                              FVector&         OutTraceLocation) const;
	void RumbleController(const float Intensity);
	void UpdateArcSpline(const bool bFoundValidLocation, const TArray<FVector>& SplinePoints);
	void UpdateArcEndpoint(const FVector& NewLocation, const bool bValidLocationFound);

	UFUNCTION()
	void HandleBeginOverlap_GrabSphere(UPrimitiveComponent* OverlappedComponent,
	                                   AActor*              OtherActor,
	                                   UPrimitiveComponent* OtherComp,
	                                   int32                OtherBodyIndex,
	                                   bool                 bFromSweep,
	                                   const FHitResult&    SweepResult);

	UFUNCTION()
	void HandleComponentHit_ControllerMesh(UPrimitiveComponent* HitComponent,
	                                       AActor*              OtherActor,
	                                       UPrimitiveComponent* OtherComp,
	                                       FVector              NormalImpulse,
	                                       const FHitResult&    Hit);

	AActor* GetActorNearHand() const;
};
