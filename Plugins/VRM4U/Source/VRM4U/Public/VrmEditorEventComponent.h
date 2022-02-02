// VRM4U Copyright (c) 2019 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "Misc/CoreDelegates.h"

#include "VrmEditorEventComponent.generated.h"

/**
 * 
 */
UCLASS(meta=(BlueprintSpawnableComponent))
class VRM4U_API UVrmEditorEventComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
	
public:

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVrmSelectionChangedEventDelegate, bool, dummy);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVrmSelectionObjectEventDelegate, bool, dummy);

	UPROPERTY(BlueprintAssignable)
	FVrmSelectionChangedEventDelegate OnSelectionChange;

	UPROPERTY(BlueprintAssignable)
	FVrmSelectionObjectEventDelegate OnSelectionObject;

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (DynamicOutputParam = "OutVrmAsset"))
	void SetSelectCheck(bool bCheckOn);

public:
	void OnRegister() override;
	void OnUnregister() override;

private:

	void OnSelectionObjectFunc(UObject *obj);

	void OnSelectionChangeFunc(UObject *obj);

	FDelegateHandle handle;
};
