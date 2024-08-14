#pragma once

struct MyPhysics
{
	MyPhysics() = delete;

	static int BodyInstanceAsyncSweepByObjectType(
					UWorld* World,
					EAsyncTraceType InTraceType,
					const FVector& Start,
					const FVector& End, 
					const FQuat& Rot, 
					const FCollisionObjectQueryParams& ObjectQueryParams, 
					const FBodyInstance* Body,
					const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, 
					const FTraceDelegate* InDelegate = nullptr, 
					uint32 UserData = 0);

	static int PrimitiveComponentAsyncSweepByObjectType(
					UWorld* World,
					EAsyncTraceType InTraceType,
					const FVector& Start,
					const FVector& End, 
					const FQuat& Rot, 
					const FCollisionObjectQueryParams& ObjectQueryParams, 
					const UPrimitiveComponent* PrimComp,
					const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, 
					const FTraceDelegate* InDelegate = nullptr, 
					uint32 UserData = 0);

};