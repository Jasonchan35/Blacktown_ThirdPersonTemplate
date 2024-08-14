#pragma once

#include "MyCharacter.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

struct FInputActionValue;

UCLASS()
class AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputMappingContext> InputMappingContext;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> IA_DPad;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> IA_Move;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> IA_Look;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> IA_Jump;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> IA_Confirm;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> IA_Cancel;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> IA_Break;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> IA_AbilityA;

	UPROPERTY(Category = UI, VisibleAnywhere)
	TSubclassOf<class UMyUIMainWidget> UI_MainWidgetClass;

	UPROPERTY(Category = UI, Transient, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr< class UMyUIMainWidget> UI_MainWidget;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TWeakObjectPtr<AActor>	AimingActor;

public:
	AMyPlayerController();

	AMyCharacter* GetMyCharacter() const;

	FVector2f	GetCrossHairPos();
	FVector		GetCameraLocation();

protected:
	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void IA_DPad_Triggered	(const FInputActionValue& Value);
	void IA_Move_Triggered	(const FInputActionValue& Value);
	void IA_Look_Triggered	(const FInputActionValue& Value);

	void IA_Jump_Started	(const FInputActionValue& Value);
	void IA_Jump_Completed	(const FInputActionValue& Value);

	void IA_Confirm_Started	(const FInputActionValue& Value);
	void IA_Cancel_Started	(const FInputActionValue& Value);

	void IA_Break_Started	(const FInputActionValue& Value);

	void IA_AbilityA_Started(const FInputActionValue& Value);
};