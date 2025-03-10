#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/RenderWorld/RenderWorldModule.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/DynamicBuffer.h>

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezRenderWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRenderWorldModule::ezRenderWorldModule(ezWorld* pWorld)
  : ezWorldModule(pWorld)
{
  ezRenderWorld::GetExtractionEvent().AddEventHandler(ezMakeDelegate(&ezRenderWorldModule::OnExtractionEvent, this));

  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(ezPerInstanceData);
  desc.m_uiTotalSize = 1024 * desc.m_uiStructSize; // TODO: make initial size configurable
  desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource;
  desc.m_ResourceAccess.m_bImmutable = false;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  m_hBuffers[BufferType::StaticInstanceData] = pDevice->CreateDynamicBuffer(desc, "Static Instance Data");
  m_hBuffers[BufferType::DynamicInstanceData] = pDevice->CreateDynamicBuffer(desc, "Dynamic Instance Data");
}

ezRenderWorldModule::~ezRenderWorldModule()
{
  ezRenderWorld::GetExtractionEvent().RemoveEventHandler(ezMakeDelegate(&ezRenderWorldModule::OnExtractionEvent, this));
}

ezArrayPtr<ezPerInstanceData> ezRenderWorldModule::EnsureInstanceDataIsAllocatedAndMapped(const ezComponent* pOwnerComponent, ezGALDynamicBufferHandle& out_hBuffer, ezUInt32& inout_uiInstanceDataOffset, ezUInt32 uiCount /*= 1*/) const
{
  EZ_LOCK(m_ExtractionMutex);

  const ezUInt32 uiBufferIndex = pOwnerComponent->GetOwner()->IsDynamic() ? BufferType::DynamicInstanceData : BufferType::StaticInstanceData;
  out_hBuffer = m_hBuffers[uiBufferIndex];

  auto pInstanceDataBuffer = m_ExtractionData.m_pBuffers[uiBufferIndex];
  if (pInstanceDataBuffer == nullptr)
  {
    pInstanceDataBuffer = ezGALDevice::GetDefaultDevice()->GetDynamicBuffer(out_hBuffer);
  }

  if (inout_uiInstanceDataOffset == ezInvalidIndex)
  {
    inout_uiInstanceDataOffset = pInstanceDataBuffer->Allocate(pOwnerComponent->GetHandle(), uiCount);
  }

  return pInstanceDataBuffer->MapForWriting<ezPerInstanceData>(inout_uiInstanceDataOffset);
}

void ezRenderWorldModule::DeallocateInstanceData(const ezComponent* pOwnerComponent, ezUInt32& inout_uiInstanceDataOffset) const
{
  EZ_LOCK(m_ExtractionMutex);

  if (inout_uiInstanceDataOffset != ezInvalidIndex)
  {
    const ezUInt32 uiBufferIndex = pOwnerComponent->GetOwner()->IsDynamic() ? BufferType::DynamicInstanceData : BufferType::StaticInstanceData;

    auto pInstanceDataBuffer = ezGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_hBuffers[uiBufferIndex]);

    pInstanceDataBuffer->Deallocate(inout_uiInstanceDataOffset);
    inout_uiInstanceDataOffset = ezInvalidIndex;
  }
}

// static
ezGALDynamicBufferHandle ezRenderWorldModule::EnsureInstanceDataIsAllocatedAndFill(const ezComponent* pOwnerComponent, ezUInt32& inout_uiInstanceDataOffset, ezUInt32 uiUniqueID /*= 0*/, const ezColor& color /*= ezColor::White*/, const ezVec4& vCustomData /*= ezVec4(0, 1, 0, 1)*/)
{
  const ezRenderWorldModule* pRenderWorldModule = pOwnerComponent->GetWorld()->GetModule<ezRenderWorldModule>();

  ezGALDynamicBufferHandle hInstanceDataBuffer;
  auto instanceData = pRenderWorldModule->EnsureInstanceDataIsAllocatedAndMapped(pOwnerComponent, hInstanceDataBuffer, inout_uiInstanceDataOffset);
  FillPerInstanceData(instanceData[0], pOwnerComponent->GetOwner(), uiUniqueID, color, vCustomData);

  return hInstanceDataBuffer;
}

// static
void ezRenderWorldModule::EnsureInstanceDataIsDeallocated(const ezComponent* pOwnerComponent, ezUInt32& inout_uiInstanceDataOffset)
{
  const ezRenderWorldModule* pRenderWorldModule = pOwnerComponent->GetWorld()->GetModule<ezRenderWorldModule>();

  pRenderWorldModule->DeallocateInstanceData(pOwnerComponent, inout_uiInstanceDataOffset);
}

void ezRenderWorldModule::OnExtractionEvent(const ezRenderWorldExtractionEvent& e)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  if (e.m_Type == ezRenderWorldExtractionEvent::Type::BeginExtraction)
  {
    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_hBuffers); ++i)
    {
      m_ExtractionData.m_pBuffers[i] = pDevice->GetDynamicBuffer(m_hBuffers[i]);
    }
  }
  else if (e.m_Type == ezRenderWorldExtractionEvent::Type::EndExtraction)
  {
    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_hBuffers); ++i)
    {
      m_ExtractionData.m_pBuffers[i]->UploadChangesForNextFrame();
    }

    m_ExtractionData = {};
  }
}
