#pragma once

#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "MyFusedComponent.generated.h"

UCLASS()
class AMyInteractableActor : public AActor
{
	GENERATED_BODY()
public:	
};

struct MyFuseHelper
{
	static void FuseActors(AActor* Actor1, AActor* Actor2, const FVector& Point);

};

UCLASS()
class UMyFusedGroup : public UObject
{
	GENERATED_BODY()
public:	
	UMyFusedGroup();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AActor>	GlueMeshClass;

friend struct MyFuseHelper;
protected:

	void AddMember(UMyFusedComponent* Comp);
	void RemoveMember(UMyFusedComponent* Comp);

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TArray<UMyFusedComponent*>	FusedComponents;
};

UCLASS()
class UMyFusedComponent : public USceneComponent
{
	GENERATED_BODY()

public:

friend struct MyFuseHelper;
friend class UMyFusedGroup;
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