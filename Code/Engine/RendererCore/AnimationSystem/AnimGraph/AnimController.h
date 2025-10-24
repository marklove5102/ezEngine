#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>

class ezGameObject;
class ezAnimGraph;

using ezAnimGraphResourceHandle = ezTypedResourceHandle<class ezAnimGraphResource>;
using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;

EZ_DEFINE_AS_POD_TYPE(ozz::math::SimdFloat4);

/// Data passed through bone weight output pins.
///
/// Contains per-bone weight information used for animation blending.
struct ezAnimGraphPinDataBoneWeights
{
  ezUInt16 m_uiOwnIndex = 0xFFFF;
  float m_fOverallWeight = 1.0f;
  const ezAnimGraphSharedBoneWeights* m_pSharedBoneWeights = nullptr;
};

/// Data passed through local pose output pins.
///
/// Contains local-space bone transforms and optional bone weights for blending.
struct ezAnimGraphPinDataLocalTransforms
{
  ezUInt16 m_uiOwnIndex = 0xFFFF;
  ezAnimPoseGeneratorCommandID m_CommandID;
  const ezAnimGraphPinDataBoneWeights* m_pWeights = nullptr;
  float m_fOverallWeight = 1.0f;
  ezVec3 m_vRootMotion = ezVec3::MakeZero();
  bool m_bUseRootMotion = false;
};

/// Data passed through model pose output pins.
///
/// Contains model-space bone transforms and optional root motion data.
struct ezAnimGraphPinDataModelTransforms
{
  ezUInt16 m_uiOwnIndex = 0xFFFF;
  ezAnimPoseGeneratorCommandID m_CommandID;
  ezVec3 m_vRootMotion = ezVec3::MakeZero();
  ezAngle m_RootRotationX;
  ezAngle m_RootRotationY;
  ezAngle m_RootRotationZ;
  bool m_bUseRootMotion = false;
};

/// Controls animation playback for an animated entity using animation graphs.
///
/// The controller uses one or more animation graph instances and an ezAnimPoseGenerator.
/// It evaluates the graphs each time it is updated (usually each frame) and generates the final
/// skeletal pose. It also calculates root motion and may hold a blackboard for
/// sharing data between animation graph nodes.
///
/// Animation clips use a mapping from name to resource, which the ezAnimController holds.
/// This allows to reuse the same animation graphs (and thus logic) on different creatures,
/// with different sets of animations.
class EZ_RENDERERCORE_DLL ezAnimController
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAnimController);

public:
  ezAnimController();
  ~ezAnimController();

  /// Initializes the controller with a skeleton and pose generator.
  ///
  /// The blackboard is used to share data between animation graph nodes (e.g., movement speed, state flags).
  void Initialize(const ezSkeletonResourceHandle& hSkeleton, ezAnimPoseGenerator& ref_poseGenerator, const ezSharedPtr<ezBlackboard>& pBlackboard = nullptr);

  /// Updates and generates the final pose. It can be retrieved through the ezAnimPoseGenerator.
  ///
  /// Returns true if animation continues, false if it was stopped indefinitely, either due to an error or because some other system took over (usually a ragdoll).
  bool Update(ezTime diff, ezGameObject* pTarget, bool bEnableIK);

  /// Retrieves the accumulated root motion for this frame.
  void GetRootMotion(ezVec3& ref_vTranslation, ezAngle& ref_rotationX, ezAngle& ref_rotationY, ezAngle& ref_rotationZ) const;

  const ezSharedPtr<ezBlackboard>& GetBlackboard() { return m_pBlackboard; }

  ezAnimPoseGenerator& GetPoseGenerator() { return *m_pPoseGenerator; }

  /// Creates shared bone weights that can be reused across multiple animation controllers.
  ///
  /// The fill delegate is called to initialize the bone weights. If bone weights with the same
  /// name already exist, they are reused.
  static ezSharedPtr<ezAnimGraphSharedBoneWeights> CreateBoneWeights(const char* szUniqueName, const ezSkeletonResource& skeleton, ezDelegate<void(ezAnimGraphSharedBoneWeights&)> fill);

  void SetOutputModelTransform(ezAnimGraphPinDataModelTransforms* pModelTransform);
  void SetRootMotion(const ezVec3& vTranslation, ezAngle rotationX, ezAngle rotationY, ezAngle rotationZ);

  void AddOutputLocalTransforms(ezAnimGraphPinDataLocalTransforms* pLocalTransforms);

  ezAnimGraphPinDataBoneWeights* AddPinDataBoneWeights();
  ezAnimGraphPinDataLocalTransforms* AddPinDataLocalTransforms();
  ezAnimGraphPinDataModelTransforms* AddPinDataModelTransforms();

  /// Adds an animation graph to be evaluated by this controller.
  ///
  /// Multiple graphs can be active simultaneously and their outputs will be combined.
  void AddAnimGraph(const ezAnimGraphResourceHandle& hGraph);

  /// Maps a clip name to an actual animation clip resource.
  struct AnimClipInfo
  {
    ezAnimationClipResourceHandle m_hClip;
  };

  /// Returns the animation clip info for the given clip name.
  const AnimClipInfo& GetAnimationClipInfo(ezTempHashedString sClipName) const;

  /// Sets which animation clip is used for the named animation.
  ///
  /// Should only be called right at the start or when it is absolutely certain that an animation clip isn't in use right now,
  /// otherwise the running animation playback may produce weird results.
  void SetAnimationClipInfo(const ezHashedString& sClipName, const AnimClipInfo& info);

private:
  void GenerateLocalResultProcessors(const ezSkeletonResource* pSkeleton);

  ezSkeletonResourceHandle m_hSkeleton;
  ezAnimGraphPinDataModelTransforms* m_pCurrentModelTransforms = nullptr;

  ezVec3 m_vRootMotion = ezVec3::MakeZero();
  ezAngle m_RootRotationX;
  ezAngle m_RootRotationY;
  ezAngle m_RootRotationZ;

  ezDynamicArray<ozz::math::SimdFloat4, ezAlignedAllocatorWrapper> m_BlendMask;

  ezAnimPoseGenerator* m_pPoseGenerator = nullptr;
  ezSharedPtr<ezBlackboard> m_pBlackboard = nullptr;

  ezHybridArray<ezUInt32, 8> m_CurrentLocalTransformOutputs;

  static ezMutex s_SharedDataMutex;
  static ezHashTable<ezString, ezSharedPtr<ezAnimGraphSharedBoneWeights>> s_SharedBoneWeights;

  struct GraphInstance
  {
    ezAnimGraphResourceHandle m_hAnimGraph;
    ezUniquePtr<ezAnimGraphInstance> m_pInstance;
  };

  ezHybridArray<GraphInstance, 2> m_Instances;

  AnimClipInfo m_InvalidClipInfo;
  ezHashTable<ezHashedString, AnimClipInfo> m_AnimationClipMapping;

private:
  friend class ezAnimGraphTriggerOutputPin;
  friend class ezAnimGraphTriggerInputPin;
  friend class ezAnimGraphBoneWeightsInputPin;
  friend class ezAnimGraphBoneWeightsOutputPin;
  friend class ezAnimGraphLocalPoseInputPin;
  friend class ezAnimGraphLocalPoseOutputPin;
  friend class ezAnimGraphModelPoseInputPin;
  friend class ezAnimGraphModelPoseOutputPin;
  friend class ezAnimGraphLocalPoseMultiInputPin;
  friend class ezAnimGraphNumberInputPin;
  friend class ezAnimGraphNumberOutputPin;
  friend class ezAnimGraphBoolInputPin;
  friend class ezAnimGraphBoolOutputPin;

  ezHybridArray<ezAnimGraphPinDataBoneWeights, 4> m_PinDataBoneWeights;
  ezHybridArray<ezAnimGraphPinDataLocalTransforms, 4> m_PinDataLocalTransforms;
  ezHybridArray<ezAnimGraphPinDataModelTransforms, 2> m_PinDataModelTransforms;
};
