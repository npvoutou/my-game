// Fill out your copyright notice in the Description page of Project Settings.


#include "HelperFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Enemy.h"
#include "Main.h"


void UHelperFunctionLibrary::AddItemToPickupArrayAndDestroy(const UObject* WorldContextObject, AActor* Item, bool bDestroy)
{
	//if (Item->GetOwner()) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White, FString::Printf(TEXT("Item has owner: %s"), *Item->GetOwner()->GetName()));
	//if (Item->GetOwner() && Item->GetOwner()->GetOwner()) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White, FString::Printf(TEXT("Item has owner->owner: %s"), *Item->GetOwner()->GetOwner()->GetName()));

	// if this Item has been dropped by an Enemy, there's no reason to be added to PickupArray
	bool bDroppedByEnemy = Item->GetOwner() != nullptr && Item->GetOwner()->GetOwner() != nullptr && Cast<AEnemy>(Item->GetOwner()->GetOwner());
	if (!bDroppedByEnemy)
	{
		// Everytime we pickup/destroy a pickable item, its details will be added to PickupArray that is saved in order not to be available again for pickup!
		AActor* PickupItem = Item->GetOwner() != nullptr ? Item->GetOwner() : Item; // wheather its a Pickup within a SpawnVolume or a standalone Pickup!
		
		// Add Item to Pickup Array
		FString ItemToBeSaved = CreateSaveNameFromItem(WorldContextObject, PickupItem);
		AMain* Main = Cast<AMain>(UGameplayStatics::GetPlayerPawn(WorldContextObject, 0));
		if (Main)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::Printf(TEXT("You just picked up: %s (from Item: %s)"), *ItemToBeSaved, *Item->GetName()));
			Main->PickupArray.AddUnique(FName(ItemToBeSaved));
		}
	}
	else
	{
		//if (Item->GetOwner() && Item->GetOwner()->GetOwner()) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, FString::Printf(TEXT("Item %s has been dropped by ENEMY %s!!!!"), *Item->GetName(), *Item->GetOwner()->GetOwner()->GetName()));
	}

	if (bDestroy)
	{
		if (Item->GetOwner() != nullptr) // if it has an owner (e.g. SpawnVolume), destroy it
		{
			//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Orange, FString::Printf(TEXT("Item has also an owner to be destroyed: %s"), *Item->GetOwner()->GetName()));
			Item->GetOwner()->Destroy();
		}

		// Destroy the Item actor itself
		Item->Destroy();
	}
}

bool UHelperFunctionLibrary::HasItemBeenSavedToPickupArray(const UObject* WorldContextObject, const AActor* Item)
{
	if (Item)
	{
		FString ItemToSearch = CreateSaveNameFromItem(WorldContextObject, Item);

		AMain* Main = Cast<AMain>(UGameplayStatics::GetPlayerPawn(WorldContextObject, 0));
		if (Main)
		{
			bool bHasBeenPickedUp = Main->PickupArray.Contains(FName(ItemToSearch));
			//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Magenta, FString::Printf(TEXT("Object: %s has %s picked up."), *ItemToSearch, bHasBeenPickedUp ? TEXT("been") : TEXT("NOT been")));
			return bHasBeenPickedUp;
		}
	}

	return false;
}

FString UHelperFunctionLibrary::CreateSaveNameFromItem(const UObject* WorldContextObject, const AActor* Item)
{
	FString Level = UGameplayStatics::GetCurrentLevelName(WorldContextObject);
	FString ObjectName = UKismetSystemLibrary::GetObjectName(Item);
	//ex. "PickupsVolume_BP_C" from UKismetSystemLibrary::GetDisplayName(UGameplayStatics::GetObjectClass(Item));
	//ex. "Pickups Volume BP" from UGameplayStatics::GetObjectClass(Item)->GetDisplayNameText().ToString(); // but produced error when packaging!
	FString ObjectClass = UKismetSystemLibrary::GetDisplayName(UGameplayStatics::GetObjectClass(Item)); // ->GetDisplayNameText().ToString();
	return FString::Printf(TEXT("%s|%s|%s"), *Level, *ObjectClass, *ObjectName);
}