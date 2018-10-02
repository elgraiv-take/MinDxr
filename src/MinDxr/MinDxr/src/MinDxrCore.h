/*
 *  Created on: 2018/09/29
 *      Author: take
 */

#pragma once

#include "DxDevice.h"
#include "IRtDevice.h"
#include "DescriptorHeap.h"
#include "ShaderTable.h"

#include "Scene.h"
#include "Image.h"

#include "Util\HandleHolder.h"

#include <memory>

namespace MinDxr {

class MinDxrCore {
private:
	std::shared_ptr<DxDevice> m_dxDevice;
	std::shared_ptr<IRtDevice> m_rtDevice;
	std::shared_ptr<DescriptorHeap> m_descriptorHeap;
	std::shared_ptr<ShaderTable> m_shaderTable;

	HandleHolder<ID3D12RootSignature> m_globalRootSignature;
	HandleHolder<ID3D12RootSignature> m_localRootSignature;

	HandleHolder<ID3D12Resource> m_resultBuffer;
	HandleHolder<ID3D12Resource> m_resultOutBuffer;
	DescriptorHandle m_resultHandle;

	std::shared_ptr<Scene> m_scene;

public:
	MinDxrCore();
	void Initialize();

	void BuildScene(std::shared_ptr<Scene>& scene);

	void ExecuteRaytracing();

	void GetResult(Image& image);

private:
	void InitializeRootSigunature();
	void InitializePiplineStateObject();
	void InitializeHeap();
	void InitializeShaderTable();

};

}

