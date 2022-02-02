// VRM4U Copyright (c) 2019 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmEditorBPFunctionLibrary.h"
#include "Materials/MaterialInterface.h"

#include "Engine/Engine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/SkeletalMesh.h"
#include "Logging/MessageLog.h"
#include "Engine/Canvas.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Animation/MorphTarget.h"
#include "Misc/EngineVersionComparison.h"
#include "AssetRegistryModule.h"
#include "ARFilter.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/LightComponent.h"

#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"

#include "Animation/AnimInstance.h"
#include "VrmAnimInstanceCopy.h"
#include "VrmUtil.h"

#if PLATFORM_WINDOWS
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWindow.h"
#endif

#include "HAL/ConsoleManager.h"


#if WITH_EDITOR
//#include "Editor.h"
//#include "EditorViewportClient.h"
//#include "EditorSupportDelegates.h"
//#include "LevelEditorActions.h"
//#include "Editor/EditorPerProjectUserSettings.h"
#endif
#include "Kismet/GameplayStatics.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

//#include "VRM4U.h"

