// VRM4U Copyright (c) 2019 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "Engine/Texture.h"
#include "Misc/EngineVersionComparison.h"
#include "VrmUtil.generated.h"

UENUM(BlueprintType)
enum class EVRMImportMaterialType : uint8
{
	VRMIMT_Auto			UMETA(DisplayName = "Auto(MToon Unlit)"),
	VRMIMT_MToon		UMETA(DisplayName = "MToon Lit"),
	VRMIMT_MToonUnlit	UMETA(DisplayName = "MToon Unlit"),
	VRMIMT_SSS			UMETA(DisplayName = "Subsurface Scattering"),
	VRMIMT_Unlit		UMETA(DisplayName = "Unlit"),
	VRMIMT_glTF			UMETA(DisplayName = "PBR(glTF2)"),
	VRMIMT_Custom		UMETA(DisplayName = "Custom"),

	VRMIMT_MAX,
};

USTRUCT(BlueprintType)
struct VRM4U_API FImportOptionData {
public:
	GENERATED_BODY()

	void init();
	bool bAPoseRetarget = true;

	bool bMobileBone = false;

	int BoneWeightInfluenceNum = 8;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	EVRMImportMaterialType MaterialType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	float ModelScale = 1.0f;

	bool bCreateHumanoidRenamedMesh = false;

	bool bCreateIKBone = false;

	bool bSkipPhysics = false;

	bool bSkipMorphTarget = false;

	bool bEnableMorphTargetNormal = false;

	bool bNoTranslucent = false;

	bool bSingleUAssetFile = true;

	bool bMipmapGenerateMode = false;

	bool bMergeMaterial = true;

	bool bMergePrimitive = true;

	bool bOptimizeVertex = true;

	bool bSimpleRoot = true;

	bool bSkipNoMeshBone = false;

	bool bDebugOneBone = false;

	UPROPERTY()
	class USkeleton* Skeleton;

};



class VRM4U_API VRMUtil {
	//GENERATED_UCLASS_BODY()
public:

	struct VRMBoneTable {
		FString BoneUE4;
		FString BoneVRM;
	};
	static const TArray<VRMBoneTable> table_ue4_vrm;
	static const TArray<VRMBoneTable> table_ue4_pmx;

	static const TArray<FString> vrm_humanoid_bone_list;
	static const TArray<FName> vrm_humanoid_bone_list_name;

	static const TArray<FString> vrm_humanoid_parent_list;

	static const TArray<FString> ue4_humanoid_bone_list;

	struct FImportImage
	{
#if	UE_VERSION_OLDER_THAN(4,23,0)
		TArray<uint8> RawData;
#else
		TArray64<uint8> RawData;
#endif
		ETextureSourceFormat Format = TSF_Invalid;
		TextureCompressionSettings CompressionSettings = TC_Default;
		int32 NumMips;
		int32 SizeX = 0;
		int32 SizeY = 0;
		bool SRGB = true;

		static int32 GetBytesPerPixel(ETextureSourceFormat Format)
		{
			int32 BytesPerPixel = 0;
			switch (Format)
			{
			case TSF_G8:		BytesPerPixel = 1; break;
#if	UE_VERSION_OLDER_THAN(4,24,0)
#else
			case TSF_G16:		BytesPerPixel = 2; break;
#endif
			case TSF_BGRA8:		BytesPerPixel = 4; break;
			case TSF_BGRE8:		BytesPerPixel = 4; break;
			case TSF_RGBA16:	BytesPerPixel = 8; break;
			case TSF_RGBA16F:	BytesPerPixel = 8; break;
			default:			BytesPerPixel = 0; break;
			}
			return BytesPerPixel;
		}


		void Init2DWithParams(int32 InSizeX, int32 InSizeY, ETextureSourceFormat InFormat, bool InSRGB);
		void Init2DWithOneMip(int32 InSizeX, int32 InSizeY, ETextureSourceFormat InFormat, const void* InData = nullptr) {
			SizeX = InSizeX;
			SizeY = InSizeY;
			NumMips = 1;
			Format = InFormat;
			RawData.AddUninitialized((int64)SizeX * SizeY * GetBytesPerPixel(Format));
			if (InData)
			{
				FMemory::Memcpy(RawData.GetData(), InData, RawData.Num());
			}
		}
		void Init2DWithMips(int32 InSizeX, int32 InSizeY, int32 InNumMips, ETextureSourceFormat InFormat, const void* InData = nullptr);

		int64 GetMipSize(int32 InMipIndex) const;
		void* GetMipData(int32 InMipIndex);
	};


	//static bool LoadImageFromMemory(const char* Buffer, const size_t Length, VRMUtil::FImportImage& OutImage);
	static bool LoadImageFromMemory(const void* Buffer, const size_t Length, VRMUtil::FImportImage& OutImage);

	static UTexture2D* CreateTexture(int32 InSizeX, int32 InSizeY, FString name, UPackage* package);
	static UTexture2D* CreateTextureFromImage(FString name, UPackage* package, const void* Buffer, const size_t Length, bool GenerateMip=false);

};
