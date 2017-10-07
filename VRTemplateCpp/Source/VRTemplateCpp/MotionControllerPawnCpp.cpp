// Fill out your copyright notice in the Description page of Project Settings.

#include "MotionControllerPawnCpp.h"


// Sets default values
AMotionControllerPawnCpp::AMotionControllerPawnCpp()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMotionControllerPawnCpp::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMotionControllerPawnCpp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMotionControllerPawnCpp::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

