// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Main.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/BoxComponent.h"
#include "Enemy.h"
#include "Engine/SkeletalMeshSocket.h"
#include "HelperFunctionLibrary.h"

AWeapon::AWeapon()
{
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(GetRootComponent());

	CombatCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("CombatCollision"));
	CombatCollision->SetupAttachment(GetRootComponent());

	bWeaponPatricles = false;

	WeaponState = EWeaponState::EWS_Pickup;
	
	Damage = 25.f;

}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	// Bind overlap events to the CombatCollision component
	CombatCollision->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::CombatOnOverlapBegin);
	CombatCollision->OnComponentEndOverlap.AddDynamic(this, &AWeapon::CombatOnOverlapEnd);
	
	// By default no collision will happen for the CombatCollision
	CombatCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	/** It's something that we can move around in the world,
	/* and has automatic overlap parameters set for it. */
	CombatCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	/** Ignore every collision channel except the Pawn Channel
	/* where we need to generate Overlap events */
	CombatCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CombatCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

}

void AWeapon::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnOverlapBegin(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	// Check if Weapon State is Pickup and then check if overlapped actor is valid 
	if ((WeaponState == EWeaponState::EWS_Pickup) && OtherActor) {

		UE_LOG(LogTemp, Warning, TEXT("AWeapon::OnOverlapBegin (%s) -> %s"), *FString(Name), *OtherActor->GetName());

		// Cast OtherActor to AMain class and store the result to Main
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main) // if indeed OtherActor was of AMain class
		{
			UE_LOG(LogTemp, Warning, TEXT("AWeapon::OnOverlapBegin: Pickup && Main"));
			// Set this weapon as his ActiveOverlappingItem in order to choose if he wants to equip it
			Main->SetActiveOverlappingItem(this);
		}
	}
}

void AWeapon::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnOverlapEnd(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);

	// Check the overlapped actor, if valid then cast it to the Main Character
	if (OtherActor) {
		// Cast OtherActor to AMain class and store the result to Main
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main) // if indeed OtherActor was of AMain class
		{
			// We no longer have an active overlapping item on the Character
			Main->SetActiveOverlappingItem(nullptr);
		}
	}
}

void AWeapon::Equip(AMain* Character, bool bToBeSaved)
{
	if (Character) // if is valid
	{ 
		SetInstigator(Character->GetController());

		/** Set the Weapon's Collision Response to Ignore for the Camera   
		/* so that the Camera doesn't Zoom In on the Character when  
		/* the Weapon gets between the Character and the Camera. */
		SkeletalMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

		// We don't want any collision happening between the Weapon and the Pawn 
		SkeletalMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

		/** If we're simulating Physics, we need to stop simulating 
		/* because we're going to attach this to the Character. */
		SkeletalMesh->SetSimulatePhysics(false);

		// Get the Character's RightHandSocket socket
		const USkeletalMeshSocket* RightHandSocket = Character->GetMesh()->GetSocketByName("RightHandSocket");
		if (RightHandSocket)
		{
			// attach this weapon to the Character's RightHandSocket
			RightHandSocket->AttachActor(this, Character->GetMesh());
			// and set it as his EquippedWeapon
			Character->SetEquippedWeapon(this);
			// and Character no longer has an active overlapping item 
			Character->SetActiveOverlappingItem(nullptr);
			// and set the Weapon's State as Equipped
			SetWeaponState(EWeaponState::EWS_Equipped);

			// Stop Rotating when attached to Character
			bRotate = false;
		}

		// Play a sound if it is set on the Weapon (instance or defaults)
		if (OnEquipSound && bToBeSaved) UGameplayStatics::PlaySound2D(this, OnEquipSound);

		// Deactivate the Idle Particles when equipping the Weapon
		if (!bWeaponPatricles) IdleParticlesComponent->Deactivate();

		// Save Weapon to PickupArray when equipped, but not when loading
		if (bToBeSaved) UHelperFunctionLibrary::AddItemToPickupArrayAndDestroy(GetWorld(), this, false);
	}
}

void AWeapon::CombatOnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("AWeapon::CombatOnOverlapBegin() -> %s"), *OtherActor->GetName());
	if (OtherActor) // if is valid
	{
		AEnemy* Enemy = Cast<AEnemy>(OtherActor);
		if (Enemy)
		{
			if (Enemy->HitParticles) // if Enemy has set HitParticles
			{
				// Spawn the HitPartilcles Emitter at the WeaponSocket Location
				// as weapon ovelaps with the enemy
				const USkeletalMeshSocket* WeaponSocket = SkeletalMesh->GetSocketByName("WeaponSocket");
				if (WeaponSocket)
				{
					FVector SocketLocation = WeaponSocket->GetSocketLocation(SkeletalMesh);
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Enemy->HitParticles, SocketLocation, FRotator(0.f), false);
				}
			}

			if (Enemy->HitSound) //if there's a HitSound for Enemy
			{
				UGameplayStatics::PlaySound2D(this, Enemy->HitSound);
			}

			if (DamageTypeClass) // if there's a DamageTypeClass set in BP
			{
				// Apply Damage to the Enemy (this calls Enemy’s TakeDamage)
				UGameplayStatics::ApplyDamage(Enemy, Damage, WeaponInstigator, this, DamageTypeClass);
			}
		}
	}
}

void AWeapon::CombatOnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{

}

void AWeapon::ActivateCollision()
{
	// Set Collision to overlap and generate overlap events
	CombatCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AWeapon::DeactivateCollision()
{
	// Disable Collision
	CombatCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
