// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MotionControllerPawnCpp.generated.h"

class AMotionControllerCpp;
UCLASS()
class VRTEMPLATECPP_API AMotionControllerPawnCpp : public APawn
{
	GENERATED_BODY()

   public:
	AMotionControllerPawnCpp();

   protected:
	UPROPERTY(EditAnywhere) float DefaultPlayerHeight      = 180.f;
	UPROPERTY(EditAnywhere) float FadeOutDuration          = 0.1f;
	UPROPERTY(EditAnywhere) float FadeInDuration           = 0.2f;
	UPROPERTY(EditAnywhere) float ThumbDeadzone            = 0.7f;
	UPROPERTY(EditAnywhere) FLinearColor TeleportFadeColor = FLinearColor::Black;
	UPROPERTY(EditAnywhere) TSubclassOf<AMotionControllerCpp> MotionControllerCppClass;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

   private:
	UPROPERTY() USceneComponent* VROrigin             = nullptr;
	UPROPERTY() AMotionControllerCpp* LeftController  = nullptr;
	UPROPERTY() AMotionControllerCpp* RightController = nullptr;

	bool     bUseControllerRollToRotate = false;
	bool     bIsTeleporting             = false;
	FRotator TeleportRotation           = FRotator::ZeroRotator;

	void SetupPlayerHeight();

	void ExecuteTeleportation(AMotionControllerCpp* MotionController);
	void FinishTeleportation(TWeakObjectPtr<AMotionControllerCpp> MotionController);

	void                  SetupMotionControllers();
	AMotionControllerCpp* SetupMotionController(EControllerHand Hand);

	void GrabLeft_HandlePressed();
	void GrabLeft_HandleReleased();

	void GrabRight_HandlePressed();
	void GrabRight_HandleReleased();

	void TeleportLeft_HandlePressed();
	void TeleportLeft_HandleReleased();

	void TeleportRight_HandlePressed();
	void TeleportRight_HandleReleased();

	FRotator GetRotationFromInput(float UpAxis, float RightAxis, const AMotionControllerCpp* MotionController) const;
};
