// Fill out your copyright notice in the Description page of Project Settings.


#include "Main.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Weapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "Enemy.h"
#include "MainPlayerController.h"
#include "SaveGameBase.h"
#include "ItemStorage.h"
#include "GameInstanceBase.h"
#include "MyGameModeBase.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerStart.h"

// Sets default values
AMain::AMain()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->SetCapsuleSize(49.f, 105.f);


	// Create Camera Boom (pulls towards the player if there's a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 600.f; // Camera follows at this distance
	// Let the Boom adjust to match the controller orientation
	CameraBoom->bUsePawnControlRotation = true; // Rotate arm based on controller

	// Create Follow Camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	// Attach the Camera to the end of the Boom, thus following along wherever it goes 
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// We don't want the camera to be dependent on the controller
	FollowCamera->bUsePawnControlRotation = false;

	// Set our turn rates for input
	BaseTurnRate = 65.f;
	BaseLookUpRate = 65.f;

	// Don't rotate when the controller rotates.
	// Let that just affect the camera only.
	bUseControllerRotationYaw = false; // if true, camera always sees character's back 
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// Turn the character towards the direction	that he's physically moving.
	GetCharacterMovement()->bOrientRotationToMovement = true;
	// Orient his Yaw rotation to the movement at this rotation rate.
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f); 

	// Set how high he jumps, how fast his velocity is
	GetCharacterMovement()->JumpZVelocity = 650.f;
	// Set how much he moves around while in the air
	GetCharacterMovement()->AirControl = 0.2f;

	// Default Player Stats values
	MaxHealth = 100.f;
	Health = 65.f;
	MaxStamina = 150.f;
	Stamina = 120.f;
	Coins = 0;
	bHasBossKey = false;
	// TODO: maybe it needs initialization?
	//PickupArray = 

	RunningSpeed = 650.f;
	SprintingSpeed = 950.f;

	bShiftKeyDown = false;
	bLMBDown = false;

	// Initialize Enums
	MovementStatus = EMovementStatus::EMS_Normal;
	StaminaStatus = EStaminaStatus::ESS_Normal;

	StaminaDrainRate = 25.f; //per second
	MinSprintStamina = 50.f;

	InterpSpeed = 15.f;
	bInterpToEnemy = false;

	bMovingForward = false;
	bMovingRight = false;
}

// Called when the game starts or when spawned
void AMain::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("AMain::BeginPlay()"));

	//Bind Player destroyed event (e.g. from falling into KillZVolume) to CharacterDestroyed function.
	if (!OnDestroyed.IsBound())
	{
		OnDestroyed.AddDynamic(this, &AMain::CharacterDestroyed);
	}

	MainPlayerController = Cast<AMainPlayerController>(GetController());
	
	// TODO: Load Game off save game before starting play
	LoadGame();
	
}

// Called every frame
void AMain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// If we're dead it will exit this function here
	if (MovementStatus == EMovementStatus::EMS_Dead) return;


	/** DeltaStamina is how much the Stamina should change in this 
	/* particular frame in order to drain or recover */
	float DeltaStamina = StaminaDrainRate * DeltaTime;

	// check in which state the StaminaStatus is
	switch (StaminaStatus) 
	{
	case EStaminaStatus::ESS_Normal:
		if (bShiftKeyDown && (bMovingForward || bMovingRight)) 
		{
			Stamina -= DeltaStamina; // Drain the Stamina

			// if we're crossing from Normal to BelowMinimum State
			if (Stamina <= MinSprintStamina)
			{
				SetStaminaStatus(EStaminaStatus::ESS_BelowMinimum);
			}

			// update our MovementStatus, because we're holding Shift
			SetMovementStatus(EMovementStatus::EMS_Sprinting);
		}
		else // Shift key up
		{
			// we don't want to keep increasing Stamina past the Max
			if (Stamina + DeltaStamina >= MaxStamina)
			{
				Stamina = MaxStamina;
			}
			else //if incrementing the Stamina won't push us past the Max
			{
				Stamina += DeltaStamina; // Recharge the Stamina
			}

			// update our MovementStatus, because we're not holding Shift
			SetMovementStatus(EMovementStatus::EMS_Normal);
		}
		
		break;
	case EStaminaStatus::ESS_BelowMinimum:
		if (bShiftKeyDown && (bMovingForward || bMovingRight))
		{
			// if we're near zero, we need to switch to Exhausted State
			if (Stamina - DeltaStamina <= 0.f)
			{
				SetStaminaStatus(EStaminaStatus::ESS_Exhausted);
				Stamina = 0.f;
				// we no longer can Sprint, because we're Exhausted
				SetMovementStatus(EMovementStatus::EMS_Normal);
			}
			else {
				Stamina -= DeltaStamina; // Drain the Stamina
				// update our MovementStatus, because we're holding Shift
				SetMovementStatus(EMovementStatus::EMS_Sprinting);
			}
		}
		else // Shift key up
		{
			Stamina += DeltaStamina; // Recharge the Stamina

			// if we cross MinSprintStamina, we need to switch to Normal State
			if (Stamina >= MinSprintStamina)
			{
				SetStaminaStatus(EStaminaStatus::ESS_Normal);
			}

			// update our MovementStatus, because we're not holding Shift
			SetMovementStatus(EMovementStatus::EMS_Normal);
		}

		break;
	case EStaminaStatus::ESS_Exhausted:
		if (bShiftKeyDown && (bMovingForward || bMovingRight))
		{
			/** We've reached the exhaustion point and we don't want the Stamina 
			/* to be recharging until we release the shift key */
			Stamina = 0.f;
		}
		else // Shift key up
		{
			Stamina += DeltaStamina; // Recharge the Stamina
			SetStaminaStatus(EStaminaStatus::ESS_ExhaustedRecovering);
		}

		// update our MovementStatus, because we're Exhausted
		SetMovementStatus(EMovementStatus::EMS_Normal);

		break;
	case EStaminaStatus::ESS_ExhaustedRecovering:
		
		Stamina += DeltaStamina; // Recharge the Stamina

		// if we cross MinSprintStamina, we need to switch to Normal State
		if (Stamina >= MinSprintStamina)
		{
			SetStaminaStatus(EStaminaStatus::ESS_Normal);
		}

		// update our MovementStatus, because we're Exhausted Recovering
		SetMovementStatus(EMovementStatus::EMS_Normal);

		break;
	default:
		;
	}

	// Check if we need to interpolate to our enemy and have a valid CombatTarget
	if (bInterpToEnemy && CombatTarget)
	{
		// Find our rotation destination, where we'll look directly at the Enemy
		FRotator LookAtYaw = GetLookAtRotationYaw(CombatTarget->GetActorLocation());
		
		/** This will give us a rotation for this frame to do a smooth transition 
		/* from our current rotation to the target rotation.*/
		FRotator InterpRotation = FMath::RInterpTo(GetActorRotation(), LookAtYaw, DeltaTime, InterpSpeed);
		//Smoothly rotate the character to where he'll be facing the Enemy
		SetActorRotation(InterpRotation);
	}

	// Update EnemyLocation for EnemyHealthBar widget to follow the Enemy.
	if (CombatTarget && MainPlayerController)
	{
		MainPlayerController->EnemyLocation = CombatTarget->GetActorLocation();
	}
}

FRotator AMain::GetLookAtRotationYaw(FVector Target)
{
	// Find what rotation we need to orient ourselves to this CombatTarget Location.
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Target);

	// We only care about the Yaw Rotation
	return FRotator(0.f, LookAtRotation.Yaw, 0.f);
}

// Called to bind functionality to input
void AMain::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// checks if PlayerInputComponent is valid otherwise stops the execution
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMain::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AMain::ShiftKeyDown);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AMain::ShiftKeyUp);

	PlayerInputComponent->BindAction("LMB", IE_Pressed, this, &AMain::LMBDown);
	PlayerInputComponent->BindAction("LMB", IE_Released, this, &AMain::LMBUp);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMain::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMain::MoveRight);

	PlayerInputComponent->BindAxis("TurnRate", this, &AMain::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMain::LookUpAtRate);

	// Whenever the mouse is moving, the mouse input value (this) is going to be 
	// returned and fed into the function we give it (the inherited one).
	PlayerInputComponent->BindAxis("Turn", this, &AMain::Turn); //&APawn::AddControllerYawInput
	PlayerInputComponent->BindAxis("LookUp", this, &AMain::LookUp); //&APawn::AddControllerPitchInput

}

bool AMain::CanMove(float Value)
{
	/** this is needed because for some reason after calling RestartPlayer, in order to 
	/* respawn him after destroyed in KillZ volume), MainPlayerController is null */
	if (MainPlayerController == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Initialize MainPlayerController again, needed after Respawn Player.."));
		MainPlayerController = Cast<AMainPlayerController>(GetController());
	}

	if (MainPlayerController)
	{
		return Value != 0.f && !bAttacking && 
			MovementStatus != EMovementStatus::EMS_Dead;
	}
	return false;
}

void AMain::MoveForward(float Value)
{
	bMovingForward = false;

	if (CanMove(Value)) {

		bMovingForward = true;

		// Get the direction that the controller is facing this frame.
		const FRotator Rotation = Controller->GetControlRotation();
		// Rotator that contains only the Yaw from the Controller
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f); 

		// Find out which way is FORWARD (with the EAxis::X)
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		// Add movement input for our character in the direction we just calculated.
		AddMovementInput(Direction, Value);
	}
}

void AMain::MoveRight(float Value)
{
	bMovingRight = false;

	if (CanMove(Value)) {

		bMovingRight = true;

		// Get the direction that the controller is facing this frame.
		const FRotator Rotation = Controller->GetControlRotation();
		// Rotator that contains only the Yaw from the Controller
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		// Find out which way is RIGHT (with the EAxis::Y)
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// Add movement input for our character in the direction we just calculated.
		AddMovementInput(Direction, Value);
	}
}

void AMain::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void AMain::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void AMain::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMain::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMain::LMBDown()
{
	bLMBDown = true;

	// If we're dead it will exit this function here
	if (MovementStatus == EMovementStatus::EMS_Dead) return;

	// Check if we're overlapping with an Item
	if (ActiveOverlappingItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("AMain::LMBDown -> %s"), *ActiveOverlappingItem->GetName());

		// Cast ActiveOverlappingItem to a Weapon
		AWeapon* Weapon = Cast<AWeapon>(ActiveOverlappingItem);
		if (Weapon) // if indeed ActiveOverlappingItem was of AWeapon class
		{
			// Weapon will be Equipped/Attached to the Main Character
			Weapon->Equip(this, true);
			// We no longer have an active overlapping item on the Character
			SetActiveOverlappingItem(nullptr);
		}
	}
	// If not overlapping with an Item, but we've equipped a weapon
	else if (EquippedWeapon) 
	{
		Attack(); // we're attacking if we click the LMB
	}
}

void AMain::LMBUp()
{
	bLMBDown = false;
}

void AMain::ObtainBossKey()
{
	bHasBossKey = true;
}

void AMain::IncrementCoins(int32 Amount)
{
	Coins += Amount;
}

void AMain::IncrementHealth(float Amount)
{
	if (Health < MaxHealth) {
		ShowChangedHealthAmount(Amount);
		PlayEmissiveMaterialEffect(FLinearColor::Green);
		//PlayEmissiveMaterialEffect(FLinearColor(0.135633, 0.219526, 0.904661, 1));

		if (Health + Amount >= MaxHealth)
		{
			Health = MaxHealth;
		}
		else
		{
			Health += Amount;
		}
	}
}

void AMain::DecrementHealth(float Amount)
{
	//Play camera shake when player gets hit, if it's set
	if (CameraShakeClass) MainPlayerController->PlayerCameraManager->StartMatineeCameraShake(CameraShakeClass);
	
	ShowChangedHealthAmount(-1 * Amount);
	PlayEmissiveMaterialEffect(FLinearColor::Red);

	Health -= Amount;
	if (Health <= 0.f) {
		Die();
	}
}

void AMain::Jump()
{
	if (MovementStatus != EMovementStatus::EMS_Dead) 
	{
		ACharacter::Jump(); // jump only if we're not dead
	}
}

void AMain::Die()
{
	// REMOVE THIS: Collision disabled in the end, already prevents this!
	//if (MovementStatus == EMovementStatus::EMS_Dead) return;

	// Get the AnimInstance of our Character
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && CombatMontage) // if these are valid
	{
		//play the Death section from the CombatMontage of AnimInstance
		AnimInstance->Montage_Play(CombatMontage, 1.f);
		AnimInstance->Montage_JumpToSection(FName("Death"), CombatMontage);
	}

	SetMovementStatus(EMovementStatus::EMS_Dead);

	// Set his Capsule Collision Volume to NoCollision when he dies
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UGameInstanceBase* GameInstanceBase = Cast<UGameInstanceBase>(GetGameInstance());
	if (GameInstanceBase)
	{
		GameInstanceBase->OnGameOver();
	}
}

void AMain::DeathEnd()
{
	// Stop all animations
	GetMesh()->bPauseAnims = true;
	//GetMesh()->bNoSkeletonUpdate = true; //REMOVE IT: it creates a glitching bug	
}

void AMain::SetMovementStatus(EMovementStatus Status)
{
	MovementStatus = Status;

	/** Change the Character's max walk speed (running speed) between 
	/* regular running and sprinting based on our MovementStatus state */
	if (MovementStatus == EMovementStatus::EMS_Sprinting) {
		GetCharacterMovement()->MaxWalkSpeed = SprintingSpeed;
	}
	else {
		GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
	}
}

void AMain::ShiftKeyDown()
{
	bShiftKeyDown = true;
}

void AMain::ShiftKeyUp()
{
	bShiftKeyDown = false;
}

void AMain::ShowPickupLocations()
{
	for (FVector PickupLocation : PickupLocations) // range-based for-loop
	{
		UKismetSystemLibrary::DrawDebugSphere(this, PickupLocation, 25.f, 12, FLinearColor::Green, 10.f, 1.f);
	}

	// Alternavelly
	/*for (int32 i = 0; i < PickupLocations.Num(); i++) // index-based iteration
	{
		UKismetSystemLibrary::DrawDebugSphere(this, PickupLocations[i], 25.f, 12, FLinearColor::Green, 10.f, 1.f);
	}*/
}

void AMain::SetEquippedWeapon(AWeapon* WeaponToSet)
{
	// First destroy any other already equipped weapon if exists
	if (EquippedWeapon) {
		EquippedWeapon->Destroy();
	}

	EquippedWeapon = WeaponToSet;
}

/** It's called when we click the LMB, and we're not overlapping with an Item, 
/* but we have a weapon equipped. Also when we keep clicking LMB in AttackEnd */
void AMain::Attack()
{
	/** if we're not attacking and not dead, then we start the attack animation, 
	/* in order to prevent re-attacking when already playing attack anim */
	if (!bAttacking && MovementStatus != EMovementStatus::EMS_Dead)
	{
		bAttacking = true;
		// Start interpolating towards the Enemy
		SetInterpToEnemy(true);

		// Get the AnimInstance of our Character
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && CombatMontage) // if these are valid
		{
			// Randomize the Attack Sequence between Attack_1 & Attack_2
			int32 Selection = FMath::RandRange(0, 1);
			switch (Selection)
			{
			case 0:
				//play the Attack_1 section from the CombatMontage of AnimInstance
				AnimInstance->Montage_Play(CombatMontage, 2.2f);
				AnimInstance->Montage_JumpToSection(FName("Attack_1"), CombatMontage);
				break;

			case 1:
				//play the Attack_2 section from the CombatMontage of AnimInstance
				AnimInstance->Montage_Play(CombatMontage, 1.8f);
				AnimInstance->Montage_JumpToSection(FName("Attack_2"), CombatMontage);
				break;

			default:
				;
			}
		}
	}
}

void AMain::AttackEnd()
{	
	bAttacking = false;
	// We no longer need to interpolating towards the Enemy
	SetInterpToEnemy(false);

	// Keep attacking if LMB is pressed near the End of Attacking
	if (bLMBDown)
	{
		Attack();
	}
}

void AMain::PlaySwingSound()
{
	if (EquippedWeapon->SwingSound) //if there's a SwingSound for Weapon
	{
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->SwingSound);
	}
}

void AMain::SetInterpToEnemy(bool Interp)
{
	bInterpToEnemy = Interp;
}

float AMain::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	DecrementHealth(DamageAmount);
	return DamageAmount;
}

void AMain::UpdateCombatTarget()
{
	UE_LOG(LogTemp, Warning, TEXT("AMain::UpdateCombatTarget()"));

	// Empty array that will be filled with all Enemy Actors overlapping Main
	TArray<AActor*> OverlappingActors;

	// Get all Enemy (or derived from Enemy) Actors who overlap with Main
	GetOverlappingActors(OverlappingActors, EnemyFilter);

	// Alternatevily we can use that for hardcoding Enemy Class
	// GetOverlappingActors(OverlappingActors, AEnemy::StaticClass());

	// if we're not overlapping with any Enemy Actors: 
	if (OverlappingActors.Num() == 0) {
		
		// Remove the EnemyHealthBar from the viewport
		if (MainPlayerController)
		{
			MainPlayerController->HideEnemyHealthBar();
		}
		// and exit this function
		return;
	}

	// Initialize ClosestEnemy to the first Enemy of the Array  
	AEnemy* ClosestEnemy = Cast<AEnemy>(OverlappingActors[0]);
	if (ClosestEnemy)
	{
		// Get Main's Location
		FVector Location = GetActorLocation();

		// Get Distance/Magnitude between ClosestEnemy and Main
		float MinDistance = (ClosestEnemy->GetActorLocation() - Location).Size();

		// Loop through the array to see if there are others that are closer
		for (auto Actor : OverlappingActors)
		{
			// Cast every Actor to Enemy
			AEnemy* Enemy = Cast<AEnemy>(Actor);
			if (Enemy)
			{
				// Get Distance/Magnitude between ClosestEnemy and Main
				float DistanceToActor = (Enemy->GetActorLocation() - Location).Size();

				// if this current Enemy is closest that the previous, update it
				if (DistanceToActor < MinDistance)
				{
					MinDistance = DistanceToActor;
					ClosestEnemy = Enemy;
				}
			}
			
		}

		// At the end of the loop, we'll have found the Closest Enemy
		SetCombatTarget(ClosestEnemy);
		if (MainPlayerController)
		{
			// Display the Enemy's Health Bar
			MainPlayerController->DisplayEnemyHealthBar();
		}
	}
}

void AMain::SwitchLevel(FName LevelName)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FName CurrentMapName(*UGameplayStatics::GetCurrentLevelName(GetWorld())); //convert FString into FName

		// Check if we're not trying to transition to the same level
		if (CurrentMapName != LevelName)
		{
			// SaveGame in order to keep new Weapons, Coins, etc. after passing through the Transition Volume
			SaveGame(LevelName.ToString()); 

			// Transition to a level/map called the given LevelName
			UGameplayStatics::OpenLevel(World, LevelName);
		}
	}
}

void AMain::SaveGame(FString LevelName)
{
	UE_LOG(LogTemp, Warning, TEXT("AMain::SaveGame()"));

	/** Store what CreateSaveGameObject returns in a SaveGameInstance variable.
	/* CreateSaveGameObject returns a pointer instance of given USaveGameBase,  
	/* so casting it to USaveGameBase pointer will be succeded */
	USaveGameBase* SaveGameInstance = Cast<USaveGameBase>(UGameplayStatics::CreateSaveGameObject(USaveGameBase::StaticClass()));
	
	// Set the CharacterStats struct's properties to Character's current ones
	SaveGameInstance->CharacterStats.Health = Health;
	SaveGameInstance->CharacterStats.MaxHealth = MaxHealth;
	SaveGameInstance->CharacterStats.Stamina = Stamina;
	SaveGameInstance->CharacterStats.MaxStamina = MaxStamina;
	SaveGameInstance->CharacterStats.Coins = Coins;
	SaveGameInstance->CharacterStats.bHasBossKey = bHasBossKey;
	SaveGameInstance->CharacterStats.PickupArray = PickupArray;

	/*GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("Just Saved the following Pickups:"));
	for (FName Pickup : PickupArray)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("%s"), *Pickup.ToString()));
	}*/
	
	
	if (!LevelName.IsEmpty()) // if we've come through a Level Transition Volume
	{
		SaveGameInstance->CharacterStats.LevelName = LevelName;
		SaveGameInstance->CharacterStats.Location = FVector(0);
		SaveGameInstance->CharacterStats.Rotation = FRotator(0);
	}
	else 
	{
		// Save the current Level's Name and Actor's current Location & Rotation
		SaveGameInstance->CharacterStats.LevelName = UGameplayStatics::GetCurrentLevelName(GetWorld());
		SaveGameInstance->CharacterStats.Location = GetActorLocation();
		SaveGameInstance->CharacterStats.Rotation = GetActorRotation();
	}

	if (EquippedWeapon)
	{
		SaveGameInstance->CharacterStats.WeaponName = EquippedWeapon->Name;
	}

	// Saves the Game associated with SlotName to a Save Slot Index 0 
	UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->SlotName, SaveGameInstance->UserIndex);

}

void AMain::LoadGame()
{
	/** Create an instance of the USaveGameBase class from CreateSaveGameObject */
	USaveGameBase* LoadGameInstance = Cast<USaveGameBase>(UGameplayStatics::CreateSaveGameObject(USaveGameBase::StaticClass()));

	/** Loads the Game associated with SlotName and Slot Index 0, 
	/* casting it to a USaveGameBase and saving it into the LoadGameInstance */
	LoadGameInstance = Cast<USaveGameBase>(UGameplayStatics::LoadGameFromSlot(LoadGameInstance->SlotName, LoadGameInstance->UserIndex));

	// If Game isn't Saved once, there's not a Game to Load From Slot, so exit!
	if (!LoadGameInstance) return;

	// Set the Character's properties to the LoadGame's CharacterStats ones
	Health = LoadGameInstance->CharacterStats.Health;
	MaxHealth = LoadGameInstance->CharacterStats.MaxHealth;
	Stamina = LoadGameInstance->CharacterStats.Stamina;
	MaxStamina = LoadGameInstance->CharacterStats.MaxStamina;
	Coins = LoadGameInstance->CharacterStats.Coins;
	bHasBossKey = LoadGameInstance->CharacterStats.bHasBossKey;
	PickupArray = LoadGameInstance->CharacterStats.PickupArray;
	
	/*GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, TEXT("Just Loaded the following Pickups:"));
	for (FName Pickup : PickupArray)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("%s"), *Pickup.ToString()));
	}*/


	if (WeaponStorage) // if it has value in BP
	{
		// Create an instance of ItemStorage by Spawning WeaponStorage Actor
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
		if (Weapons) // if it's valid
		{
			FString WeaponName = LoadGameInstance->CharacterStats.WeaponName;
			UE_LOG(LogTemp, Warning, TEXT("WeaponName: %s"), *FString(WeaponName));
			if (!WeaponName.IsEmpty() && Weapons->WeaponMap.Contains(WeaponName))
			{
				// Spawn Weapon Actor of specific type we retrieved by WeaponName key
				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponName]);

				// Equip that newly created spawned Weapon
				WeaponToEquip->Equip(this, false); // Weapon will be Attached to Main
			}
			else
			{
				// He didn't have a weapon when we saved game, so he must drop it
				DropWeapon();
			}
		}
	}
	
	// if SavedLevelName equals CurrentLevelName and Saved Location & Rotation is not 0, then set also Character's 
	// Location & Rotation when we're loading the Saved Level Map Name.
	// We don't want to set Location & Rotation i fwe've come through a LevelTransitionVolume
	FString SavedLevelName = LoadGameInstance->CharacterStats.LevelName;
	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(GetWorld());

	if (!SavedLevelName.IsEmpty() && SavedLevelName.Equals(CurrentLevelName) && 
		!LoadGameInstance->CharacterStats.Location.Equals(FVector(0)) &&
		!LoadGameInstance->CharacterStats.Rotation.Equals(FRotator(0))) 
	{
		SetActorLocation(LoadGameInstance->CharacterStats.Location);
		SetActorRotation(LoadGameInstance->CharacterStats.Rotation);
	}

	// If after save we die, we don't want to be Dead when we load the game again
	SetMovementStatus(EMovementStatus::EMS_Normal);
	GetMesh()->bPauseAnims = false;
	// Reset his Capsule Collision Presets to Pawn
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
}

bool AMain::SaveGameExists()
{
	/** Create an instance of the USaveGameBase class from CreateSaveGameObject */
	USaveGameBase* LoadGameInstance = Cast<USaveGameBase>(UGameplayStatics::CreateSaveGameObject(USaveGameBase::StaticClass()));

	/** Returns true if Game associated with SlotName and Slot Index 0 already exists, or false otherwise */
	return UGameplayStatics::DoesSaveGameExist(LoadGameInstance->SlotName, LoadGameInstance->UserIndex);
}

/* The only bad with that is that when he appears in PlayerStart location, 
it stil has the falling animation if falling, or the running animation if running */
void AMain::RespawnAtPlayerStart()
{
	TArray<AActor*> PlayerStartActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStartActors);

	if (PlayerStartActors.Num() > 0)
	{
		APlayerStart* PlayerStart = Cast<APlayerStart>(PlayerStartActors[0]);
		SetActorLocation(PlayerStart->GetActorLocation());
	}
}

void AMain::DropWeapon()
{
	SetEquippedWeapon(nullptr);
}

// Animates Emissive Material pulse for 1 sec
void AMain::PlayEmissiveMaterialEffect(FLinearColor InLinearColor)
{
	FVector Color = UKismetMathLibrary::Conv_LinearColorToVector(InLinearColor);
	GetMesh()->SetVectorParameterValueOnMaterials(TEXT("EffectColor"), Color);
	GetMesh()->SetScalarParameterValueOnMaterials(TEXT("StartTime"), GetWorld()->GetTimeSeconds());
}

void AMain::DisplayHUDMainText(FString DisplayText, bool bIsTextInformative)
{
	HUDDisplayText = DisplayText;
	bHUDIsTextInformative = bIsTextInformative;
	float DisplayTime = bIsTextInformative ? 5.f : 3.f;

	// Set the HideHUDTextTimer to Hide the text after waiting for 0.3 sec
	GetWorldTimerManager().SetTimer(HideHUDTextTimer, this, &AMain::HideHUDInformativeText, DisplayTime);
}

void AMain::HideHUDInformativeText()
{
	HUDDisplayText = FString("");
	bHUDIsTextInformative = true;
	GetWorldTimerManager().ClearTimer(HideHUDTextTimer);
}

void AMain::CharacterDestroyed(AActor* DestroyedActor)
{
	//Get a reference to the Pawn Controller.
	AController* CortollerRef = MainPlayerController;

	//Get the World and GameMode in the world to invoke its restart player function.
	if (UWorld* World = GetWorld())
	{
		//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("Just Destroyed!"));
		World->GetAuthGameMode()->RestartPlayer(CortollerRef);
	}
}

