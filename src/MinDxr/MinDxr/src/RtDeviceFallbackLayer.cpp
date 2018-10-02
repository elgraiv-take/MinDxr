/*
 *  Created on: 2018/09/29
 *      Author: take
 */

#include "RtDeviceFallbackLayer.h"

#include "DxDevice.h"

#include "Util\HelperFunctions.h"
#include "ShaderTable.h"

namespace MinDxr {


namespace {
inline void Check(HRESULT result) {
	HelperFunctions::ThrowIfFailed(result);
}
}

RtDeviceFallbackLayer::RtDeviceFallbackLayer(DescriptorHeap * descriptorHeap) :
	m_descriptorHeap(descriptorHeap)
{
}

void RtDeviceFallbackLayer::Initialize(DxDevice& device)
{
	Check(D3D12CreateRaytracingFallbackDevice(device.GetDevice(), CreateRaytracingFallbackDeviceFlags::None, 0, IID_PPV_ARGS(&m_rtDevice.Get())));
	m_rtDevice->QueryRaytracingCommandList(device.GetCommandList(), IID_PPV_ARGS(&m_rtCommandList.Get()));
	m_dxDevice = device.GetDevice();
}

void RtDeviceFallbackLayer::CreateStateObject(const D3D12_STATE_OBJECT_DESC & desc)
{
	Check(m_rtDevice->CreateStateObject(&desc, IID_PPV_ARGS(&m_pipelineStateObject.Get())));
}

void RtDeviceFallbackLayer::CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC & desc, ID3D12RootSignature** rootSignature)
{
	HandleHolder<ID3DBlob> blob;
	HandleHolder<ID3DBlob> error;
	Check(m_rtDevice->D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob.Get(), &error.Get()));
	Check(m_rtDevice->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(rootSignature)));
}

void * RtDeviceFallbackLayer::GetShaderIdentifier(LPCWSTR name)
{
	return m_pipelineStateObject->GetShaderIdentifier(name);
}

UINT RtDeviceFallbackLayer::GetShaderIdentifierSize()
{
	return m_rtDevice->GetShaderIdentifierSize();
}

void RtDeviceFallbackLayer::GetRaytracingAccelerationStructurePrebuildInfo(D3D12_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_DESC & prebuildDesc, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO & prebuildInfo)
{
	m_rtDevice->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildDesc, &prebuildInfo);
}

D3D12_RESOURCE_STATES RtDeviceFallbackLayer::GetAccelerationStructureResourceState()
{
	return m_rtDevice->GetAccelerationStructureResourceState();
}

void RtDeviceFallbackLayer::ConstructRaytracingInstance(
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& topInfo,
	ID3D12Resource* topResource,
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& bottomInfo,
	ID3D12Resource* bottomReource,
	ID3D12Resource*& instance
)
{
	// トップレベルに登録するインスタンス記述子
	D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC desc{};
	desc.Transform[0] = desc.Transform[5] = desc.Transform[10] = 1;		// トランスフォーム行列は単位行列
	desc.InstanceMask = 1;
	UINT numBufferElements = static_cast<UINT>(bottomInfo.ResultDataMaxSizeInBytes) / sizeof(UINT32);
	desc.AccelerationStructure = CreateFallbackWrappedPointer(bottomReource, numBufferElements);
	HelperFunctions::CreateBuffer(m_dxDevice, &desc, sizeof(desc), &instance);

	numBufferElements = static_cast<UINT>(topInfo.ResultDataMaxSizeInBytes) / sizeof(UINT32);
	m_topLevelAccelerationStructurePtr = CreateFallbackWrappedPointer(topResource, numBufferElements);
}


WRAPPED_GPU_POINTER RtDeviceFallbackLayer::CreateFallbackWrappedPointer(ID3D12Resource * resource, UINT bufferNumElements)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
	desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	desc.Buffer.NumElements = bufferNumElements;


	UINT index = 0;
	if (!m_rtDevice->UsingRaytracingDriver())
	{
		auto heap = m_descriptorHeap->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		index = heap.Index;
		m_dxDevice->CreateUnorderedAccessView(resource, nullptr, &desc, heap.CpuHandle);
	}
	return m_rtDevice->GetWrappedPointerSimple(index, resource->GetGPUVirtualAddress());
}

void RtDeviceFallbackLayer::BuildAccelerationStructure(
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& topDesc,
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& bottomDesc,
	ID3D12Resource* bottomReource,
	ID3D12GraphicsCommandList* dxCommandList
)
{
	ID3D12DescriptorHeap* pDescriptorHeaps[] = { m_descriptorHeap->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	m_rtCommandList->SetDescriptorHeaps(1, pDescriptorHeaps);


	m_rtCommandList->BuildRaytracingAccelerationStructure(&bottomDesc);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = bottomReource;
	dxCommandList->ResourceBarrier(1, &barrier);

	m_rtCommandList->BuildRaytracingAccelerationStructure(&topDesc);
}

void RtDeviceFallbackLayer::ExecuteRaytracing(
	UINT width,UINT height,
	ShaderTable* shaderTable,
	ID3D12GraphicsCommandList* dxCommandList,
	DescriptorHandle& resultHandle

)
{

	// グローバルルートシグネチャを設定

	ID3D12DescriptorHeap* descHeaps[] = {
		m_descriptorHeap->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
		m_descriptorHeap->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),
	};


	m_rtCommandList->SetDescriptorHeaps(2, descHeaps);
	dxCommandList->SetComputeRootDescriptorTable(0, resultHandle.GpuHandle);
	m_rtCommandList->SetTopLevelAccelerationStructure(1, m_topLevelAccelerationStructurePtr);


	D3D12_FALLBACK_DISPATCH_RAYS_DESC desc{};
	auto hitGroup = shaderTable->GetShaderResource(ShaderType::HitGroup);
	desc.HitGroupTable.StartAddress = hitGroup->GetGPUVirtualAddress();
	desc.HitGroupTable.SizeInBytes = hitGroup->GetDesc().Width;
	desc.HitGroupTable.StrideInBytes = desc.HitGroupTable.SizeInBytes;
	auto miss = shaderTable->GetShaderResource(ShaderType::Miss);
	desc.MissShaderTable.StartAddress = miss->GetGPUVirtualAddress();
	desc.MissShaderTable.SizeInBytes = miss->GetDesc().Width;
	desc.MissShaderTable.StrideInBytes = desc.MissShaderTable.SizeInBytes;
	auto rayGeneration = shaderTable->GetShaderResource(ShaderType::RayGeneration);
	desc.RayGenerationShaderRecord.StartAddress = rayGeneration->GetGPUVirtualAddress();
	desc.RayGenerationShaderRecord.SizeInBytes = rayGeneration->GetDesc().Width;
	desc.Width = width;
	desc.Height = height;
	m_rtCommandList->DispatchRays(m_pipelineStateObject.Get(), &desc);
}

}  // namespace MinDxr
