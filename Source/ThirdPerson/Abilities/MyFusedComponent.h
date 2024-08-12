#pragma once

#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "MyFusedComponent.generated.h"

class UMyFusedGroup;

UCLASS()
class AMyInteractableActor : public AActor
{
	GENERATED_BODY()
public:	
};

struct MyFuseHelper
{
	static void FuseActors(AActor* Actor1, AActor* Actor2, const FVector& Point);
	static void AddMember(UMyFusedGroup* Group, UMyFusedComponent* Comp);
	static void RemoveMember(UMyFusedGroup* Group, UMyFusedComponent* Comp);
	static UMyFusedGroup* FindGroup(AActor* Actor);

	static bool MatchActorOrGroup(AActor* Actor, AActor* ActorOrGroup);
	static bool MatchGroup(AActor* Actor, UMyFusedGroup* Group);
};

UCLASS()
class UMyFusedGroup : public UObject
{
	GENERATED_BODY()
public:	
	UMyFusedGroup();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AActor>	GlueMeshClass;

	TArrayView<UMyFusedComponent*>	GetMembers() { return Members; }

	bool	HasMember(AActor* Actor);

friend struct MyFuseHelper;
protected:


	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TArray<UMyFusedComponent*>	Members;
};

UCLASS()
class UMyFusedComponent : public USceneComponent
{
	GENERATED_BODY()

public:

	UMyFusedGroup*	GetFusedGroup() { return FusedGroup.Get(); }

friend struct MyFuseHelper;
protected:
	UFUNCTION()
	void OnConstraintBroken(int32 ConstraintIndex);

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMyFusedGroup>	FusedGroup;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPhysicsConstraintComponent>	Constraint;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor>	GlueMesh;
};