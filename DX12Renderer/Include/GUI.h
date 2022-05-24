#pragma once

class GUI
{
public:
	GUI();
	~GUI();

	void Initialize(HWND hWnd);
	void Update(float deltaTime);
	void BeginFrame();
	void EndFrame();
	void Finalize();

private:
	ComPtr<ID3D12DescriptorHeap> m_d3d12DescriptorHeap;

};
