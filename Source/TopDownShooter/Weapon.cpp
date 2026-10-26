// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "ProjectileDefault.h"
#include "Types.h"
#include "Particles/ParticleSystemComponent.h" 
#include "TopDownShooterCharacter.h"


using namespace std;

// Sets default values
AWeapon::AWeapon()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Создаём корневой компонент
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	SkeletalMeshWeapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Skeletal Mesh"));
	SkeletalMeshWeapon->SetGenerateOverlapEvents(false);
	SkeletalMeshWeapon->SetCollisionProfileName(TEXT("NoCollision"));
	SkeletalMeshWeapon->SetupAttachment(RootComponent);

	// Создаём меш оружия
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootScene);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Отключаем коллизию

	// Создаём точку дула (отсюда летят пули)
	MuzzleSocket = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleSocket"));
	MuzzleSocket->SetupAttachment(WeaponMesh);
	MuzzleSocket->SetRelativeLocation(FVector(50.0f, 0.0f, 0.0f)); // Сдвиг вперёд от центра меша

	// Создаём сокет магазина
	MagazineSocket = CreateDefaultSubobject<USceneComponent>(TEXT("MagazineSocket"));
	MagazineSocket->SetupAttachment(WeaponMesh);
	MagazineSocket->SetRelativeLocation(FVector(0, 0, 15));

	Damage = 10.0f;
	FireRate = 5.0f; // 5 выстрелов в секунду
	Range = 10000.0f;
	CurrentAmmo = 30;
	MaxAmmo = 30;
	LastFireTime = 0.0f;
	bIsReloading = false;
	ReloadTime = 1.5f;

}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	FireTick(DeltaTime);
	ReloadTick(DeltaTime);
	DispersionTick(DeltaTime);
}

void AWeapon::FireTick(float DeltaTime)
{
	if (GetWeaponRound() > 0)
	{
		if (WeaponFiring)
			if (LastFireTime < 0.f)
			{
				if (!bIsReloading)
					Fire();
			}
			else
				LastFireTime -= DeltaTime;
	}
	else
	{
		if (!bIsReloading)
		{
			Reloading();
		}
	}
}

void AWeapon::ReloadTick(float DeltaTime)
{
	if (bIsReloading)
	{
		if (ReloadTime < 0.0f)
		{
			FinishReload();
		}
		else
		{
			ReloadTime -= DeltaTime;
		}
	}
}

void AWeapon::DispersionTick(float DeltaTime)
{
	if (!bIsReloading)
	{
		if (!WeaponFiring)
		{
			if (ShouldReduceDispersion)
				CurrentDispersion = CurrentDispersion - CurrentDispersionReduction;
			else
				CurrentDispersion = CurrentDispersion + CurrentDispersionReduction;
		}

		if (CurrentDispersion < CurrentDispersionMin)
		{

			CurrentDispersion = CurrentDispersionMin;

		}
		else
		{
			if (CurrentDispersion > CurrentDispersionMax)
			{
				CurrentDispersion = CurrentDispersionMax;
			}
		}
	}
	if (ShowDebug)
		UE_LOG(LogTemp, Warning, TEXT("Dispersion: MAX = %f. MIN = %f. Current = %f"), CurrentDispersionMax, CurrentDispersionMin, CurrentDispersion);
}

void AWeapon::Fire()
{
	if (!CanFire())
	{
		UE_LOG(LogTemp, Error, TEXT("NO AMMO!"));
		return;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastFireTime < (0.4f / FireRate))
	{
		return;
	}

	CurrentAmmo--;
	LastFireTime = CurrentTime;

	UE_LOG(LogTemp, Warning, TEXT("=== FIRE ==="));
	UE_LOG(LogTemp, Warning, TEXT("Fire! Ammo: %d"), CurrentAmmo);

	// Параметры спавна
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Загружаем класс Blueprint пули
	UClass* BulletClass = LoadClass<AActor>(
		nullptr,
		TEXT("/Game/Weapons_Free/Meshes/Actors/Projectile/BP_Projectile.BP_Projectile_C")
	);

	if (!BulletClass)
	{
		UE_LOG(LogTemp, Error, TEXT("ERROR: BP_Projectile class not found!"));
		return;
	}

	// 1. Получаем позицию дула
	FVector MuzzleLoc = GetActorLocation() + (GetActorForwardVector() * 100.0f);

	// 2. Получаем направление ОТ ПЕРСОНАЖА к курсору мыши
	FVector LaunchDirection = FVector::ZeroVector;
	APlayerController* PC = GetWorld()->GetFirstPlayerController();

	if (PC)
	{
		// Получаем курсор мыши в мире
		FHitResult HitResult;
		bool bHit = PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

		if (bHit && HitResult.bBlockingHit)
		{
			// Направление от дула к точке попадания курсора
			LaunchDirection = HitResult.Location - MuzzleLoc;
			LaunchDirection.Z = 0.0f; // Горизонтальный полёт
			LaunchDirection.Normalize();

			UE_LOG(LogTemp, Warning, TEXT("✓ Mouse Hit: %s"), *HitResult.Location.ToString());
		}
		else
		{
			// Если курсор ни во что не попал — используем направление вперёд
			LaunchDirection = GetActorForwardVector();
			LaunchDirection.Z = 0.0f;
			LaunchDirection.Normalize();

			UE_LOG(LogTemp, Warning, TEXT(" Mouse miss - using forward vector"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("✗ No PlayerController!"));
		LaunchDirection = GetActorForwardVector();
	}

	UE_LOG(LogTemp, Warning, TEXT("LaunchDirection: %s"), *LaunchDirection.ToString());

	// 3. Спавним пулю СРАЗУ в правильной позиции
	FRotator LaunchRotation = LaunchDirection.Rotation();

	AActor* BulletActor = GetWorld()->SpawnActor<AActor>(
		BulletClass,
		MuzzleLoc,
		LaunchRotation,
		SpawnParams
	);

	if (BulletActor)
	{
		UE_LOG(LogTemp, Warning, TEXT(" BULLET SPAWNED at: %s"), *MuzzleLoc.ToString());

		// === ПЕРЕДАЁМ УРОН ИЗ ОРУЖИЯ В ПУЛЮ ===
		AProjectileDefault* Bullet = Cast<AProjectileDefault>(BulletActor);
		if (Bullet)
		{
			UE_LOG(LogTemp, Warning, TEXT("=== FIRE DAMAGE DEBUG ==="));
			UE_LOG(LogTemp, Warning, TEXT("Weapon Damage (this): %f"), Damage);
			UE_LOG(LogTemp, Warning, TEXT("Setting bullet damage to: %f"), Damage);
			Bullet->SetProjectileInfo(WeaponSetting.ProjectileSetting);
			UE_LOG(LogTemp, Warning, TEXT("Bullet damage after set: %f"), Bullet->GetDamage());
		}

		// Пытаемся получить компонент движения
		UProjectileMovementComponent* Movement =
			BulletActor->FindComponentByClass<UProjectileMovementComponent>();

		if (Movement)
		{
			Movement->Velocity = LaunchDirection * Movement->InitialSpeed;
			UE_LOG(LogTemp, Warning, TEXT("✓ Velocity set: %s"), *Movement->Velocity.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("✗ No ProjectileMovementComponent found!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("✗ Spawn failed!"));
	}

	// Эффекты
	if (BulletActor)
	{
		SpawnMuzzleFlash();
		PlayFireSound();
	}
}

void AWeapon::WeaponInit()
{
	if (SkeletalMeshWeapon && !SkeletalMeshWeapon->SkeletalMesh)
	{
		SkeletalMeshWeapon->DestroyComponent(true);
	}

	if (WeaponMesh && !WeaponMesh->GetStaticMesh())
	{
		WeaponMesh->DestroyComponent();
	}

	UpdateStateWeapon(EMovementState::Run);
}


void AWeapon::SetWeaponStateFire(bool bIsFire)
{
	if (CanFire())
		WeaponFiring = bIsFire;
	else
		WeaponFiring = false;
	LastFireTime = 0.01f;//!!!!!
}

void AWeapon::UpdateStateWeapon(EMovementState NewMovementState)
{
	//ToDo Dispersion
	BlockFire = false;

	switch (NewMovementState)
	{
	case EMovementState::Aim:

		CurrentDispersionMax = WeaponSetting.DispersionWeapon.Aim_StateDispersionAimMax;
		CurrentDispersionMin = WeaponSetting.DispersionWeapon.Aim_StateDispersionAimMin;
		CurrentDispersionRecoil = WeaponSetting.DispersionWeapon.Aim_StateDispersionAimRecoil;
		CurrentDispersionReduction = WeaponSetting.DispersionWeapon.Aim_StateDispersionReduction;
		break;
	case EMovementState::AimWalk:

		CurrentDispersionMax = WeaponSetting.DispersionWeapon.AimWalk_StateDispersionAimMax;
		CurrentDispersionMin = WeaponSetting.DispersionWeapon.AimWalk_StateDispersionAimMin;
		CurrentDispersionRecoil = WeaponSetting.DispersionWeapon.AimWalk_StateDispersionAimRecoil;
		CurrentDispersionReduction = WeaponSetting.DispersionWeapon.Aim_StateDispersionReduction;
		break;
	case EMovementState::Walk:

		CurrentDispersionMax = WeaponSetting.DispersionWeapon.Walk_StateDispersionAimMax;
		CurrentDispersionMin = WeaponSetting.DispersionWeapon.Walk_StateDispersionAimMin;
		CurrentDispersionRecoil = WeaponSetting.DispersionWeapon.Walk_StateDispersionAimRecoil;
		CurrentDispersionReduction = WeaponSetting.DispersionWeapon.Aim_StateDispersionReduction;
		break;
	case EMovementState::Run:

		CurrentDispersionMax = WeaponSetting.DispersionWeapon.Run_StateDispersionAimMax;
		CurrentDispersionMin = WeaponSetting.DispersionWeapon.Run_StateDispersionAimMin;
		CurrentDispersionRecoil = WeaponSetting.DispersionWeapon.Run_StateDispersionAimRecoil;
		CurrentDispersionReduction = WeaponSetting.DispersionWeapon.Aim_StateDispersionReduction;
		break;
	case EMovementState::SprintRun:
		BlockFire = true;
		SetWeaponStateFire(false);//set fire trigger to false
		//Block Fire
		break;
	default:
		break;
	}
}

void AWeapon::ChangeDispersionByShot()
{
	CurrentDispersion = CurrentDispersion + CurrentDispersionRecoil;
}

float AWeapon::GetCurrentDispersion() const
{
	float Result = CurrentDispersion;
	return Result;
}

FVector AWeapon::ApplyDispersionToShoot(FVector DirectionShoot) const
{
	return FMath::VRandCone(DirectionShoot, GetCurrentDispersion() * PI / 180.f);
}

FVector AWeapon::GetFireEndLocation() const
{
	bool bShootDirection = false;
	FVector EndLocation = FVector(0.f);

	FVector tmpV = (ShootLocation->GetComponentLocation() - ShootEndLocation);
	//UE_LOG(LogTemp, Warning, TEXT("Vector: X = %f. Y = %f. Size = %f"), tmpV.X, tmpV.Y, tmpV.Size());

	if (tmpV.Size() > SizeVectorToChangeShootDirectionLogic)
	{
		EndLocation = ShootLocation->GetComponentLocation() + ApplyDispersionToShoot((ShootLocation->GetComponentLocation() - ShootEndLocation).GetSafeNormal()) * -20000.0f;
		if (ShowDebug)
			DrawDebugCone(GetWorld(), ShootLocation->GetComponentLocation(), -(ShootLocation->GetComponentLocation() - ShootEndLocation), WeaponSetting.DistanceTrace, GetCurrentDispersion() * PI / 180.f, GetCurrentDispersion() * PI / 180.f, 32, FColor::Emerald, false, .1f, (uint8)'\000', 1.0f);
	}
	else
	{
		EndLocation = ShootLocation->GetComponentLocation() + ApplyDispersionToShoot(ShootLocation->GetForwardVector()) * 20000.0f;
		if (ShowDebug)
			DrawDebugCone(GetWorld(), ShootLocation->GetComponentLocation(), ShootLocation->GetForwardVector(), WeaponSetting.DistanceTrace, GetCurrentDispersion() * PI / 180.f, GetCurrentDispersion() * PI / 180.f, 32, FColor::Emerald, false, .1f, (uint8)'\000', 1.0f);
	}


	if (ShowDebug)
	{
		//direction weapon look
		DrawDebugLine(GetWorld(), ShootLocation->GetComponentLocation(), ShootLocation->GetComponentLocation() + ShootLocation->GetForwardVector() * 500.0f, FColor::Cyan, false, 5.f, (uint8)'\000', 0.5f);
		//direction projectile must fly
		DrawDebugLine(GetWorld(), ShootLocation->GetComponentLocation(), ShootEndLocation, FColor::Red, false, 5.f, (uint8)'\000', 0.5f);
		//Direction Projectile Current fly
		DrawDebugLine(GetWorld(), ShootLocation->GetComponentLocation(), EndLocation, FColor::Black, false, 5.f, (uint8)'\000', 0.5f);

		//DrawDebugSphere(GetWorld(), ShootLocation->GetComponentLocation() + ShootLocation->GetForwardVector() * SizeVectorToChangeShootDirectionLogic, 10.f, 8, FColor::Red, false, 4.0f);
	}


	return EndLocation;
}

int8 AWeapon::GetNumberProjectileByShot() const
{
	return WeaponSetting.NumberProjectileByShot;
}

int32 AWeapon::GetWeaponRound()
{
	return WeaponInfo.Round;
}

void AWeapon::Reloading()
{
	// Проверяем, не идёт ли уже перезарядка
	if (bIsReloading)
	{
		UE_LOG(LogTemp, Warning, TEXT("Already reloading!"));
		return;
	}

	// Проверяем, есть ли смысл перезаряжаться
	if (CurrentAmmo >= MaxAmmo)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ammo is full!"));
		return;
	}

	// Начинаем перезарядку
	bIsReloading = true;
	UE_LOG(LogTemp, Warning, TEXT("Reloading started..."));

	FTimerHandle ReloadTimerHandle;
	GetWorldTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&AWeapon::FinishReload,
		ReloadTime,
		false
	);

	PlayReloadSound();
	// Здесь можно добавить звук перезарядки и анимацию
}

bool AWeapon::CanFire() const
{
	return CurrentAmmo > 0 && !bIsReloading;
}

// === ТРАССИРОВКА (LINE TRACE) ===
void AWeapon::PerformLineTrace(const FVector& Start, const FVector& End)
{
	FHitResult HitResult;

	// Параметры трассировки
	FCollisionQueryParams TraceParams;
	TraceParams.bTraceComplex = true; // Точная проверка коллизии
	TraceParams.bReturnPhysicalMaterial = true;
	TraceParams.AddIgnoredActor(this); // Игнорируем само оружие

	// Выполняем трассировку
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECollisionChannel::ECC_Visibility, // Канал видимости
		TraceParams
	);

	// Отладочная линия (видна в редакторе)
	DrawDebugLine(
		GetWorld(),
		Start,
		bHit ? HitResult.Location : End,
		FColor::Red,
		false, // Не постоянная
		2.0f, // Длительность отображения
		0,
		2.0f // Толщина линии
	);

	if (bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit: %s at distance %f"),
			*HitResult.GetActor()->GetName(),
			HitResult.Distance);

		// Наносим урон
		AActor* HitActor = HitResult.GetActor();
		if (HitActor)
		{
			// Проверяем, есть ли у актора интерфейс урона
			UGameplayStatics::ApplyDamage(
				HitActor,
				Damage,
				nullptr, // Controller (можно добавить позже)
				this, // Instigator (кто стреляет)
				UDamageType::StaticClass()
			);
		}

		// Эффекты попадания
		SpawnImpactEffects(HitResult.Location, HitResult.ImpactNormal);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Miss!"));
	}
}

// === ЭФФЕКТ ВЫСТРЕЛА (ВСПЫШКА ИЗ ДУЛА) ===
void AWeapon::SpawnMuzzleFlash()
{
	if (MuzzleFlashEffect && MuzzleSocket)
	{
		UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			MuzzleFlashEffect,
			MuzzleSocket->GetComponentLocation(),
			MuzzleSocket->GetComponentRotation(),
			FVector(1.0f)
		);

		// Автоматически уничтожаем частицы через 0.1 секунды
		if (PSC)
		{
			// UE4 способ: используем таймер
			FTimerHandle TimerHandle;
			GetWorldTimerManager().SetTimer(
				TimerHandle,
				[PSC]()
				{
					if (PSC && PSC->IsValidLowLevel())
					{
						PSC->DestroyComponent();
					}
				},
				0.1f,  // Задержка в секундах
				false  // Не повторять
			);
		}
	}
}

// === ЭФФЕКТЫ ПОПАДАНИЯ ===
void AWeapon::SpawnImpactEffects(const FVector& Location, const FVector& Normal)
{
	if (ImpactEffect)
	{
		// Поворачиваем эффект по нормали поверхности
		FRotator ImpactRotation = Normal.Rotation();

		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ImpactEffect,
			Location,
			ImpactRotation,
			FVector(1.0f)
		);
	}

	// Звук попадания
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			ImpactSound,
			Location
		);
	}

	// Декали (следы от пуль) - опционально
	// UGameplayStatics::SpawnDecalAtLocation(...)
}

// === ЗВУК ВЫСТРЕЛА ===
void AWeapon::PlayFireSound()
{
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			FireSound,
			GetActorLocation()
		);
	}
}

void AWeapon::PlayReloadSound()
{
	if (ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			ReloadSound,
			GetActorLocation()
		);
	};
}

void AWeapon::FinishReload()
{
	CurrentAmmo = MaxAmmo;
	bIsReloading = false;

	UE_LOG(LogTemp, Warning, TEXT("Reload complete! Ammo: %d"), CurrentAmmo);
}

void AWeapon::ApplyRecoil()
{
}

