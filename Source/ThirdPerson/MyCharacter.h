// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "MyCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

class AMyPlayerController;
class UMyAbilityComponent;
class UMyUltraHandComponent;

UENUM()
enum class EMyAbility
{
	None,
	UltraHand,
};

UCLASS(config=Game)
class AMyCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(Category = Ability, VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMyUltraHandComponent>	UltraHandComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UNiagaraComponent> NiagaraComponent;

public:
	AMyCharacter();
	void		SetCurrentAbility(EMyAbility Ability);
	EMyAbility	GetCurrentAbility() const { return CurrentAbility; }
	UMyAbilityComponent* GetCurrentAbilityComponent() { return CurrentAbilityComponent.Get(); }

	AMyPlayerController* GetPlayerController();

	class UNiagaraComponent* GetNiagaraComponent() { return NiagaraComponent.Get(); }


	void IA_DPad_Triggered(const FVector2D& Value);
	void IA_Confirm_Started();
	void IA_Cancel_Started();
	void IA_Break_Started();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
	TObjectPtr<UMyAbilityComponent>	CurrentAbilityComponent;
	EMyAbility	CurrentAbility = EMyAbility::None;
};
