// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameInstanceBase.generated.h"

/**
 * Base class for GameInstance, should be blueprinted
 * Most games will need to make a game-specific subclass of GameInstance
 * Once you make a blueprint subclass of your native subclass you will want to set it to be the default in project settings
 */
UCLASS()
class MYGAME_API UGameInstanceBase : public UGameInstance
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = Save)
	void DeleteSaveGame();

	UFUNCTION(BlueprintCallable, Category = Save)
	FName GetLevelToOpen();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Game, meta = (DisplayName = "OnGameOver", ScriptName = "OnGameOver"))
	void OnGameOver();

};
