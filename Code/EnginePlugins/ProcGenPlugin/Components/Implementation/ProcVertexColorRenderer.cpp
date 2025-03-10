#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <ProcGenPlugin/Components/ProcVertexColorComponent.h>
#include <ProcGenPlugin/Components/ProcVertexColorRenderer.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>


// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcVertexColorRenderer, 1, ezRTTIDefaultAllocator<ezProcVertexColorRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezProcVertexColorRenderer::ezProcVertexColorRenderer() = default;
ezProcVertexColorRenderer::~ezProcVertexColorRenderer() = default;

void ezProcVertexColorRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(ezGetStaticRTTI<ezProcVertexColorRenderData>());
}

void ezProcVertexColorRenderer::SetAdditionalData(const ezRenderViewContext& renderViewContext, const ezMeshRenderData* pRenderData) const
{
  SUPER::SetAdditionalData(renderViewContext, pRenderData);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;

  auto pProcVertexColorRenderData = static_cast<const ezProcVertexColorRenderData*>(pRenderData);
  if (auto pVertexColorBuffer = pDevice->GetDynamicBuffer(pProcVertexColorRenderData->m_hVertexColorBuffer))
  {
    ezBindGroupBuilder& bindGroupDraw = renderViewContext.m_pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
    bindGroupDraw.BindBuffer("perInstanceVertexColors", pVertexColorBuffer->GetBufferForRendering());
  }
}


EZ_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Components_Implementation_ProcVertexColorRenderer);
