// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Types.h"
#include "TopDownShooterCharacter.generated.h"

UCLASS(Blueprintable)
class ATopDownShooterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATopDownShooterCharacter();

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns CursorToWorld subobject **/
	FORCEINLINE class UDecalComponent* GetCursorToWorld() { return CursorToWorld; }

private:
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** A decal that projects to the cursor location. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UDecalComponent* CursorToWorld;

public:
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Movement")
		EMovementState MovementState = EMovementState::Run_State;
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Movement")
		FCharacterSpeed MovementSpeedInfo;
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Movement")
		bool SprintRunEnabled = false;
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Movement")
		bool WalkEnabled = false;
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Movement")
		bool AimEnabled = false;

	UFUNCTION()
		void InputAxisX(float Value);
	UFUNCTION()
		void InputAxisY(float Value);

	float AxisX = 0.0f;
	float AxisY = 0.0f;

	UFUNCTION()
		void MovementTick(float DeltaTime);

	UFUNCTION(BlueprintCallable)
		void CharacterUpdate();
	UFUNCTION(BlueprintCallable)
		void ChangeMovementState();
};

