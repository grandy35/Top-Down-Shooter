// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileDefault.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
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
	
    // Автоматическое уничтожение через 5 секунд
    SetLifeSpan(5.0f);
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
    UE_LOG(LogTemp, Warning, TEXT("Пуля попала в: %s"), *OtherActor->GetName());

    // Наносим урон
    if (OtherActor && OtherActor != this)
    {
        UGameplayStatics::ApplyDamage(
            OtherActor,
            Damage,
            nullptr,
            this,
            UDamageType::StaticClass()
        );
    }

    // Эффект попадания
    // UGameplayStatics::SpawnEmitterAtLocation(...)

    // Уничтожаем пулю
    Destroy();
}

