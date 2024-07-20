// All Rights reserved. 


#include "HealthComponent.h"
#include <Net/UnrealNetwork.h>

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	MaxHealth = 120.0f;
	CurrentHealth = MaxHealth;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
}

float UHealthComponent::TakeDamage(float DamageAmount)
{
	float damageTaken;

	if (DamageAmount >= CurrentHealth)
	{
		damageTaken = CurrentHealth;
		CurrentHealth = 0; 
	}
	else
	{
		damageTaken = DamageAmount;
		CurrentHealth -= DamageAmount;
	}

	return damageTaken;
}

float UHealthComponent::GetMaxHealth() const
{
	return MaxHealth;
}

float UHealthComponent::GetCurrentHealth() const
{
	return CurrentHealth;
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, CurrentHealth);
}
