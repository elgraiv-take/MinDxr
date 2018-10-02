/*
 *  Created on: 2018/09/30
 *      Author: take
 */

#pragma once

#include "Util\HandleHolder.h"

#include "DxDevice.h"
#include <d3d12_1.h>

namespace MinDxr {

class Object {
private:
	HandleHolder<ID3D12Resource> m_vertexBuffer;
	HandleHolder<ID3D12Resource> m_indexBuffer;
public:
	void Build(DxDevice& device);
	void ConstructGeometryDesc(D3D12_RAYTRACING_GEOMETRY_DESC& desc);
};

}


