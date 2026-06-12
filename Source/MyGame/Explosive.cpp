// Fill out your copyright notice in the Description page of Project Settings.


#include "Explosive.h"
#include "Main.h"
#include "Enemy.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Sound/SoundCue.h"
#include "HelperFunctionLibrary.h"

AExplosive::AExplosive()
{
	Damage = 15.f;
}

void AExplosive::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnOverlapBegin(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	// Check the overlapped actor, if valid then cast it to the Main and Enemy
	if (OtherActor) {

		// If OtherActor is not of AMain or AEnemy class, Main or Enemy will be null
		AMain* Main = Cast<AMain>(OtherActor);
		AEnemy* Enemy = Cast<AEnemy>(OtherActor);
		if (Main || Enemy) // if indeed OtherActor was of Main or Enemy class
		{ 
			// May needed if Enemy's AgroSphere/Combat Sphere trigger Explosives!
			// UCapsuleComponent* CapsuleComponent = Cast<UCapsuleComponent>(OtherComp);
			// if (CapsuleComponent)
			// {
				if (OverlapParticles) { //if it has a value (in the BP)
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), OverlapParticles, GetActorLocation(), FRotator(0.f), true);
				}

				if (OverlapSound) { //if it has a value (in the BP)
					UGameplayStatics::PlaySound2D(this, OverlapSound);
				}

				// decrease OtherActor's Health with Damage
				UGameplayStatics::ApplyDamage(OtherActor, Damage, nullptr, this, DamageTypeClass);

				UHelperFunctionLibrary::AddItemToPickupArrayAndDestroy(GetWorld(), this);
			//}
		}
	}
}

void AExplosive::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnOverlapEnd(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);
}
