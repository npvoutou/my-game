// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy.h"
#include "Components/SphereComponent.h"
#include "AIController.h"
#include "Main.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Sound/SoundCue.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "MainPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "HelperFunctionLibrary.h"
#include "SpawnVolume.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"

// Sets default values
AEnemy::AEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AgroSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AgroSphere"));
	AgroSphere->SetupAttachment(GetRootComponent());
	AgroSphere->InitSphereRadius(550.f);
	
	CombatSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CombatSphere"));
	CombatSphere->SetupAttachment(GetRootComponent());
	CombatSphere->InitSphereRadius(75.f);

	/** Ignore every collision channel except the Pawn Channel
	/* where we need to generate Overlap events */ 
	AgroSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AgroSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	CombatSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CombatSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

	CombatCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("CombatCollision"));
	// Attach CombatCollision to the EnemySocket we created on the Spider's Left Claw
	CombatCollision->SetupAttachment(GetMesh(), FName("EnemySocket"));
	// this does the same as above line, but should not exist in the constructor, only on other places if we want to use it
	//CombatCollision->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("EnemySocket"));

	SetEnemyMovementStatus(EEnemyMovementStatus::EEMS_Idle);

	bIsBoss = false;

	Health = 100.f;
	MaxHealth = 100.f;
	Damage = 20.f;

	AttackMinTime = 0.5f;
	AttackMaxTime = 3.5f;

	DeathDelay = 3.f;
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	// if spawned, owner is SpawnVolume (a.k.a. EnemiesVolume_BP)
	AActor* SpawnedOwner = Cast<ASpawnVolume>(GetOwner()); 

	// After making Enemies Spawnables we must add this before setting AIController
	SpawnDefaultController(); // owner becomes AIController

	// we want to keep SpawnedOwner if exists, instead of AIController, else SetOwner to null
	SpawnedOwner ? SetOwner(SpawnedOwner) : SetOwner(nullptr);

	// Needs a little bit of Delay to be run after the Array has been Loaded..
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [&]()
	{
		if (UHelperFunctionLibrary::HasItemBeenSavedToPickupArray(GetWorld(), this))
		{
			UE_LOG(LogTemp, Warning, TEXT("AEnemy::BeginPlay -> %s Destroyed"), *GetName());
			Destroy();
		}
		else
		{
			/** Set its reference to the GetController() result, after Casting
			/*  it into AIController, in BeginPlay. GetController returns
			/*  AController which is parent of AIController */
			AIController = Cast<AAIController>(GetController());

			// Bind overlap events to the AgroSphere component
			AgroSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::AgroSphereOnOverlapBegin);
			AgroSphere->OnComponentEndOverlap.AddDynamic(this, &AEnemy::AgroSphereOnOverlapEnd);

			// Bind overlap events to the CombatSphere component
			CombatSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::CombatSphereOnOverlapBegin);
			CombatSphere->OnComponentEndOverlap.AddDynamic(this, &AEnemy::CombatSphereOnOverlapEnd);

			// Bind overlap events to the CombatCollision component
			CombatCollision->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::CombatOnOverlapBegin);
			CombatCollision->OnComponentEndOverlap.AddDynamic(this, &AEnemy::CombatOnOverlapEnd);

			// By default no collision will happen for the CombatCollision
			CombatCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			/** It's something that we can move around in the world,
			/* and has automatic overlap parameters set for it. */
			CombatCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
			/** Ignore every collision channel except the Pawn Channel
			/* where we need to generate Overlap events */
			CombatCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CombatCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

			// Enemy's Mesh & Capsule Component to Ignore Collision with the Camera, so that
			// the Camera doesn't zoom in between Enemy and Main when is obscured by the Enemy 
			GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
			GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
		}
	}, 0.2, false);
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Check if Enemy needs to interpolate to us and has a valid CombatTarget
	if (bAttacking && CombatTarget)
	{
		// Find what rotation we need to orient ourselves to this CombatTarget Location.
		FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), CombatTarget->GetActorLocation());

		// Find our rotation destination, where we'll look directly at the Enemy
		FRotator LookAtYaw = FRotator(0.f, LookAtRotation.Yaw, 0.f); // We only care about the Yaw Rotation

		/** This will give us a rotation for this frame to do a smooth transition
		/* from our current rotation to the target rotation.*/
		FRotator InterpRotation = FMath::RInterpTo(GetActorRotation(), LookAtYaw, DeltaTime, 10.f);
		//Smoothly rotate the character to where he'll be facing the Enemy
		SetActorRotation(InterpRotation);
	}

	/* Start moving towards our player if he's reachable now after 
	Aborted or Invalid Movement Result. This is needed for situations 
	where Enemy's movement has stopped because we jumped to the moving 
	platform but haven't exit its AgroSphere yet. */
	if (CombatTarget && IsTargetReachable(CombatTarget) && bRefocusWhenTargetReachable)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Purple, FString::Printf(TEXT("RefocusWhenTargetReachable")));

		bRefocusWhenTargetReachable = false;
		MoveToTarget(CombatTarget);
	}
}

// Called to bind functionality to input
void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AEnemy::AgroSphereOnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("AEnemy::AgroSphereOnOverlapBegin() -> %s"), *OtherActor->GetName());
	if (OtherActor && Alive()) // if OtherActor is valid & Enemy is Alive
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main)
		{
			// Set it for MoveToTarget to be called from blueprints
			CombatTarget = Main;

			if (IsTargetReachable(Main))
			{
				Main->DisplayHUDMainText(FString("Enemy Approaching!!"), false);
				MoveToTarget(Main);
			}			
		}
	}
}

void AEnemy::AgroSphereOnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("AEnemy::AgroSphereOnOverlapEnd() -> %s"), *OtherActor->GetName());
	if (OtherActor) // if is valid
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main)
		{
			// We don't need a combat target here
			CombatTarget = nullptr; //TODO: CHECK IF THIS IS NEEDED HERE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

			// Check if Main's current CombatTarget is this particular Enemy
			if (Main->CombatTarget == this)
			{
				Main->SetCombatTarget(nullptr);
			}

			// Update Closest CombatTarget & Hide Enemy's Health Bar
			Main->UpdateCombatTarget();

			// Change its state to Idle and stop the movement
			StopMovementIdle();
		}
	}
}

void AEnemy::CombatSphereOnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("AEnemy::CombatSphereOnOverlapBegin() -> %s"), *OtherActor->GetName());
	if (OtherActor && Alive()) // if OtherActor is valid & Enemy is Alive
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main)
		{			
			bOverlappingCombatSphere = true;
			Main->SetCombatTarget(this);

			// Update Closest CombatTarget & Display Enemy's Health Bar
			Main->UpdateCombatTarget();

			// Set it for MoveToTarget to be called from blueprints
			//CombatTarget = Main; // TODO: check if this must be Commented out
			
			/** Set a second Timer to Attack again after waiting for a random AttackTime
			/* and it'll seem like it's hesitating or thinking for a second. */
			float AttackTime = FMath::FRandRange(AttackMinTime, AttackMaxTime);
			GetWorldTimerManager().SetTimer(AttackTimer2, this, &AEnemy::Attack, AttackTime);
		}
	}
}

void AEnemy::CombatSphereOnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("AEnemy::CombatSphereOnOverlapEnd() -> %s"), *OtherActor->GetName());
	if (OtherActor) // if is valid
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main)
		{
			bOverlappingCombatSphere = false;

			if (EnemyMovementStatus == EEnemyMovementStatus::EEMS_Attacking)
			{
				// Call MoveToTarget if Enemy is Attacking
				MoveToTarget(Main);
				// We don't need a combat target here
				//CombatTarget = nullptr; //TODO: UNCOMMENT THIS!!
			}

			/** If this Enemy is the current Main's CombatTarget, then
			/* as soon as he ends overlapping, we should check for another
			/* Combat Target, otherwise he may already have one */
			if (Main->CombatTarget == this)
			{
				Main->SetCombatTarget(nullptr);
				Main->UpdateCombatTarget();
			}				

			// Clear the Timer that calls Attack function
			GetWorldTimerManager().ClearTimer(AttackTimer);

			/** MoveToTarget will also be called from SpiderAnim_BP as soon 
			/* as the attack animation is finished with EndAttack notify */
		}
	}
}

void AEnemy::MoveToTarget(AMain* Target)
{
	if (!Target) return;

	//Set enemy's movement status to MoveToTarget
	SetEnemyMovementStatus(EEnemyMovementStatus::EEMS_MoveToTarget);

	// Move to the Target functionality
	if (AIController) // if is valid
	{
		// Create MoveRequest by setting GoalActor & AcceptanceRadius
		FAIMoveRequest MoveRequest;
		MoveRequest.SetGoalActor(Target);
		MoveRequest.SetAcceptanceRadius(bIsBoss ? 40.f : 20.f);
		MoveRequest.SetUsePathfinding(true); // in order to use regular pathfinding and not direct between two points
		//MoveRequest.SetAllowPartialPath(false);

		// Bind OnRequestFinished to the AIController
		AIController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(this, &AEnemy::OnMoveToCompleted);

		// Create an empty NavPath that will be filled with info after 
		FNavPathSharedPtr NavPath;

		// The AI MoveTo function in C++ is called from the AI Controller.
		AIController->MoveTo(MoveRequest, &NavPath);

		/** Draw debug spheres in order to see where these path points 
		/* are and how often they're generated */
		/*TArray<FNavPathPoint> PathPoints = NavPath->GetPathPoints();
		for (auto Point : PathPoints) 
		{
			// for each point in PathPoints, access its location
			FVector Location = Point.Location;

			// and print debug spheres at that location
			UKismetSystemLibrary::DrawDebugSphere(this, Location, 25.f, 12, FLinearColor::Red, 3.f, 2.f);
		}*/
	}
}

void AEnemy::OnMoveToCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	if (AIController) // if is valid
	{
		// Unbind OnRequestFinished from the AIController
		AIController->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);

		//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("RESULT: %s"), *Result.ToString()));
		UE_LOG(LogTemp, Warning, TEXT("RESULT: %s"), *Result.ToString());

		// If MoveTo Completion Interrupted of Failed while there's still a CombatTarget, retry to Move to it!
		//Invalid Request na min to proxwraei
		if ((Result.IsInterrupted() || Result.IsFailure()) && Result.Code != EPathFollowingResult::Invalid && CombatTarget) {
			UE_LOG(LogTemp, Warning, TEXT("RETRIES TO MOVE TO TARGET!"));
			MoveToTarget(CombatTarget);
		}
		else if (!Result.IsSuccess())
		{
			//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White, FString::Printf(TEXT("STOP MOVEMENT with RESULT: %s!"), *Result.ToString()));
			UE_LOG(LogTemp, Warning, TEXT("STOP MOVEMENT with RESULT: %s!"), *Result.ToString());

			// Change its state to Idle and stop the movement
			StopMovementIdle(); // TODO: Check if we need this

			bRefocusWhenTargetReachable = true;
		}
	}
}

void AEnemy::CombatOnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor) // if is valid
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main)
		{
			if (Main->HitParticles) // if Main has set HitParticles
			{
				// Spawn the HitPartilcles Emitter at the TipSocket Location 
				// of the Enemy's Claw as it ovelaps with our Main Character
				const USkeletalMeshSocket* TipSocket = GetMesh()->GetSocketByName(bIsBoss ? "BossTipSocket" : "TipSocket");
				if (TipSocket)
				{
					FVector SocketLocation = TipSocket->GetSocketLocation(GetMesh());
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Main->HitParticles, SocketLocation, FRotator(0.f), false);
				}
			}

			if (Main->HitSound) //if there's a HitSound for Main
			{
				UGameplayStatics::PlaySound2D(this, Main->HitSound);
			}

			if (DamageTypeClass) // if there's a DamageTypeClass set in BP
			{
				// Apply Damage to the Main Character (this calls Main’s TakeDamage)
				UGameplayStatics::ApplyDamage(Main, Damage, AIController, this, DamageTypeClass);
			}
		}
	}
}

void AEnemy::CombatOnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	
}

void AEnemy::ActivateCollision()
{
	// Set Collision to overlap and generate overlap events
	CombatCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	if (SwingSound) //if there's a SwingSound, play it
	{
		UGameplayStatics::PlaySound2D(this, SwingSound);
	}
}

void AEnemy::DeactivateCollision()
{
	// Disable Collision
	CombatCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

/* It's called when Enemy's CombatSphere collides with our Main Character
/* Also, when we're keep overlapping with Enemy’s CombatSphere in AttackEnd */
void AEnemy::Attack()
{
	// If his CombatTarget is dead, don't attack and restart his death animation -> for multiple enemies!
	if (CombatTarget && CombatTarget->MovementStatus == EMovementStatus::EMS_Dead) return;

	if (Alive()) // If Enemy is not Alive there's no need to Attack
	{
		// If Enemy moves, stop movement and set status to Attacking
		if (AIController) // if is valid
		{
			AIController->StopMovement();
			SetEnemyMovementStatus(EEnemyMovementStatus::EEMS_Attacking);
		}

		/** if it's not attacking, then it starts the attack animation,
		/* in order to prevent re-attacking when already playing attack anim */
		if (!bAttacking)
		{
			bAttacking = true;

			// Get the AnimInstance of the Enemy
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance && CombatMontage) // if these are valid
			{
				//play the Attack section from the CombatMontage AnimInstance
				AnimInstance->Montage_Play(CombatMontage, 1.35f);
				AnimInstance->Montage_JumpToSection(FName("Attack"), CombatMontage);
			}
		}
	}
}

// It's called within SpiderAnim_BP upon AttackEnd Anim Notify
void AEnemy::AttackEnd()
{
	bAttacking = false;

	/** Keep attacking if our player is still overlapping with 
	/* Enemy’s CombatSphere near the End of Attacking */
	if (bOverlappingCombatSphere)
	{
		/** Set the Timer to Attack again after waiting for a random AttackTime
		/* and it'll seem like it's hesitating or thinking for a second. */
		float AttackTime = FMath::FRandRange(AttackMinTime, AttackMaxTime);
		GetWorldTimerManager().SetTimer(AttackTimer, this, &AEnemy::Attack, AttackTime);
	}
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	UE_LOG(LogTemp, Warning, TEXT("AEnemy::TakeDamage()"));
	
	ShowDamageAmount(-1 * DamageAmount);

	PlayEmissiveMaterialEffect();
	Health -= DamageAmount;
	if (Health <= 0.f) {
		Die();
	}
	return DamageAmount;
}

void AEnemy::Die()
{
	UE_LOG(LogTemp, Warning, TEXT("AEnemy::Die()"));

	// Get the AnimInstance of the Enemy
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && CombatMontage) // if these are valid
	{
		//play the Death section from the CombatMontage 
		AnimInstance->Montage_Play(CombatMontage, 1.35f);
		AnimInstance->Montage_JumpToSection(FName("Death"), CombatMontage);
	}
	SetEnemyMovementStatus(EEnemyMovementStatus::EEMS_Dead);

	// Set all of Enemy's Collision Volumes to NoCollision when it dies
	// THESE FIRE OnOverlapEnd FOR THESE VOLUMES, SO BE CAREFULL WHAT TO CONTAIN! 
	CombatCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AgroSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CombatSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// Clear the Timers that call Attack function, in case they already had been 
	// initiated earlier which prevent the Enemy from actually Dying! 
	GetWorldTimerManager().ClearTimer(AttackTimer);
	GetWorldTimerManager().ClearTimer(AttackTimer2);
}

void AEnemy::DeathEnd()
{
	// Stop all animations
	GetMesh()->bPauseAnims = true;
	// GetMesh()->bNoSkeletonUpdate = true; //REMOVE IT: it creates a glitching bug

	// Set the DeathTimer to Disappear after waiting for DeathDelay time
	GetWorldTimerManager().SetTimer(DeathTimer, this, &AEnemy::Disappear, DeathDelay);
}

bool AEnemy::Alive()
{
	return GetEnemyMovementStatus() != EEnemyMovementStatus::EEMS_Dead;
}

void AEnemy::Disappear()
{
	if (ItemToDrop) // If Enemy has an item to Drop in BP
	{
		if (bIsBoss) // and if we've killed the Boss 
		{
			DropItem();
		}
		else // else if it's every other Enemy
		{
			// Randomize the frequency of Dropping Items to 30%
			if (ShouldDropItem(30))
			{
				DropItem();
			}
		}
	}

	//Destroy();
	UHelperFunctionLibrary::AddItemToPickupArrayAndDestroy(GetWorld(), this);
}

bool AEnemy::ShouldDropItem(int Percentage)
{
	// return FMath::FRand() < (Percentage / 100); // FMath::FRand() returns a float between 0.0 and 1.0
	return FMath::RandRange(1, 100 / Percentage) == 1;
}

// Animates Emissive Material pulse for 1 sec
void AEnemy::PlayEmissiveMaterialEffect()
{
	GetMesh()->SetScalarParameterValueOnMaterials(TEXT("StartTime"), GetWorld()->GetTimeSeconds());
}

void AEnemy::DropItem()
{
	FVector Location = GetActorLocation();
	// Offset Z axis to keep item at player level
	Location.Z -= bIsBoss ? 50.f : -50.f;

	if (ItemSpawnSound) //if it has a value (in the BP)
	{
		UGameplayStatics::PlaySound2D(this, ItemSpawnSound);
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	//Spawn the item into the world
	AActor* SpawnedItem = GetWorld()->SpawnActor<AActor>(ItemToDrop, Location, FRotator(0.f), SpawnParams);
	//SpawnedItem->SetActorLocation(Location); // maybe it's needed for the SpawnVolume's RandomPointInBoundingBox
}

void AEnemy::StopMovementIdle()
{
	SetEnemyMovementStatus(EEnemyMovementStatus::EEMS_Idle);
	if (AIController)
	{
		// Will stop moving to wherever it was it was going to.
		AIController->StopMovement();
	}
}

bool AEnemy::IsTargetReachable(AMain* Target)
{
	UNavigationPath* NavPath = UNavigationSystemV1::FindPathToActorSynchronously(GetWorld(), GetActorLocation(), Target);

	if (!NavPath)
		return false;

	return NavPath->IsValid();
}
