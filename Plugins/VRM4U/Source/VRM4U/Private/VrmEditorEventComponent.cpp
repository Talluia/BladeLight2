// VRM4U Copyright (c) 2019 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmEditorEventComponent.h"

#if WITH_EDITOR
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "Engine/Selection.h"
#endif




UVrmEditorEventComponent::UVrmEditorEventComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void UVrmEditorEventComponent::OnRegister() {
	Super::OnRegister();
}
void UVrmEditorEventComponent::OnUnregister() {
	Super::OnUnregister();
}

void UVrmEditorEventComponent::OnSelectionChangeFunc(UObject *obj) {
#if WITH_EDITOR
	bool bFound = false;
	USelection* Selection = Cast<USelection>(obj);
	if (Selection == GEditor->GetSelectedComponents() || Selection == GEditor->GetSelectedActors()){
		for (int32 Idx = 0; Idx < Selection->Num(); Idx++)
		{
			const auto *a = Selection->GetSelectedObject(Idx);
			if (a == this->GetOwner()) {
				bFound = true;
				break;
			}
		}
		if (bFound && Selection->Num() == 1) {
			OnSelectionChange.Broadcast(false);
		}
	}
#endif
}
void UVrmEditorEventComponent::OnSelectionObjectFunc(UObject *obj) {
#if WITH_EDITOR
	bool bFound = false;
	USelection* Selection = Cast<USelection>(obj);
	if (Selection == GEditor->GetSelectedComponents() || Selection == GEditor->GetSelectedActors()) {

		for (int32 Idx = 0; Idx < Selection->Num(); Idx++)
		{
			if (Selection->GetSelectedObject(Idx) == this->GetOwner()) {
				bFound = true;
				break;
			}
		}
	}
	if (bFound) {
		OnSelectionObject.Broadcast(false);
	}
#endif
}

void UVrmEditorEventComponent::SetSelectCheck(bool bCheckOn) {
#if WITH_EDITOR
	USelection::SelectionChangedEvent.AddUObject(this, &UVrmEditorEventComponent::OnSelectionChangeFunc);
	USelection::SelectObjectEvent.AddUObject(this, &UVrmEditorEventComponent::OnSelectionObjectFunc);
	//SelectObjectEvent
	if (bCheckOn) {
		//GEditor->OnEndCameraMovement.Assign(OnCameraMove)
		//if (GEditor->GetActiveViewport()) {
		//	FEditorViewportClient* ViewportClient = StaticCast<FEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
		//	if (ViewportClient) {
		//		ViewportClient->SetActorLock(this->GetOwner());
		//	}
		//}
		//GCurrentLevelEditingViewportClient->SetActorLock(this->GetOwner());
		//GEditor->OnEndCameraMovement().AddUObject(this, &UVrmCameraCheckComponent::OnCameraTransformChanged);
		//GEditor->OnBeginCameraMovement().AddUObject(this, &UVrmCameraCheckComponent::OnCameraTransformChanged);
		//handle = FEditorDelegates::OnEditorCameraMoved.AddUObject(this, &UVrmCameraCheckComponent::OnCameraTransformChanged);
	} else {
		//if (handle.IsValid()) {
			//FEditorDelegates::OnEditorCameraMoved.Remove(handle);
	//	}
		//GEditor->OnEndCameraMovement().Remove
	}
	//FOnEndTransformCamera& () { return OnEndCameraTransformEvent; }
#endif
}

