// Copyright Epic Games, Inc. All Rights Reserved.

#include "MidProgrammerTestCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "HealthComponent.h"
#include "Blueprint/UserWidget.h"
#include <Kismet/GameplayStatics.h>

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AMidProgrammerTestCharacter::AMidProgrammerTestCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->SetIsReplicated(true);

	SetCanBeDamaged(true);
}

void AMidProgrammerTestCharacter::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetController<APlayerController>();
	if (PC)
		UE_LOG(LogTemplateCharacter, Log, TEXT("PC is set!"));

}

void AMidProgrammerTestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMidProgrammerTestCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMidProgrammerTestCharacter::Look);

		// Firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &AMidProgrammerTestCharacter::Fire);

	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

#pragma region Move&Look

void AMidProgrammerTestCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AMidProgrammerTestCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

#pragma endregion

float AMidProgrammerTestCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// Apply damage - server only code
	float damageTaken = 0.0f;
	if (HasAuthority() && HealthComponent->GetCurrentHealth() > 0.0f)
	{
		damageTaken = HealthComponent->TakeDamage(DamageAmount);
		UE_LOG(LogTemplateCharacter, Log, TEXT("Took %f damage!"), damageTaken);

		// Check if the player died
		if (HealthComponent->GetCurrentHealth() <= 0)
		{
			UE_LOG(LogTemplateCharacter, Log, TEXT("Character has died! X("));
			ClientDisableInput();
		}
	}

	return damageTaken;
}


void AMidProgrammerTestCharacter::ClientDisableInput_Implementation()
{
	if (APlayerController* playerController = Cast<APlayerController>(GetController()))
	{
		DisableInput(playerController);
	}
}


void AMidProgrammerTestCharacter::Fire()
{
	if (IsLocallyControlled())
	{
		ServerFire();
	}
}


void AMidProgrammerTestCharacter::ServerFire_Implementation()
{
	FVector cameraLocation = FollowCamera->GetComponentLocation();
	
	FHitResult hitResult;
	FVector startPos = cameraLocation;
	FVector endPos = cameraLocation + FollowCamera->GetComponentRotation().Vector() * 10000.0f;

	GetWorld()->LineTraceSingleByChannel(hitResult, startPos, endPos, ECC_Pawn, {});

	if (hitResult.bBlockingHit && IsValid(hitResult.GetActor()))
	{
		FVector hitLocation = hitResult.ImpactPoint;
		UGameplayStatics::ApplyRadialDamage(this, 30.0f, hitLocation, 200.0f, nullptr, {}, this, Controller);
		MulticastSpawnExplosion(hitLocation);
	}
}

void AMidProgrammerTestCharacter::MulticastSpawnExplosion_Implementation(FVector spawnLocation)
{
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, spawnLocation, FRotator(0.0, 0.0, 0.0));
}
