// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HelperFunctionLibrary.generated.h"

/**
 * Static class with useful gameplay utility functions that can be called from both Blueprint and C++
 */
UCLASS()
class MYGAME_API UHelperFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Saving", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static void AddItemToPickupArrayAndDestroy(const UObject* WorldContextObject, AActor* Item, bool bDestroy = true);
	
	/* Returns true if Item's name (and other details) is contained in saved PickupsArray */
	UFUNCTION(BlueprintCallable, Category = "Saving", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static bool HasItemBeenSavedToPickupArray(const UObject* WorldContextObject, const AActor* Item);

private:

	UFUNCTION(Category = "Saving", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static FString CreateSaveNameFromItem(const UObject* WorldContextObject, const AActor* Item);

};
