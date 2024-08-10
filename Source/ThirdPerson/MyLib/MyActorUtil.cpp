#include "MyActorUtil.h"

void MyActorUtil::SetOverlayMaterial(AActor* Actor, UMaterialInterface* Material)
{
	if (!Actor)
		return;

	auto* MeshComp = Actor->FindComponentByClass<UMeshComponent>();
	if (!MeshComp)
		return;

	MeshComp->SetOverlayMaterial(Material);
}

UMaterialInterface* MyActorUtil::GetOverlayMaterial(AActor* Actor)
{
	if (!Actor)
		return nullptr;

	auto* MeshComp = Actor->FindComponentByClass<UMeshComponent>();
	if (!MeshComp)
		return nullptr;

	return MeshComp->GetOverlayMaterial();
}

void MyActorUtil::SetMaterial(AActor* Actor, int ElementIndex, UMaterialInterface* Material)
{
	if (Actor)
		return;

	auto* PrimComp = Actor->FindComponentByClass<UPrimitiveComponent>();
	if (!PrimComp)
		return;

	PrimComp->SetMaterial(ElementIndex, Material);
}

UMaterialInterface* MyActorUtil::GetMaterial(AActor* Actor, int ElementIndex)
{
	if (!Actor)
		return nullptr;

	auto* PrimComp = Actor->FindComponentByClass<UPrimitiveComponent>();
	if (!PrimComp)
		return nullptr;

	return PrimComp->GetMaterial(ElementIndex);
}


UMaterialInstanceDynamic* MyActorUtil::CreateDynamicMaterialInstance(AActor* Actor, int ElementIndex)
{
	if (!Actor)
		return nullptr;

	auto* PrimComp = Actor->FindComponentByClass<UPrimitiveComponent>();
	if (!PrimComp)
		return nullptr;

	return PrimComp->CreateDynamicMaterialInstance(ElementIndex);
}

void MyActorUtil::SetEnableGravity(AActor* Actor, bool bEnable)
{
	if (!Actor)
		return;

	auto* PrimComp = Actor->FindComponentByClass<UPrimitiveComponent>();
	if (!PrimComp)
		return;

	PrimComp->SetEnableGravity(bEnable);
}

bool MyActorUtil::IsGravityEnabled(AActor* Actor)
{
	if (!Actor)
		return false;

	auto* PrimComp = Actor->FindComponentByClass<UPrimitiveComponent>();
	if (!PrimComp)
		return false;

	return PrimComp->IsGravityEnabled();
}
