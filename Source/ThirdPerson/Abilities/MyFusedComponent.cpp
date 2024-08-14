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
		if (Member && Member->GetOwner() == Actor)
			return true;
	}

	return false;
}

void MyFuseHelper::FuseActors(	AActor* Actor1, const FVector& RefPoint1,
								AActor* Actor2, const FVector& RefPoint2)
{
	if (!Actor1 || !Actor2)
		return;

	auto* Fuse1 = MyActorUtil::GetOrNewComponent<UMyFusedComponent>(Actor1);
	auto* Fuse2 = MyActorUtil::GetOrNewComponent<UMyFusedComponent>(Actor2);

	UMyFusedGroup* Group = nullptr;
	{ // Pick Group from FusedComponents
		auto* Group1 = Fuse1->FusedGroup.Get();
		auto* Group2 = Fuse2->FusedGroup.Get();

		if (Group1 && Group2)
		{
			MergeGroup(Group1, Group2);
			Group  = Group1;
		}
		else if (Group1)
			Group = Group1;
		else if (Group2)
			Group = Group2;
		else
			Group = NewObject<UMyFusedGroup>();
	}

	if (!Group->GlueActorClass)
		return;

	AddGroupMember(Group, Fuse1);
	AddGroupMember(Group, Fuse2);

	auto* Prim1 = Actor1->FindComponentByClass<UPrimitiveComponent>();
	auto* Prim2 = Actor2->FindComponentByClass<UPrimitiveComponent>();

	if (!Prim1 || !Prim2)
		return;

	auto* World = Actor1->GetWorld();

	auto* NewGlue = World->SpawnActor<AMyFusedGlue>(Group->GlueActorClass.Get(), RefPoint1, FRotator::ZeroRotator);
	if (!NewGlue)
		return;
	
	auto& Constraint = NewGlue->Constraint;

	Constraint->SetWorldLocation(RefPoint1);
	Constraint->SetConstrainedComponents(Prim1, NAME_None, Prim2, NAME_None);

	auto ReletiveRefPoint1 = Actor1->GetActorTransform().InverseTransformPosition(RefPoint1);
	auto ReletiveRefPoint2 = Actor2->GetActorTransform().InverseTransformPosition(RefPoint2);

	Constraint->SetConstraintReferencePosition(EConstraintFrame::Frame1, ReletiveRefPoint1);
	Constraint->SetConstraintReferencePosition(EConstraintFrame::Frame2, ReletiveRefPoint2);

	Constraint->UpdateConstraintFrames();

	NewGlue->AttachToActor(Actor1, FAttachmentTransformRules::KeepWorldTransform);

	NewGlue->Fuse1 = Fuse1;
	NewGlue->Fuse2 = Fuse2;
	NewGlue->Group = Group;

	Group->Glues.Add(NewGlue);
	Fuse1->Glues.Add(NewGlue);
	Fuse2->Glues.Add(NewGlue);
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
			if (!Member)
				continue;

			auto* P = Member->GetOwner();
			if (P != Actor)
				SetToActor(P, SimulatePhysics, OverlayMaterial);
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
			if (!Member)
				continue;
			auto* P = Member->GetOwner();
			if (P != Actor)
				SetToActor(P, P->GetActorTransform() * ReletiveTran);
		}
	}
}

void MyFuseHelper::BreakFusedActor(AActor* Actor)
{
	if (!Actor)
		return;

	auto* Fuse = Actor->FindComponentByClass<UMyFusedComponent>();
	if (!Fuse)
		return;

	UMaterialInterface* OverlayMaterial = MyActorUtil::GetOverlayMaterial(Actor);

	SetActorState(Actor, true, nullptr);
	DestroyGluesInFuseComponent(Fuse);
	SetActorState(Actor, true, OverlayMaterial);
}

void MyFuseHelper::DestroyFuseComponent(UMyFusedComponent* Fuse)
{
	if (!Fuse)
		return;

	DestroyGluesInFuseComponent(Fuse);

	if (auto* Group = Fuse->FusedGroup.Get())
	{
		RemoveGroupMember(Group, Fuse);
		SeparateIslandGroup(Group);
	}

	if (!Fuse->IsBeingDestroyed())
		Fuse->DestroyComponent();
}

void MyFuseHelper::DestroyGluesInFuseComponent(UMyFusedComponent* Fuse)
{
	if (!Fuse)
		return;

	int GlueCount = Fuse->Glues.Num();
	for (int i = 0; i < GlueCount; i++)
	{
		if (Fuse->Glues.Num() <= 0)
			break;

		DestroyGlue(Fuse->Glues.Last());
	}
}

void MyFuseHelper::DestroyGlue(AMyFusedGlue* Glue)
{
	if (!Glue)
		return;

	auto Func = [](AMyFusedGlue* Glue, UMyFusedComponent* Fuse) -> void
	{
		if (!Fuse)
			return;

		Fuse->Glues.Remove(Glue);
		Fuse->Glues.Remove(nullptr);
		if (Fuse->Glues.Num() <= 0)
			DestroyFuseComponent(Fuse);
	};

	Func(Glue, Glue->Fuse1);
	Func(Glue, Glue->Fuse2);

	if (!Glue->IsActorBeingDestroyed())
		Glue->Destroy();
}

AMyFusedGlue::AMyFusedGlue()
{
	MyCDO::CreateComponent(this, Constraint);
	SetRootComponent(Constraint);

	Constraint->SetDisableCollision(true);

	Constraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0);
	Constraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0);
	Constraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0);

	Constraint->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
	Constraint->SetAngularTwistLimit( EAngularConstraintMotion::ACM_Locked, 0);
	Constraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
	Constraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
}

void AMyFusedGlue::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	Constraint->OnConstraintBroken.AddDynamic(this, &ThisClass::OnConstraintBroken);

	if (auto* Prim = FindComponentByClass<UPrimitiveComponent>())
	{
		Prim->SetEnableGravity(false);
		Prim->SetSimulatePhysics(false);
		Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AMyFusedGlue::BeginDestroy()
{
	MyFuseHelper::DestroyGlue(this);
	Super::BeginDestroy();
}

void AMyFusedGlue::OnConstraintBroken(int32 ConstraintIndex)
{
	this->Destroy();
}

UMyFusedGroup::UMyFusedGroup()
{
	MY_CDO_FINDER(GlueActorClass, TEXT("/Game/ThirdPerson/Blueprints/Objects/BP_FusedGlue"));
}

bool UMyFusedGroup::HasMember(AActor* Actor)
{
	if (!Actor)
		return false;

	for (auto& Member : Members)
	{
		if (Member && Member->GetOwner() == Actor)
			return true;
	}

	return false;
}

void MyFuseHelper::MergeGroup(UMyFusedGroup* Group, UMyFusedGroup* FromGroup)
{
	if (!Group || !FromGroup || FromGroup == Group)
		return;

	while (FromGroup->Members.Num() > 0)
	{
		AddGroupMember(Group, FromGroup->Members.Last());
	}
}

void MyFuseHelper::SeparateIslandGroup(UMyFusedGroup* Group)
{
	if (!Group)
		return;

	Group->Members.Remove(nullptr);

	for (auto& Fuse : Group->Members)
		Fuse->TempVisited = 0;

	TArray<UMyFusedComponent*, TInlineAllocator<64>>	PendingList;
	TArray<UMyFusedComponent*, TInlineAllocator<64>>	VisitedList;

	while (Group->Members.Num() > 0) {
		PendingList.Reset();
		VisitedList.Reset();

		PendingList.Add(Group->Members[0]);
		// scan	reachable from Members[0]

		while (PendingList.Num() > 0)
		{
			auto* Fuse = PendingList.Pop(EAllowShrinking::No);
			Fuse->TempVisited = true;
			VisitedList.Add(Fuse);

			for (auto& Glue : Fuse->Glues)
			{
				if (!Glue)
					continue;

				auto Fuse1 = Glue->Fuse1;
				auto Fuse2 = Glue->Fuse2;

				if (Fuse1 && !Fuse1->TempVisited)
				{
					Fuse1->TempVisited = true;
					PendingList.Add(Fuse1);
				}

				if (Fuse2 && !Fuse2->TempVisited)
				{
					Fuse2->TempVisited = true;
					PendingList.Add(Fuse2);
				}
			}
		}

		if (VisitedList.Num() == Group->Members.Num())
			return; // no more separate group

		auto* NewGroup = NewObject<UMyFusedGroup>();

		NewGroup->Members.Reserve(VisitedList.Num());

		for (auto* Fuse : VisitedList)
		{
			AddGroupMember(NewGroup, Fuse);
		}
	}

}

void MyFuseHelper::AddGroupMember(UMyFusedGroup* Group, UMyFusedComponent* Comp)
{
	if (!Comp)
		return;

	if (Comp->FusedGroup == Group)
		return;

	if (Comp->FusedGroup)
		RemoveGroupMember(Comp->FusedGroup, Comp);

	Comp->FusedGroup = Group;
	Group->Members.Add(Comp);
}

void MyFuseHelper::RemoveGroupMember(UMyFusedGroup* Group, UMyFusedComponent* Comp)
{
	if (!Group || !Comp)
		return;

	if (Comp->FusedGroup != Group)
		return;

	Comp->FusedGroup = nullptr;
	Group->Members.Remove(Comp);
}

void UMyFusedComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	MyFuseHelper::DestroyFuseComponent(this);
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}
