/*
 *  Created on: 2018/09/30
 *      Author: take
 */

#pragma once

#include "Object.h"

#include "DxDevice.h"
#include "IRtDevice.h"
#include <vector>
#include <memory>
namespace MinDxr {

class Scene {
private:
	std::vector<std::shared_ptr<Object>> m_objects;

	HandleHolder<ID3D12Resource> m_topLevelAccelerationStructure;
	HandleHolder<ID3D12Resource> m_bottomLevelAccelerationStructure;

public:
	void Init_Dummy();
	void Build(DxDevice& device, IRtDevice& rtDevice);


private:
	void BuildAccelerationStructure(DxDevice& device, IRtDevice& rtDevice);
};

}

