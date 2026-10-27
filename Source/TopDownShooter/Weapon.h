// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Types.h"


struct FWeaponInfo;
class UStaticMeshComponent;
class USceneComponent;
class UParticleSystem;
class USoundBase;
class UMaterialInterface;
class UAnimMontage;

#include "Weapon.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReloadStart, UAnimMontage*, Anim);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponReloadEnd);

UCLASS()
class TOPDOWNSHOOTER_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWeapon();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void FireTick(float DeltaTime);
	void ReloadTick(float DeltaTime);
	void DispersionTick(float DeltaTime);

	void WeaponInit();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireLogic")
	bool WeaponFiring = false;
	UFUNCTION(BlueprintCallable)
	void SetWeaponStateFire(bool bIsFire);

	FProjectileInfo GetProjectile();

	// Таблица настроек пули (связана с оружием)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	class UDataTable* ProjectileDataTable;

	// Имя строки в таблице (какую пулю использовать)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	FName ProjectileRowName;

	// Кэшированные настройки пули
	FProjectileInfo ProjectileSetting;

	// Корневой компонент
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = Components)
	class USkeletalMeshComponent* SkeletalMeshWeapon = nullptr;

	// Меш оружия (точка, откуда летит пуля)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* WeaponMesh;

	// Точка спавна пуль ( muzzle )
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* MuzzleSocket;

	// Точка магазина (для IK перезарядки)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* MagazineSocket;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = Components)
	class UArrowComponent* ShootLocation = nullptr;

	UPROPERTY()
	FWeaponInfo WeaponSetting;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Info")
	FAddicionalWeaponInfo WeaponInfo;

	// === НАСТРОЙКИ ОРУЖИЯ ===

	// Урон оружия
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float Damage;
	// Скорострельность (выстрелов в секунду)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FireRate;
	// Дальность стрельбы
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float Range;
	// Текущий боезапас
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	int32 CurrentAmmo;
	// Максимальный боезапас
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	int32 MaxAmmo;
	// Флаг перезарядки
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	bool bIsReloading;

private:
	// Длительность перезарядки (секунды)
	UPROPERTY(EditAnywhere, Category = "Weapon")
	float ReloadTime;
public:
	// Эффект выстрела (частицы из дула)
	UPROPERTY(EditAnywhere, Category = "Effects")
	UParticleSystem* MuzzleFlashEffect;
	// Эффект попадания в поверхность
	UPROPERTY(EditAnywhere, Category = "Effects")
	UParticleSystem* ImpactEffect;
	// Звук выстрела
	UPROPERTY(EditAnywhere, Category = "Effects")
	USoundBase* FireSound;
	// Звук попадания
	UPROPERTY(EditAnywhere, Category = "Effects")
	USoundBase* ImpactSound;
	UPROPERTY(EditAnywhere, Category = "Effects")
	USoundBase* ReloadSound;

	//Dispersion
	bool ShouldReduceDispersion = false;
	float CurrentDispersion = 0.0f;
	float CurrentDispersionMax = 1.0f;
	float CurrentDispersionMin = 0.1f;
	float CurrentDispersionRecoil = 0.1f;
	float CurrentDispersionReduction = 0.1f;

	FVector ShootEndLocation = FVector(0);

	// === ВАШИ ФУНКЦИИ ===

	void UpdateStateWeapon(EMovementState NewMovementState);
	void ChangeDispersionByShot();
	float GetCurrentDispersion() const;
	FVector ApplyDispersionToShoot(FVector DirectionShoot)const;
	FVector GetFireEndLocation()const;
	int8 GetNumberProjectileByShot() const;

	// Функция стрельбы (вызывается из Blueprint)
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Fire();
	// Функция перезарядки
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Reloading();
	// Функция проверки, можно ли стрелять
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool CanFire() const;
	UFUNCTION(BlueprintCallable)
	int32 GetWeaponRound();

	UFUNCTION(BlueprintPure, Category = "Animation")
	UAnimMontage* GetReloadAnimation() const;
	
	UFUNCTION(BlueprintPure, Category = "Animation")
	UAnimMontage* GetCurrentFireAnimation() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool ShowDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	float SizeVectorToChangeShootDirectionLogic = 100.0f;

private:
	// Внутренние функции
	void PerformLineTrace(const FVector& Start, const FVector& End);
	void SpawnMuzzleFlash();
	void SpawnImpactEffects(const FVector& Location, const FVector& Normal);
	void PlayFireSound();
	void PlayReloadSound();
	void ApplyRecoil();
	void FinishReload();
	void DecreaseAmmo();
	void SpawnBullet();

	bool BlockFire = false;
	// Таймер скорострельности
	float LastFireTime;
	float ReloadTimeRemaining = 0.0f;

};
