/*
 *  Created on: 2018/09/30
 *      Author: take
 */


#include "Scene.h"
#include "Util\HelperFunctions.h"
#include <algorithm>

namespace MinDxr {
void Scene::Init_Dummy()
{
	auto obj = std::make_shared<Object>();
	m_objects.push_back(obj);
}
void Scene::Build(DxDevice& device,IRtDevice& rtDevice)
{
	for (auto& object : m_objects) {
		object->Build(device);
	}

	BuildAccelerationStructure(device, rtDevice);
}
void Scene::BuildAccelerationStructure(DxDevice& device, IRtDevice& rtDevice)
{
	auto commandList = device.GetCommandList();
	HelperFunctions::ThrowIfFailed(commandList->Reset(device.GetCommandAllocator(), nullptr));

	auto geometryNum = m_objects.size();
	std::unique_ptr<D3D12_RAYTRACING_GEOMETRY_DESC[]> geometryDescs = std::make_unique<D3D12_RAYTRACING_GEOMETRY_DESC[]>(geometryNum);

	for (auto i = 0; i < geometryNum; i++) {
		m_objects[i]->ConstructGeometryDesc(geometryDescs[i]);
	}

	auto buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topPrebuildInfo{};
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomPrebuildInfo{};
	{
		D3D12_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_DESC desc{};
		desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		desc.Flags = buildFlags;
		desc.NumDescs = geometryNum;
		desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		desc.pGeometryDescs = nullptr;

		rtDevice.GetRaytracingAccelerationStructurePrebuildInfo(desc, topPrebuildInfo);
		
		if (topPrebuildInfo.ResultDataMaxSizeInBytes == 0) {
			throw std::exception();
		}

		desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		desc.pGeometryDescs = geometryDescs.get();
		rtDevice.GetRaytracingAccelerationStructurePrebuildInfo(desc, bottomPrebuildInfo);

		if (bottomPrebuildInfo.ResultDataMaxSizeInBytes == 0) {
			throw std::exception();
		}
	}

	// スクラッチリソースを作成する
	// スクラッチリソースはAS構築時に使用する一時バッファ
	// ASを生成してしまえば基本不要
	HandleHolder<ID3D12Resource> pScrachResource;
	HelperFunctions::CreateAccelerationStructure(
		device.GetDevice(),
		std::max(topPrebuildInfo.ScratchDataSizeInBytes, bottomPrebuildInfo.ScratchDataSizeInBytes),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		&pScrachResource.Get());
	

	// トップとボトムのASを生成する
	{
		D3D12_RESOURCE_STATES initialState = rtDevice.GetAccelerationStructureResourceState();

		HelperFunctions::CreateAccelerationStructure(
			device.GetDevice(),
			topPrebuildInfo.ResultDataMaxSizeInBytes,
			initialState,
			&m_topLevelAccelerationStructure.Get());

		HelperFunctions::CreateAccelerationStructure(
			device.GetDevice(),
			bottomPrebuildInfo.ResultDataMaxSizeInBytes,
			initialState,
			&m_bottomLevelAccelerationStructure.Get());
	}

	// トップレベルに登録するインスタンスのバッファを構築する
	HandleHolder<ID3D12Resource> instance;
	
	{
		rtDevice.ConstructRaytracingInstance(
			topPrebuildInfo,m_topLevelAccelerationStructure.Get(),
			bottomPrebuildInfo,m_bottomLevelAccelerationStructure.Get(),
			instance.Get()
		);
	}

	// ボトムレベルASを構築するための記述子
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomBuildDesc{};
	{
		bottomBuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		bottomBuildDesc.Flags = buildFlags;
		bottomBuildDesc.ScratchAccelerationStructureData = { pScrachResource->GetGPUVirtualAddress(), pScrachResource->GetDesc().Width };
		bottomBuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		bottomBuildDesc.DestAccelerationStructureData = { m_bottomLevelAccelerationStructure->GetGPUVirtualAddress(), bottomPrebuildInfo.ResultDataMaxSizeInBytes };
		bottomBuildDesc.NumDescs = geometryNum;
		bottomBuildDesc.pGeometryDescs = geometryDescs.get();
	}

	// トップレベルASを構築するための記述子
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topBuildDesc = bottomBuildDesc;
	{
		topBuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		topBuildDesc.DestAccelerationStructureData = { m_topLevelAccelerationStructure->GetGPUVirtualAddress(), topPrebuildInfo.ResultDataMaxSizeInBytes };
		topBuildDesc.NumDescs = 1;
		topBuildDesc.pGeometryDescs = nullptr;
		topBuildDesc.InstanceDescs = instance->GetGPUVirtualAddress();
		topBuildDesc.ScratchAccelerationStructureData = { pScrachResource->GetGPUVirtualAddress(), pScrachResource->GetDesc().Width };
	}

	rtDevice.BuildAccelerationStructure(topBuildDesc, bottomBuildDesc, m_bottomLevelAccelerationStructure.Get(), commandList);

	// コマンド実行
	HelperFunctions::ThrowIfFailed(commandList->Close());
	ID3D12CommandList* commandListArray[] = { commandList };
	device.ExecuteCommandLists(commandListArray,1);
	
}

}

