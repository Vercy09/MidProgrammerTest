// All Rights reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MIDPROGRAMMERTEST_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHealthComponent();

protected:
	virtual void BeginPlay() override;

public:
	float TakeDamage(float DamageAmount);

#pragma region Getters

public:
	UFUNCTION(BlueprintCallable, Category = "Health")
	FORCEINLINE float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Health")
	FORCEINLINE float GetCurrentHealth() const;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

# pragma endregion

protected:
	float MaxHealth;

	UPROPERTY(replicated)
	float CurrentHealth;
};
