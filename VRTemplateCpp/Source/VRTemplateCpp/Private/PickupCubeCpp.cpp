// Fill out your copyright notice in the Description page of Project Settings.

#include "PickupCubeCpp.h"
#include "Components/StaticMeshComponent.h"


void APickupCubeCpp::AttachTo(USceneComponent* const SceneComponent)
{
	UStaticMeshComponent* const StaticMeshComponent = GetStaticMeshComponent();
	check(IsValid(StaticMeshComponent));
	StaticMeshComponent->SetSimulatePhysics(false);
	USceneComponent* const Root = GetRootComponent();
	check(IsValid(Root));
	Root->AttachToComponent(SceneComponent, FAttachmentTransformRules::KeepWorldTransform);
}

void APickupCubeCpp::Drop()
{
	UStaticMeshComponent* const StaticMeshComponent = GetStaticMeshComponent();
	check(IsValid(StaticMeshComponent));
	StaticMeshComponent->SetSimulatePhysics(true);
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}
