#include "DxDevice.h"
/*
 *  Created on: 2018/09/29
 *      Author: take
 */

#include "DxDevice.h"
#include "Util\HelperFunctions.h"

namespace MinDxr {

namespace {
inline void Check(HRESULT result) {
	HelperFunctions::ThrowIfFailed(result);
}
}

void DxDevice::Initialize()
{
	auto factoryFlags = 0;

	{
		ID3D12Debug* debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			debugController->Release();
		}
	}
	factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

	Check(CreateDXGIFactory2(factoryFlags,IID_PPV_ARGS(&m_factory.Get())));
	Check(m_factory->EnumAdapters1(0, &m_adapter.Get()));
	Check(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device.Get())));
	{
		D3D12_COMMAND_QUEUE_DESC desc{};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		Check(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_commandQueue.Get())));
	}
	Check(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator.Get())));
	Check(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList.Get())));
	m_commandList->Close();

	Check(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence.Get())));
	m_fenceValue = 1;

	m_fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (!m_fenceEvent)
	{
		throw std::exception();
	}
}

void DxDevice::ExecuteCommandLists(ID3D12CommandList ** commandLists, UINT commandListNum)
{
	m_commandQueue->ExecuteCommandLists(commandListNum, commandLists);

	auto fvalue = m_fenceValue;
	Check(m_commandQueue->Signal(m_fence.Get(), fvalue));
	m_fenceValue++;
	if (m_fence->GetCompletedValue() < fvalue)
	{
		m_fence->SetEventOnCompletion(fvalue, m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

}

