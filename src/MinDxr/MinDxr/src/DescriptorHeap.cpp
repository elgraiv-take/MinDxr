/*
 *  Created on: 2018/09/30
 *      Author: take
 */

#include "DescriptorHeap.h"
#include "Util\HelperFunctions.h"

namespace MinDxr {

namespace {

inline void Check(HRESULT result) {
	HelperFunctions::ThrowIfFailed(result);
}
}

 void DescriptorHeap::Initialize(DxDevice& device)
 {
	 m_device = device.GetDevice();
	 for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++)
	 {
		 D3D12_DESCRIPTOR_HEAP_DESC desc{};

		 desc.Type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i);
		 desc.NumDescriptors = 100;
		 if (i == D3D12_DESCRIPTOR_HEAP_TYPE_RTV || i == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
		 {
			 desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		 }
		 else
		 {
			 desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		 }

		 Check(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap[i].Get())));

		 m_size[i] = m_device->GetDescriptorHandleIncrementSize(desc.Type);
		 m_currentHandle[i].CpuHandle = m_heap[i]->GetCPUDescriptorHandleForHeapStart();
		 m_currentHandle[i].GpuHandle = m_heap[i]->GetGPUDescriptorHandleForHeapStart();
		 m_currentHandle[i].Index = 0;
	 }
 }

 DescriptorHandle DescriptorHeap::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type)
 {
	 DescriptorHandle newHandle;
	 newHandle.CpuHandle = m_currentHandle[type].CpuHandle;
	 newHandle.GpuHandle = m_currentHandle[type].GpuHandle;
	 newHandle.Index = m_currentHandle[type].Index;
	 m_currentHandle[type].CpuHandle.ptr += m_size[type];
	 m_currentHandle[type].GpuHandle.ptr += m_size[type];
	 m_currentHandle[type].Index++;
	 return newHandle;
 }

 ID3D12DescriptorHeap * DescriptorHeap::GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE type)
 {
	 return m_heap[type].Get();
 }

 }  // namespace MinDxr
