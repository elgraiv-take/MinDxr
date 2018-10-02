/*
 *  Created on: 2018/09/30
 *      Author: take
 */

#include "Object.h"
#include "Util\HelperFunctions.h"

namespace MinDxr {
void Object::Build(DxDevice & device)
{
	const float size = 0.7f;
	const float depth = 1.0f;
	float vertices[] = {
		size, -size, depth,0.0f,0.0f,1.0f,
		-size,  size, depth,0.0f,0.0f,1.0f,
		size,  size, depth,0.0f,0.0f,1.0f,
	};
	UINT32 indices[] =
	{
		0, 1, 2,
	};
	HelperFunctions::CreateBuffer(device.GetDevice(), vertices, sizeof(vertices), &m_vertexBuffer.Get());
	HelperFunctions::CreateBuffer(device.GetDevice(), indices, sizeof(indices), &m_indexBuffer.Get());
}
void Object::ConstructGeometryDesc(D3D12_RAYTRACING_GEOMETRY_DESC & desc)
{
	desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	desc.Triangles.IndexBuffer = m_indexBuffer->GetGPUVirtualAddress();
	desc.Triangles.IndexCount = static_cast<UINT>(m_indexBuffer->GetDesc().Width) / sizeof(UINT32);
	desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
	desc.Triangles.Transform = 0;
	desc.Triangles.VertexBuffer.StartAddress = m_vertexBuffer->GetGPUVirtualAddress();
	desc.Triangles.VertexBuffer.StrideInBytes = sizeof(float) * 3 *2;
	desc.Triangles.VertexCount = static_cast<UINT>(m_vertexBuffer->GetDesc().Width) / desc.Triangles.VertexBuffer.StrideInBytes;
	desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
}
}



