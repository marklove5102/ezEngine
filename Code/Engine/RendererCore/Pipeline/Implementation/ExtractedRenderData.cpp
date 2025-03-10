#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

ezExtractedRenderData::ezExtractedRenderData() = default;

void ezExtractedRenderData::AddRenderData(const ezRenderData* pRenderData, ezRenderData::Category category)
{
  m_DataPerCategory.EnsureCount(category.m_uiValue + 1);

  auto& sortableRenderData = m_DataPerCategory[category.m_uiValue].m_SortableRenderData.ExpandAndGetRef();
  sortableRenderData.m_pRenderData = pRenderData;
  sortableRenderData.m_uiSortingKey = pRenderData->GetFinalSortingKey(category, m_Camera);
}

void ezExtractedRenderData::AddFrameData(const ezRenderData* pFrameData)
{
  m_FrameData.PushBack(pFrameData);
}

void ezExtractedRenderData::SortAndBatch()
{
  EZ_PROFILE_SCOPE("ezExtractedRenderData::SortAndBatch");

  for (ezUInt32 i = 0; i < m_DataPerCategory.GetCount(); ++i)
  {
    auto& dataPerCategory = m_DataPerCategory[i];
    if (dataPerCategory.m_SortableRenderData.IsEmpty())
      continue;

    if (i == ezDefaultRenderDataCategories::Selection.m_uiValue)
    {
      int y = 0;
    }

    SortAndBatchCategory(dataPerCategory);
  }
}

void ezExtractedRenderData::Clear()
{
  for (auto& dataPerCategory : m_DataPerCategory)
  {
    dataPerCategory.m_Batches.Clear();
    dataPerCategory.m_SortableRenderData.Clear();
    dataPerCategory.m_DataOffsets.Clear();
  }

  m_FrameData.Clear();
}

ezRenderDataBatchList ezExtractedRenderData::GetRenderDataBatchesWithCategory(ezRenderData::Category category) const
{
  if (category.m_uiValue < m_DataPerCategory.GetCount())
  {
    ezRenderDataBatchList list;
    list.m_Batches = m_DataPerCategory[category.m_uiValue].m_Batches;

    return list;
  }

  return ezRenderDataBatchList();
}

ezArrayPtr<const ezRenderDataBatch::SortableRenderData> ezExtractedRenderData::GetRawRenderDataWithCategory(ezRenderData::Category category) const
{
  if (category.m_uiValue < m_DataPerCategory.GetCount())
  {
    return m_DataPerCategory[category.m_uiValue].m_SortableRenderData;
  }

  return {};
}

const ezRenderData* ezExtractedRenderData::GetFrameData(const ezRTTI* pRtti) const
{
  for (auto pData : m_FrameData)
  {
    if (pData->IsInstanceOf(pRtti))
    {
      return pData;
    }
  }

  return nullptr;
}

void ezExtractedRenderData::SortAndBatchCategory(DataPerCategory& dataPerCategory)
{
  struct RenderDataComparer
  {
    EZ_FORCE_INLINE bool Less(const ezRenderDataBatch::SortableRenderData& a, const ezRenderDataBatch::SortableRenderData& b) const
    {
      if (a.m_uiSortingKey != b.m_uiSortingKey)
      {
        return a.m_uiSortingKey < b.m_uiSortingKey;
      }

      return a.m_pRenderData->m_hOwner < b.m_pRenderData->m_hOwner;
    }
  };

  EZ_PROFILE_SCOPE("SortCategory");

  auto& data = dataPerCategory.m_SortableRenderData;

  // Sort
  data.Sort(RenderDataComparer());

  auto FillDataOffsets = [&](const ezRenderData* pRenderData)
  {
    if (pRenderData->m_uiNumInstances > 0)
    {
      auto& dataOffsets = pRenderData->m_DataOffsets;
      for (ezUInt32 uiInstanceIndex = 0; uiInstanceIndex < pRenderData->m_uiNumInstances; ++uiInstanceIndex)
      {
        auto& instanceDataOffset = dataPerCategory.m_DataOffsets.ExpandAndGetRef();
        instanceDataOffset.m_uiInstance = dataOffsets.m_uiInstance + uiInstanceIndex;
        instanceDataOffset.m_uiCustomInstance = dataOffsets.m_uiCustomInstance + uiInstanceIndex;
        instanceDataOffset.m_uiMaterial = dataOffsets.m_uiMaterial;
        instanceDataOffset.m_uiSkinning = dataOffsets.m_uiSkinning;
      }
    }
  };

  // Find batches
  const ezRenderData* pCurrentBatchRenderData = data[0].m_pRenderData;
  const ezRTTI* pCurrentBatchType = pCurrentBatchRenderData->GetDynamicRTTI();
  ezUInt32 uiCurrentBatchStartIndex = 0;
  ezUInt32 uiCurrentDataOffsetIndex = 0;
  FillDataOffsets(pCurrentBatchRenderData);

  for (ezUInt32 uiRenderDataIndex = 1; uiRenderDataIndex < data.GetCount(); ++uiRenderDataIndex)
  {
    const ezRenderData* pRenderData = data[uiRenderDataIndex].m_pRenderData;

    if (pRenderData->GetDynamicRTTI() != pCurrentBatchType || pRenderData->CanBatch(*pCurrentBatchRenderData) == false)
    {
      auto& batch = dataPerCategory.m_Batches.ExpandAndGetRef();
      batch.m_Data = ezMakeArrayPtr(&data[uiCurrentBatchStartIndex], uiRenderDataIndex - uiCurrentBatchStartIndex);
      batch.m_uiFirstDataOffsetIndex = uiCurrentDataOffsetIndex;
      batch.m_uiInstanceCount = dataPerCategory.m_DataOffsets.GetCount() - uiCurrentDataOffsetIndex;

      pCurrentBatchRenderData = pRenderData;
      pCurrentBatchType = pRenderData->GetDynamicRTTI();
      uiCurrentBatchStartIndex = uiRenderDataIndex;
      uiCurrentDataOffsetIndex = dataPerCategory.m_DataOffsets.GetCount();
    }

    FillDataOffsets(pRenderData);
  }

  auto& batch = dataPerCategory.m_Batches.ExpandAndGetRef();
  batch.m_Data = ezMakeArrayPtr(&data[uiCurrentBatchStartIndex], data.GetCount() - uiCurrentBatchStartIndex);
  batch.m_uiFirstDataOffsetIndex = uiCurrentDataOffsetIndex;
  batch.m_uiInstanceCount = dataPerCategory.m_DataOffsets.GetCount() - uiCurrentDataOffsetIndex;

  // Create or update data offsets buffer
  if (dataPerCategory.m_DataOffsets.IsEmpty() == false)
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

    if (!dataPerCategory.m_hDataOffsetsBuffer.IsInvalidated())
    {
      auto& bufferDesc = pDevice->GetBuffer(dataPerCategory.m_hDataOffsetsBuffer)->GetDescription();
      if (bufferDesc.m_uiTotalSize < dataPerCategory.m_DataOffsets.GetCount() * sizeof(ezRenderData::DataOffsets))
      {
        pDevice->DestroyBuffer(dataPerCategory.m_hDataOffsetsBuffer);
      }
    }

    const ezUInt32 uiNumDataOffsets = ezMemoryUtils::AlignSize(dataPerCategory.m_DataOffsets.GetCount(), 64u);

    if (dataPerCategory.m_hDataOffsetsBuffer.IsInvalidated())
    {
      dataPerCategory.m_DataOffsets.SetCount(uiNumDataOffsets); // make sure the buffer is large enough

      ezGALBufferCreationDescription bufferDesc;
      bufferDesc.m_uiStructSize = sizeof(ezRenderData::DataOffsets);
      bufferDesc.m_uiTotalSize = uiNumDataOffsets * bufferDesc.m_uiStructSize;
      bufferDesc.m_BufferFlags = ezGALBufferUsageFlags::VertexBuffer;
      bufferDesc.m_ResourceAccess.m_bImmutable = false;

      dataPerCategory.m_hDataOffsetsBuffer = pDevice->CreateBuffer(bufferDesc, dataPerCategory.m_DataOffsets.GetByteArrayPtr());
    }
    else
    {
      pDevice->UpdateBufferForNextFrame(dataPerCategory.m_hDataOffsetsBuffer, dataPerCategory.m_DataOffsets.GetByteArrayPtr(), 0);
    }

    // Set buffer handle on batches
    for (auto& batch : dataPerCategory.m_Batches)
    {
      batch.m_hDataOffsetsBuffer = dataPerCategory.m_hDataOffsetsBuffer;
    }
  }
}
