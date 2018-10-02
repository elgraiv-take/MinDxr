/*
 *  Created on: 2018/09/29
 *      Author: take
 */

#pragma once

#include <d3d12_1.h>

namespace MinDxr {

class DxDevice;
class ShaderTable;
struct DescriptorHandle;

class IRtDevice {
public:
	virtual ~IRtDevice() = default;
	virtual void Initialize(DxDevice& device) = 0;
	virtual void CreateStateObject(const D3D12_STATE_OBJECT_DESC& desc) = 0;
	virtual void CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& desc, ID3D12RootSignature** rootSignature) = 0;
	virtual void* GetShaderIdentifier(LPCWSTR name) = 0;
	virtual UINT GetShaderIdentifierSize() = 0;
	virtual void GetRaytracingAccelerationStructurePrebuildInfo(
		D3D12_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_DESC& prebuildDesc,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& prebuildInfo
	) = 0;

	virtual D3D12_RESOURCE_STATES GetAccelerationStructureResourceState() = 0;
	virtual void ConstructRaytracingInstance(
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& topInfo,
		ID3D12Resource* topResource,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& bottomInfo,
		ID3D12Resource* bottomReource,
		ID3D12Resource*& instance
	) = 0;
	virtual void BuildAccelerationStructure(
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& topDesc,
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& bottomDesc,
		ID3D12Resource* bottomReource,
		ID3D12GraphicsCommandList* dxCommandList
	) = 0;
	virtual void ExecuteRaytracing(
		UINT width, UINT height,
		ShaderTable* shaderTable,
		ID3D12GraphicsCommandList* dxCommandList,
		DescriptorHandle& resultHandle
	) = 0;
};
}


