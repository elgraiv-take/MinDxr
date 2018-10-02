/*
 *  Created on: 2018/09/29
 *      Author: take
 */



#include "MinDxrCore.h"

#include "RtDeviceFallbackLayer.h"


#include "Util\HelperFunctions.h"

#include <vector>
#include <array>

#include "CompiledShader\Raytracing.h"

namespace MinDxr {

namespace {
struct Viewport
{
	float left;
	float top;
	float right;
	float bottom;
};

struct RayGenConstantBuffer
{
	Viewport viewport;
	Viewport stencil;
};


static LPCWSTR kRayGenName = L"RayGenerator";
static LPCWSTR kClosestHitName = L"ClosestHitProcessor";
static LPCWSTR kMissName = L"MissProcessor";
static LPCWSTR kHitGroupName = L"HitGroup";
static const int kWindowWidth = 1280;
static const int kWindowHeight = 720;

inline void Check(HRESULT result) {
	HelperFunctions::ThrowIfFailed(result);
}
}

MinDxrCore::MinDxrCore()
{
}

void MinDxrCore::Initialize()
{
	auto apiType = HelperFunctions::EnebleRaytracing();
	m_dxDevice = std::make_shared<DxDevice>();
	m_dxDevice->Initialize();
	m_descriptorHeap = std::make_shared<DescriptorHeap>();
	m_descriptorHeap->Initialize(*m_dxDevice);
	switch (apiType)
	{
	case DxrApiType::Dxr:
		break;
	case DxrApiType::FallbackLayer:
		m_rtDevice = std::make_shared<RtDeviceFallbackLayer>(m_descriptorHeap.get());
		break;
	default:
		break;
	}
	m_rtDevice->Initialize(*m_dxDevice);
	
	InitializeRootSigunature();
	InitializePiplineStateObject();
	InitializeHeap();

	m_shaderTable = std::make_shared<ShaderTable>();
	m_shaderTable->Initialize(*m_dxDevice);
	InitializeShaderTable();
}

void MinDxrCore::InitializeRootSigunature()
{
	{
		D3D12_DESCRIPTOR_RANGE ranges[] = {
			{ D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
		{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
		};
		D3D12_ROOT_PARAMETER params[2];
		for (int i = 0; i < 2; i++)
		{
			params[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			params[i].DescriptorTable.NumDescriptorRanges = 1;
			params[i].DescriptorTable.pDescriptorRanges = ranges + i;
		}
		D3D12_ROOT_SIGNATURE_DESC sigDesc{};
		sigDesc.NumParameters = 2;
		sigDesc.pParameters = params;
		sigDesc.NumStaticSamplers = 0;
		sigDesc.pStaticSamplers = nullptr;
		sigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

		m_rtDevice->CreateRootSignature(sigDesc, &m_globalRootSignature.Get());
	}
	{
		D3D12_ROOT_PARAMETER params[1];
		params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		params[0].Constants.ShaderRegister = 0;
		params[0].Constants.RegisterSpace = 0;
		params[0].Constants.Num32BitValues = sizeof(RayGenConstantBuffer);
		D3D12_ROOT_SIGNATURE_DESC sigDesc{};
		sigDesc.NumParameters = 1;
		sigDesc.pParameters = params;
		sigDesc.NumStaticSamplers = 0;
		sigDesc.pStaticSamplers = nullptr;
		sigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		m_rtDevice->CreateRootSignature(sigDesc, &m_localRootSignature.Get());
	}
}

void MinDxrCore::InitializePiplineStateObject()
{
	std::vector<D3D12_STATE_SUBOBJECT> subobjects;
	subobjects.reserve(32);
	auto AddSubobject = [&](D3D12_STATE_SUBOBJECT_TYPE type, const void* desc)
	{
		D3D12_STATE_SUBOBJECT sub;
		sub.Type = type;
		sub.pDesc = desc;
		subobjects.push_back(sub);
	};
	D3D12_EXPORT_DESC libExport[] = {
		{ kRayGenName,		nullptr, D3D12_EXPORT_FLAG_NONE },
	    { kClosestHitName,	nullptr, D3D12_EXPORT_FLAG_NONE },
	    { kMissName,		nullptr, D3D12_EXPORT_FLAG_NONE },
	};

	D3D12_DXIL_LIBRARY_DESC dxilDesc{};
	dxilDesc.DXILLibrary.pShaderBytecode = g_ShaderBin;
	dxilDesc.DXILLibrary.BytecodeLength = sizeof(g_ShaderBin);
	dxilDesc.NumExports = static_cast<UINT>(HelperFunctions::ArraySize(libExport));
	dxilDesc.pExports = libExport;
	AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &dxilDesc);

	D3D12_HIT_GROUP_DESC hitGroupDesc{};
	hitGroupDesc.HitGroupExport = kHitGroupName;
	hitGroupDesc.ClosestHitShaderImport = kClosestHitName;
	AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitGroupDesc);

	D3D12_RAYTRACING_SHADER_CONFIG shaderConfigDesc{};
	shaderConfigDesc.MaxPayloadSizeInBytes = sizeof(float) * 4;		// float4 color
	shaderConfigDesc.MaxAttributeSizeInBytes = sizeof(float) * 2;	// float2 barycentrics
	AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &shaderConfigDesc);

	AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, &m_localRootSignature.Get());

	LPCWSTR kExports[] = {
		kRayGenName,
		kMissName,
		kHitGroupName,
	};
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION assocDesc{};
	assocDesc.pSubobjectToAssociate = &subobjects.back();
	assocDesc.NumExports = static_cast<UINT>(HelperFunctions::ArraySize(kExports));
	assocDesc.pExports = kExports;
	AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &assocDesc);

	AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, &m_globalRootSignature.Get());

	D3D12_RAYTRACING_PIPELINE_CONFIG rtConfigDesc{};
	rtConfigDesc.MaxTraceRecursionDepth = 1;
	AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &rtConfigDesc);

	D3D12_STATE_OBJECT_DESC psoDesc{};
	psoDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	psoDesc.pSubobjects = subobjects.data();
	psoDesc.NumSubobjects = (UINT)subobjects.size();

	m_rtDevice->CreateStateObject(psoDesc);
}

void MinDxrCore::InitializeHeap()
{
	auto backbufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	{

		D3D12_RESOURCE_DESC uavDesc{};
		uavDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		uavDesc.Alignment = 0;
		uavDesc.Width = kWindowWidth;
		uavDesc.Height = kWindowHeight;
		uavDesc.DepthOrArraySize = 1;
		uavDesc.MipLevels = 1;
		uavDesc.Format = backbufferFormat;
		uavDesc.SampleDesc.Count = 1;
		uavDesc.SampleDesc.Quality = 0;
		uavDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		uavDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		D3D12_HEAP_PROPERTIES heapProp{};
		heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProp.CreationNodeMask = 1;
		heapProp.VisibleNodeMask = 1;

		Check(m_dxDevice->GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&m_resultBuffer.Get())));
	}
	{
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp;
		UINT nrow;
		UINT64 rowsize, size;
		m_dxDevice->GetDevice()->GetCopyableFootprints(&m_resultBuffer->GetDesc(), 0, 1, 0, &fp, &nrow, &rowsize, &size);

		D3D12_RESOURCE_DESC uavDesc{};
		uavDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		uavDesc.Alignment = 0;
		uavDesc.Width = size;
		uavDesc.Height = 1;
		uavDesc.DepthOrArraySize = 1;
		uavDesc.MipLevels = 1;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.SampleDesc.Count = 1;
		uavDesc.SampleDesc.Quality = 0;
		uavDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		uavDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12_HEAP_PROPERTIES heapProp{};
		heapProp.Type = D3D12_HEAP_TYPE_READBACK;
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProp.CreationNodeMask = 1;
		heapProp.VisibleNodeMask = 1;
		Check(m_dxDevice->GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_resultOutBuffer.Get())));
	}

	m_resultHandle = m_descriptorHeap->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};
	viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	m_dxDevice->GetDevice()->CreateUnorderedAccessView(m_resultBuffer.Get(), nullptr, &viewDesc, m_resultHandle.CpuHandle);
}

void MinDxrCore::InitializeShaderTable()
{
	auto shaderIdentifierSize= m_rtDevice->GetShaderIdentifierSize();

	struct RootArguments {
		RayGenConstantBuffer cb;
	} rootArguments;
	rootArguments.cb.viewport = { -1.0f, -1.0f, 1.0f, 1.0f };
	rootArguments.cb.stencil = { -0.9f, -0.9f, 0.9f, 0.9f };
	UINT rootArgumentsSize = sizeof(rootArguments);


	{
		auto rayGenShaderIdentifier = m_rtDevice->GetShaderIdentifier(kRayGenName);
		m_shaderTable->InitializeResource(ShaderType::RayGeneration, 1, shaderIdentifierSize + rootArgumentsSize);
		m_shaderTable->AddShader(ShaderType::RayGeneration, rayGenShaderIdentifier, shaderIdentifierSize, &rootArguments, rootArgumentsSize);
	}
	{
		auto missShaderIdentifier = m_rtDevice->GetShaderIdentifier(kMissName);
		m_shaderTable->InitializeResource(ShaderType::Miss, 1, shaderIdentifierSize);
		m_shaderTable->AddShader(ShaderType::Miss, missShaderIdentifier, shaderIdentifierSize, nullptr, 0);
	}
	{
		auto hitGroupShaderIdentifier = m_rtDevice->GetShaderIdentifier(kHitGroupName);
		m_shaderTable->InitializeResource(ShaderType::HitGroup, 1, shaderIdentifierSize);
		m_shaderTable->AddShader(ShaderType::HitGroup, hitGroupShaderIdentifier, shaderIdentifierSize, nullptr, 0);

	}
}


void MinDxrCore::BuildScene(std::shared_ptr<Scene>& scene)
{
	m_scene = scene;
	m_scene->Build(*m_dxDevice,*m_rtDevice);
}

void MinDxrCore::ExecuteRaytracing()
{

	auto commandList = m_dxDevice->GetCommandList();
	Check(commandList->Reset(m_dxDevice->GetCommandAllocator(), nullptr));

	commandList->SetComputeRootSignature(m_globalRootSignature.Get());
	m_rtDevice->ExecuteRaytracing(kWindowWidth, kWindowHeight, m_shaderTable.get(),commandList,m_resultHandle);

	{

		D3D12_RESOURCE_BARRIER barrier[2]{};
		{

			barrier[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier[1].Transition.pResource = m_resultOutBuffer.Get();
			barrier[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		}
		{
			barrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier[0].Transition.pResource = m_resultBuffer.Get();
			barrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
		}
		commandList->ResourceBarrier(1, barrier);


		//commandList->CopyResource(m_resultOutBuffer.Get(), m_resultBuffer.Get());
		D3D12_TEXTURE_COPY_LOCATION td, ts;
		{
			ts.pResource = m_resultBuffer.Get();
			ts.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			ts.SubresourceIndex = 0;
		}
		{

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp;
			UINT nrow;
			UINT64 rowsize, size;
			m_dxDevice->GetDevice()->GetCopyableFootprints(&m_resultBuffer->GetDesc(), 0, 1, 0, &fp, &nrow, &rowsize, &size);
			td.pResource = m_resultOutBuffer.Get();
			td.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			td.PlacedFootprint = fp;
		}

		commandList->CopyTextureRegion(&td, 0, 0, 0, &ts, nullptr);
		{

			barrier[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier[1].Transition.pResource = m_resultOutBuffer.Get();
			barrier[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		}
		{
			barrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier[0].Transition.pResource = m_resultBuffer.Get();
			barrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
			barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}
		commandList->ResourceBarrier(1, barrier);
	}

	Check(commandList->Close());
	ID3D12CommandList* cmdLists[] = { commandList };
	m_dxDevice->ExecuteCommandLists(cmdLists, 1);
}

void MinDxrCore::GetResult(Image& image)
{
	void *buffer;
	Check(m_resultOutBuffer->Map(0, nullptr, &buffer));
	memcpy(image.GetData(), buffer, image.GetSize());
	m_resultOutBuffer->Unmap(0, nullptr);
}


}

