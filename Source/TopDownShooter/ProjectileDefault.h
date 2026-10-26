// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Particles/ParticleSystemComponent.h"
#include "Types.h"

#include "ProjectileDefault.generated.h"

class UStaticMeshComponent;
class UProjectileMovementComponent;

UCLASS()
class TOPDOWNSHOOTER_API AProjectileDefault : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectileDefault();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float Damage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = Components)
	class UParticleSystemComponent* BulletFX = nullptr;

	// Материал декали для следов от пуль
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UMaterialInterface* BulletDecalMaterial;

	// Размер декали
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	FVector DecalSize = FVector(10.0f, 10.0f, 10.0f);

	// Время жизни декали (0 = навсегда)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float DecalLifeSpan = 10.0f;


	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse,
		const FHitResult& Hit);

	// Функция установки урона из оружия
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void SetBulletDamage(float NewDamage) { Damage = NewDamage; }

	// Геттер для отладки
	UFUNCTION(BlueprintPure, Category = "Damage")
	float GetBulletDamage() const { return Damage; }

	// === СЕТТЕРЫ ДЛЯ ПАРАМЕТРОВ ИЗ ТАБЛИЦЫ ===
	UFUNCTION(BlueprintCallable, Category = "Setup")
	void SetProjectileInfo(const FProjectileInfo& Info);

	// Геттер урона
	UFUNCTION(BlueprintPure, Category = "Damage")
	float GetDamage() const { return Damage; }

private:
	// Функция создания декали
	void SpawnBulletDecal(const FVector& Location, const FVector& Normal);
};
