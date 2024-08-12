#include "MyFusedComponent.h"

UMyFusedGroup* MyFuseHelper::FindGroup(AActor* Actor)
{
	if (!Actor)
		return nullptr;

	auto* Fuse = Actor->FindComponentByClass<UMyFusedComponent>();
	if (!Fuse)
		return nullptr;

	return Fuse->GetFusedGroup();
}

bool MyFuseHelper::MatchActorOrGroup(AActor* Actor, AActor* ActorOrGroup)
{
	if (!Actor || !ActorOrGroup)
		return false;

	if (Actor == ActorOrGroup)
		return true;

	return MatchGroup(Actor, FindGroup(ActorOrGroup));
}

bool MyFuseHelper::MatchGroup(AActor* Actor, UMyFusedGroup* Group)
{
	if (!Group)
		return false;

	for (auto& Member : Group->GetMembers())
	{
		if (Member)
		{
			if (Member->GetOwner() == Actor)
				return true;
		}
	}

	return false;
}

void MyFuseHelper::FuseActors(AActor* Actor1, AActor* Actor2, const FVector& Point)
{
	if (!Actor1 || !Actor2)
		return;

	auto* Prim1 = Actor1->FindComponentByClass<UPrimitiveComponent>();
	auto* Prim2 = Actor2->FindComponentByClass<UPrimitiveComponent>();

	if (!Prim1 || !Prim2)
		return;

	UMyFusedGroup* Group = nullptr;

	{ // pick exists group if possible
		auto* Group1 = FindGroup(Actor1);
		auto* Group2 = FindGroup(Actor2);

		if (Group1)
			Group = Group1;
		else if (Group2)
			Group = Group2;
		else
			Group = NewObject<UMyFusedGroup>();	
	}

	Group->Members.Remove(nullptr);

	auto* NewFuse1 = MyActorUtil::NewComponent<UMyFusedComponent>(Actor1);
	auto* NewFuse2 = MyActorUtil::NewComponent<UMyFusedComponent>(Actor2);

	AddMember(Group, NewFuse1);
	AddMember(Group, NewFuse2);

	//Prim1->SetSimulatePhysics(true);
	//Prim2->SetSimulatePhysics(true);

	auto* Constraint = MyActorUtil::NewComponent<UPhysicsConstraintComponent>(NewFuse1);

	Constraint->SetDisableCollision(true);

	Constraint->OnConstraintBroken.AddDynamic(NewFuse1, &UMyFusedComponent::OnConstraintBroken);
	Constraint->OnConstraintBroken.AddDynamic(NewFuse2, &UMyFusedComponent::OnConstraintBroken);

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

	if (auto* GluePrim = GlueMesh->FindComponentByClass<UPrimitiveComponent>())
	{
		GluePrim->SetEnableGravity(false);
		GluePrim->SetSimulatePhysics(false);
	}

	NewFuse1->GlueMesh = GlueMesh;
}

UMyFusedGroup::UMyFusedGroup()
{
	MY_CDO_FINDER(GlueMeshClass, TEXT("/Game/ThirdPerson/Blueprints/Objects/BP_FusedGlue"));
}

bool UMyFusedGroup::HasMember(AActor* Actor)
{
	if (!Actor)
		return false;

	for (auto& Comp : Members)
	{
		if (Comp && Comp->GetOwner() == Actor)
			return true;
	}

	return false;
}

void UMyFusedComponent::OnConstraintBroken(int32 ConstraintIndex)
{
	this->DestroyComponent();
}

void MyFuseHelper::AddMember(UMyFusedGroup* Group, UMyFusedComponent* Comp)
{
	if (!Comp)
		return;

	if (Comp->FusedGroup == Group)
		return;

	if (Comp->FusedGroup)
		RemoveMember(Comp->FusedGroup, Comp);

	Comp->FusedGroup = Group;
	Group->Members.Add(Comp);
}

void MyFuseHelper::RemoveMember(UMyFusedGroup* Group, UMyFusedComponent* Comp)
{
	if (!Comp)
		return;

	if (Comp->FusedGroup != Group)
		return;

	Comp->FusedGroup = nullptr;
	Group->Members.Remove(Comp);
}

void UMyFusedComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);

	if (FusedGroup)
		MyFuseHelper::RemoveMember(FusedGroup, this);

	if (Constraint)
		Constraint->DestroyComponent();

	if (GlueMesh)
		GlueMesh->Destroy();
}
