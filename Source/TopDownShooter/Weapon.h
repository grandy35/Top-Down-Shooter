// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileDefault.h"

#include "Weapon.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UParticleSystem;
class USoundBase;
class UMaterialInterface;

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

    // Корневой компонент
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootScene;

    // Меш оружия (точка, откуда летит пуля)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* WeaponMesh;

    // Точка спавна пуль ( muzzle )
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* MuzzleSocket;

    // === НАСТРОЙКИ ОРУЖИЯ ===

    // Урон оружия
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
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

    // === ВАШИ ФУНКЦИИ ===

    // Функция стрельбы (вызывается из Blueprint)
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void Fire();

    // Функция перезарядки
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void Reloading();

    // Функция проверки, можно ли стрелять
    UFUNCTION(BlueprintPure, Category = "Weapon")
    bool CanFire() const;

private:
    // Внутренние функции
    void PerformLineTrace(const FVector& Start, const FVector& End);
    void SpawnMuzzleFlash();
    void SpawnImpactEffects(const FVector& Location, const FVector& Normal);
    void PlayFireSound();
    void ApplyRecoil();

    // Таймер скорострельности
    float LastFireTime;

};
