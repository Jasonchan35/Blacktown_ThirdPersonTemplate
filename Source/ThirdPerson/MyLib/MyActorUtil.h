#pragma once

class AActor;
class UMaterialInterface;
class UMaterialInstanceDynamic;

struct MyActorUtil
{
	MyActorUtil() = delete;

	static void					SetOverlayMaterial(AActor* Actor, UMaterialInterface* Material);
	static UMaterialInterface*	GetOverlayMaterial(AActor* Actor);

	static void					SetMaterial(AActor* Actor, int ElementIndex, UMaterialInterface* Material);
	static UMaterialInterface*	GetMaterial(AActor* Actor, int ElementIndex);

	static UMaterialInstanceDynamic*	CreateDynamicMaterialInstance(AActor* Actor, int ElementIndex);
};