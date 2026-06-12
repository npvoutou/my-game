// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Enemy.h"

void UEnemyAnimInstance::NativeInitializeAnimation()
{
	if (Pawn == nullptr) {
		//Sets the owner of this AnimInstance, if it has one, to Pawn
		Pawn = TryGetPawnOwner();

		// If Pawn is valid, cast it to a class of AEnemy and set it to Enemy
		if (Pawn) {
			Enemy = Cast<AEnemy>(Pawn);
		}
	}
}

void UEnemyAnimInstance::UpdateAnimationProperties()
{
	if (Pawn == nullptr) { //first time goes here
		//Sets the owner of this AnimInstance, if it has one, to Pawn
		Pawn = TryGetPawnOwner();
	}

	if (Pawn) { //all the other times goes here, Pawn is instantiated
		// how fast the Pawn is going at that particular time/frame
		FVector Speed = Pawn->GetVelocity();

		//We don't want to update the animation if the Pawn's velocity is in any 
		//direction other than the horizontal direction. So, if it's on the ground
		//we only care about the velocity in the horizontal direction.
		FVector LateralSpeed = FVector(Speed.X, Speed.Y, 0.f);
		// Updates movement speed with the lateral speed's magnitude
		MovementSpeed = LateralSpeed.Size();

		// If Enemy is null, cast Pawn to a class of AEnemy and set it to Enemy
		if (Enemy == nullptr) {
			Enemy = Cast<AEnemy>(Pawn);
		}
	}
}

