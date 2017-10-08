// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "PickupActor.h"
#include "PickupCubeCpp.generated.h"


/**
 * 
 */
UCLASS()
class VRTEMPLATECPP_API APickupCubeCpp : public AStaticMeshActor, public IPickupActor
{
	GENERATED_BODY()
	
	
public:
	void AttachTo(USceneComponent* SceneComponent) override;
	void Drop() override;
};
