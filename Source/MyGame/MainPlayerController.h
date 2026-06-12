// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API AMainPlayerController : public APlayerController
{
	GENERATED_BODY()

public: 

	/** Reference to the UMG asset class in the editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets")
	TSubclassOf<class UUserWidget> HUDOverlayClass;

	/** Variable to hold the widget after creating it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets")
	UUserWidget* HUDOverlay;

	/** Reference to the EnemyHealthBar asset class in the editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets")
	TSubclassOf<class UUserWidget> WEnemyHealthBarClass;

	/** Variable to hold the EnemyHealthBar widget after creating it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets")
	UUserWidget* WEnemyHealthBar;

	bool bEnemyHealthBarVisible;

	void DisplayEnemyHealthBar();
	void HideEnemyHealthBar();

	FVector EnemyLocation;

protected:

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
	
};
