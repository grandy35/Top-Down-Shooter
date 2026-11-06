// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "ProjectileDefault.h"
#include "Types.h"
#include "Particles/ParticleSystemComponent.h" 
#include "Engine/StaticMeshActor.h"
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
	ClipDropTick(DeltaTime);
	ShellDropTick(DeltaTime);
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
	if (!bIsReloading)
	{
		return;
	}

	ReloadTimeRemaining -= DeltaTime;

	if (ReloadTimeRemaining < 0.0f)
	{
		FinishReload();
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

void AWeapon::ClipDropTick(float DeltaTime)
{
	if (DropClipFlag)
	{
		if (DropClipTimer < 0.0f)
		{
			DropClipFlag = false;
			InitDropMesh(WeaponSetting.ClipDropMesh.DropMesh, WeaponSetting.ClipDropMesh.DropMeshOffset, WeaponSetting.ClipDropMesh.DropMeshImpulseDir, WeaponSetting.ClipDropMesh.DropMeshLifeTime, WeaponSetting.ClipDropMesh.ImpulseRandomDispersion, WeaponSetting.ClipDropMesh.PowerImpulse, WeaponSetting.ClipDropMesh.CustomMass);
		}
		else
			DropClipTimer -= DeltaTime;
	}
}

void AWeapon::ShellDropTick(float DeltaTime)
{
	if (DropShellFlag)
	{
		if (DropShellTimer < 0.0f)
		{
			DropShellFlag = false;
			InitDropMesh(WeaponSetting.ShellBullets.DropMesh, WeaponSetting.ShellBullets.DropMeshOffset, WeaponSetting.ShellBullets.DropMeshImpulseDir, WeaponSetting.ShellBullets.DropMeshLifeTime, WeaponSetting.ShellBullets.ImpulseRandomDispersion, WeaponSetting.ShellBullets.PowerImpulse, WeaponSetting.ShellBullets.CustomMass);
		}
		else
			DropShellTimer -= DeltaTime;
	}
}

void AWeapon::Fire()
{
	if (!CanFire())
	{
		UE_LOG(LogTemp, Error, TEXT("NO AMMO!"));
		return;
	}

	//вынести в перем
	float CurrentTime = GetWorld()->GetTimeSeconds();
	bool ReadyToFire = CurrentTime - LastFireTime < (0.4f / WeaponSetting.RateOfFire);
	if (ReadyToFire)
	{
		return;
	}

	if (WeaponSetting.ShellBullets.DropMesh)
	{
		if (WeaponSetting.ShellBullets.DropMeshTime < 0.0f)
		{
			InitDropMesh(WeaponSetting.ShellBullets.DropMesh, WeaponSetting.ShellBullets.DropMeshOffset, WeaponSetting.ShellBullets.DropMeshImpulseDir, WeaponSetting.ShellBullets.DropMeshLifeTime, WeaponSetting.ShellBullets.ImpulseRandomDispersion, WeaponSetting.ShellBullets.PowerImpulse, WeaponSetting.ShellBullets.CustomMass);
		}
		else
		{
			DropShellFlag = true;
			DropShellTimer = WeaponSetting.ShellBullets.DropMeshTime;
		}
	}

	DecreaseAmmo();
	LastFireTime = CurrentTime;

	UE_LOG(LogTemp, Warning, TEXT("=== FIRE ==="));
	UE_LOG(LogTemp, Warning, TEXT("Fire! Ammo: %d"), CurrentAmmo);

	SpawnBullet();
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
	if (CurrentAmmo >= WeaponSetting.MaxRound)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ammo is full!"));
		return;
	}

	// Начинаем перезарядку
	bIsReloading = true;

	ReloadTimeRemaining = ReloadTime;
	UE_LOG(LogTemp, Warning, TEXT("Reloading started"));

	if (WeaponSetting.ClipDropMesh.DropMesh)
	{
		DropClipFlag = true;
		DropClipTimer = WeaponSetting.ClipDropMesh.DropMeshTime;
	}

	PlayReloadSound();
}

bool AWeapon::CanFire() const
{
	return CurrentAmmo > 0 && !bIsReloading;
}

bool AWeapon::IsReadyToFire() const
{
	// Проверяем кулдаун
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float FireDelay = 0.4f / WeaponSetting.RateOfFire;
	bool ReadyToFire = CurrentTime - LastFireTime < FireDelay;

	if (ReadyToFire)
	{
		return false;  // Кулдаун не прошёл
	}
	return true;
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
	if (WeaponSetting.EffectfireWeapon && MuzzleSocket)
	{
		UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			WeaponSetting.EffectfireWeapon,
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
	if (WeaponSetting.SoundFireWeapon)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			WeaponSetting.SoundFireWeapon,
			GetActorLocation()
		);
	}
}

void AWeapon::PlayReloadSound()
{
	if (WeaponSetting.SoundReloadWeapon)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			WeaponSetting.SoundReloadWeapon,
			GetActorLocation()
		);
	};
}

void AWeapon::FinishReload()
{
	CurrentAmmo = WeaponSetting.MaxRound;
	bIsReloading = false;
	ReloadTimeRemaining = 0.0f;

	UE_LOG(LogTemp, Warning, TEXT("Reload complete! Ammo: %d"), CurrentAmmo);
}

void AWeapon::DecreaseAmmo()
{
	CurrentAmmo -= GetNumberProjectileByShot();
}

void AWeapon::SpawnBullet()
{
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
	int32 NumProjectiles = GetNumberProjectileByShot();  // из таблицы (для дробовика = 8-12)

	UE_LOG(LogTemp, Warning, TEXT("Spawning %d projectiles with dispersion"), NumProjectiles);

	for (int32 i = 0; i < NumProjectiles; i++)
	{
		// Применяем разброс к направлению для каждой пули
		LaunchDirection = ApplyDispersionToShoot(LaunchDirection);
		FRotator LaunchRotation = LaunchDirection.Rotation();

		AActor* BulletActor;
		BulletActor = GetWorld()->SpawnActor<AActor>(
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
	}

	SpawnMuzzleFlash();
	PlayFireSound();
}

void AWeapon::InitDropMesh(UStaticMesh* DropMesh, FTransform Offset, FVector DropImpulseDirection, float LifeTimeMesh, float ImpulseRandomDispersion, float PowerImpulse, float CustomMass)
{
	if (DropMesh)
	{
		FTransform Transform;

		FVector LocalDir = this->GetActorForwardVector() * Offset.GetLocation().X +
			this->GetActorRightVector() * Offset.GetLocation().Y +
			this->GetActorUpVector() * Offset.GetLocation().Z;

		Transform.SetLocation(GetActorLocation() + LocalDir);
		Transform.SetScale3D(Offset.GetScale3D());

		Transform.SetRotation((GetActorRotation() + Offset.Rotator()).Quaternion());
		AStaticMeshActor* NewActor = nullptr;

		FActorSpawnParameters Param;
		Param.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Param.Owner = this;
		NewActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Transform, Param);

		if (NewActor && NewActor->GetStaticMeshComponent())
		{
			NewActor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("IgnoreOnlyPawn"));
			NewActor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);

			//set parameter for new actor
			NewActor->SetActorTickEnabled(false);

			NewActor->GetStaticMeshComponent()->Mobility = EComponentMobility::Movable;
			NewActor->GetStaticMeshComponent()->SetSimulatePhysics(true);
			NewActor->GetStaticMeshComponent()->SetStaticMesh(DropMesh);
			//NewActor->GetStaticMeshComponent()->SetCollisionObjectType()



			NewActor->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
			NewActor->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
			NewActor->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Ignore);
			NewActor->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECollisionResponse::ECR_Block);
			NewActor->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECollisionResponse::ECR_Block);
			NewActor->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECollisionResponse::ECR_Block);



			if (CustomMass > 0.0f)
			{
				NewActor->GetStaticMeshComponent()->SetMassOverrideInKg(NAME_None, CustomMass, true);
			}

			if (!DropImpulseDirection.IsNearlyZero())
			{
				FVector FinalDir;
				LocalDir = LocalDir + (DropImpulseDirection * 1000.0f);

				if (!FMath::IsNearlyZero(ImpulseRandomDispersion))
					FinalDir += UKismetMathLibrary::RandomUnitVectorInConeInDegrees(LocalDir, ImpulseRandomDispersion);
				FinalDir.GetSafeNormal(0.0001f);

				NewActor->GetStaticMeshComponent()->AddImpulse(FinalDir * PowerImpulse);
			}

			// === УСТАНОВКА ВРЕМЕНИ ЖИЗНИ ===
			if (LifeTimeMesh > 0.0f)
			{
				// Способ 1: InitialLifeSpan (может не работать с физикой)
				NewActor->InitialLifeSpan = LifeTimeMesh;

				// Способ 2: Явный таймер (надёжнее)
				FTimerHandle DestroyTimer;
				GetWorldTimerManager().SetTimer(
					DestroyTimer,
					[NewActor]()
					{
						if (NewActor && NewActor->IsValidLowLevel())
						{
							NewActor->Destroy();
							UE_LOG(LogTemp, Warning, TEXT("✓ Shell bullet destroyed"));
						}
					},
					LifeTimeMesh,
					false
				);

				UE_LOG(LogTemp, Warning, TEXT("✓ Shell bullet spawned, lifetime: %.1f"), LifeTimeMesh);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("⚠ LifeTimeMesh = 0, shell will not disappear!"));
			}
		}
	}
}

UAnimMontage* AWeapon::GetReloadAnimation() const
{
	return WeaponSetting.AnimCharReload;
}

UAnimMontage* AWeapon::GetCurrentFireAnimation() const
{
	ATopDownShooterCharacter* Character = Cast<ATopDownShooterCharacter>(GetOwner());
	if (Character && Character->AimEnabled && WeaponSetting.AnimCharAimFire)
	{
		return WeaponSetting.AnimCharAimFire;
	}
	return WeaponSetting.AnimCharFire;
}

void AWeapon::ApplyRecoil()
{

}

