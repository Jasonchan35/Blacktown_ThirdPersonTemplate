#pragma once

//-- Unreal Header --
#include "CoreMinimal.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "SceneManagement.h"
#include "Runtime/Engine/Classes/Engine/UserInterfaceSettings.h"
#include "UObject/WeakInterfacePtr.h"
#include "UObject/ConstructorHelpers.h"

#include "Engine/OverlapResult.h"
#include "Engine/TriggerBox.h"
#include "Engine/TriggerCapsule.h"
#include "Engine/TriggerSphere.h"
#include "Engine/TriggerVolume.h"

// UI
#include "Widgets/SCompoundWidget.h"
#include "UMG.h"

#if WITH_EDITOR
#include "Editor.h"
#endif // WITH_EDITOR

#include "MyLib/MyActorUtil.h"
#include "MyLib/MyCDO.h"
#include "MyLib/MyConsole.h"
#include "MyLib/MyLog.h"
#include "MyLib/MyUI.h"
#include "MyLib/MyPhysics.h"

