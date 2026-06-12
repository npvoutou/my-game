// Fill out your copyright notice in the Description page of Project Settings.


#include "MainPlayerController.h"
#include "Blueprint/UserWidget.h"

void AMainPlayerController::BeginPlay()
{
	// Must call the Super when overriding BeginPlay
	Super::BeginPlay();

	//Check if in blueprints we've selected the HUD Overlay Asset Class
	if (HUDOverlayClass) 
	{
		/** Constructs HUDOverlay with a template constructor from the owning 
		/* controller and sets UUserWidget to what we chose in the BP dropdown */
		HUDOverlay = CreateWidget<UUserWidget>(this, HUDOverlayClass);
		HUDOverlay->AddToViewport();
		HUDOverlay->SetVisibility(ESlateVisibility::Visible);
	}

	//Check if in blueprints we've selected the EnemyHealthBar Class
	if (WEnemyHealthBarClass) 
	{
		/** Constructs EnemyHealthBar with a template constructor from the owning
		/* controller and sets UUserWidget to what we chose in the BP dropdown */
		WEnemyHealthBar = CreateWidget<UUserWidget>(this, WEnemyHealthBarClass);
		if (WEnemyHealthBar)
		{
			WEnemyHealthBar->AddToViewport();
			// Hide it because it won't be shown at all times
			WEnemyHealthBar->SetVisibility(ESlateVisibility::Hidden);
			// Set its alignment to be ordinarily flattened facing the screen
			WEnemyHealthBar->SetAlignmentInViewport(FVector2D(0.f));
		}
	}	
}

void AMainPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (WEnemyHealthBar)
	{
		/** For the location of WEnemyHealthBar in the viewport. Actually
		/*  it's the 2D position that corresponds to the Enemy's 3D Location */
		FVector2D PositionInViewport;

		// Convert Enemy's 3D world location into a 2D location in the screen
		ProjectWorldLocationToScreen(EnemyLocation, PositionInViewport);

		// Subtract from Y-Axis in order to be above Enemy's center location
		PositionInViewport.Y -= 85.f;

		// For the size of WEnemyHealthBar in the viewport
		FVector2D SizeInViewport = FVector2D(300.f, 25.f);

		// Set position and size for the WEnemyHealthBar widget
		WEnemyHealthBar->SetPositionInViewport(PositionInViewport);
		WEnemyHealthBar->SetDesiredSizeInViewport(SizeInViewport);
	}
}

void AMainPlayerController::DisplayEnemyHealthBar()
{
	if (WEnemyHealthBar) // if is valid, show it on screen
	{
		bEnemyHealthBarVisible = true;
		WEnemyHealthBar->SetVisibility(ESlateVisibility::Visible);
	}
}

void AMainPlayerController::HideEnemyHealthBar()
{
	if (WEnemyHealthBar) // if is valid, hide it from screen
	{
		bEnemyHealthBarVisible = false;
		WEnemyHealthBar->SetVisibility(ESlateVisibility::Hidden);
	}
}
