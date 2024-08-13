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
		if (Member == Actor)
			return true;
	}

	return false;
}

void MyFuseHelper::FuseActors(	AActor* Actor1, const FVector& RefPoint1,
								AActor* Actor2, const FVector& RefPoint2)
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

	Constraint->SetWorldLocation(RefPoint1);

	Constraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0);
	Constraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0);
	Constraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0);

	Constraint->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
	Constraint->SetAngularTwistLimit( EAngularConstraintMotion::ACM_Locked, 0);
	Constraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
	Constraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);

	Constraint->SetConstrainedComponents(Prim1, NAME_None, Prim2, NAME_None);

	auto ReletiveRefPoint1 = Actor1->GetActorTransform().InverseTransformPosition(RefPoint1);
	auto ReletiveRefPoint2 = Actor2->GetActorTransform().InverseTransformPosition(RefPoint2);

	Constraint->SetConstraintReferencePosition(EConstraintFrame::Frame1, ReletiveRefPoint1);
	Constraint->SetConstraintReferencePosition(EConstraintFrame::Frame2, ReletiveRefPoint2);

	Constraint->UpdateConstraintFrames();

	if (!Group->GlueMeshClass)
		return;

	auto* GlueMesh = Actor1->GetWorld()->SpawnActor(Group->GlueMeshClass.Get(), &RefPoint1);
	if (!GlueMesh)
		return;

	GlueMesh->AttachToActor(Actor1, FAttachmentTransformRules::KeepWorldTransform);

	if (auto* GluePrim = GlueMesh->FindComponentByClass<UPrimitiveComponent>())
	{
		GluePrim->SetEnableGravity(false);
		GluePrim->SetSimulatePhysics(false);
		GluePrim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	NewFuse1->GlueMesh = GlueMesh;
}

void MyFuseHelper::SetActorState(AActor* Actor, bool SimulatePhysics, UMaterialInterface* OverlayMaterial)
{
	auto SetToActor = [](AActor* Actor, bool SimulatePhysics, UMaterialInterface* OverlayMaterial) -> void
	{
		if (!Actor)
			return;

		auto* Prim = Actor->FindComponentByClass<UPrimitiveComponent>();
		if (!Prim)
			return;

		Prim->SetSimulatePhysics(SimulatePhysics);
		Prim->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Prim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

		MyActorUtil::SetOverlayMaterial(Actor, OverlayMaterial);
	};

	if (!Actor)
		return;

	SetToActor(Actor, SimulatePhysics, OverlayMaterial);

	if (auto* Group = MyFuseHelper::FindGroup(Actor))
	{
		for (auto* Member : Group->GetMembers())
		{
			if (Member && Member != Actor)
				SetToActor(Member, SimulatePhysics, OverlayMaterial);
		}
	}
}

void MyFuseHelper::SetActorTransform(AActor* Actor, const FTransform& NewTran)
{
	auto SetToActor = [](AActor* Actor, const FTransform& NewTran) -> void
	{
		if (!Actor)
			return;

		Actor->SetActorTransform(NewTran, false, nullptr, ETeleportType::TeleportPhysics);
		if (auto* PrimComp = Actor->FindComponentByClass<UPrimitiveComponent>())
		{
			PrimComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
			PrimComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
	};

	if (!Actor)
		return;

	auto& ActorTran = Actor->GetActorTransform();
	auto InvActorTran = ActorTran.Inverse();
	auto ReletiveTran = InvActorTran * NewTran;

	SetToActor(Actor, NewTran);

	if (auto* Group = MyFuseHelper::FindGroup(Actor))
	{
		for (auto* Member : Group->GetMembers())
		{
			if (Member && Member != Actor)
				SetToActor(Member, Member->GetActorTransform() * ReletiveTran);
		}
	}
}

UMyFusedGroup::UMyFusedGroup()
{
	MY_CDO_FINDER(GlueMeshClass, TEXT("/Game/ThirdPerson/Blueprints/Objects/BP_FusedGlue"));
}

bool UMyFusedGroup::HasMember(AActor* Actor)
{
	if (!Actor)
		return false;

	for (auto& Member : Members)
	{
		if (Member == Actor)
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
	Group->Members.AddUnique(Comp->GetOwner());
}

void MyFuseHelper::RemoveMember(UMyFusedGroup* Group, UMyFusedComponent* Comp)
{
	if (!Comp)
		return;

	if (Comp->FusedGroup != Group)
		return;

	Comp->FusedGroup = nullptr;
	Group->Members.Remove(Comp->GetOwner());
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
