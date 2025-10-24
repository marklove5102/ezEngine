#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class ezGameObject;
class ezAnimGraph;
class ezAnimController;

/// Runtime instance of an animation graph that evaluates it for a specific entity.
///
/// Each animated entity needs its own graph instance to maintain state (playback position,
/// blend weights, etc.). The instance references a shared ezAnimGraph and allocates instance
/// data for all nodes based on the graph's instance data allocator.
/// Call Configure() to bind it to a graph, then Update() each frame to evaluate the graph.
class EZ_RENDERERCORE_DLL ezAnimGraphInstance
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAnimGraphInstance);

public:
  ezAnimGraphInstance();
  ~ezAnimGraphInstance();

  /// Binds this instance to an animation graph and allocates per-node instance data.
  ///
  /// The graph must have been prepared via PrepareForUse() before configuring instances.
  void Configure(const ezAnimGraph& animGraph);

  /// Updates the animation graph for one frame, evaluating all nodes and generating the pose.
  void Update(ezAnimController& ref_controller, ezTime diff, ezGameObject* pTarget, const ezSkeletonResource* pSekeltonResource);

  /// Returns the instance data for a specific node.
  ///
  /// Node instance data stores per-instance state like animation playback positions or blend factors.
  template <typename T>
  T* GetAnimNodeInstanceData(const ezAnimGraphNode& node)
  {
    return reinterpret_cast<T*>(ezInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), node.m_uiInstanceDataOffset));
  }


private:
  const ezAnimGraph* m_pAnimGraph = nullptr;

  ezBlob m_InstanceData;

  // EXTEND THIS if a new type is introduced
  ezInt8* m_pTriggerInputPinStates = nullptr;
  double* m_pNumberInputPinStates = nullptr;
  bool* m_pBoolInputPinStates = nullptr;
  ezUInt16* m_pBoneWeightInputPinStates = nullptr;
  ezDynamicArray<ezHybridArray<ezUInt16, 1>> m_LocalPoseInputPinStates;
  ezUInt16* m_pModelPoseInputPinStates = nullptr;

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
};
