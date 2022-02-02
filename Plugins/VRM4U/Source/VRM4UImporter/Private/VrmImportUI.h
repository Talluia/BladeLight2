// VRM4U Copyright (c) 2019 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Factories/ImportSettings.h"
#include "VrmConvert.h"

#include "VrmImportUI.generated.h"

DECLARE_DELEGATE(FOnResolveFbxReImport);

UCLASS(config=EditorPerProjectUserSettings, AutoExpandCategories=(FTransform), HideCategories=Object, MinimalAPI)
class UVrmImportUI : public UObject, public IImportSettingsParser
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "Thumbnail"))
	UTexture2D *Thumbnail;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "VRM Title / Author"))
	FString TitleAuthor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "A-pose(Off to T-pose)"))
	bool bAPoseRetarget = true;

	/** for Mobile. Import root bone only */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "Reduce bonemap<=75 for mobile"))
	bool bMobileBone = false;

	/** Materal Type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
	EVRMImportMaterialType MaterialType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "Model scale"))
	float ModelScale = 1.0f;

	/** Duplicate mesh and renamed humanoid bone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="Create renamed humanoid mesh"))
	bool bCreateHumanoidRenamedMesh = false;

	/** Add IK Bone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "Create IK Bone"))
	bool bCreateIKBone = false;

	/** Physics asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="No physics asset"))
	bool bSkipPhysics = false;

	/** MorphTarget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="No MorphTarget"))
	bool bSkipMorphTarget = false;

	/** MorphTarget Normal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "Eable MorphTarget Normal(TangentZDelta)"))
	bool bEnableMorphTargetNormal = false;

	/** No Transparent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "No Transparent"))
	bool bNoTranslucent = false;

	/** Single uasset pkg file */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "Single uasset file"))
	bool bSingleUAssetFile = true;

	/** Mipmap Generate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "Generate Mipmap"))
	bool bMipmapGenerateMode = false;

	/** Material merge */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="Merge material"))
	bool bMergeMaterial = true;

	/** Primitive merge */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "Merge primitive"))
	bool bMergePrimitive = true;

	/** Material optimize */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="Optimize material"))
	bool bOptimizeMaterial = true;

	/** Vertex optimize */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "Optimize vertex"))
	bool bOptimizeVertex = true;

	/** BoneWeight Influence */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "Optimize Bone Weight Influence Num"))
	int BoneWeightInfluenceNum = 8;

	/** Remove bone has no mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName = "Remove bone used DCC tool"))
	bool bSimpleRoot = true;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="Delete bone without mesh"))
	bool bSkipNoMeshBone = false;

	/** for DEBUG. Import root bone only */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="[Debug]One bone only"))
	bool bDebugOneBone = false;

	/** Skeleton to use for imported asset. When importing a mesh, leaving this as "None" will create a new skeleton. When importing an animation this MUST be specified to import the asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category=Mesh, meta=(ImportType="SkeletalMesh"))
	class USkeleton* Skeleton;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "License_Personation/CharacterizationPermission")
		FString allowedUserName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "License_Personation/CharacterizationPermission")
		FString violentUssageName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "License_Personation/CharacterizationPermission")
		FString sexualUssageName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "License_Personation/CharacterizationPermission")
		FString commercialUssageName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "License_Personation/CharacterizationPermission")
		FString otherPermissionUrl;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "License_Redistribution/ModificationsLicense")
		FString licenseName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "License_Redistribution/ModificationsLicense")
		FString otherLicenseUrl;

private:
	UPROPERTY()
	FImportOptionData data;
public:
	const FImportOptionData *GenerateOptionData();

	/** If checked, create new PhysicsAsset if it doesn't have it */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, config, Category=Mesh, meta=(ImportType="SkeletalMesh"))
	//uint32 bCreatePhysicsAsset:1;

	/** If this is set, use this PhysicsAsset. It is possible bCreatePhysicsAsset == false, and PhysicsAsset == NULL. It is possible they do not like to create anything. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category=Mesh, meta=(ImportType="SkeletalMesh", editcondition="!bCreatePhysicsAsset"))
	//class UPhysicsAsset* PhysicsAsset;

	UFUNCTION(BlueprintCallable, Category = Miscellaneous)
	void ResetToDefault();

	/** IImportSettings Interface */
	virtual void ParseFromJson(TSharedRef<class FJsonObject> ImportSettingsJson) override;

	/** sets MeshTypeToImport */
	void SetMeshTypeToImport()
	{
		//MeshTypeToImport = bImportAsSkeletal ? FBXIT_SkeletalMesh : FBXIT_StaticMesh;
	}

	/* Whether this UI is construct for a reimport */
	bool bIsReimport;
};


