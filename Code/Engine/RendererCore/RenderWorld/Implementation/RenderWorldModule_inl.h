#pragma once

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

// static
EZ_FORCE_INLINE void ezRenderWorldModule::FillPerInstanceData(ezPerInstanceData& out_perInstanceData, const ezGameObject* pObject, ezUInt32 uiUniqueID /*= 0*/, const ezColor& color /*= ezColor::White*/, const ezVec4& vCustomData /*= ezVec4(0, 1, 0, 1)*/)
{
  const auto& globalTransform = pObject->GetGlobalTransform();

  ezMat4 objectToWorld = globalTransform.GetAsMat4();
  out_perInstanceData.ObjectToWorld = objectToWorld;

  if (globalTransform.ContainsUniformScale())
  {
    out_perInstanceData.ObjectToWorldNormal = objectToWorld;
  }
  else
  {
    ezMat3 mInverse = objectToWorld.GetRotationalPart();
    mInverse.Invert(0.0f).IgnoreResult();
    // we explicitly ignore the return value here (success / failure)
    // because when we have a scale of 0 (which happens temporarily during editing) that would be annoying

    out_perInstanceData.ObjectToWorldNormal = mInverse.GetTranspose();
  }

  out_perInstanceData.BoundingSphereRadius = pObject->GetGlobalBounds().m_fSphereRadius;
  out_perInstanceData.GameObjectID = uiUniqueID;
  out_perInstanceData.RandomSeed = pObject->GetStableRandomSeed();
  out_perInstanceData.Reserved = 0;

  out_perInstanceData.Color = color;
  out_perInstanceData.CustomData = vCustomData;
}
