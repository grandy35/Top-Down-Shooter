// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Types.generated.h"

UENUM(BlueprintType)
enum class EMovementState : uint8
{
	Aim UMETA(DisplayName = "Aim State"),
	AimWalk UMETA(DisplayName = "AimWalk State"),
	Walk UMETA(DisplayName = "Walk State"),
	Run UMETA(DisplayName = "Run State"),
	SprintRun UMETA(DisplayName = "SprintRun State"),
};

USTRUCT(BlueprintType)
struct FCharacterSpeed
{
	GENERATED_BODY()
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Movement")
		float AimSpeed = 300.0f;
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Movement")
		float AimSpeedWalk = 100.0f;
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Movement")
		float WalkSpeed = 200.0f;
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Movement")
		float RunSpeed = 600.0f;
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Movement")
		float SprintRunSpeedRun = 800.0f;
	
};

UCLASS()
class TOPDOWNSHOOTER_API UTypes : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
};