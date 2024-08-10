#include "MyPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

AMyPlayerController::AMyPlayerController()
{

	MY_OBJECT_FINDER(InputMappingContext,
		TEXT("/Game/ThirdPerson/Input/IMC_Default"));

	MY_OBJECT_FINDER(IA_Jump,		TEXT("/Game/ThirdPerson/Input/Actions/IA_Jump"));
	MY_OBJECT_FINDER(IA_Move,		TEXT("/Game/ThirdPerson/Input/Actions/IA_Move"));
	MY_OBJECT_FINDER(IA_Look,		TEXT("/Game/ThirdPerson/Input/Actions/IA_Look"));
	MY_OBJECT_FINDER(IA_Skill,		TEXT("/Game/ThirdPerson/Input/Actions/IA_Skill"));
}

void AMyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	auto* InputComp = CastChecked<UEnhancedInputComponent>(InputComponent);
	if (!InputComp) {
		MY_LOG_ERROR(TEXT("PlayerController missing UEnhancedInputComponent"));
		return;
	}

	if (auto* InputSys = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer())) {
		InputSys->AddMappingContext(InputMappingContext, 0);
	}

	auto BindAction = [&](const TCHAR* ActionName, UInputAction* Action, ETriggerEvent Event, void (ThisClass::*Func)(const FInputActionValue& Value)) {
		if (!Action)
			MY_LOG_ERROR(TEXT("InputAction {} is null"), ActionName);
		else
			InputComp->BindAction(Action, Event, this, Func);
	};

	#define MY_BIND_INPUT_ACTION(Action, Event) \
		BindAction(TEXT(#Action), Action, ETriggerEvent::Event, &ThisClass::Action##_##Event); \
	//---
		MY_BIND_INPUT_ACTION(IA_Move,		Triggered);
		MY_BIND_INPUT_ACTION(IA_Look,		Triggered);
		MY_BIND_INPUT_ACTION(IA_Jump,		Started);
		MY_BIND_INPUT_ACTION(IA_Jump,		Completed);
		MY_BIND_INPUT_ACTION(IA_Skill,		Started);
		MY_BIND_INPUT_ACTION(IA_Skill,		Completed);

	#undef MY_BIND_INPUT_ACTION
}

void AMyPlayerController::IA_Move_Triggered(const FInputActionValue& Value)
{
	auto* Ch = GetCharacter();
	if (!Ch) return;

	FVector2D MovementVector = Value.Get<FVector2D>();

	// find out which way is forward
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	// get forward vector
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
	// get right vector 
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// add movement 
	Ch->AddMovementInput(ForwardDirection, MovementVector.Y);
	Ch->AddMovementInput(RightDirection, MovementVector.X);
}

void AMyPlayerController::IA_Look_Triggered(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddYawInput(LookAxisVector.X);
	AddPitchInput(LookAxisVector.Y);
}

void AMyPlayerController::IA_Jump_Started(const FInputActionValue& Value)
{
	auto* Ch = GetCharacter();
	if (!Ch) return;
	
	Ch->Jump();
}

void AMyPlayerController::IA_Jump_Completed(const FInputActionValue& Value)
{
	auto* Ch = GetCharacter();
	if (!Ch) return;

	Ch->StopJumping();
}

void AMyPlayerController::IA_Skill_Started(const FInputActionValue& Value)
{
}

void AMyPlayerController::IA_Skill_Completed(const FInputActionValue& Value)
{
}
