/*
 *  Created on: 2018/09/29
 *      Author: take
 */

#include "HelperFunctions.h"

#include "HandleHolder.h"

namespace MinDxr {

DxrApiType HelperFunctions::EnebleRaytracing()
{
	{
		HandleHolder<ID3D12Device> testDevice;
		UUID experimentalFeatures[] = { D3D12ExperimentalShaderModels, D3D12RaytracingPrototype };
		auto result=
			SUCCEEDED(D3D12EnableExperimentalFeatures(2, experimentalFeatures, nullptr, nullptr))
			&& SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice.Get())));
		if (result) {
			return DxrApiType::Dxr;
		}
	}
	{
		HandleHolder<ID3D12Device> testDevice;
		UUID experimentalFeatures[] = { D3D12ExperimentalShaderModels };
		auto result =
			SUCCEEDED(D3D12EnableExperimentalFeatures(1, experimentalFeatures, nullptr, nullptr))
			&& SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice.Get())));
		if (result) {
			return DxrApiType::FallbackLayer;
		}
	}
	return DxrApiType::None;
}

void HelperFunctions::CreateBuffer(ID3D12Device* device, void * data, size_t dataSize, ID3D12Resource ** resource)
{
	D3D12_HEAP_PROPERTIES heapProp{};
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = dataSize;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ThrowIfFailed(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(resource)));

	void *buffer;
	(*resource)->Map(0, nullptr, &buffer);
	memcpy(buffer, data, dataSize);
	(*resource)->Unmap(0, nullptr);
}

void HelperFunctions::CreateAccelerationStructure(ID3D12Device* device, size_t size, D3D12_RESOURCE_STATES initialState, ID3D12Resource** ppRes)
{
	D3D12_HEAP_PROPERTIES heapProp{};
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ThrowIfFailed(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &desc, initialState, nullptr, IID_PPV_ARGS(ppRes)));
}


}  // namespace MinDxr
