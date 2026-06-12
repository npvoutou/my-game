// Fill out your copyright notice in the Description page of Project Settings.


#include "SpawnVolume.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "Enemy.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "HelperFunctionLibrary.h"

// Sets default values
ASpawnVolume::ASpawnVolume()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpawningBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawningBox"));

}

// Called when the game starts or when spawned
void ASpawnVolume::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("ASpawnVolume::BeginPlay"));

	// Needs a little bit of Delay to be run after the Array has been Loaded..
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [&]()
	{
		if (UHelperFunctionLibrary::HasItemBeenSavedToPickupArray(GetWorld(), this))
		{
			UE_LOG(LogTemp, Warning, TEXT("ASpawnVolume::BeginPlay -> %s Destroyed"), *GetName());
			Destroy();
		}
		else
		{
			SpawnOurActor(GetSpawnActor(), GetSpawnPoint());
		}
	}, 0.2, false);
}

// Called every frame
void ASpawnVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FVector ASpawnVolume::GetSpawnPoint()
{
	FVector Extent = SpawningBox->GetScaledBoxExtent();
	FVector Origin = SpawningBox->GetComponentLocation();
	FVector Point = UKismetMathLibrary::RandomPointInBoundingBox(Origin, Extent);

	return Point;
}

TSubclassOf<AActor> ASpawnVolume::GetSpawnActor()
{
	// if SpawnActors is not empty
	if (SpawnActors.Num() > 0)
	{
		// Return a random Actor (element) from SpawnActors
		int32 Selection = FMath::RandRange(0, SpawnActors.Num() - 1);
		return SpawnActors[Selection];
	}
	else // else return null pointer
	{
		return nullptr; 
	}
}

void ASpawnVolume::SpawnOurActor_Implementation(UClass* ToSpawn, const FVector& Location)
{
	if (ToSpawn) {
		UWorld* World = GetWorld();
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (World) {
			//Spawn the actor into the world
			World->SpawnActor<AActor>(ToSpawn, Location, FRotator(0, FMath::FRand() * 360.f, 0), SpawnParams);
			
			// NO NEED FOR THIS! IS ALREADY DONE IN AEnemy::BeginPlay!
			// Check if it's an Enemy, because we may add other spawning actors
			//AEnemy* Enemy = Cast<AEnemy>(Actor);
			//if (Enemy) //if it is indeed of Enemy class
			//{
				/** Set its Spawned Controller as its AIController (property),
				/* as we cast it in its AEnemy::BeginPlay, because Spawned Pawns 
				/* don't get their AIControllers spawned automatically */
				//AAIController* AIController = Cast<AAIController>(Enemy->GetController());
				//if (AIController)
				//{
					//Enemy->AIController = AIController; 
				//}
			//}
		}
	}
}

