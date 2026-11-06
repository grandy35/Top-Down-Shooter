// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileDefault.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AProjectileDefault::AProjectileDefault()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Меш пули
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	RootComponent = ProjectileMesh;
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProjectileMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	ProjectileMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	ProjectileMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	ProjectileMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	BulletFX = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Bullet FX"));
	BulletFX->SetupAttachment(RootComponent);

	// Движение пули
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 5000.0f; // Скорость пули (см/с)
	ProjectileMovement->MaxSpeed = 5000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	Damage = 25.0f;

	// Обработка столкновений
	ProjectileMesh->OnComponentHit.AddDynamic(this, &AProjectileDefault::OnHit);
}

// Called when the game starts or when spawned
void AProjectileDefault::BeginPlay()
{
	Super::BeginPlay();

	// Автоматическое уничтожение через 10 секунд
	SetLifeSpan(10.0f);
}

// Called every frame
void AProjectileDefault::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectileDefault::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse,
	const FHitResult& Hit)
{
	UE_LOG(LogTemp, Warning, TEXT(" Пуля попала в: %s"), *OtherActor->GetName());
	UE_LOG(LogTemp, Warning, TEXT(" Урон пули: %f"), Damage);

	if (OtherActor && OtherActor != this)
	{
		// Получаем контроллер instigator (кто стрелял)
		AController* InstigatorController = nullptr;
		AActor* InstigatorActor = GetInstigator();
		if (InstigatorActor)
		{
			InstigatorController = InstigatorActor->GetInstigatorController();
		}

		float AppliedDamage = UGameplayStatics::ApplyDamage(
			OtherActor,
			Damage,
			InstigatorController,  //  кто нанёс урон
			this,                  //  чем нанёс урон
			UDamageType::StaticClass()
		);
		SpawnBulletDecal(Hit.ImpactPoint, Hit.ImpactNormal);
		UE_LOG(LogTemp, Warning, TEXT("Apply Damage: %f"), AppliedDamage);
	}

	// Уничтожаем пулю
	Destroy();
}



void AProjectileDefault::SpawnBulletDecal(const FVector& Location, const FVector& Normal)
{
	if (!BulletDecalMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("BulletDecalMaterial not set!"));
		return;
	}

	// Поворот декали по нормали поверхности
	FRotator DecalRotation = Normal.Rotation();

	// Спавним декаль напрямую через GameplayStatics
	UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(
		GetWorld(),
		BulletDecalMaterial,  // Материал декали
		DecalSize,            // Размер (FVector)
		Location,             // Позиция
		DecalRotation,        // Поворот
		DecalLifeSpan         // Время жизни (0 = навсегда)
	);

	if (Decal)
	{
		UE_LOG(LogTemp, Warning, TEXT("Bullet decal spawned at: %s"), *Location.ToString());
	}
}

void AProjectileDefault::SetProjectileInfo(const FProjectileInfo& Info)
{
	Damage = Info.ProjectileDamage;

	// Устанавливаем время жизни
	SetLifeSpan(Info.ProjectileLifeTime);

	// Устанавливаем скорость
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = Info.ProjectileInitSpeed;
		ProjectileMovement->MaxSpeed = Info.ProjectileInitSpeed * 1.5f;
	}

	UE_LOG(LogTemp, Warning, TEXT("Projectile info set: Damage=%.1f, Life=%.1f, Speed=%.0f"),
		Damage, Info.ProjectileLifeTime, ProjectileMovement->InitialSpeed);
}