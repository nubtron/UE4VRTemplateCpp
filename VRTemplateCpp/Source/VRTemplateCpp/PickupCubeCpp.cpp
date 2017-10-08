// Fill out your copyright notice in the Description page of Project Settings.

#include "PickupCubeCpp.h"
#include "Components/StaticMeshComponent.h"


void APickupCubeCpp::AttachTo(USceneComponent* const SceneComponent)
{
	auto* const StaticMeshComponent = GetStaticMeshComponent();
	check(IsValid(StaticMeshComponent));
	StaticMeshComponent->SetSimulatePhysics(false);
	auto* const Root = GetRootComponent();
	check(IsValid(Root));
	Root->AttachToComponent(SceneComponent, FAttachmentTransformRules::KeepWorldTransform);
}

void APickupCubeCpp::Drop()
{
	auto* const StaticMeshComponent = GetStaticMeshComponent();
	check(IsValid(StaticMeshComponent));
	StaticMeshComponent->SetSimulatePhysics(false);
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}
