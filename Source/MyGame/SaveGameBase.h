// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SaveGameBase.generated.h"

USTRUCT(BlueprintType)
struct FCharacterStats
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
	float Health;

	UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
	float MaxHealth;

	UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
	float Stamina;

	UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
	float MaxStamina;

	UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
	int32 Coins;

	UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
	bool bHasBossKey;

	UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
	FVector Location;

	UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
	FRotator Rotation;

	UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
	FString WeaponName;

	UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
	FString LevelName;


	UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
	TArray<FName> PickupArray;
};

/** Object that is written to and read from the save game archive, with a data version */
UCLASS(BlueprintType)
class MYGAME_API USaveGameBase : public USaveGame
{
	GENERATED_BODY()
	
public:
	/** Constructor */
	USaveGameBase()
	{
		SlotName = TEXT("Default");
		UserIndex = 0;

		CharacterStats.WeaponName = TEXT("");
		CharacterStats.LevelName = TEXT("");
	}

	UPROPERTY(VisibleAnywhere, Category = "Basic")
	FString SlotName;

	UPROPERTY(VisibleAnywhere, Category = "Basic")
	uint32 UserIndex;

	UPROPERTY(VisibleAnywhere, Category = "Basic")
	FCharacterStats CharacterStats;
};
