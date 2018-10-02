/*
 *  Created on: 2018/09/30
 *      Author: take
 */

#include "ShaderTable.h"

#include "Util\HelperFunctions.h"

namespace MinDxr {

namespace {
size_t AlignSize(size_t size) {
	auto alignment = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
	return ((size + alignment - 1) / alignment) * alignment;
}
}

void ShaderTable::Initialize(DxDevice & device)
{
	m_device = device.GetDevice();
}

void ShaderTable::InitializeResource(ShaderType type, UINT shaderRecordSize, UINT shaderRecordNum)
{
	D3D12_HEAP_PROPERTIES heapProp{};
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Alignment = 0;
	resDesc.Width = AlignSize(shaderRecordSize)*shaderRecordNum;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	auto& resource = m_shaderTable[static_cast<UINT>(type)].Get();

	HelperFunctions::ThrowIfFailed(m_device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource)));
}

void ShaderTable::AddShader(ShaderType type, void * shaderRecord, size_t shaderRecordSize, void * arg, size_t argSize)
{
	void* buffer;
	auto resource = m_shaderTable[static_cast<UINT>(type)].Get();
	resource->Map(0, nullptr, &buffer);
	memcpy(buffer, shaderRecord, shaderRecordSize);
	if (arg) {
		memcpy(reinterpret_cast<char*>(buffer) + shaderRecordSize, arg, argSize);
	}
	resource->Unmap(0, nullptr);
}

ID3D12Resource * ShaderTable::GetShaderResource(ShaderType type)
{
	return m_shaderTable[static_cast<UINT>(type)].Get();
}

}  // namespace MinDxr
