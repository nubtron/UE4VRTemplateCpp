// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MotionControllerPawnCpp.generated.h"

class AMotionControllerCpp;
class UInputComponent;

UCLASS()
class VRTEMPLATECPP_API AMotionControllerPawnCpp : public APawn
{
	GENERATED_BODY()

   public:
	AMotionControllerPawnCpp();

   protected:
	UPROPERTY(EditAnywhere)
	float DefaultPlayerHeight = 180.f;

	UPROPERTY(EditAnywhere)
	float FadeOutDuration = 0.1f;

	UPROPERTY(EditAnywhere)
	float FadeInDuration = 0.2f;

	UPROPERTY(EditAnywhere)
	float ThumbDeadzone = 0.7f;

	UPROPERTY(EditAnywhere)
	FLinearColor TeleportFadeColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AMotionControllerCpp> MotionControllerCppClass;

	virtual void BeginPlay() override;
	virtual void Tick(const float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

   private:
	UPROPERTY()
	USceneComponent* VROrigin = nullptr;

	UPROPERTY()
	AMotionControllerCpp* LeftController = nullptr;

	UPROPERTY()
	AMotionControllerCpp* RightController = nullptr;

	bool bUseControllerRollToRotate = false;
	bool bIsTeleporting             = false;

	FVector2D ThumbLeftInput  = FVector2D::ZeroVector;
	FVector2D ThumbRightInput = FVector2D::ZeroVector;

	void SetupPlayerHeight();

	void ExecuteTeleportation(AMotionControllerCpp* const MotionController);
	void FinishTeleportation(const TWeakObjectPtr<AMotionControllerCpp> MotionController);

	void                  SetupMotionControllers();
	AMotionControllerCpp* SetupMotionController(const EControllerHand Hand);

	void GrabLeft_HandlePressed();
	void GrabLeft_HandleReleased();

	void GrabRight_HandlePressed();
	void GrabRight_HandleReleased();

	void TeleportLeft_HandlePressed();
	void TeleportLeft_HandleReleased();

	void TeleportRight_HandlePressed();
	void TeleportRight_HandleReleased();

	void MotionControllerThumbLeft_X_HandleAxisInput(float Value) { ThumbLeftInput.X = Value; }
	void MotionControllerThumbLeft_Y_HandleAxisInput(float Value) { ThumbLeftInput.Y = Value; }
	void MotionControllerThumbRight_X_HandleAxisInput(float Value) { ThumbRightInput.X = Value; }
	void MotionControllerThumbRight_Y_HandleAxisInput(float Value) { ThumbRightInput.Y = Value; }

	FRotator GetRotationFromInput(const float                 UpAxis,
	                              const float                 RightAxis,
	                              const AMotionControllerCpp* MotionController) const;
};
