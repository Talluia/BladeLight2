// VRM4U Copyright (c) 2019 Haruyoshi Yamamoto. This software is released under the MIT License.


#include "VrmAnimInstanceCopy.h"
#include "VrmMetaObject.h"
#include "VrmAssetListObject.h"
#include "VrmUtil.h"
#include "VrmBPFunctionLibrary.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/Morphtarget.h"
#include "Animation/Rig.h"
//#include "BoneControllers/AnimNode_Fabrik.h"
//#include "BoneControllers/AnimNode_TwoBoneIK.h"
//#include "BoneControllers/AnimNode_SplineIK.h"
#include "AnimNode_VrmSpringBone.h"


namespace {
	const FString table[] = {
		"hips",
		"leftUpperLeg",
		"rightUpperLeg",
		"leftLowerLeg",
		"rightLowerLeg",
		"leftFoot",
		"rightFoot",
		"spine",
		"chest",
		"neck",
		"head",
		"leftShoulder",
		"rightShoulder",
		"leftUpperArm",
		"rightUpperArm",
		"leftLowerArm",
		"rightLowerArm",
		"leftHand",
		"rightHand",
		"leftToes",
		"rightToes",
		"leftEye",
		"rightEye",
		"jaw",
		"leftThumbProximal",
		"leftThumbIntermediate",
		"leftThumbDistal",
		"leftIndexProximal",
		"leftIndexIntermediate",
		"leftIndexDistal",
		"leftMiddleProximal",
		"leftMiddleIntermediate",
		"leftMiddleDistal",
		"leftRingProximal",
		"leftRingIntermediate",
		"leftRingDistal",
		"leftLittleProximal",
		"leftLittleIntermediate",
		"leftLittleDistal",
		"rightThumbProximal",
		"rightThumbIntermediate",
		"rightThumbDistal",
		"rightIndexProximal",
		"rightIndexIntermediate",
		"rightIndexDistal",
		"rightMiddleProximal",
		"rightMiddleIntermediate",
		"rightMiddleDistal",
		"rightRingProximal",
		"rightRingIntermediate",
		"rightRingDistal",
		"rightLittleProximal",
		"rightLittleIntermediate",
		"rightLittleDistal",
		"upperChest"
	};
	/*
	const FString table[] = {
		"Root",
		"Pelvis",
		"spine_01",
		"spine_02",
		"spine_03",
		"clavicle_l",
		"UpperArm_L",
		"lowerarm_l",
		"Hand_L",
		"index_01_l",
		"index_02_l",
		"index_03_l",
		"middle_01_l",
		"middle_02_l",
		"middle_03_l",
		"pinky_01_l",
		"pinky_02_l",
		"pinky_03_l",
		"ring_01_l",
		"ring_02_l",
		"ring_03_l",
		"thumb_01_l",
		"thumb_02_l",
		"thumb_03_l",
		"lowerarm_twist_01_l",
		"upperarm_twist_01_l",
		"clavicle_r",
		"UpperArm_R",
		"lowerarm_r",
		"Hand_R",
		"index_01_r",
		"index_02_r",
		"index_03_r",
		"middle_01_r",
		"middle_02_r",
		"middle_03_r",
		"pinky_01_r",
		"pinky_02_r",
		"pinky_03_r",
		"ring_01_r",
		"ring_02_r",
		"ring_03_r",
		"thumb_01_r",
		"thumb_02_r",
		"thumb_03_r",
		"lowerarm_twist_01_r",
		"upperarm_twist_01_r",
		"neck_01",
		"head",
		"Thigh_L",
		"calf_l",
		"calf_twist_01_l",
		"Foot_L",
		"ball_l",
		"thigh_twist_01_l",
		"Thigh_R",
		"calf_r",
		"calf_twist_01_r",
		"Foot_R",
		"ball_r",
		"thigh_twist_01_r",
		"ik_foot_root",
		"ik_foot_l",
		"ik_foot_r",
		"ik_hand_root",
		"ik_hand_gun",
		"ik_hand_l",
		"ik_hand_r",
		"Custom_1",
		"Custom_2",
		"Custom_3",
		"Custom_4",
		"Custom_5",
	};
	*/
}

namespace {
	// for UE4.19-4.22
	template<class BaseType, class PoseType>
	static void ConvertToLocalPoses(const BaseType &basePose, PoseType& OutPose)
	{
		checkSlow(basePose.Pose.IsValid());
		OutPose = basePose.GetPose();

		// now we need to convert back to local bases
		// only convert back that has been converted to mesh base
		// if it was local base, and if it hasn't been modified
		// that is still okay even if parent is changed, 
		// that doesn't mean this local has to change
		// go from child to parent since I need parent inverse to go back to local
		// root is same, so no need to do Index == 0
		const FCSPose<FCompactPose>::BoneIndexType RootBoneIndex(0);
		if (basePose.GetComponentSpaceFlags()[RootBoneIndex])
		{
			OutPose[RootBoneIndex] = basePose.GetPose()[RootBoneIndex];
		}

		const int32 NumBones = basePose.GetPose().GetNumBones();
		for (int32 Index = NumBones - 1; Index > 0; Index--)
		{
			const FCSPose<FCompactPose>::BoneIndexType BoneIndex(Index);
			if (basePose.GetComponentSpaceFlags()[BoneIndex])
			{
				const FCSPose<FCompactPose>::BoneIndexType ParentIndex = basePose.GetPose().GetParentBoneIndex(BoneIndex);
				OutPose[BoneIndex].SetToRelativeTransform(OutPose[ParentIndex]);
				OutPose[BoneIndex].NormalizeRotation();
			}
		}
	}
}

FVrmAnimInstanceCopyProxy::FVrmAnimInstanceCopyProxy()
{
	if (SpringBoneNode.Get() == nullptr) {
		SpringBoneNode = MakeShareable(new FAnimNode_VrmSpringBone());
	}
}

FVrmAnimInstanceCopyProxy::FVrmAnimInstanceCopyProxy(UAnimInstance* InAnimInstance)
	: FAnimInstanceProxy(InAnimInstance)
{
	if (SpringBoneNode.Get() == nullptr) {
		SpringBoneNode = MakeShareable(new FAnimNode_VrmSpringBone());
	}
}


void FVrmAnimInstanceCopyProxy::Initialize(UAnimInstance* InAnimInstance) {
}
bool FVrmAnimInstanceCopyProxy::Evaluate(FPoseContext& Output) {

	if (bCopyStop) {
		return false;
	}

	if (bAnimStop && bUseAnimStop) {
		Output.Pose.CopyBonesFrom(CachedPose);
		return true;
	}


	//Output.ResetToRefPose();

	UVrmAnimInstanceCopy *animInstance = Cast<UVrmAnimInstanceCopy>(GetAnimInstanceObject());
	if (animInstance == nullptr) {
		return false;
	}
	if (animInstance->DstVrmAssetList == nullptr) {
	//if (animInstance->SrcVrmAssetList == nullptr || animInstance->DstVrmAssetList == nullptr) {
			return false;
	}

	const UVrmMetaObject *srcMeta = nullptr;
	if (animInstance->SrcVrmAssetList) {
		srcMeta = animInstance->SrcVrmAssetList->VrmMetaObject;
	}

	const UVrmMetaObject *dstMeta = animInstance->DstVrmAssetList->VrmMetaObject;
	if (dstMeta) {
		if (animInstance->DstVrmMetaForCustomSpring) {
			dstMeta = animInstance->DstVrmMetaForCustomSpring;
		}
	}

	if (dstMeta == nullptr) {
	//if (srcMeta == nullptr || dstMeta == nullptr) {
		return false;
	}


	auto srcSkeletalMeshComp = animInstance->SrcSkeletalMeshComponent;

	if (srcSkeletalMeshComp == nullptr) {
		return false;
	}

	// ref pose
	const auto &dstRefSkeletonTransform = GetSkelMeshComponent()->SkeletalMesh->RefSkeleton.GetRefBonePose();
	const auto &srcRefSkeletonTransform = srcSkeletalMeshComp->SkeletalMesh->RefSkeleton.GetRefBonePose();

	auto &pose = Output.Pose;


	float HeightScale = 1.f;
	bool bUniqueRootBone = true;
	int BoneCount = 0;
	for (const auto &t : table) {
		FName srcName, dstName;
		srcName = dstName = NAME_None;

		++BoneCount;
		{
			if (srcMeta) {
				auto a = srcMeta->humanoidBoneTable.Find(t);
				if (a) {
					srcName = **a;
				}
			} else {
				for (auto &bonemap : VRMUtil::table_ue4_vrm) {
					if (bonemap.BoneVRM == t) {
						srcName = *bonemap.BoneUE4;
					}
				}
			}
		}
		//auto srcName = srcSkeletalMeshComp->SkeletalMesh->Skeleton->GetRigBoneMapping(t);
		if (srcName == NAME_None) {
			continue;
		}

		{
			auto a = dstMeta->humanoidBoneTable.Find(t);
			if (a) {
				dstName = **a;
			}
		}
		//auto dstName = GetSkelMeshComponent()->SkeletalMesh->Skeleton->GetRigBoneMapping(t);
		if (dstName == NAME_None) {
			continue;
		}

		auto dstIndex = GetSkelMeshComponent()->GetBoneIndex(dstName);
		if (dstIndex < 0) {
			continue;
		}
		auto srcIndex = srcSkeletalMeshComp->GetBoneIndex(srcName);
		if (srcIndex < 0) {
			continue;
		}


		const auto srcCurrentTrans = srcSkeletalMeshComp->GetSocketTransform(srcName, RTS_ParentBoneSpace);
		const auto srcRefTrans = srcRefSkeletonTransform[srcIndex];
		const auto dstRefTrans = dstRefSkeletonTransform[dstIndex];

		FTransform dstTrans = dstRefTrans;

		{
			FQuat diff = srcCurrentTrans.GetRotation()*srcRefTrans.GetRotation().Inverse();
			dstTrans.SetRotation(dstTrans.GetRotation()*diff);
		}
		//dstTrans.SetLocation(dstRefTrans.GetLocation());// +srcCurrentTrans.GetLocation() - srcRefTrans.GetLocation());

		//FVector newLoc = dstTrans.GetLocation();
		if (BoneCount == 1) {
			int32_t p = dstIndex;
			float HipHeight = 0;
			while (p != INDEX_NONE) {

				HipHeight += dstRefSkeletonTransform[p].GetLocation().Z;
				p = GetSkelMeshComponent()->SkeletalMesh->RefSkeleton.GetParentIndex(p);
			}
			//FVector diff = srcCurrentTrans.GetLocation() - srcRefTrans.GetLocation();
			//FVector scale = dstRefTrans.GetLocation() / srcRefTrans.GetLocation();
			FVector diff = srcCurrentTrans.GetLocation() - srcRefTrans.GetLocation();
			float scale = HipHeight / srcRefTrans.GetLocation().Z;

			if (bIgnoreHeightScale) {
				scale = 1.f;
			}

			dstTrans.SetTranslation(dstRefTrans.GetLocation() + diff * scale);
			HeightScale = scale;
			if (dstIndex == 0) {
				bUniqueRootBone = false;
			}
			//if (newLoc.Normalize()) {
				//auto dstRefLocation = dstRefTrans.GetLocation();
				//newLoc *= dstRefLocation.Size();
				//dstTrans.SetLocation(newLoc);
			//}
		}

		FCompactPose::BoneIndexType bi(dstIndex);
		pose[bi] = dstTrans;
	}

	// root bone
	if (bUniqueRootBone){
		FCompactPose::BoneIndexType bi(0);
		const auto srcTrans = srcSkeletalMeshComp->GetSocketTransform(srcSkeletalMeshComp->GetBoneName(0), RTS_ParentBoneSpace);

		auto t = srcTrans;
		t.SetTranslation(t.GetTranslation() * HeightScale);
		pose[bi] = t;
	}


	if (bIgnoreVRMSwingBone == false) {
		bool bCalc = true;
		{
			bool bPlay, bSIE, bEditor;
			bPlay = bSIE = bEditor = true;
			UVrmBPFunctionLibrary::VRMGetPlayMode(bPlay, bSIE, bEditor);
			if (bPlay == false) {
				bCalc = false;
			}
		}

		if (bCalc) {
			if (SpringBoneNode.Get() == nullptr) {
				SpringBoneNode = MakeShareable(new FAnimNode_VrmSpringBone());
			}

			if (SpringBoneNode.Get()) {
				auto& springBone = *SpringBoneNode.Get();

				springBone.VrmMetaObject = dstMeta;
				springBone.bCallByAnimInstance = true;
				springBone.CurrentDeltaTime = CurrentDeltaTime;

				// Evaluate the child and convert
				//FComponentSpacePoseContext InputCSPose(Output.AnimInstanceProxy);
				//f.EvaluateSkeletalControl_AnyThread(
				FAnimationInitializeContext InitContext(this);


				if (springBone.IsSprintInit() == false) {
					springBone.Initialize_AnyThread(InitContext);
					springBone.ComponentPose.SetLinkNode(&springBone);
					CalcCount = 0;
					springBone.CurrentDeltaTime = 1.f / 20.f;
				}
				if (CalcCount < 3) {
					FVector bak = springBone.gravityAdd;
					++CalcCount;
					FComponentSpacePoseContext InputCSPose(this);
					for (int i = 0; i < 20; ++i) {
						InputCSPose.Pose.InitPose(Output.Pose);
						springBone.EvaluateComponentSpace_AnyThread(InputCSPose);
						ConvertToLocalPoses(InputCSPose.Pose, Output.Pose);
					}
					springBone.gravityAdd = bak;

				} else {
					FComponentSpacePoseContext InputCSPose(this);
					InputCSPose.Pose.InitPose(Output.Pose);
					springBone.EvaluateComponentSpace_AnyThread(InputCSPose);
					ConvertToLocalPoses(InputCSPose.Pose, Output.Pose);
				}

			}
		}
	} else {
		SpringBoneNode = nullptr;
	}


	if (bUseAnimStop) {
		CachedPose.CopyBonesFrom(Output.Pose);
	}

	return true;
}
#if	UE_VERSION_OLDER_THAN(4,24,0)
void FVrmAnimInstanceCopyProxy::UpdateAnimationNode(float DeltaSeconds) {
	CurrentDeltaTime = DeltaSeconds;
}
#else
void FVrmAnimInstanceCopyProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext) {
	CurrentDeltaTime = InContext.GetDeltaTime();
}
#endif

/////

UVrmAnimInstanceCopy::UVrmAnimInstanceCopy(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

FAnimInstanceProxy* UVrmAnimInstanceCopy::CreateAnimInstanceProxy() {
	myProxy = new FVrmAnimInstanceCopyProxy(this);
	myProxy->bIgnoreVRMSwingBone = bIgnoreVRMSwingBone;
	return myProxy;
}


void UVrmAnimInstanceCopy::NativeInitializeAnimation() {
	for (int i = 0; i < FMath::Min(InitialMorphValue.Num(), InitialMorphName.Num()); ++i) {
		SetMorphTarget(InitialMorphName[i], InitialMorphValue[i]);
	}
}
void UVrmAnimInstanceCopy::NativeUpdateAnimation(float DeltaSeconds) {
}
void UVrmAnimInstanceCopy::NativePostEvaluateAnimation() {
	myProxy->bUseAnimStop = bUseAnimStop;
	myProxy->bAnimStop = bAnimStop;
	myProxy->bIgnoreHeightScale = bIgnoreHeightScale;
	myProxy->bCopyStop = bCopyStop;
}
void UVrmAnimInstanceCopy::NativeUninitializeAnimation() {
}
void UVrmAnimInstanceCopy::NativeBeginPlay() {
}

void UVrmAnimInstanceCopy::SetSkeletalMeshCopyData(UVrmAssetListObject *dstAssetList, USkeletalMeshComponent *srcSkeletalMesh, UVrmAssetListObject *srcAssetList) {
	SrcSkeletalMeshComponent = srcSkeletalMesh;
	SrcVrmAssetList = srcAssetList;
	DstVrmAssetList = dstAssetList;
}

void UVrmAnimInstanceCopy::SetSkeletalMeshCopyDataForCustomSpring(UVrmMetaObject *dstMetaForCustomSpring) {
	DstVrmMetaForCustomSpring = dstMetaForCustomSpring;
}



void UVrmAnimInstanceCopy::SetVrmSpringBoneParam(float gravityScale, FVector gravityAdd, float stiffnessScale, float stiffnessAdd) {
	if (myProxy == nullptr) return;
	auto a = myProxy->SpringBoneNode.Get();
	if (a == nullptr) return;

	a->gravityScale = gravityScale;
	a->gravityAdd = gravityAdd;
	a->stiffnessScale = stiffnessScale;
	a->stiffnessAdd = stiffnessAdd;
}

void UVrmAnimInstanceCopy::SetVrmSpringBoneBool(bool bIgnoreVrmSpringBone, bool bIgnorePhysicsCollision, bool bIgnoreVRMCollision) {
	if (myProxy == nullptr) return;
	auto a = myProxy->SpringBoneNode.Get();
	if (a == nullptr) return;

	this->bIgnoreVRMSwingBone = bIgnoreVrmSpringBone;
	myProxy->bIgnoreVRMSwingBone = bIgnoreVrmSpringBone;
	a->bIgnorePhysicsCollision = bIgnorePhysicsCollision;
	a->bIgnoreVRMCollision = bIgnoreVRMCollision;
}

void UVrmAnimInstanceCopy::SetVrmSpringBoneIgnoreWingBone(const TArray<FName> &boneNameList) {
	if (myProxy == nullptr) return;
	auto a = myProxy->SpringBoneNode.Get();
	if (a == nullptr) return;

	a->NoWindBoneNameList = boneNameList;
}


