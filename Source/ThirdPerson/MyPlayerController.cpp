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
	MY_CDO_FINDER(IA_Skill,				TEXT("/Game/ThirdPerson/Input/Actions/IA_Skill"));
	MY_CDO_FINDER(UIMainWidgetClass,	TEXT("/Game/ThirdPerson/UI/WBP_MyUIMainWidget"));
	MY_CDO_FINDER(MI_SelectionOverlay,	TEXT("/Game/ThirdPerson/Materials/M_SelectionOverlay"));
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

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UIMainWidget = MyUI::CreateWidget(this, UIMainWidgetClass);
}

void AMyPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TraceAimingActor();
}

void AMyPlayerController::TraceAimingActor()
{
	if (!UIMainWidget)
		return;

	if (!PlayerCameraManager)
		return;

	auto* Ch = GetCharacter();
	if (!Ch)
		return;

	auto CameraLoc = PlayerCameraManager->GetCameraLocation();

	auto Pos = UIMainWidget->GetCrossHairPos();

	auto LineStart = CameraLoc;
	auto LineEnd   = LineStart + GetControlRotation().Vector() * 1000.0;

	if (!TraceAimingActorDelegate.IsBound()) {
		TraceAimingActorDelegate.BindUObject(this, &ThisClass::TraceAimingActorResult);
	}

	FCollisionQueryParams	ObjectQueryParams;
	ObjectQueryParams.AddIgnoredActor(Ch);

	FCollisionObjectQueryParams Params;
	Params.AddObjectTypesToQuery(ECC_WorldDynamic);
	Params.AddObjectTypesToQuery(ECC_PhysicsBody);
	Params.AddObjectTypesToQuery(ECC_Pawn);

//	DrawDebugLine(GetWorld(), LineStart, LineEnd, FColor::Red, false, 5.0f, 0, 0);

	GetWorld()->AsyncLineTraceByObjectType(	EAsyncTraceType::Single,
											LineStart,
											LineEnd, 
											Params,
											ObjectQueryParams,
											&TraceAimingActorDelegate);
}

void AMyPlayerController::TraceAimingActorResult(const FTraceHandle& TraceHandle, FTraceDatum& Data)
{
	auto* Actor = Data.OutHits.Num() > 0 ? Data.OutHits[0].GetActor() : nullptr;
	SetAimingActor(Actor);
}

void AMyPlayerController::SetAimingActor(AActor* Actor)
{
	auto* Cur = AimingActor.Get();
	if (Cur == Actor)
		return;

	if (Cur)
		MyActorUtil::SetOverlayMaterial(Cur, nullptr);

	AimingActor = Actor;

	if (Actor)
		MyActorUtil::SetOverlayMaterial(Actor, MI_SelectionOverlay);
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
	auto* Ch = GetCharacter();
	if (!Ch) return;

	Ch->UltraHandPickActor(AimingActor.Get());
}

void AMyPlayerController::IA_Skill_Completed(const FInputActionValue& Value)
{
}

