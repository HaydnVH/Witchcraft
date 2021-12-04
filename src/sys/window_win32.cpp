#ifdef _WIN32
#ifdef RENDERER_D3D12
#include "window.h"

#include <algorithm>
#include <chrono>
#include "appconfig.h"
#include "debug.h"

#include <Windows.h>
#include <wrl.h>
using namespace Microsoft::WRL;
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include "tools/dxmathhelper.h"
//#include <d3dx12.h>



namespace {

	inline void ThrowIfFailed(HRESULT hr) {
		if (FAILED(hr)) {
			throw std::exception();
		}
	}

	const uint8_t numFrames = 3;
	bool useWarp = false;

	uint32_t clientWidth = 1280;
	uint32_t clientHeight = 720;

	bool isInitialized = false;


	HWND hwnd = nullptr;
	RECT wndRect;

	ComPtr<ID3D12Device2> device;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<IDXGISwapChain4> swapChain;
	ComPtr<ID3D12Resource> backBuffers[numFrames];
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<ID3D12CommandAllocator> commandAllocators[numFrames];
	ComPtr<ID3D12DescriptorHeap> RTVDescriptorHeap;
	UINT RTVDescriptorSize;
	UINT currentBackBufferIndex;

	ComPtr<ID3D12Fence> fence;
	uint64_t fenceValue = 0;
	uint64_t frameFenceValues[numFrames] = {0};
	HANDLE fenceEvent;

	bool vsync = true;
	bool tearingSupported = false;
	bool fullscreen = false;

	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	void EnableDebugLayer() {
#ifdef _DEBUG
		ComPtr<ID3D12Debug> debugInterface;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
		debugInterface->EnableDebugLayer();
#endif
	}

}; // namespace <anon>

namespace sys {
namespace window {

	bool Init() {
		if (hwnd) {
			debug::fatal("Trying to open the window a second time.\n");
			return false;
		}

		// Register window class
		{
			WNDCLASSEXW wc = { 0 };
			wc.cbSize = sizeof(WNDCLASSEX);
			wc.style = CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc = &WndProc;
			wc.hInstance = GetModuleHandle(nullptr);
			wc.lpszClassName = L"witchcraft_window_class";
			if (RegisterClassExW(&wc) == 0) {
				debug::fatal("Failed to register window class.\n");
				debug::errmore("Error code : ", GetLastError(), ".\n");
				return false;
			}
		}

		// Create the window
		{
			int screenWidth = GetSystemMetrics(SM_CXSCREEN);
			int screenHeight = GetSystemMetrics(SM_CYSCREEN);

			RECT windowRect = { 0, 0, (LONG)clientWidth, (LONG)clientHeight };
			AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);
			int windowWidth = windowRect.right - windowRect.left;
			int windowHeight = windowRect.bottom - windowRect.top;

			int windowX = std::max(0, (screenWidth - windowWidth) / 2);
			int windowY = std::max(0, (screenHeight - windowHeight) / 2);

			hwnd = CreateWindowExW(
				0,
				L"witchcraft_window_class",
				utf8_to_utf16(appconfig::getAppName()).c_str(),
				WS_OVERLAPPEDWINDOW,
				windowX, windowY,
				windowWidth, windowHeight,
				nullptr, nullptr,
				GetModuleHandle(nullptr), nullptr);

			if (!hwnd) {
				debug::fatal("Failed to open window.\n");
				debug::errmore("Error code : ", GetLastError(), ".\n");
				return false;
			}
		}

		// Get adapter & create device
		{
			ComPtr<IDXGIFactory4> dxgiFactory;
			UINT createFactoryFlags = 0;
#ifdef _DEBUG
			createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
			ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

			ComPtr<IDXGIAdapter1> dxgiAdapter1;
			ComPtr<IDXGIAdapter4> dxgiAdapter4;

			size_t maxDedicatedVideoMemory = 0;
			for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i) {
				DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
				dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);
				if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
					SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
					dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
				{
					maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
					ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
				}
			}

			ComPtr<ID3D12Device2> d3d12Device2;
			ThrowIfFailed(D3D12CreateDevice(dxgiAdapter4.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));

#ifdef _DEBUG
			ComPtr<ID3D12InfoQueue> pInfoQueue;
			if (SUCCEEDED(d3d12Device2.As(&pInfoQueue))) {
				pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
				pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
				pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
			}

			D3D12_MESSAGE_SEVERITY severities[] = {
				D3D12_MESSAGE_SEVERITY_INFO
			};

			D3D12_MESSAGE_ID denyIds[] = {
				D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
				D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
				D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
			};

			D3D12_INFO_QUEUE_FILTER newFilter = {};
			newFilter.DenyList.NumSeverities = _countof(severities);
			newFilter.DenyList.pSeverityList = severities;
			newFilter.DenyList.NumIDs = _countof(denyIds);
			newFilter.DenyList.pIDList = denyIds;
			if (pInfoQueue) { ThrowIfFailed(pInfoQueue->PushStorageFilter(&newFilter)); }
#endif

			device = d3d12Device2;
		}

		ShowWindow(hwnd, SW_SHOW);
		return true;
	}

	void Shutdown() {
		if (hwnd) DestroyWindow(hwnd);
	}

	bool HandleMessages() {
		return true;
	}

	void getWindowSize(int& w, int& h) {}
	void getDrawableSize(int& w, int& h) {}

}} // namespace sys::window

#endif // RENDERER_D3D12
#endif // _WIN32