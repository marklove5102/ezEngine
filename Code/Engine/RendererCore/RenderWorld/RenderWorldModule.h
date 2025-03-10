#pragma once

#include <Core/World/WorldModule.h>

struct ezPerInstanceData;
struct ezRenderWorldExtractionEvent;

class EZ_RENDERERCORE_DLL ezRenderWorldModule : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderWorldModule, ezWorldModule);

public:
  ezRenderWorldModule(ezWorld* pWorld);
  virtual ~ezRenderWorldModule();

  ezArrayPtr<ezPerInstanceData> EnsureInstanceDataIsAllocatedAndMapped(const ezComponent* pOwnerComponent, ezGALDynamicBufferHandle& out_hBuffer, ezUInt32& inout_uiInstanceDataOffset, ezUInt32 uiCount = 1) const;
  void DeallocateInstanceData(const ezComponent* pOwnerComponent, ezUInt32& inout_uiInstanceDataOffset) const;

  static void FillPerInstanceData(ezPerInstanceData& out_perInstanceData, const ezGameObject* pObject, ezUInt32 uiUniqueID = 0, const ezColor& color = ezColor::White, const ezVec4& vCustomData = ezVec4(0, 1, 0, 1));

  static ezGALDynamicBufferHandle EnsureInstanceDataIsAllocatedAndFill(const ezComponent* pOwnerComponent, ezUInt32& inout_uiInstanceDataOffset, ezUInt32 uiUniqueID = 0, const ezColor& color = ezColor::White, const ezVec4& vCustomData = ezVec4(0, 1, 0, 1));
  static void EnsureInstanceDataIsDeallocated(const ezComponent* pOwnerComponent, ezUInt32& inout_uiInstanceDataOffset);

private:
  void OnExtractionEvent(const ezRenderWorldExtractionEvent& e);

  mutable ezMutex m_ExtractionMutex;

  struct BufferType
  {
    enum Enum
    {
      StaticInstanceData,
      DynamicInstanceData,

      Count
    };
  };

  ezGALDynamicBufferHandle m_hBuffers[BufferType::Count];

  struct ExtractionData
  {
    ezGALDynamicBuffer* m_pBuffers[BufferType::Count] = {};
  };

  ExtractionData m_ExtractionData;
};

#include <RendererCore/RenderWorld/Implementation/RenderWorldModule_inl.h>
