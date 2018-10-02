/*
 *  Created on: 2018/09/30
 *      Author: take
 */

#pragma once

#include "DxDevice.h"
#include "Util\HandleHolder.h"

#include <d3d12.h>

namespace MinDxr {

struct DescriptorHandle {
	D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle;
	UINT Index;
};

class DescriptorHeap{
private:
	ID3D12Device * m_device;
	UINT m_size[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	HandleHolder<ID3D12DescriptorHeap> m_heap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	DescriptorHandle m_currentHandle[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
public:
	void Initialize(DxDevice& device);
	DescriptorHandle Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type);
	ID3D12DescriptorHeap* GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);
};

}  // namespace MinDxr


