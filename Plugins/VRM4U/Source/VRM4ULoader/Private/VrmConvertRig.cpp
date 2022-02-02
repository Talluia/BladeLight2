// VRM4U Copyright (c) 2019 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmConvertRig.h"
#include "VrmConvert.h"
#include "VrmUtil.h"

#include "VrmAssetListObject.h"
#include "VrmMetaObject.h"
#include "LoaderBPFunctionLibrary.h"
#include "VrmBPFunctionLibrary.h"

#include "Engine/SkeletalMesh.h"
#include "RenderingThread.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Animation/MorphTarget.h"
#include "Animation/NodeMappingContainer.h"
#include "Animation/Rig.h"
#include "Animation/PoseAsset.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"

#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"

#if WITH_EDITOR
#include "IPersonaToolkit.h"
#include "PersonaModule.h"
#include "Modules/ModuleManager.h"
#include "Animation/DebugSkelMeshComponent.h"
#endif

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
#include <assimp/vrm/vrmmeta.h>

//#include "Engine/.h"

namespace {
// utility function 
#if WITH_EDITOR
	FSmartName GetUniquePoseName(USkeleton* Skeleton, const FString &Name)
	{
		check(Skeleton);
		int32 NameIndex = 0;

		SmartName::UID_Type NewUID;
		FName NewName;

		do
		{
			NewName = FName(*FString::Printf(TEXT("%s_%d"), *Name,NameIndex++));

			if (NameIndex == 1) {
				NewName = *Name;
			}

			NewUID = Skeleton->GetUIDByName(USkeleton::AnimCurveMappingName, NewName);
		} while (NewUID != SmartName::MaxUID);

		// if found, 
		FSmartName NewPoseName;
		Skeleton->AddSmartNameAndModify(USkeleton::AnimCurveMappingName, NewName, NewPoseName);

		return NewPoseName;
	}
#endif 
}

namespace {

	static int GetChildBoneLocal(const FReferenceSkeleton &skeleton, const int32 ParentBoneIndex, TArray<int32> & Children) {
		Children.Reset();
		//auto &r = skeleton->GetReferenceSkeleton();
		auto &r = skeleton;

		const int32 NumBones = r.GetRawBoneNum();
		for (int32 ChildIndex = ParentBoneIndex + 1; ChildIndex < NumBones; ChildIndex++)
		{
			if (ParentBoneIndex == r.GetParentIndex(ChildIndex))
			{
				Children.Add(ChildIndex);
			}
		}
		return Children.Num();
	}

	static bool isSameOrChild(const FReferenceSkeleton &skeleton, const int32 TargetBoneIndex, const int32 SameOrChildBoneIndex) {
		auto &r = skeleton;

		int32 c = SameOrChildBoneIndex;
		for (int i = 0; i < skeleton.GetRawBoneNum(); ++i) {

			if (TargetBoneIndex < 0 || SameOrChildBoneIndex < 0) {
				return false;
			}

			if (c == TargetBoneIndex) {
				return true;
			}


			c = skeleton.GetParentIndex(c);
		}
		return false;
	}
}


bool VRMConverter::ConvertRig(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr) {

	if (VRMConverter::Options::Get().IsDebugOneBone()) {
		return true;
	}

	if (vrmAssetList->SkeletalMesh == nullptr) {
		return false;
	}

	bool bPlay = false;
	{
		bool b1, b2, b3;
		b1 = b2 = b3 = false;
		UVrmBPFunctionLibrary::VRMGetPlayMode(b1, b2, b3);
		bPlay = b1;
	}
	if (bPlay) {
		// set dummy collision only
		//return;
	}

#if	UE_VERSION_OLDER_THAN(4,20,0)
#else
#if WITH_EDITOR

	UNodeMappingContainer* mc = nullptr;
	{
		FString name = FString(TEXT("RIG_")) + vrmAssetList->BaseFileName;
		mc = VRM4U_NewObject<UNodeMappingContainer>(vrmAssetList->Package, *name, RF_Public | RF_Standalone);
	}

	auto *k = vrmAssetList->SkeletalMesh->Skeleton;
	vrmAssetList->SkeletalMesh->NodeMappingData.Add(mc);

	//USkeletalMeshComponent* PreviewMeshComp = PreviewScenePtr.Pin()->GetPreviewMeshComponent();
	//USkeletalMesh* PreviewMesh = PreviewMeshComp->SkeletalMesh;

	URig *EngineHumanoidRig = LoadObject<URig>(nullptr, TEXT("/Engine/EngineMeshes/Humanoid.Humanoid"), nullptr, LOAD_None, nullptr);
	//FSoftObjectPath r(TEXT("/Engine/EngineMeshes/Humanoid.Humanoid"));
	//UObject *u = r.TryLoad();
	mc->SetSourceAsset(EngineHumanoidRig);

	vrmAssetList->SkeletalMesh->Skeleton->SetRigConfig(EngineHumanoidRig);


	mc->SetTargetAsset(vrmAssetList->SkeletalMesh);
	mc->AddDefaultMapping();

	FString PelvisBoneName;
	{
		const VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(mScenePtr->mVRMMeta);

		auto func = [&](const FString &a, const FString b) {
			mc->AddMapping(*a, *b);
			vrmAssetList->SkeletalMesh->Skeleton->SetRigBoneMapping(*a, *b);
		};
		auto func2 = [&](const FString &a, FName b) {
			func(a, b.ToString());
		};

		if (meta) {
			for (auto &t : VRMUtil::table_ue4_vrm) {
				FString target = t.BoneVRM;
				const FString &ue4 = t.BoneUE4;

				if (ue4.Compare(TEXT("Root"), ESearchCase::IgnoreCase) == 0) {
					auto &a = vrmAssetList->SkeletalMesh->Skeleton->GetReferenceSkeleton().GetRefBoneInfo();
					target = a[0].Name.ToString();
				}

				if (target.Len() == 0) {
					continue;
				}


				for (auto b : meta->humanoidBone) {

					if (target.Compare(b.humanBoneName.C_Str()) != 0) {
						continue;
					}
					target = b.nodeName.C_Str();
					break;
				}

				if (PelvisBoneName.Len() == 0) {
					if (ue4.Compare(TEXT("Pelvis"), ESearchCase::IgnoreCase) == 0) {
						PelvisBoneName = target;
					}
				}
				func(ue4, target);
				//mc->AddMapping(*ue4, *target);
				//vrmAssetList->SkeletalMesh->Skeleton->SetRigBoneMapping(*ue4, *target);
			}
			{
				const TArray<FString> cc = {
					TEXT("Root"),
					TEXT("Pelvis"),
					TEXT("spine_01"),
					TEXT("spine_02"),
					TEXT("spine_03"),
					TEXT("neck_01"),
				};

				// find bone from child bone
				for (int i = cc.Num() - 2; i > 0; --i) {
					const auto &m = mc->GetNodeMappingTable();

					{
						const auto p0 = m.Find(*cc[i]);
						if (p0) {
							// map exist
							continue;
						}
					}
					const auto p = m.Find(*cc[i + 1]);
					if (p == nullptr) {
						// child none
						continue;
					}

					const FName *parentRoot = nullptr;
					for (int toParent = i - 1; toParent > 0; --toParent) {
						parentRoot = m.Find(*cc[toParent]);
						if (parentRoot) {
							break;
						}
					}
					if (parentRoot == nullptr) {
						continue;
					}

					// find (child) p -> (parent)parentRoot
					FString newTarget = parentRoot->ToString();
					{

						const int32 index = k->GetReferenceSkeleton().FindBoneIndex(*p);
						const int32 indexParent = k->GetReferenceSkeleton().GetParentIndex(index);
						const int32 indexRoot = k->GetReferenceSkeleton().FindBoneIndex(*parentRoot);

						if (isSameOrChild(k->GetReferenceSkeleton(), indexRoot, indexParent)) {
							newTarget = k->GetReferenceSkeleton().GetBoneName(indexParent).ToString();
						}
					}
					func(cc[i], newTarget);
				}

				// set null -> parent bone
				for (int i = 1; i < cc.Num(); ++i) {
					const auto &m = mc->GetNodeMappingTable();

					{
						const auto p0 = m.Find(*cc[i]);
						if (p0) {
							// map exist
							continue;
						}
					}
					const auto pp = m.Find(*cc[i-1]);
					if (pp == nullptr) {
						// parent none
						continue;
					}

					// map=nullptr, parent=exist
					FString newTarget = pp->ToString();

					{
						int32 index = k->GetReferenceSkeleton().FindBoneIndex(*pp);
						TArray<int32> child;
						GetChildBoneLocal(k->GetReferenceSkeleton(), index, child);
						if (child.Num() == 1) {
							// use one child
							// need neck check...
							//newTarget = k->GetReferenceSkeleton().GetBoneName(child[0]).ToString();
						}
					}

					func(cc[i], newTarget);
				}
			}
		} else {
			// BVH auto mapping
			
			const auto &rSk = k->GetReferenceSkeleton(); //EngineHumanoidRig->GetSourceReferenceSkeleton();
			
			TArray<FString> rBoneList;
			{
				for (auto &a : rSk.GetRawRefBoneInfo()) {
					rBoneList.Add(a.Name.ToString());
				}
			}
			TArray<FString> existTable;
			int boneIndex = -1;
			for (auto &b : rBoneList) {
				++boneIndex;

				if (b.Find(TEXT("neck")) >= 0) {
					const FString t = TEXT("neck_01");
					if (existTable.Find(t) >= 0) {
						continue;
					}
					existTable.Add(t);
					func(t, b);

					int p = boneIndex;

					const TArray<FString> cc = {
						TEXT("spine_03"),
						TEXT("spine_02"),
						TEXT("spine_01"),
						TEXT("Pelvis"),
						TEXT("Root"),
					};

					TArray<FName> targetBone;
					targetBone.SetNum(5);

					for (int i = 0; i < 5; ++i) {
						auto& a = cc[i];
						int p2 = rSk.GetParentIndex(p);
						if (p2 >= 0) {
							p = p2;
						}
						targetBone[i] = rSk.GetBoneName(p);
					}

					// upper chest skip
					for (int i = 0; i < 5; ++i) {
						if (targetBone[2] == targetBone[3]) {
							targetBone[2] = targetBone[1];
							targetBone[1] = targetBone[0];
						}
					}

					//regist
					for (int i = 0; i < 5; ++i) {
						func2(cc[i], targetBone[i]);
					}
				}
				if (b.Find(TEXT("head")) >= 0) {
					const FString t = TEXT("head");
					if (existTable.Find(t) >= 0) {
						continue;
					}
					existTable.Add(t);
					func(t, b);
				}

				if (b.Find(TEXT("hand")) >= 0) {
					if (b.Find("r") >= 0) {
						const FString t = TEXT("Hand_R");
						if (existTable.Find(t) >= 0){
							continue;
						}
						existTable.Add(t);

						func(t, b);

						int p = rSk.GetParentIndex(boneIndex);
						func2(TEXT("lowerarm_r"), rSk.GetBoneName(p));

						p = rSk.GetParentIndex(p);
						func2(TEXT("UpperArm_R"), rSk.GetBoneName(p));

						p = rSk.GetParentIndex(p);
						func2(TEXT("clavicle_r"), rSk.GetBoneName(p));

						{
							// finger right
							TArray<int32> childBone;
							GetChildBoneLocal(rSk, boneIndex, childBone);
							FString tt[6][2] = {
								"thumb",	"thumb_01_r",
								"index",	"index_01_r",
								"middle",	"middle_01_r",
								"ring",		"ring_01_r",
								"little",	"pinky_01_r",	// little
								"pinky",	"pinky_01_r",	// little
							};
							for (auto& c : childBone) {
								auto name = rSk.GetBoneName(c);
								for (int i = 0; i < 6; ++i) {
									{
										int tmp = name.ToString().Find(tt[i][0]);
										if (tmp < 0) continue;
									}
									int ind = VRMUtil::ue4_humanoid_bone_list.Find(tt[i][1]);
									if (ind < 0) continue;

									func2(VRMUtil::ue4_humanoid_bone_list[ind], rSk.GetBoneName(c));

									TArray<int32> tmp = { c };

									GetChildBoneLocal(rSk, tmp[0], tmp);
									if (tmp.Num() <= 0) continue;
									func2(VRMUtil::ue4_humanoid_bone_list[ind + 1], rSk.GetBoneName(tmp[0]));

									GetChildBoneLocal(rSk, tmp[0], tmp);
									if (tmp.Num() <= 0) continue;
									func2(VRMUtil::ue4_humanoid_bone_list[ind + 2], rSk.GetBoneName(tmp[0]));
								}
							}
						}
					}
					if (b.Find("l") >= 0) {
						const FString t = TEXT("Hand_L");
						if (existTable.Find(t) >= 0) {
							continue;
						}
						existTable.Add(t);

						func(t, b);

						int p = rSk.GetParentIndex(boneIndex);
						func2(TEXT("lowerarm_l"), rSk.GetBoneName(p));

						p = rSk.GetParentIndex(p);
						func2(TEXT("UpperArm_L"), rSk.GetBoneName(p));

						p = rSk.GetParentIndex(p);
						func2(TEXT("clavicle_l"), rSk.GetBoneName(p));

						{
							// finger left
							TArray<int32> childBone;
							GetChildBoneLocal(rSk, boneIndex, childBone);
							FString tt[6][2] = {
								"thumb",	"thumb_01_l",
								"index",	"index_01_l",
								"middle",	"middle_01_l",
								"ring",		"ring_01_l",
								"little",	"pinky_01_l",	// little
								"pinky",	"pinky_01_l",	// little
							};
							for (auto& c : childBone) {
								auto name = rSk.GetBoneName(c);
								for (int i = 0; i < 6; ++i) {
									{
										int tmp = name.ToString().Find(tt[i][0]);
										if (tmp < 0) continue;
									}
									int ind = VRMUtil::ue4_humanoid_bone_list.Find(tt[i][1]);
									if (ind < 0) continue;

									func2(VRMUtil::ue4_humanoid_bone_list[ind], rSk.GetBoneName(c));

									TArray<int32> tmp = { c };

									GetChildBoneLocal(rSk, tmp[0], tmp);
									if (tmp.Num() <= 0) continue;
									func2(VRMUtil::ue4_humanoid_bone_list[ind + 1], rSk.GetBoneName(tmp[0]));

									GetChildBoneLocal(rSk, tmp[0], tmp);
									if (tmp.Num() <= 0) continue;
									func2(VRMUtil::ue4_humanoid_bone_list[ind + 2], rSk.GetBoneName(tmp[0]));
								}
							}
						}
					}
				}
				if (b.Find(TEXT("foot")) >= 0) {
					if (b.Find("r") >= 0) {
						const FString t = TEXT("Foot_R");
						if (existTable.Find(t) >= 0) {
							continue;
						}
						existTable.Add(t);

						func(t, b);

						{
							TArray<int32> c;
							GetChildBoneLocal(rSk, boneIndex, c);
							if (c.Num()) {
								func2(TEXT("ball_r"), rSk.GetBoneName(c[0]));
							}
						}

						int p = rSk.GetParentIndex(boneIndex);
						func2(TEXT("calf_r"), rSk.GetBoneName(p));

						p = rSk.GetParentIndex(p);
						func2(TEXT("Thigh_R"), rSk.GetBoneName(p));
					}
					if (b.Find("l") >= 0) {
						const FString t = TEXT("Foot_L");
						if (existTable.Find(t) >= 0) {
							continue;
						}
						existTable.Add(t);

						func(t, b);

						{
							TArray<int32> c;
							GetChildBoneLocal(rSk, boneIndex, c);
							if (c.Num()) {
								func2(TEXT("ball_l"), rSk.GetBoneName(c[0]));
							}
						}

						int p = rSk.GetParentIndex(boneIndex);
						func2(TEXT("calf_l"), rSk.GetBoneName(p));

						p = rSk.GetParentIndex(p);
						func2(TEXT("Thigh_L"), rSk.GetBoneName(p));
					}
				}

				{
					// pmx bone map
					for (const auto &t : VRMUtil::table_ue4_pmx) {
						FString target = t.BoneVRM;
						const FString &ue4 = t.BoneUE4;

						auto ind = k->GetReferenceSkeleton().FindBoneIndex(*target);
						if (ind != INDEX_NONE) {
							func(ue4, target);

							for (const auto &v : VRMUtil::table_ue4_vrm) {
								if (v.BoneUE4 == t.BoneUE4){
									if (v.BoneVRM.IsEmpty()) {
										continue;
									}
									// renew bonemap
									vrmAssetList->VrmMetaObject->humanoidBoneTable.Add(v.BoneVRM) = t.BoneVRM;
									break;
								}
							}
						}
					}
					vrmAssetList->VrmMetaObject->humanoidBoneTable.Add("leftEye") = TEXT("左目");
					vrmAssetList->VrmMetaObject->humanoidBoneTable.Add("rightEye") = TEXT("右目");

				}// pmx map
			}
		}// map end
	}

	{
		int bone = -1;
		for (int i = 0; i < k->GetReferenceSkeleton().GetRawBoneNum(); ++i) {
			//const int32 BoneIndex = k->GetReferenceSkeleton().FindBoneIndex(InBoneName);
			k->SetBoneTranslationRetargetingMode(i, EBoneTranslationRetargetingMode::Skeleton);
			//FAssetNotifications::SkeletonNeedsToBeSaved(k);
			if (k->GetReferenceSkeleton().GetBoneName(i).Compare(*PelvisBoneName) == 0) {
				bone = i;
			}
		}

		bool first = true;
		while(bone >= 0){
			if (first) {
				k->SetBoneTranslationRetargetingMode(bone, EBoneTranslationRetargetingMode::AnimationScaled);
			} else {
				k->SetBoneTranslationRetargetingMode(bone, EBoneTranslationRetargetingMode::Animation);
			}
			first = false;

			bone = k->GetReferenceSkeleton().GetParentIndex(bone);
		}
		{
			FName n[] = {
				TEXT("ik_foot_root"),
				TEXT("ik_foot_l"),
				TEXT("ik_foot_r"),
				TEXT("ik_hand_root"),
				TEXT("ik_hand_gun"),
				TEXT("ik_hand_l"),
				TEXT("ik_hand_r"),
			};

			for (auto &s : n) {
				int32 ind = k->GetReferenceSkeleton().FindBoneIndex(s);
				if (ind < 0) continue;

				k->SetBoneTranslationRetargetingMode(ind, EBoneTranslationRetargetingMode::Animation, false);
			}
		}
		if (VRMConverter::Options::Get().IsPMXModel()) {
			// center to animscale
			int32 ind = k->GetReferenceSkeleton().FindBoneIndex(*(VRMUtil::table_ue4_pmx[1].BoneVRM));
			if (ind >= 0) {
				k->SetBoneTranslationRetargetingMode(ind, EBoneTranslationRetargetingMode::AnimationScaled, false);
			}
		}

		if (VRMConverter::Options::Get().IsVRMModel() == false) {
			k->SetBoneTranslationRetargetingMode(0, EBoneTranslationRetargetingMode::Animation, false);
		}

	}
	//mc->AddMapping
	mc->PostEditChange();
	vrmAssetList->HumanoidRig = mc;

#if 1
	if (VRMConverter::Options::Get().IsDebugOneBone() == false && bPlay==false){
		USkeletalMesh *sk = vrmAssetList->SkeletalMesh;

		FString name = FString(TEXT("POSE_")) + vrmAssetList->BaseFileName;
		
		UPoseAsset *pose = nullptr;

		//if (VRMConverter::Options::Get().IsSingleUAssetFile()) {
			pose = VRM4U_NewObject<UPoseAsset>(vrmAssetList->Package, *name, RF_Public | RF_Standalone);
		//} else {
		//	FString originalPath = vrmAssetList->Package->GetPathName();
		//	const FString PackagePath = FPaths::GetPath(originalPath);

		//	FString NewPackageName = FPaths::Combine(*PackagePath, *name);
		//	UPackage* Pkg = CreatePackage(nullptr, *NewPackageName);

		//	pose = VRM4U_NewObject<UPoseAsset>(Pkg, *name, RF_Public | RF_Standalone);
		//}



		pose->SetSkeleton(k);
		pose->SetPreviewMesh(sk);
		pose->Modify();

		{
			/*
			type 0:
				poseasset +1: T-pose,
			type 1:
				poseasset +1: A-pose,
				retarget +1 : T-pose or A-pose
			type 2:
				poseasset +1: T-pose(footA)
			type 3:
				poseasset +1: A-pose(footT)
			*/
			enum class PoseType {
				TYPE_T,
				TYPE_A,
			};
			PoseType poseType_hand;
			PoseType poseType_foot;
			for (int poseCount = 0; poseCount < 4; ++poseCount) {

				switch (poseCount) {
				case 0:
					poseType_hand = PoseType::TYPE_T;
					poseType_foot = PoseType::TYPE_T;
					break;
				case 1:
					poseType_hand = PoseType::TYPE_A;
					poseType_foot = PoseType::TYPE_A;
					break;
				case 2:
					poseType_hand = PoseType::TYPE_T;
					poseType_foot = PoseType::TYPE_A;
					break;
				case 3:
				default:
					poseType_hand = PoseType::TYPE_A;
					poseType_foot = PoseType::TYPE_T;
					break;
				}

				FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>("Persona");
				auto PersonaToolkit = PersonaModule.CreatePersonaToolkit(sk);

				UDebugSkelMeshComponent* PreviewComponent = PersonaToolkit->GetPreviewMeshComponent();

				auto *kk = Cast<USkeletalMeshComponent>(PreviewComponent);
				kk->SetComponentSpaceTransformsDoubleBuffering(false);

				{
					struct RetargetParts {
						FString BoneUE4;
						FString BoneVRM;
						FString BoneModel;

						FRotator rot;
					};

					TArray<RetargetParts> retargetTable;
					if (VRMConverter::Options::Get().IsVRMModel() || VRMConverter::Options::Get().IsBVHModel()) {
						if (poseType_hand== PoseType::TYPE_A) {
							{
								RetargetParts t;
								t.BoneUE4 = TEXT("UpperArm_R");
								t.rot = FRotator(40, 0, 0);
								retargetTable.Push(t);
							}
							{
								RetargetParts t;
								t.BoneUE4 = TEXT("lowerarm_r");
								t.rot = FRotator(0, -30, 0);
								retargetTable.Push(t);
							}
							{
								RetargetParts t;
								t.BoneUE4 = TEXT("Hand_R");
								t.rot = FRotator(10, 0, 0);
								retargetTable.Push(t);
							}
							{
								RetargetParts t;
								t.BoneUE4 = TEXT("UpperArm_L");
								t.rot = FRotator(-40, 0, 0);
								retargetTable.Push(t);
							}
							{
								RetargetParts t;
								t.BoneUE4 = TEXT("lowerarm_l");
								t.rot = FRotator(-0, 30, 0);
								retargetTable.Push(t);
							}
							{
								RetargetParts t;
								t.BoneUE4 = TEXT("Hand_L");
								t.rot = FRotator(-10, 0, 0);
								retargetTable.Push(t);
							}
						}
					}
					if (VRMConverter::Options::Get().IsPMXModel()) {
						if (poseType_hand == PoseType::TYPE_T) {
							auto& poseList = k->GetReferenceSkeleton().GetRefBonePose();
							FString* boneName = vrmAssetList->VrmMetaObject->humanoidBoneTable.Find(TEXT("rightLowerArm"));
							float degRot = 0.f;
							if (boneName) {
								int ind = k->GetReferenceSkeleton().FindBoneIndex(**boneName);
								if (ind >= 0) {
									FVector v = poseList[ind].GetLocation();
									v.Z = FMath::Abs(v.Z);
									v.X = FMath::Abs(v.X);
									degRot = FMath::Abs(FMath::Atan2(v.Z, v.X)) * 180.f / PI;
								}
							}
							if (degRot) {
								{
									RetargetParts t;
									t.BoneUE4 = TEXT("UpperArm_R");
									t.rot = FRotator(-degRot, 0, 0);
									retargetTable.Push(t);
								}
								{
									RetargetParts t;
									t.BoneUE4 = TEXT("UpperArm_L");
									t.rot = FRotator(degRot, 0, 0);
									retargetTable.Push(t);
								}
							}
						}
						if (poseType_hand == PoseType::TYPE_A) {
							{
								RetargetParts t;
								t.BoneUE4 = TEXT("lowerarm_r");
								t.rot = FRotator(0, -30, 0);
								retargetTable.Push(t);
							}
							{
								RetargetParts t;
								t.BoneUE4 = TEXT("Hand_R");
								t.rot = FRotator(10, 0, 0);
								retargetTable.Push(t);
							}
							{
								RetargetParts t;
								t.BoneUE4 = TEXT("lowerarm_l");
								t.rot = FRotator(-0, 30, 0);
								retargetTable.Push(t);
							}
							{
								RetargetParts t;
								t.BoneUE4 = TEXT("Hand_L");
								t.rot = FRotator(-10, 0, 0);
								retargetTable.Push(t);
							}
						}
					}
					if (poseType_foot == PoseType::TYPE_A) {
						{
							RetargetParts t;
							t.BoneUE4 = TEXT("Thigh_R");
							t.rot = FRotator(-7, 0, 0);
							retargetTable.Push(t);
						}
						{
							RetargetParts t;
							t.BoneUE4 = TEXT("Thigh_L");
							t.rot = FRotator(7, 0, 0);
							retargetTable.Push(t);
						}
					}

					TMap<FString, RetargetParts> mapTable;
					for (auto &a : retargetTable) {
						bool bFound = false;
						//vrm
						for (auto &t : VRMUtil::table_ue4_vrm) {
							if (t.BoneUE4.Compare(a.BoneUE4) != 0) {
								continue;
							}
							auto *m = vrmAssetList->VrmMetaObject->humanoidBoneTable.Find(t.BoneVRM);
							if (m) {
								bFound = true;
								a.BoneVRM = t.BoneVRM;
								a.BoneModel = *m;
								mapTable.Add(a.BoneModel, a);
							}
							break;
						}
						if (bFound) {
							continue;
						}
						//pmx
						for (auto &t : VRMUtil::table_ue4_pmx) {
							if (t.BoneUE4.Compare(a.BoneUE4) != 0) {
								continue;
							}
							auto *m = vrmAssetList->VrmMetaObject->humanoidBoneTable.Find(t.BoneVRM);
							if (m) {
								bFound = true;
								a.BoneVRM = t.BoneVRM;
								a.BoneModel = *m;
								mapTable.Add(a.BoneModel, a);
							}
							break;
						}
						//bvh
						{
							const auto &m = mc->GetNodeMappingTable();
							auto *value = m.Find(*a.BoneUE4);
							if (value) {
								bFound = true;
								mapTable.Add(value->ToString(), a);
							}
						}
						if (bFound) {
							continue;
						}
					}

					auto &rk = k->GetReferenceSkeleton();
					auto &dstTrans = kk->GetEditableComponentSpaceTransforms();

					// init retarget pose
					for (int i = 0; i < dstTrans.Num(); ++i) {
						auto &t = dstTrans[i];
						t = rk.GetRefBonePose()[i];
					}
					if (poseCount == 1) {
						sk->RetargetBasePose = dstTrans;
					}

					// override
					for (int i = 0; i < dstTrans.Num(); ++i) {
						auto &t = dstTrans[i];

						auto *m = mapTable.Find(rk.GetBoneName(i).ToString());
						if (m) {
							t.SetRotation(FQuat(m->rot));
						}
					}

					// current pose retarget. local
					if (VRMConverter::Options::Get().IsAPoseRetarget() == true) {
						if (poseCount == 1) {
							sk->RetargetBasePose = dstTrans;
						}
					}

					// for rig asset. world
					for (int i = 0; i < dstTrans.Num(); ++i) {
						int parent = rk.GetParentIndex(i);
						if (parent == INDEX_NONE) continue;

						dstTrans[i] = dstTrans[i] * dstTrans[parent];
					}
					// ik bone hand
					{
						int32 ik_g = sk->RefSkeleton.FindBoneIndex(TEXT("ik_hand_gun"));
						int32 ik_r = sk->RefSkeleton.FindBoneIndex(TEXT("ik_hand_r"));
						int32 ik_l = sk->RefSkeleton.FindBoneIndex(TEXT("ik_hand_l"));

						if (ik_g >= 0 && ik_r >= 0 && ik_l >= 0) {
							const VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(mScenePtr->mVRMMeta);

							auto ar = vrmAssetList->VrmMetaObject->humanoidBoneTable.Find(TEXT("rightHand"));
							auto al = vrmAssetList->VrmMetaObject->humanoidBoneTable.Find(TEXT("leftHand"));
							if (ar && al) {
								int32 kr = sk->RefSkeleton.FindBoneIndex(**ar);
								int32 kl = sk->RefSkeleton.FindBoneIndex(**al);

								dstTrans[ik_g] = dstTrans[kr];
								dstTrans[ik_r] = dstTrans[kr];
								dstTrans[ik_l] = dstTrans[kl];

								// local
								sk->RetargetBasePose[ik_g] = dstTrans[kr];
								sk->RetargetBasePose[ik_r].SetIdentity();
								sk->RetargetBasePose[ik_l] = dstTrans[kl] * dstTrans[kr].Inverse();
							}
						}
					}
					// ik bone foot
					{
						int32 ik_r = sk->RefSkeleton.FindBoneIndex(TEXT("ik_foot_r"));
						int32 ik_l = sk->RefSkeleton.FindBoneIndex(TEXT("ik_foot_l"));

						if (ik_r >= 0 && ik_l >= 0) {
							const VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(mScenePtr->mVRMMeta);

							auto ar = vrmAssetList->VrmMetaObject->humanoidBoneTable.Find(TEXT("rightFoot"));
							auto al = vrmAssetList->VrmMetaObject->humanoidBoneTable.Find(TEXT("leftFoot"));
							if (ar && al) {
								int32 kr = sk->RefSkeleton.FindBoneIndex(**ar);
								int32 kl = sk->RefSkeleton.FindBoneIndex(**al);

								dstTrans[ik_r] = dstTrans[kr];
								dstTrans[ik_l] = dstTrans[kl];

								// local
								sk->RetargetBasePose[ik_r] = dstTrans[kr];
								sk->RetargetBasePose[ik_l] = dstTrans[kl];
							}
						}
					}

				}
				{
					FSmartName PoseName;
					switch(poseCount) {
					case 0:
						PoseName = GetUniquePoseName(kk->SkeletalMesh->Skeleton, TEXT("POSE_T"));
						break;
					case 1:
						PoseName = GetUniquePoseName(kk->SkeletalMesh->Skeleton, TEXT("POSE_A"));
						break;
					case 2:
						PoseName = GetUniquePoseName(kk->SkeletalMesh->Skeleton, TEXT("POSE_T(foot_A)"));
						break;
					case 3:
					default:
						PoseName = GetUniquePoseName(kk->SkeletalMesh->Skeleton, TEXT("POSE_A(foot_T)"));
						break;
					}
					//pose->AddOrUpdatePose(PoseName, Cast<USkeletalMeshComponent>(PreviewComponent));

					FSmartName newName;
					pose->AddOrUpdatePoseWithUniqueName(Cast<USkeletalMeshComponent>(PreviewComponent), &newName);
					pose->ModifyPoseName(newName.DisplayName, PoseName.DisplayName, nullptr);
				}
			}
		}
	}
#endif

#endif
#endif //420

#if 1
	if (vrmAssetList) {
		// dummy Collision

		const VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(mScenePtr->mVRMMeta);
		USkeletalMesh *sk = vrmAssetList->SkeletalMesh;
		UPhysicsAsset *pa = sk->PhysicsAsset;
		const FString dummy_target[] = {
			TEXT("hips"),
			TEXT("head"),

			TEXT("rightHand"),
			TEXT("leftHand"),
			TEXT("leftMiddleDistal"),
			TEXT("rightMiddleDistal"),


			TEXT("rightFoot"),
			TEXT("leftFoot"),

			TEXT("leftToes"),
			TEXT("rightToes"),

			TEXT("rightLowerArm"),
			TEXT("leftLowerArm"),

			TEXT("rightLowerLeg"),
			TEXT("leftLowerLeg"),
		};
		if (meta && pa) {
			for (const auto &a : meta->humanoidBone) {

				{
					bool bFound = false;
					for (auto &d : dummy_target) {
						if (d.Compare(a.humanBoneName.C_Str()) == 0) {
							bFound = true;
						}
					}
					if (bFound == false) {
						continue;
					}
				}

				{
					bool b = false;
					for (const auto *bs : pa->SkeletalBodySetups) {
						FString s = bs->BoneName.ToString();
						if (s.Compare(a.nodeName.C_Str(), ESearchCase::IgnoreCase) == 0) {
							b = true;
							break;
						}
					}
					if (b) {
						continue;
					}
				}

				const int targetBone = sk->RefSkeleton.FindRawBoneIndex(a.nodeName.C_Str());
				if (targetBone == INDEX_NONE) {
					break;
				}


				/*
				FVector center(0, 0, 0);
				{
					int i = targetBone;
					while (sk->RefSkeleton.GetParentIndex(i) != INDEX_NONE) {

						center += sk->RefSkeleton.GetRefBonePose()[i].GetLocation();
						i = sk->RefSkeleton.GetParentIndex(i);
					}
				}
				*/

				USkeletalBodySetup *bs = nullptr;
				int BodyIndex1 = -1;

				bs = NewObject<USkeletalBodySetup>(pa, *(FString(TEXT("dummy_for_clip"))+ a.humanBoneName.C_Str()), RF_Transactional);

				FKAggregateGeom agg;
				FKSphereElem SphereElem;
				SphereElem.Center = FVector(0);
				SphereElem.Radius = 1.f;// center.Size();// 1.f;
				agg.SphereElems.Add(SphereElem);
				SphereElem.SetName(TEXT("dummy_for_clip"));

				bs->Modify();
				bs->BoneName = a.nodeName.C_Str();
				bs->AddCollisionFrom(agg);
				bs->CollisionTraceFlag = CTF_UseSimpleAsComplex;
				// newly created bodies default to simulating
				bs->PhysicsType = PhysType_Kinematic;	// fix
														//bs->get
				bs->CollisionReponse = EBodyCollisionResponse::BodyCollision_Disabled;
				bs->DefaultInstance.InertiaTensorScale.Set(2, 2, 2);
				bs->DefaultInstance.LinearDamping = 0.f;
				bs->DefaultInstance.AngularDamping = 0.f;

				bs->InvalidatePhysicsData();
				bs->CreatePhysicsMeshes();
				BodyIndex1 = pa->SkeletalBodySetups.Add(bs);

				//break;
			}

			pa->UpdateBoundsBodiesArray();
			pa->UpdateBodySetupIndexMap();
			RefreshSkelMeshOnPhysicsAssetChange(sk);
#if WITH_EDITOR
			pa->RefreshPhysicsAssetChange();
#endif

#if WITH_EDITOR
			if (VRMConverter::IsImportMode()) {
				pa->PostEditChange();
			}
#endif
		}
	}
#endif

	return true;

}


