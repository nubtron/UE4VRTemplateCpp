// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Grip.h"
#include "HandAnimInstance.generated.h"


/**
 * 
 */
UCLASS()
class VRTEMPLATECPP_API UHandAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	void SetGripState(const EGrip InGripState) { GripState = InGripState; }
protected:
	UFUNCTION(BlueprintPure) EGrip GetGripState() const { return GripState; }
	
private:
	EGrip GripState = EGrip::Open;
};
