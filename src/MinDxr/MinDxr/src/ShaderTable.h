/*
 *  Created on: 2018/09/30
 *      Author: take
 */

#pragma once


#include "DxDevice.h"

#include "Util\HandleHolder.h"

#include <d3d12_1.h>

namespace MinDxr {

enum class ShaderType {
	RayGeneration,
	HitGroup,
	Miss,
};

class ShaderTable {
private:
	HandleHolder<ID3D12Resource> m_shaderTable[3];
	ID3D12Device * m_device;
public:
	void Initialize(DxDevice& device);
	void InitializeResource(ShaderType type, UINT shaderRecordNum, UINT shaderRecordSize);
	void AddShader(ShaderType type,void* shaderRecord, size_t shaderRecordSize, void* arg, size_t argSize);
	ID3D12Resource* GetShaderResource(ShaderType type);
};


}  // namespace MinDxr

