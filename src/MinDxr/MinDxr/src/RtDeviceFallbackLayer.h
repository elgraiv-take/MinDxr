/*
 *  Created on: 2018/09/29
 *      Author: take
 */

#pragma once

#include "IRtDevice.h"

#include "DescriptorHeap.h"

#include "Util\HandleHolder.h"
#include <D3D12RaytracingFallback.h>

namespace MinDxr {

class RtDeviceFallbackLayer :public IRtDevice{
private:
	HandleHolder<ID3D12RaytracingFallbackDevice> m_rtDevice;
	HandleHolder<ID3D12RaytracingFallbackCommandList> m_rtCommandList;
	HandleHolder<ID3D12RaytracingFallbackStateObject> m_pipelineStateObject;

	ID3D12Device* m_dxDevice;
	DescriptorHeap* m_descriptorHeap;

	WRAPPED_GPU_POINTER m_topLevelAccelerationStructurePtr;
public:
	RtDeviceFallbackLayer(DescriptorHeap* descriptorHeap);
	void Initialize(DxDevice& device)override;
	void CreateStateObject(const D3D12_STATE_OBJECT_DESC& desc) override;
	void CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& desc, ID3D12RootSignature** rootSignature)override;
	void* GetShaderIdentifier(LPCWSTR name) override;
	UINT GetShaderIdentifierSize() override;
	void GetRaytracingAccelerationStructurePrebuildInfo(
		D3D12_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_DESC& prebuildDesc,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& prebuildInfo
	) override;
	D3D12_RESOURCE_STATES GetAccelerationStructureResourceState()override;
	void ConstructRaytracingInstance(
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& topInfo,
		ID3D12Resource* topResource,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& bottomInfo,
		ID3D12Resource* bottomReource,
		ID3D12Resource*& instance
	)override;

	void BuildAccelerationStructure(
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& topDesc,
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& bottomDesc,
		ID3D12Resource* bottomReource,
		ID3D12GraphicsCommandList* dxCommandList
	) override;

	void ExecuteRaytracing(
		UINT width, UINT height,
		ShaderTable* shaderTable,
		ID3D12GraphicsCommandList* dxCommandList,
		DescriptorHandle& resultHandle)override;
private:
	WRAPPED_GPU_POINTER CreateFallbackWrappedPointer(ID3D12Resource* resource, UINT bufferNumElements);
};

}

