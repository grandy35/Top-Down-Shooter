// Copyright Epic Games, Inc. All Rights Reserved.

#include "TopDownShooterCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Materials/Material.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "TDSGameInstance.h"
#include "Weapon.h"

ATopDownShooterCharacter::ATopDownShooterCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create a decal in the world to show the cursor's location
	CursorToWorld = CreateDefaultSubobject<UDecalComponent>("CursorToWorld");
	CursorToWorld->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UMaterial> DecalMaterialAsset(TEXT("Material'/Game/TopDownCPP/Blueprints/M_Cursor_Decal.M_Cursor_Decal'"));
	if (DecalMaterialAsset.Succeeded())
	{
		CursorToWorld->SetDecalMaterial(DecalMaterialAsset.Object);
	}
	CursorToWorld->DecalSize = FVector(16.0f, 32.0f, 32.0f);
	CursorToWorld->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f).Quaternion());

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ATopDownShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitWeapon(InitWeaponName);
}

void ATopDownShooterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CursorToWorld != nullptr)
	{
		if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
		{
			if (UWorld* World = GetWorld())
			{
				FHitResult HitResult;
				FCollisionQueryParams Params(NAME_None, FCollisionQueryParams::GetUnknownStatId());
				FVector StartLocation = TopDownCameraComponent->GetComponentLocation();
				FVector EndLocation = TopDownCameraComponent->GetComponentRotation().Vector() * 2000.0f;
				Params.AddIgnoredActor(this);
				World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params);
				FQuat SurfaceRotation = HitResult.ImpactNormal.ToOrientationRotator().Quaternion();
				CursorToWorld->SetWorldLocationAndRotation(HitResult.Location, SurfaceRotation);
			}
		}
		else if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FHitResult TraceHitResult;
			PC->GetHitResultUnderCursor(ECC_Visibility, true, TraceHitResult);
			FVector CursorFV = TraceHitResult.ImpactNormal;
			FRotator CursorR = CursorFV.Rotation();
			CursorToWorld->SetWorldLocation(TraceHitResult.Location);
			CursorToWorld->SetWorldRotation(CursorR);
		}
	}

	MovementTick(DeltaSeconds);

	CompareVectors(this);

	if (MovementState == EMovementState::SprintRun)
	{
		DecreaseStamina(DeltaSeconds);
	}

	if (!SprintRunEnabled)
	{
		IncreaseStamina(DeltaSeconds);
	}
}

void ATopDownShooterCharacter::SetupPlayerInputComponent(UInputComponent* NewInputComponent)
{
	Super::SetupPlayerInputComponent(NewInputComponent);

	NewInputComponent->BindAxis(TEXT("MoveForward"), this, &ATopDownShooterCharacter::InputAxisX);
	NewInputComponent->BindAxis(TEXT("MoveRight"), this, &ATopDownShooterCharacter::InputAxisY);
}

void ATopDownShooterCharacter::InputAxisX(float Value)
{
	AxisX = Value;
}

void ATopDownShooterCharacter::InputAxisY(float Value)
{
	AxisY = Value;
}

void ATopDownShooterCharacter::MovementTick(float DeltaTime)
{
	AddMovementInput(FVector(1.0f, 0.0f, 0.0f), AxisX);
	AddMovementInput(FVector(0.0f, 1.0f, 0.0f), AxisY);

	APlayerController* myController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (myController)
	{
		FHitResult ResultHit;
		myController->GetHitResultUnderCursorByChannel(ETraceTypeQuery::TraceTypeQuery6, false, ResultHit);

		float FindRotatorResultYaw = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), ResultHit.Location).Yaw;
		SetActorRotation(FQuat(FRotator(0.0f, FindRotatorResultYaw, 0.0f)));
	}
}

void ATopDownShooterCharacter::AttackCharEvent(bool bIsFiring)
{
	AWeapon* myWeapon = nullptr;
	myWeapon = GetCurrentWeapon();
	if (myWeapon)
	{
		//ToDo Check melee or range
		myWeapon->SetWeaponStateFire(bIsFiring);
	}
	else
		UE_LOG(LogTemp, Warning, TEXT("ATPSCharacter::AttackCharEvent - CurrentWeapon -NULL"));
}

void ATopDownShooterCharacter::CharacterUpdate()
{
	float ResSpeed = 600.0f;
	switch (MovementState)
	{
	case EMovementState::Aim:
		ResSpeed = MovementSpeedInfo.AimSpeed;
		break;
	case EMovementState::AimWalk:
		ResSpeed = MovementSpeedInfo.AimSpeedWalk;
		break;
	case EMovementState::Run:
		ResSpeed = MovementSpeedInfo.RunSpeed;
		break;
	case EMovementState::Walk:
		ResSpeed = MovementSpeedInfo.WalkSpeed;
		break;
	case EMovementState::SprintRun:
		ResSpeed = MovementSpeedInfo.SprintRunSpeedRun;
		break;
	default:
		break;
	}

	GetCharacterMovement()->MaxWalkSpeed = ResSpeed;
}

void ATopDownShooterCharacter::ChangeMovementState()
{
	if (!WalkEnabled && !SprintRunEnabled && !AimEnabled)
	{
		MovementState = EMovementState::Run;
	}
	else
	{
		if (SprintRunEnabled)
		{
			WalkEnabled = false;
			AimEnabled = false;
			MovementState = EMovementState::SprintRun;
		}
		if (WalkEnabled && !SprintRunEnabled && AimEnabled)
		{
			MovementState = EMovementState::AimWalk;
		}
		else
		{
			if (WalkEnabled && !SprintRunEnabled && !AimEnabled)
			{
				MovementState = EMovementState::Walk;
			}
			else
			{
				if (!WalkEnabled && !SprintRunEnabled && AimEnabled)
				{
					MovementState = EMovementState::Aim;
				}
			}
		}
	}
	CharacterUpdate();

	AWeapon* MyWeapon = GetCurrentWeapon();
	if (MyWeapon)
	{
		MyWeapon->UpdateStateWeapon(MovementState);
	}
}

void ATopDownShooterCharacter::DecreaseStamina(float DeltaSeconds)
{
	FullStamina += -ChangeStamina * DeltaSeconds;

	if (FullStamina <= 0.0f)
	{
		FullStamina = 0.0f;
		if (MovementState == EMovementState::SprintRun)
		{
			StopSprint();
		}
	}
}

void ATopDownShooterCharacter::IncreaseStamina(float DeltaSeconds)
{
	FullStamina += ChangeStamina * DeltaSeconds;

	if (FullStamina > 100.0f)
	{
		FullStamina = 100.0f;
	}
}

void ATopDownShooterCharacter::StopSprint()
{
	SprintRunEnabled = false;
	ChangeMovementState();
}

void ATopDownShooterCharacter::CompareVectors(AActor* Actor)
{
	if (!Actor) return;

	FVector ForwardVector = Actor->GetActorForwardVector();
	FVector VelocityVector = Actor->GetVelocity();

	ForwardVector.Normalize();
	VelocityVector.Normalize();

	float DotProduct = FVector::DotProduct(ForwardVector, VelocityVector);
	if (DotProduct > 0.7f && GetWorld()->GetFirstPlayerController()->IsInputKeyDown(EKeys::LeftShift))
	{
		SprintRunEnabled = true;
		ChangeMovementState();
	}
	else
	{
		StopSprint();
	}
}

AWeapon* ATopDownShooterCharacter::GetCurrentWeapon()
{
	return CurrentWeapon;
}

void ATopDownShooterCharacter::InitWeapon(FName IdWeapon)
{
	UTDSGameInstance* MyGI = Cast<UTDSGameInstance>(GetGameInstance());

	if (!MyGI)
	{
		//UE_LOG(LogTemp, Error, TEXT("GameInstance is NULL!"));
		return;
	}

	FWeaponInfo MyWeaponInfo;

	if (!MyGI->GetWeaponInfoByName(IdWeapon, MyWeaponInfo))
	{
		//UE_LOG(LogTemp, Error, TEXT("Weapon not found in table: %s"), *IdWeapon.ToString());
		return;
	}

	if (!MyWeaponInfo.WeaponClass)
	{
		//UE_LOG(LogTemp, Error, TEXT("WeaponClass is NULL for: %s"), *IdWeapon.ToString());
		return;
	}

	if (MyGI)
	{
		if (MyGI->GetWeaponInfoByName(IdWeapon, MyWeaponInfo))
		{
			if (MyWeaponInfo.WeaponClass)
			{
				FVector SpawnLocation = FVector::ZeroVector;
				FRotator SpawnRotation = FRotator::ZeroRotator;

				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				SpawnParams.Owner = this;
				SpawnParams.Instigator = this;

				AWeapon* MyWeapon = Cast<AWeapon>(GetWorld()->SpawnActor(MyWeaponInfo.WeaponClass, &SpawnLocation, &SpawnRotation, SpawnParams));
				if (MyWeapon)
				{
					FAttachmentTransformRules Rule(EAttachmentRule::SnapToTarget, false);
					MyWeapon->AttachToComponent(GetMesh(), Rule, FName("Weapon_Socket"));
					CurrentWeapon = MyWeapon;

					MyWeapon->WeaponSetting = MyWeaponInfo;
					MyWeapon->WeaponInfo.Round = MyWeaponInfo.MaxRound;
					MyWeapon->Damage = MyWeaponInfo.WeaponDamage;
					UE_LOG(LogTemp, Warning, TEXT("=== INIT WEAPON ==="));
					UE_LOG(LogTemp, Warning, TEXT("WeaponDamage from DataTable: %f"), MyWeaponInfo.WeaponDamage);
					UE_LOG(LogTemp, Warning, TEXT("Weapon Damage after set: %f"), MyWeapon->Damage);
				}
			}
		}
		else
		{
			//UE_LOG(LogTemp, Warning, TEXT("Weapon not found in table!!!"));
		}
	}

}
