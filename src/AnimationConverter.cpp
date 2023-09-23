#include "AnimationConverter.h"

namespace Converter
{
	AnimationConverter::AnimationConverter(FbxScene* scene, igSkeleton* skeleton, igAnimationListRef animations) :
		scene(scene),
		boneNodes(skeleton->getBoneCount()),
		traversal(igCommonTraversal::instantiateRefFromPool(kIGMemoryPoolTemporary)),
		actor(igActor::instantiateRefFromPool(kIGMemoryPoolTemporary)),
		animations(animations)
	{
		igAnimationCombinerRef combiner = igAnimationCombiner::instantiateRefFromPool(kIGMemoryPoolActor);

		combiner->configure(skeleton);
		actor->configure(combiner);

		igMatrix44f* matrix = combiner->getBoneMatrixArray();

		for (igInt i = 0; i < combiner->getBoneCount(); i++, matrix++)
			matrix->makeIdentity();

		matrix = combiner->getBlendMatrixArray();

		for (igInt i = 0; i < combiner->getJointCount(); i++, matrix++)
			matrix->makeIdentity();

		matrix = combiner->getAnimationCacheMatrixArray();

		for (igInt i = 0; i < combiner->getAnimationCacheMatrixArrayLength(); i++, matrix++)
			matrix->makeIdentity();

		combiner->setBindPose(-1);
	}

	AnimationConverter::~AnimationConverter()
	{
		animations = NULL;
		actor = NULL;
		traversal = NULL;
		boneNodes.Clear();
	}

	fbxsdk::FbxNode* AnimationConverter::ProcessBone(igInt boneIndex)
	{
		igSkeleton* skeleton = actor->getCombiner()->getSkeleton();
		igSkeletonBoneInfo* bone = skeleton->getBoneInfo(boneIndex);
		igInt childCount = skeleton->getChildCount(boneIndex);
		fbxsdk::FbxNode* node = fbxsdk::FbxNode::Create(scene, bone->getName());
		fbxsdk::FbxSkeleton* skeletonAttr = fbxsdk::FbxSkeleton::Create(scene, bone->getName());

		node->SetNodeAttribute(skeletonAttr);

		if (skeleton->getParent(boneIndex) <= 0)
			skeletonAttr->SetSkeletonType(fbxsdk::FbxSkeleton::eRoot);
		else
			skeletonAttr->SetSkeletonType(childCount > 0 ? fbxsdk::FbxSkeleton::eLimb : fbxsdk::FbxSkeleton::eEffector);

		igMatrix44f* matrix = actor->getCombiner()->getAnimationCacheMatrix(boneIndex);
		igVec3f translation;
		igFloat rotX, rotY, rotZ;

		matrix->getTranslation(translation);
		matrix->getRotation(rotX, rotY, rotZ);
		node->LclTranslation = Utils::ig2FbxDouble3(translation);
		node->LclRotation = FbxDouble3(rotX, rotY, rotZ);
		boneNodes.SetAt(boneIndex, node);

		for (igInt i = 0; i < childCount; i++)
			node->AddChild(ProcessBone(skeleton->getSpecifiedChild(boneIndex, i)));
		return node;
	}

	void AnimationConverter::Convert()
	{
		fbxsdk::FbxNode* actorRoot = ProcessBone();

		actorRoot->SetName("ActorRoot");
		scene->GetRootNode()->AddChild(actorRoot);

		igAnimationCombiner* combiner = actor->getCombiner();
		igLong increment = 1000000000L / 30L;
		fbxsdk::FbxTime keyframeTime;

		for (igInt i = 0; i < animations->getCount(); i++)
		{
			igAnimationBlendParameters parameters;
			igAnimation* animation = animations->get(i);
			fbxsdk::FbxAnimStack* animStack = fbxsdk::FbxAnimStack::Create(scene, animation->getName());
			fbxsdk::FbxAnimLayer* animLayer = fbxsdk::FbxAnimLayer::Create(scene, animation->getName());

			animStack->AddMember(animLayer);
			parameters._animation = animation;
			parameters._startRatio = 0.0F;
			parameters._endRatio = 1.0F;
			parameters._transitionMode = igAnimationState::TransitionMode::kNone;
			parameters._combineMode = igAnimationState::CombineMode::kOverride;
			combiner->add(parameters);

			for (igLong time = 0; time <= animation->getDuration(); time += increment)
			{
				igDouble timeInSeconds = time / 1000000000.0;

				actor->setActorCacheUsed(false);
				traversal->setUserTimeState(true);
				traversal->setTimeAsLong(time);
				actor->updateAnimation(traversal);

				for (igInt j = 1; j < combiner->getBoneCount(); j++)
				{
					FbxNode* boneNode = boneNodes[j];
					igMatrix44f* matrix = combiner->getAnimationCacheMatrix(j);
					igVec3f translation;
					igFloat rotX, rotY, rotZ;

					matrix->getTranslation(translation);
					matrix->getRotation(rotX, rotY, rotZ);

					fbxsdk::FbxAnimCurve* curve = boneNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
					igInt keyIndex;

					if (curve)
					{
						curve->KeyModifyBegin();
						keyframeTime.SetSecondDouble(timeInSeconds);
						curve->KeySetValue(keyIndex = curve->KeyAdd(keyframeTime), translation[0]);
						curve->KeySetInterpolation(keyIndex, fbxsdk::FbxAnimCurveDef::eInterpolationLinear);
						curve->KeyModifyEnd();
					}

					if (curve = boneNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y, true))
					{
						curve->KeyModifyBegin();
						keyframeTime.SetSecondDouble(timeInSeconds);
						curve->KeySetValue(keyIndex = curve->KeyAdd(keyframeTime), translation[1]);
						curve->KeySetInterpolation(keyIndex, fbxsdk::FbxAnimCurveDef::eInterpolationLinear);
						curve->KeyModifyEnd();
					}

					if (curve = boneNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z, true))
					{
						curve->KeyModifyBegin();
						keyframeTime.SetSecondDouble(timeInSeconds);
						curve->KeySetValue(keyIndex = curve->KeyAdd(keyframeTime), translation[2]);
						curve->KeySetInterpolation(keyIndex, fbxsdk::FbxAnimCurveDef::eInterpolationLinear);
						curve->KeyModifyEnd();
					}

					if (curve = boneNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X, true))
					{
						curve->KeyModifyBegin();
						keyframeTime.SetSecondDouble(timeInSeconds);
						curve->KeySetValue(keyIndex = curve->KeyAdd(keyframeTime), rotX);
						curve->KeySetInterpolation(keyIndex, fbxsdk::FbxAnimCurveDef::eInterpolationLinear);
						curve->KeyModifyEnd();
					}

					if (curve = boneNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y, true))
					{
						curve->KeyModifyBegin();
						keyframeTime.SetSecondDouble(timeInSeconds);
						curve->KeySetValue(keyIndex = curve->KeyAdd(keyframeTime), rotY);
						curve->KeySetInterpolation(keyIndex, fbxsdk::FbxAnimCurveDef::eInterpolationLinear);
						curve->KeyModifyEnd();
					}

					if (curve = boneNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z, true))
					{
						curve->KeyModifyBegin();
						keyframeTime.SetSecondDouble(timeInSeconds);
						curve->KeySetValue(keyIndex = curve->KeyAdd(keyframeTime), rotZ);
						curve->KeySetInterpolation(keyIndex, fbxsdk::FbxAnimCurveDef::eInterpolationLinear);
						curve->KeyModifyEnd();
					}
				}
			}
		}

		fbxsdk::FbxPose* bindPose = fbxsdk::FbxPose::Create(scene, "Bind Pose");
		fbxsdk::FbxPose* restPose = fbxsdk::FbxPose::Create(scene, "Rest Pose");
		fbxsdk::FbxTime time;

		time.SetSecondDouble(0.0);
		bindPose->SetIsBindPose(true);
		scene->AddPose(bindPose);
		scene->AddPose(restPose);

		for (igInt i = 1; i < scene->GetNodeCount(); i++)
		{
			fbxsdk::FbxNode* node = scene->GetNode(i);
			bindPose->Add(node, node->EvaluateGlobalTransform());
			restPose->Add(node, node->EvaluateGlobalTransform(time));
		}
	}
}