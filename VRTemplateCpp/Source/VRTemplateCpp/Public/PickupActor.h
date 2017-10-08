// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PickupActor.generated.h"

/**
 * Interface for an actor that can be picked up by the player.
 */
UINTERFACE()
class UPickupActor : public UInterface
{
	GENERATED_BODY()
};

class IPickupActor : public IInterface
{
	GENERATED_BODY()

   public:
	virtual void AttachTo(USceneComponent* SceneComponent) = 0;
	virtual void Drop()                                    = 0;
};
