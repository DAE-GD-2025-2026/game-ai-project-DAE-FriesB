// Fill out your copyright notice in the Description page of Project Settings.

#include "SteeringAgent.h"

#include "AIController.h"


// Sets default values
ASteeringAgent::ASteeringAgent()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ASteeringAgent::BeginPlay()
{
	Super::BeginPlay();
}

void ASteeringAgent::BeginDestroy()
{
	Super::BeginDestroy();
}

// Called every frame
void ASteeringAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (SteeringBehavior)
	{
		SteeringOutput output = SteeringBehavior->CalculateSteering(DeltaTime, *this);

		AddMovementInput(FVector{output.LinearVelocity, 0.f});


		if (!IsAutoOrienting())
		{
			if (AController* aiController = GetController())
			{
				float const deltaYaw = FMath::Clamp(output.AngularVelocity, -1.f, 1.f) * GetMaxAngularSpeed()*DeltaTime;
				FRotator currentRotation{GetActorForwardVector().ToOrientationRotator()};
				FRotator deltaRot = FRotator(0.0f, deltaYaw , 0.0f);
				FRotator targetRot =  currentRotation + deltaRot;
				
				if (!FMath::IsNearlyEqual(currentRotation.Yaw, targetRot.Yaw))
				{
					aiController->SetControlRotation(targetRot);
					FaceRotation(targetRot);
				}
				
			}
		}
	}
}

// Called to bind functionality to input
void ASteeringAgent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ASteeringAgent::SetSteeringBehavior(ISteeringBehavior* NewSteeringBehavior)
{
	SteeringBehavior = NewSteeringBehavior;
}
