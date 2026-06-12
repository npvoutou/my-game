// Fill out your copyright notice in the Description page of Project Settings.


#include "MainAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Main.h"

void UMainAnimInstance::NativeInitializeAnimation() 
{
	if (Pawn == nullptr) {
		//Sets the owner of this AnimInstance, if it has one, to Pawn
		Pawn = TryGetPawnOwner();

		// If Pawn is valid, cast it to a class of AMain and set it to Main
		if (Pawn) {
			Main = Cast<AMain>(Pawn);
		}
	}
}

void UMainAnimInstance::UpdateAnimationProperties()
{
	if (Pawn == nullptr) { //first time goes here
		//Sets the owner of this AnimInstance, if it has one, to Pawn
		Pawn = TryGetPawnOwner();
	}

	if (Pawn) { //all the other times goes here, Pawn is instantiated
		// how fast the Pawn is going at that particular time/frame
		FVector Speed = Pawn->GetVelocity();

		//We don't want to update the animation if the Pawn's velocity is in any 
		//direction other than the horizontal direction. If he's in the air, he's 
		//going to be playing the falling and or the flying or jumping	animation. 
		//But if he's on the ground, we only care about the velocity in the 
		//horizontal direction.
		FVector LateralSpeed = FVector(Speed.X, Speed.Y, 0.f);
		// Updates movement speed with the lateral speed's magnitude
		MovementSpeed = LateralSpeed.Size();

		// Updates bIsInAir with IsFalling. This is true if Pawn's currently falling
		bIsInAir = Pawn->GetMovementComponent()->IsFalling();
		
		// If Main is null, cast Pawn to a class of AMain and set it to Main
		if (Main == nullptr) {
			Main = Cast<AMain>(Pawn);
		}
	}
}
