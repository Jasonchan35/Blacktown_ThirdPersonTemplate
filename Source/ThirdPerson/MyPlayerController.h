
#include "MyCharacter.h"
#include "MyPlayerController.generated.h"

struct FInputActionValue;

UCLASS()
class AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputMappingContext> InputMappingContext;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> IA_Move;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> IA_Look;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> IA_Jump;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> IA_Cancel;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> IA_Skill;

	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UMaterialInterface> MI_SelectionOverlay;

	UPROPERTY(Category = UI, VisibleAnywhere)
	TSubclassOf<class UMyUIMainWidget>		UIMainWidgetClass;

	UPROPERTY(Category = UI, Transient, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr< class UMyUIMainWidget>		UIMainWidget;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TWeakObjectPtr<AActor>	AimingActor;

public:
	AMyPlayerController();

	FORCEINLINE AMyCharacter* GetCharacter() const { return CastChecked<AMyCharacter>(Super::GetCharacter()); }

protected:
	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void IA_Move_Triggered	(const FInputActionValue& Value);
	void IA_Look_Triggered	(const FInputActionValue& Value);

	void IA_Jump_Started	(const FInputActionValue& Value);
	void IA_Jump_Completed	(const FInputActionValue& Value);

	void IA_Skill_Started	(const FInputActionValue& Value);
	void IA_Cancel_Started	(const FInputActionValue& Value);

private:
	void UpdateAimingActor();

	FTraceDelegate AimingActorAsyncTraceDelegate;
	void AimingActorAsyncTraceResult(const FTraceHandle& TraceHandle, FTraceDatum& Data);

	void SetAimingActor(AActor* Actor);

};