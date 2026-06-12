// Fill out your copyright notice in the Description page of Project Settings.


#include "GameInstanceBase.h"
#include "SaveGameBase.h"
#include "Kismet/GameplayStatics.h"

FName UGameInstanceBase::GetLevelToOpen()
{
	UE_LOG(LogTemp, Warning, TEXT("UGameInstanceBase::GetLevelToOpen()"));

	/** Create an instance of the USaveGameBase class from CreateSaveGameObject */
	USaveGameBase* LoadGameInstance = Cast<USaveGameBase>(UGameplayStatics::CreateSaveGameObject(USaveGameBase::StaticClass()));

	/** Loads the Game associated with SlotName and Slot Index 0,
	/* casting it to a USaveGameBase and saving it into the LoadGameInstance */
	LoadGameInstance = Cast<USaveGameBase>(UGameplayStatics::LoadGameFromSlot(LoadGameInstance->SlotName, LoadGameInstance->UserIndex));

	if (LoadGameInstance && !LoadGameInstance->CharacterStats.LevelName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Exoume kanei Save sto Level: %s"), *LoadGameInstance->CharacterStats.LevelName);
		return *LoadGameInstance->CharacterStats.LevelName;
	}
	return FName("Dungeon"); //defaultMap
}

void UGameInstanceBase::DeleteSaveGame()
{
	UE_LOG(LogTemp, Warning, TEXT("UGameInstanceBase::DeleteSaveGame()"));

	/** Create an instance of the USaveGameBase class from CreateSaveGameObject */
	USaveGameBase* DeleteGameInstance = Cast<USaveGameBase>(UGameplayStatics::CreateSaveGameObject(USaveGameBase::StaticClass()));

	// Deletes the Game associated with SlotName to a Save Slot Index 0 
	UGameplayStatics::DeleteGameInSlot(DeleteGameInstance->SlotName, DeleteGameInstance->UserIndex);
}