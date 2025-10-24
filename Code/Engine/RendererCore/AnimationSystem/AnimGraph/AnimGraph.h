#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

/// Defines an animation graph with nodes and connections.
///
/// An animation graph is a node-based system for procedural animation blending and control.
/// Nodes represent operations like sampling animation clips, blending or switching poses.
/// The graph is prepared once via PrepareForUse() which validates connections and optimizes the graph.
/// Multiple ezAnimGraphInstance objects can share the same graph to evaluate it for different entities.
class EZ_RENDERERCORE_DLL ezAnimGraph
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAnimGraph);

public:
  ezAnimGraph();
  ~ezAnimGraph();

  void Clear();

  /// Adds a node to the graph and returns a pointer to it.
  ezAnimGraphNode* AddNode(ezUniquePtr<ezAnimGraphNode>&& pNode);

  /// Creates a connection from a source node's output pin to a destination node's input pin.
  void AddConnection(const ezAnimGraphNode* pSrcNode, ezStringView sSrcPinName, ezAnimGraphNode* pDstNode, ezStringView sDstPinName);

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

  const ezInstanceDataAllocator& GetInstanceDataAlloator() const { return m_InstanceDataAllocator; }
  ezArrayPtr<const ezUniquePtr<ezAnimGraphNode>> GetNodes() const { return m_Nodes; }

  /// Prepares the graph for evaluation by sorting nodes, validating connections, and allocating instance data.
  ///
  /// Must be called after all nodes and connections have been added and before creating instances.
  void PrepareForUse();

private:
  friend class ezAnimGraphInstance;

  struct ConnectionTo
  {
    ezString m_sSrcPinName;
    const ezAnimGraphNode* m_pDstNode = nullptr;
    ezString m_sDstPinName;
    ezAnimGraphPin* m_pSrcPin = nullptr;
    ezAnimGraphPin* m_pDstPin = nullptr;
  };

  struct ConnectionsTo
  {
    ezHybridArray<ConnectionTo, 2> m_To;
  };

  void SortNodesByPriority();
  void PreparePinMapping();
  void AssignInputPinIndices();
  void AssignOutputPinIndices();
  ezUInt16 ComputeNodePriority(const ezAnimGraphNode* pNode, ezMap<const ezAnimGraphNode*, ezUInt16>& inout_Prios, ezUInt16& inout_uiOutputPrio) const;

  bool m_bPreparedForUse = true;
  ezUInt32 m_uiInputPinCounts[ezAnimGraphPin::Type::ENUM_COUNT];
  ezUInt32 m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::ENUM_COUNT];
  ezMap<const ezAnimGraphNode*, ConnectionsTo> m_From;

  ezDynamicArray<ezUniquePtr<ezAnimGraphNode>> m_Nodes;
  ezDynamicArray<ezHybridArray<ezUInt16, 1>> m_OutputPinToInputPinMapping[ezAnimGraphPin::ENUM_COUNT];
  ezInstanceDataAllocator m_InstanceDataAllocator;

  friend class ezAnimGraphTriggerOutputPin;
  friend class ezAnimGraphNumberOutputPin;
  friend class ezAnimGraphBoolOutputPin;
  friend class ezAnimGraphBoneWeightsOutputPin;
  friend class ezAnimGraphLocalPoseOutputPin;
  friend class ezAnimGraphModelPoseOutputPin;
};
