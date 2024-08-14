#pragma once

#include "MyAbilityComponent.h"
#include "../MyPlayerController.h"
#include "MyUltraHandComponent.generated.h"

UENUM()
enum class EMyUltraHandMode
{
	None,
	SearchTarget,
	GrabTarget,
};

UCLASS()
class UMyUltraHandComponent : public UMyAbilityComponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float		SearchTargetRadius;

	UPROPERTY(EditAnywhere, Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float		MaxFusableDistance;

	UPROPERTY(EditAnywhere, Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float		GrabTargetDampingFactor;

	UPROPERTY(EditAnywhere, Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float		GrabTargetDistanceMin;

	UPROPERTY(EditAnywhere, Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float		GrabTargetDistanceMax;

	UPROPERTY(EditAnywhere, Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float		GrabTargetHeightMax;

	UPROPERTY(EditAnywhere, Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float		FuseTargetDampingFactor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UMaterialInterface> MI_GrabbableOverlay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UMaterialInterface> MI_FusableOverlay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UNiagaraSystem>	FXS_GrabTarget;

public:
	UMyUltraHandComponent();

	void	SetTargetActor(AActor* Actor);
	void	SetAbilityActive(bool Active);

	AActor* GetTargetActor() { return TargetActor.Get(); }

	bool	OnLookPitchInputAdded(float Pitch);
	EMyUltraHandMode GetMode() const { return Mode; }

	bool	HasFusable() const { return bool(Fusable); }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	virtual void IA_DPad_Triggered(const FVector2D& Value) override;
	virtual void IA_Confirm_Started() override;
	virtual void IA_Cancel_Started() override;
	virtual void IA_Break_Started() override;

private:
	TWeakObjectPtr<AActor>	TargetActor;

	EMyUltraHandMode Mode = EMyUltraHandMode::None;

// Search Target Mode
	void SetSearchTargetMode();

	void TickSearchTarget(float DeltaTime);
	bool DoTickSearchTarget(float DeltaTime);

	FTraceDelegate SearchTargetAsyncTraceDelegate;
	void SearchTargetAsyncTraceResult(const FTraceHandle& TraceHandle, FTraceDatum& Data);

// Hold Target Mode
	void SetGrabTargetMode();

	void TickGrabTarget(float DeltaTime);
	bool DoTickGrabTarget(float DeltaTime);
	bool MoveTargetLocation(float DeltaTime);

	FTraceDelegate MoveTargetAsyncDelegate;
	void MoveTargetAsyncResult(const FTraceHandle& TraceHandle, FTraceDatum& Data);

	struct FMoveTargetAsyncData
	{
		int		Count = 0;
		float	MinDistance = 0;
		FVector	Direction;
		FQuat	RelativeQuat;
	};
	FMoveTargetAsyncData	MoveTargetAsyncData;

	void MoveTargetForward(float V);

	FQuat	GrabTargetQuat;
	FVector	GrabTargetVector;

	TWeakObjectPtr<class UNiagaraComponent> NiagaraComponent;

	struct FFusable
	{
		FFusable() { Reset(); }
		void Reset() { DestActor = nullptr; }

		explicit operator bool() const { return DestActor.Get() != nullptr; }

		TWeakObjectPtr<AActor>	SourceActor;
		TWeakObjectPtr<AActor>	DestActor;

		FVector		Location;
		float		Distance;
		FVector		SourcePoint;
		FVector		ImpactPoint;
	};

	FFusable	Fusable;

	bool DoSearchFusable();
	void ResetSearchFusableTempOverlaps();

	TArray<FOverlapResult>	SearchFusableTempOverlaps;
	FFusable				SearchFusableAsyncData;
	FTraceDelegate			SearchFusableAsyncDelegate;
	void SearchFusableAsyncResult(const FTraceHandle& TraceHandle, FTraceDatum& Data);

	bool AcceptFusableTarget();
};

