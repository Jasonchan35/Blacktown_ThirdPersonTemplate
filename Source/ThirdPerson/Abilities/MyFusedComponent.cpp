#include "MyFusedComponent.h"

void MyFuseHelper::FuseActors(AActor* Actor1, AActor* Actor2, const FVector& Point)
{
	if (!Actor1 || !Actor2)
		return;

	auto* Prim1 = Actor1->FindComponentByClass<UPrimitiveComponent>();
	auto* Prim2 = Actor2->FindComponentByClass<UPrimitiveComponent>();

	if (!Prim1 || !Prim2)
		return;

	auto* Fuse1 = MyActorUtil::NewComponent<UMyFusedComponent>(Actor1);
	auto* Fuse2 = MyActorUtil::NewComponent<UMyFusedComponent>(Actor2);

	UMyFusedGroup* Group = nullptr;

	if (Fuse1->FusedGroup)
		Group = Fuse1->FusedGroup;
	else if (Fuse2->FusedGroup)
		Group = Fuse2->FusedGroup;
	else
		Group = NewObject<UMyFusedGroup>();
	
	Group->FusedComponents.Remove(nullptr);

	Group->AddMember(Fuse1);
	Group->AddMember(Fuse2);

	Prim1->SetSimulatePhysics(true);
	Prim2->SetSimulatePhysics(true);

	auto* Constraint = MyActorUtil::NewComponent<UPhysicsConstraintComponent>(Fuse1);

	Constraint->OnConstraintBroken.AddDynamic(Fuse1, &UMyFusedComponent::OnConstraintBroken);
	Constraint->OnConstraintBroken.AddDynamic(Fuse2, &UMyFusedComponent::OnConstraintBroken);

	Constraint->SetWorldLocation(Point);

	Constraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0);
	Constraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0);
	Constraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0);

	Constraint->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
	Constraint->SetAngularTwistLimit( EAngularConstraintMotion::ACM_Locked, 0);
	Constraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
	Constraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);

	Constraint->SetConstrainedComponents(Prim1, NAME_None, Prim2, NAME_None);

	if (!Group->GlueMeshClass)
		return;

	auto* GlueMesh = Actor1->GetWorld()->SpawnActor(Group->GlueMeshClass.Get(), &Point);
	if (!GlueMesh)
		return;

	GlueMesh->AttachToActor(Actor1, FAttachmentTransformRules::KeepWorldTransform);
	Fuse1->GlueMesh = GlueMesh;
}

UMyFusedGroup::UMyFusedGroup()
{
	MY_CDO_FINDER(GlueMeshClass, TEXT("/Game/ThirdPerson/Blueprints/Objects/BP_FusedGlue"));
}

void UMyFusedComponent::OnConstraintBroken(int32 ConstraintIndex)
{
	if (!Constraint)
		return;
	
	this->DestroyComponent();
}

void UMyFusedGroup::AddMember(UMyFusedComponent* Comp)
{
	if (!Comp)
		return;

	if (Comp->FusedGroup == this)
		return;

	if (Comp->FusedGroup)
		Comp->FusedGroup->RemoveMember(Comp);

	Comp->FusedGroup = this;

	FusedComponents.Add(Comp);
}

void UMyFusedGroup::RemoveMember(UMyFusedComponent* Comp)
{
	if (!Comp)
		return;

	if (Comp->FusedGroup != this)
		return;

	Comp->FusedGroup = nullptr;
	FusedComponents.Remove(Comp);
}

void UMyFusedComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);
	if (Constraint)
		Constraint->DestroyComponent();

	if (GlueMesh)
		GlueMesh->Destroy();
}
