// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "ProjectileDefault.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"

using namespace std;

// Sets default values
AWeapon::AWeapon()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Создаём корневой компонент
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	// Создаём меш оружия
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootScene);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Отключаем коллизию

	// Создаём точку дула (отсюда летят пули)
	MuzzleSocket = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleSocket"));
	MuzzleSocket->SetupAttachment(WeaponMesh);
	MuzzleSocket->SetRelativeLocation(FVector(50.0f, 0.0f, 0.0f)); // Сдвиг вперёд от центра меша

	Damage = 10.0f;
	FireRate = 5.0f; // 5 выстрелов в секунду
	Range = 10000.0f;
	CurrentAmmo = 30;
	MaxAmmo = 30;
	LastFireTime = 0.0f;

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
		UE_LOG(LogTemp, Warning, TEXT("✓ BULLET SPAWNED at: %s"), *MuzzleLoc.ToString());

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
	SpawnMuzzleFlash();
	PlayFireSound();
}

void AWeapon::Reloading()
{
	if (CurrentAmmo < MaxAmmo)
	{
		CurrentAmmo = MaxAmmo;
		UE_LOG(LogTemp, Warning, TEXT("Reloaded! Ammo: %d"), CurrentAmmo);

		// Здесь можно добавить звук перезарядки и анимацию
	}
}

bool AWeapon::CanFire() const
{
	return CurrentAmmo > 0;
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

void AWeapon::ApplyRecoil()
{
}

