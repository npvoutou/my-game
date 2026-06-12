// Fill out your copyright notice in the Description page of Project Settings.


#include "MovingPlatform.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AMovingPlatform::AMovingPlatform()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);

}

// Called when the game starts or when spawned
void AMovingPlatform::BeginPlay()
{
	Super::BeginPlay();

	/* We need to shift the StartPoint in the Sin, in order to be moving 
	only between the Initial Actor Location and the EndPoint we declared */
	StartPoint = GetActorLocation() + (EndPoint/2);
	
}

// Called every frame
void AMovingPlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float RunningTime = GetGameTimeSinceCreation();
	float DeltaHeight = FMath::Sin(MovingSpeed * RunningTime);
	FVector NewLocation = StartPoint + (EndPoint/2) * DeltaHeight;

	SetActorLocation(NewLocation);
}

