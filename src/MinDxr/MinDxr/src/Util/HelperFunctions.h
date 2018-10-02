/*
 *  Created on: 2018/09/29
 *      Author: take
 */

#pragma once

#include <d3d12_1.h>
#include <exception>

namespace MinDxr {

enum class DxrApiType{
	None,
	Dxr,
	FallbackLayer
};

class HelperFunctions{
public:
	static void ThrowIfFailed(HRESULT result){
		if(FAILED(result)){
			throw std::exception();
		}
	}

	static DxrApiType EnebleRaytracing();

	template<typename T,std::size_t SIZE>
	static std::size_t ArraySize(const T(&)[SIZE] ) {
		return SIZE;
	}

	static void CreateBuffer(ID3D12Device* device, void * data, size_t dataSize, ID3D12Resource ** resource);
	static void CreateAccelerationStructure(ID3D12Device* device, size_t size, D3D12_RESOURCE_STATES initialState, ID3D12Resource** ppRes);
};

}  // namespace MinDxr


