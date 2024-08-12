#include "MyPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

#include "UI/MyUIMainWidget.h"
#include "Animation/SkeletalMeshActor.h"

AMyPlayerController::AMyPlayerController()
{
	MY_CDO_FINDER(InputMappingContext,	TEXT("/Game/ThirdPerson/Input/IMC_Default"));
	MY_CDO_FINDER(IA_Jump,				TEXT("/Game/ThirdPerson/Input/Actions/IA_Jump"));
	MY_CDO_FINDER(IA_Move,				TEXT("/Game/ThirdPerson/Input/Actions/IA_Move"));
	MY_CDO_FINDER(IA_Look,				TEXT("/Game/ThirdPerson/Input/Actions/IA_Look"));
	MY_CDO_FINDER(IA_Confirm,			TEXT("/Game/ThirdPerson/Input/Actions/IA_Confirm"));
	MY_CDO_FINDER(IA_Cancel,			TEXT("/Game/ThirdPerson/Input/Actions/IA_Cancel"));
	MY_CDO_FINDER(IA_AbilityA,			TEXT("/Game/ThirdPerson/Input/Actions/IA_AbilityA"));
	MY_CDO_FINDER(UI_MainWidgetClass,	TEXT("/Game/ThirdPerson/UI/WBP_MyUIMainWidget"));
}

AMyCharacter* AMyPlayerController::GetMyCharacter() const
{
	return CastChecked<AMyCharacter>(Super::GetCharacter());
}

FVector2f AMyPlayerController::GetCrossHairPos()
{
	return UI_MainWidget ? UI_MainWidget->GetCrossHairPos() : FVector2f::ZeroVector;
}

FVector AMyPlayerController::GetCameraLocation()
{
	auto* CamMgr = PlayerCameraManager.Get();
	return CamMgr ? CamMgr->GetCameraLocation() : FVector::ZeroVector;
}

void AMyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	auto* InputComp = CastChecked<UEnhancedInputComponent>(InputComponent);
	if (!InputComp)
	{
		MY_LOG_ERROR(TEXT("PlayerController missing UEnhancedInputComponent"));
		return;
	}

	if (auto* InputSys = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
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
		MY_BIND_INPUT_ACTION(IA_Confirm,	Started);
		MY_BIND_INPUT_ACTION(IA_Cancel,		Started);
		MY_BIND_INPUT_ACTION(IA_AbilityA,	Started);

	#undef MY_BIND_INPUT_ACTION
}

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UI_MainWidget = MyUI::CreateWidget(this, UI_MainWidgetClass);
}

void AMyPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AMyPlayerController::IA_Move_Triggered(const FInputActionValue& Value)
{
	auto* Ch = GetCharacter();
	if (!Ch) return;

	FVector2D MovementVector = Value.Get<FVector2D>();

	// find out which way is forward
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	const FVector ForwardDirection	= FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection	= FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// add movement 
	Ch->AddMovementInput(ForwardDirection, MovementVector.Y);
	Ch->AddMovementInput(RightDirection,   MovementVector.X);
}

void AMyPlayerController::IA_Look_Triggered(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddYawInput(  LookAxisVector.X);
	AddPitchInput(LookAxisVector.Y);
}

void AMyPlayerController::IA_Jump_Started(const FInputActionValue& Value)
{
	auto* Ch = GetMyCharacter();
	if (!Ch) return;
	
	Ch->Jump();
}

void AMyPlayerController::IA_Jump_Completed(const FInputActionValue& Value)
{
	auto* Ch = GetMyCharacter();
	if (!Ch)
		return;

	Ch->StopJumping();
}

void AMyPlayerController::IA_AbilityA_Started(const FInputActionValue& Value)
{
	auto* Ch = GetMyCharacter();
	if (!Ch)
		return;

	Ch->SetCurrentAbility(EMyAbility::UltraHand);
}


void AMyPlayerController::IA_Confirm_Started(const FInputActionValue& Value)
{
	auto* Ch = GetMyCharacter();
	if (!Ch)
		return;

	Ch->IA_Confirm_Started();
}

void AMyPlayerController::IA_Cancel_Started(const FInputActionValue& Value)
{
	auto* Ch = GetMyCharacter();
	if (!Ch)
		return;

	Ch->SetCurrentAbility(EMyAbility::None);
}

