// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup.h"
#include "Main.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "Sound/SoundCue.h"
#include "HelperFunctionLibrary.h"

APickup::APickup()
{
	
}

void APickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnOverlapBegin(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	UE_LOG(LogTemp, Warning, TEXT("APickup::OnOverlapBegin"));

	// Check the overlapped actor, if valid then cast it to the Main Character
	if (OtherActor) {
		// Cast OtherActor to AMain class and store the result to Main
		// If OtherActor is not of AMain class, Main will be null
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main) // if indeed OtherActor was of AMain class
		{
			if (OverlapParticles) { //if it has a value (in the BP)
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), OverlapParticles, GetActorLocation(), FRotator(0.f), true);
			}

			if (OverlapSound) { //if it has a value (in the BP)
				UGameplayStatics::PlaySound2D(this, OverlapSound);
			}

			OnPickupBP(Main);
			//Main->IncrementCoins(CointCount); // increase its Coins with CoinCount

			// Everytime we pickup a pickable, its location will be added to PickupLocations array
			Main->PickupLocations.Add(GetActorLocation());

			UHelperFunctionLibrary::AddItemToPickupArrayAndDestroy(GetWorld(), this);			
		}
	}
}

void APickup::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnOverlapEnd(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);
}

