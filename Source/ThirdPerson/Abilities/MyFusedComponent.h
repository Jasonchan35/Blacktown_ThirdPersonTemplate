#pragma once

#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "MyFusedComponent.generated.h"

class UMyFusedGroup;
class AMyFusedGlue;

struct MyFuseHelper
{
	static void FuseActors(	AActor* Actor1, const FVector& RefPoint1,
							AActor* Actor2, const FVector& RefPoint2);

	static void AddGroupMember(UMyFusedGroup* Group, UMyFusedComponent* Comp);
	static void RemoveGroupMember(UMyFusedGroup* Group, UMyFusedComponent* Comp);

	static void MergeGroup(UMyFusedGroup* Group, UMyFusedGroup* FromGroup);
	static void SeparateIslandGroup(UMyFusedGroup* Group);

	static UMyFusedGroup* FindGroup(AActor* Actor);

	static void SetActorState(AActor* Actor, bool SimulatePhysics, UMaterialInterface* OverlayMaterial);
	static void SetActorTransform(AActor* Actor, const FTransform& NewTran);

	static void BreakFusedActor(AActor* Actor);

	static void DestroyGlue(AMyFusedGlue* Glue);
	static void DestroyGluesInFuseComponent(UMyFusedComponent* Fuse);
	static void DestroyFuseComponent(UMyFusedComponent* Fuse);
};

UCLASS()
class AMyFusedGlue : public AActor
{
	GENERATED_BODY()

public:
	AMyFusedGlue();
	
friend struct MyFuseHelper;
protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginDestroy() override;

	UFUNCTION()
	void OnConstraintBroken(int32 ConstraintIndex);

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPhysicsConstraintComponent>	Constraint;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMyFusedGroup>	Group;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMyFusedComponent>	Fuse1;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMyFusedComponent>	Fuse2;
};

UCLASS()
class UMyFusedGroup : public UObject
{
	GENERATED_BODY()
public:	
	UMyFusedGroup();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AMyFusedGlue>	GlueActorClass;

	TArrayView<UMyFusedComponent*>	GetMembers() { return Members; }

	bool HasMember(AActor* Actor);

friend struct MyFuseHelper;
protected:

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TArray<UMyFusedComponent*>	Members;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TArray<AMyFusedGlue*>	Glues;
};

UCLASS()
class UMyFusedComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UMyFusedGroup*	GetFusedGroup() { return FusedGroup.Get(); }

friend class AMyFusedGlue;
friend struct MyFuseHelper;
protected:

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMyFusedGroup>	FusedGroup;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TArray<AMyFusedGlue*>		Glues;

	bool TempVisited;
};