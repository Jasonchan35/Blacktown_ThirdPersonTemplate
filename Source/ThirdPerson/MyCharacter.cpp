// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "Abilities/MyUltraHandComponent.h"
#include "MyPlayerController.h"

//////////////////////////////////////////////////////////////////////////
// AMyCharacter

AMyCharacter::AMyCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 250.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	CameraBoom->SocketOffset = FVector(0, 40, 60);

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	MyCDO::CreateComponent(this, UltraHandComponent);
}

void AMyCharacter::SetCurrentAbility(EMyAbility Ability)
{
	if (CurrentAbility == Ability)
		return;

	CurrentAbility = Ability;

	if (CurrentAbilityComponent)
		CurrentAbilityComponent->SetAbilityActive(false);

	switch (Ability)
	{
		case EMyAbility::UltraHand:	CurrentAbilityComponent = UltraHandComponent; break;
		default:					CurrentAbilityComponent = nullptr; break;
	}

	if (CurrentAbilityComponent)
		CurrentAbilityComponent->SetAbilityActive(true);
}

void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AMyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

AMyPlayerController* AMyCharacter::GetPlayerController()
{
	return GetController<AMyPlayerController>();
}


void AMyCharacter::IA_DPad_Triggered(const FVector2D& Value)
{
	if (CurrentAbilityComponent)
		CurrentAbilityComponent->IA_DPad_Triggered(Value);
}

void AMyCharacter::IA_Confirm_Started()
{
	if (CurrentAbilityComponent)
		CurrentAbilityComponent->IA_Confirm_Started();
}

void AMyCharacter::IA_Cancel_Started()
{
	if (CurrentAbilityComponent)
		CurrentAbilityComponent->IA_Cancel_Started();
}

void AMyCharacter::IA_Break_Started()
{
	if (CurrentAbilityComponent)
		CurrentAbilityComponent->IA_Break_Started();
}
