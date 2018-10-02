/*
 *  Created on: 2018/09/29
 *      Author: take
 */

#pragma once

#include "Util\HandleHolder.h"

#include <d3d12.h>
#include <dxgi1_6.h>

namespace MinDxr {

class DxDevice {
private:
	HandleHolder<IDXGIFactory5> m_factory;
	HandleHolder<IDXGIAdapter1> m_adapter;
	//HandleHolder<IDXGIOutput6> m_output;
	HandleHolder<ID3D12Device> m_device;
	HandleHolder<ID3D12CommandQueue> m_commandQueue;
	HandleHolder<ID3D12CommandAllocator> m_commandAllocator;
	HandleHolder<ID3D12GraphicsCommandList> m_commandList;


	HandleHolder<ID3D12Fence> m_fence;
	UINT m_fenceValue = 0;
	HANDLE m_fenceEvent = nullptr;
public:

	void Initialize();

	ID3D12Device* GetDevice() { return m_device.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() { return m_commandList.Get(); }
	ID3D12CommandAllocator* GetCommandAllocator() { return m_commandAllocator.Get(); }
	ID3D12CommandQueue* GetCommandQueue() { return m_commandQueue.Get(); }

	void ExecuteCommandLists(ID3D12CommandList** commandLists, UINT commandListNum);
private:
};

}  // namespace MinDxr

