#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>
#include <xnamath.h>

//Global Declarations//
IDXGISwapChain* SwapChain;
ID3D11Device* d3d11Device;
ID3D11DeviceContext* d3d11DevCon;
ID3D11RenderTargetView* renderTargetView;
ID3D11Buffer* squareIndexBuffer;
ID3D11Buffer* squareVertBuffer;

ID3D11VertexShader* VS;
ID3D11PixelShader* PS;
ID3D10Blob* VS_Buffer;
ID3D10Blob* PS_Buffer;
ID3D11InputLayout* vertLayout;

ID3D11DepthStencilView* depthStencilView;
ID3D11Texture2D* depthStencilBuffer;

ID3D11Buffer* cbPerObjectBuffer;
XMMATRIX WVP;
XMMATRIX World;
XMMATRIX camView;
XMMATRIX camProjection;

XMVECTOR camPosition;
XMVECTOR camTarget;
XMVECTOR camUp;

XMMATRIX cube1World;
XMMATRIX cube2World;

XMMATRIX Rotation;
XMMATRIX Scale;
XMMATRIX Translation;
float rot = 0.01f;

ID3D11RasterizerState* WireFrame;

ID3D11ShaderResourceView* CubesTexture;
ID3D11SamplerState* CubesTexSamplerState;

//ID3D11BlendState* Transparency;
//ID3D11RasterizerState* CCWcullMode;
//ID3D11RasterizerState* CWcullMode;

ID3D11RasterizerState* noCull;

LPCTSTR WndClassName = L"DX11Game";			//Define our window class name
HWND hwnd = NULL;							//Sets our windows handle to NULL

const int Width = 800;
const int Height = 600;

struct Vertex    //Overloaded Vertex Structure
{
	Vertex() {}
	Vertex(float x, float y, float z, float u, float v) : pos(x, y, z), texCoord(u, v) {}
	XMFLOAT3 pos;
	XMFLOAT2 texCoord;
};

D3D11_INPUT_ELEMENT_DESC layout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
UINT numElements = ARRAYSIZE(layout);


//Functions
bool InitializeWindow(HINSTANCE hInstance, int ShowWnd, int width, int height, bool windowed);
int messageloop();

//Windows callback procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//Create effects constant buffer's structure
struct cbPerObject
{
	XMMATRIX  WVP;
};

cbPerObject cbPerObj;


//DX11 methods
bool InitializeDirect3d11App(HINSTANCE hInstance);
void ReleaseObjects();
bool InitScene();
void UpdateScene();
void DrawScene();


//Main windows function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	if (!InitializeWindow(hInstance, nShowCmd, Width, Height, true))
	{
		MessageBox(0, L"Window Initialization - Failed", L"Error", MB_OK);
		return 0;
	}

	if (!InitializeDirect3d11App(hInstance))
	{
		MessageBox(0, L"Direct3D Initialization - Failed", L"Error", MB_OK);
		return 0;
	}

	if (!InitScene())
	{
		MessageBox(0, L"Scene Initialization - Failed", L"Error", MB_OK);
		return 0;
	}

	messageloop();
	ReleaseObjects();
	return 0;
}

bool InitializeWindow(HINSTANCE hInstance, int ShowWnd, int width, int height, bool windowed)
{
	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);					//Size of our windows class
	wc.style = CS_HREDRAW | CS_VREDRAW;				//class styles
	wc.lpfnWndProc = WndProc;						//Default windows procedure function
	wc.cbClsExtra = NULL;							//Extra bytes after our wc structure
	wc.cbWndExtra = NULL;							//Extra bytes after our windows instance
	wc.hInstance = hInstance;						//Instance to current application
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);			//Title bar Icon
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);		//Default mouse Icon
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);	//Window bg color
	wc.lpszMenuName = NULL;							//Name of the menu attached to our window
	wc.lpszClassName = WndClassName;				//Name of our windows class
	wc.hIconSm = LoadIcon(NULL, IDI_WINLOGO);		//Icon in your taskbar

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Error registering class", L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	hwnd = CreateWindowEx(				//Create our Extended Window
		NULL,							//Extended style
		WndClassName,					//Name of our windows class
		L"DirectX 11 Game",				//Name in the title bar of our window
		WS_OVERLAPPEDWINDOW,			//style of our window
		CW_USEDEFAULT, CW_USEDEFAULT,	//Top left corner of window
		width,							//Width of our window
		height,							//Height of our window
		NULL,							//Handle to parent window
		NULL,							//Handle to a Menu
		hInstance,						//Specifies instance of current program
		NULL							//used for an MDI client window
	);

	if (!hwnd)
	{
		MessageBox(NULL, L"Error creating window", L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	ShowWindow(hwnd, ShowWnd);    //Shows our window
	UpdateWindow(hwnd);           //Its good to update our window

	return true;
}


bool InitializeDirect3d11App(HINSTANCE hInstance)
{
	HRESULT hr;

	//Describe our Buffer
	DXGI_MODE_DESC bufferDesc = { 0 };

	bufferDesc.Width = Width;
	bufferDesc.Height = Height;
	bufferDesc.RefreshRate.Numerator = 60;
	bufferDesc.RefreshRate.Denominator = 1;
	bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	//Describe our SwapChain
	DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };

	swapChainDesc.BufferDesc = bufferDesc;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Set the feature level to DirectX 11. Set NULL to use the highest features available
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};

	//Gets the max feature level supported
	D3D_FEATURE_LEVEL maxFeatureLevel; //same as: d3d11Device->GetFeatureLevel();

	//Create our SwapChain and Device
	hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, featureLevels, 6, D3D11_SDK_VERSION, &swapChainDesc, &SwapChain, &d3d11Device, &maxFeatureLevel, &d3d11DevCon);

	//Create our BackBuffer
	ID3D11Texture2D* BackBuffer;
	hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);

	//Create our Render Target
	hr = d3d11Device->CreateRenderTargetView(BackBuffer, NULL, &renderTargetView);
	BackBuffer->Release();


	//Describe our Depth/Stencil Buffer
	D3D11_TEXTURE2D_DESC depthStencilDesc;

	depthStencilDesc.Width = Width;
	depthStencilDesc.Height = Height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	//Create the Depth/Stencil View
	d3d11Device->CreateTexture2D(&depthStencilDesc, NULL, &depthStencilBuffer);
	d3d11Device->CreateDepthStencilView(depthStencilBuffer, NULL, &depthStencilView);

	//Set our Render Target
	d3d11DevCon->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
	return true;
}

void ReleaseObjects()
{
	//Release the COM Objects we created
	SwapChain->Release();
	d3d11Device->Release();
	d3d11DevCon->Release();

	VS->Release();
	PS->Release();
	VS_Buffer->Release();
	PS_Buffer->Release();
	vertLayout->Release();
	squareVertBuffer->Release();
	squareIndexBuffer->Release();

	depthStencilView->Release();
	depthStencilBuffer->Release();

	cbPerObjectBuffer->Release();
	WireFrame->Release();

	//Transparency->Release();
	//CCWcullMode->Release();
	//CWcullMode->Release();

	noCull->Release();
}

bool InitScene()
{
	//Compile Shaders from shader file
	HRESULT hr;
	if (d3d11Device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0)
	{
		hr = D3DX11CompileFromFile(L"Shader.hlsl", 0, 0, "VS", "vs_5_0", 0, 0, 0, &VS_Buffer, 0, 0);
		hr = D3DX11CompileFromFile(L"Shader.hlsl", 0, 0, "PS", "ps_5_0", 0, 0, 0, &PS_Buffer, 0, 0);
	}
	else
	{
		hr = D3DX11CompileFromFile(L"Shader.hlsl", 0, 0, "VS", "vs_4_0", 0, 0, 0, &VS_Buffer, 0, 0);
		hr = D3DX11CompileFromFile(L"Shader.hlsl", 0, 0, "PS", "ps_4_0", 0, 0, 0, &PS_Buffer, 0, 0);
	}

	//Create the Shader Objects
	hr = d3d11Device->CreateVertexShader(VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), NULL, &VS);
	hr = d3d11Device->CreatePixelShader(PS_Buffer->GetBufferPointer(), PS_Buffer->GetBufferSize(), NULL, &PS);

	//Set Vertex and Pixel Shaders
	d3d11DevCon->VSSetShader(VS, 0, 0);
	d3d11DevCon->PSSetShader(PS, 0, 0);

	//Create the vertex buffer
	Vertex v[] =
	{
		// Front Face
		Vertex(-1.0f, -1.0f, -1.0f, 0.0f, 1.0f),
		Vertex(-1.0f,  1.0f, -1.0f, 0.0f, 0.0f),
		Vertex(1.0f,  1.0f, -1.0f, 1.0f, 0.0f),
		Vertex(1.0f, -1.0f, -1.0f, 1.0f, 1.0f),

		// Back Face
		Vertex(-1.0f, -1.0f, 1.0f, 1.0f, 1.0f),
		Vertex(1.0f, -1.0f, 1.0f, 0.0f, 1.0f),
		Vertex(1.0f,  1.0f, 1.0f, 0.0f, 0.0f),
		Vertex(-1.0f,  1.0f, 1.0f, 1.0f, 0.0f),

		// Top Face
		Vertex(-1.0f, 1.0f, -1.0f, 0.0f, 1.0f),
		Vertex(-1.0f, 1.0f,  1.0f, 0.0f, 0.0f),
		Vertex(1.0f, 1.0f,  1.0f, 1.0f, 0.0f),
		Vertex(1.0f, 1.0f, -1.0f, 1.0f, 1.0f),

		// Bottom Face
		Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f),
		Vertex(1.0f, -1.0f, -1.0f, 0.0f, 1.0f),
		Vertex(1.0f, -1.0f,  1.0f, 0.0f, 0.0f),
		Vertex(-1.0f, -1.0f,  1.0f, 1.0f, 0.0f),

		// Left Face
		Vertex(-1.0f, -1.0f,  1.0f, 0.0f, 1.0f),
		Vertex(-1.0f,  1.0f,  1.0f, 0.0f, 0.0f),
		Vertex(-1.0f,  1.0f, -1.0f, 1.0f, 0.0f),
		Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f),

		// Right Face
		Vertex(1.0f, -1.0f, -1.0f, 0.0f, 1.0f),
		Vertex(1.0f,  1.0f, -1.0f, 0.0f, 0.0f),
		Vertex(1.0f,  1.0f,  1.0f, 1.0f, 0.0f),
		Vertex(1.0f, -1.0f,  1.0f, 1.0f, 1.0f),
	};

	DWORD indices[] = {
		// Front Face
		0,  1,  2,
		0,  2,  3,

		// Back Face
		4,  5,  6,
		4,  6,  7,

		// Top Face
		8,  9, 10,
		8, 10, 11,

		// Bottom Face
		12, 13, 14,
		12, 14, 15,

		// Left Face
		16, 17, 18,
		16, 18, 19,

		// Right Face
		20, 21, 22,
		20, 22, 23
	};



	D3D11_BUFFER_DESC indexBufferDesc = { 0 };

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(DWORD) * 12 * 3;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;

	iinitData.pSysMem = indices;
	d3d11Device->CreateBuffer(&indexBufferDesc, &iinitData, &squareIndexBuffer);
	d3d11DevCon->IASetIndexBuffer(squareIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	D3D11_BUFFER_DESC vertexBufferDesc = { 0 };

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * 24;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
	vertexBufferData.pSysMem = v;
	hr = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &squareVertBuffer);

	//Set the vertex buffer
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	d3d11DevCon->IASetVertexBuffers(0, 1, &squareVertBuffer, &stride, &offset);

	//Create the Input Layout
	hr = d3d11Device->CreateInputLayout(layout, numElements, VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), &vertLayout);

	//Set the Input Layout
	d3d11DevCon->IASetInputLayout(vertLayout);

	//Set Primitive Topology
	d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//Create the Viewport
	D3D11_VIEWPORT viewport = { 0 };
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Width;
	viewport.Height = Height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	//Set the Viewport
	d3d11DevCon->RSSetViewports(1, &viewport);


	//Create the buffer to send to the cbuffer in effect file
	D3D11_BUFFER_DESC cbbd = { 0 };

	cbbd.Usage = D3D11_USAGE_DEFAULT;
	cbbd.ByteWidth = sizeof(cbPerObject);
	cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbbd.CPUAccessFlags = 0;
	cbbd.MiscFlags = 0;

	hr = d3d11Device->CreateBuffer(&cbbd, NULL, &cbPerObjectBuffer);

	//Camera information
	camPosition = XMVectorSet(0.0f, 3.0f, -8.0f, 0.0f);
	camTarget = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	camUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	//Set the View matrix
	camView = XMMatrixLookAtLH(camPosition, camTarget, camUp);

	//Set the Projection matrix
	camProjection = XMMatrixPerspectiveFovLH(0.4f*3.14f, (float)Width / Height, 1.0f, 1000.0f);


	D3D11_RASTERIZER_DESC wfdesc;
	ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
	wfdesc.FillMode = D3D11_FILL_WIREFRAME;
	wfdesc.CullMode = D3D11_CULL_NONE;
	hr = d3d11Device->CreateRasterizerState(&wfdesc, &WireFrame);

	//load texture
	hr = D3DX11CreateShaderResourceViewFromFile(d3d11Device, L"cage.png", NULL, NULL, &CubesTexture, NULL);
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	hr = d3d11Device->CreateSamplerState(&sampDesc, &CubesTexSamplerState);

	//12 Blending
	//D3D11_RENDER_TARGET_BLEND_DESC rtbd = { 0 };

	//rtbd.BlendEnable = true;
	//rtbd.SrcBlend = D3D11_BLEND_SRC_COLOR;
	//rtbd.DestBlend = D3D11_BLEND_BLEND_FACTOR;
	//rtbd.BlendOp = D3D11_BLEND_OP_ADD;
	//rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
	//rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;
	//rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	//rtbd.RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;

	//D3D11_BLEND_DESC blendDesc = { 0 };
	//blendDesc.AlphaToCoverageEnable = false;
	//blendDesc.RenderTarget[0] = rtbd;

	//d3d11Device->CreateBlendState(&blendDesc, &Transparency);

	//D3D11_RASTERIZER_DESC cmdesc;
	//ZeroMemory(&cmdesc, sizeof(D3D11_RASTERIZER_DESC));

	//cmdesc.FillMode = D3D11_FILL_SOLID;
	//cmdesc.CullMode = D3D11_CULL_BACK;

	//cmdesc.FrontCounterClockwise = true;
	//hr = d3d11Device->CreateRasterizerState(&cmdesc, &CCWcullMode);

	//cmdesc.FrontCounterClockwise = false;
	//hr = d3d11Device->CreateRasterizerState(&cmdesc, &CWcullMode);

	//#13 Cliping
	D3D11_RASTERIZER_DESC rastDesc;
	ZeroMemory(&rastDesc, sizeof(D3D11_RASTERIZER_DESC));
	rastDesc.FillMode = D3D11_FILL_SOLID;
	rastDesc.CullMode = D3D11_CULL_NONE;
	d3d11Device->CreateRasterizerState(&rastDesc, &noCull);

	return true;
}

void UpdateScene()
{
	//Keep the cubes rotating
	rot += .0005f;
	if (rot > 6.28f)
		rot = 0.0f;

	//Reset cube1World
	cube1World = XMMatrixIdentity();

	//Define cube1's world space matrix
	XMVECTOR rotaxis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	Rotation = XMMatrixRotationAxis(rotaxis, rot);
	Translation = XMMatrixTranslation(0.0f, 0.0f, 4.0f);

	//Set cube1's world space using the transformations
	cube1World = Translation * Rotation;

	//Reset cube2World
	cube2World = XMMatrixIdentity();

	//Define cube2's world space matrix
	Rotation = XMMatrixRotationAxis(rotaxis, -rot);
	Scale = XMMatrixScaling(1.3f, 1.3f, 1.3f);

	//Set cube2's world space matrix
	cube2World = Rotation * Scale;
}

void DrawScene()
{
	float bgColor[4] = { (0.0f, 0.0f, 0.0f, 0.0f) };
	d3d11DevCon->ClearRenderTargetView(renderTargetView, bgColor);

	//Refresh the Depth/Stencil view
	d3d11DevCon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//Enable the Default Rasterizer State
	d3d11DevCon->RSSetState(NULL);
	//Draw objects that will use backface culling

	//Turn off backface culling
	d3d11DevCon->RSSetState(noCull);

	////"fine-tune" the blending equation
	//float blendFactor[] = { 0.75f, 0.75f, 0.75f, 1.0f };

	////Set the default blend state (no blending) for opaque objects
	//d3d11DevCon->OMSetBlendState(0, 0, 0xffffffff);

	//Render opaque objects//

	//Set the blend state for transparent objects
	//d3d11DevCon->OMSetBlendState(Transparency, blendFactor, 0xffffffff);

	//*****Transparency Depth Ordering*****//
	//Find which transparent object is further from the camera
	//So we can render the objects in depth order to the render target

	////Find distance from first cube to camera
	//XMVECTOR cubePos = XMVectorZero();

	//cubePos = XMVector3TransformCoord(cubePos, cube1World);

	//float distX = XMVectorGetX(cubePos) - XMVectorGetX(camPosition);
	//float distY = XMVectorGetY(cubePos) - XMVectorGetY(camPosition);
	//float distZ = XMVectorGetZ(cubePos) - XMVectorGetZ(camPosition);

	//float cube1Dist = distX*distX + distY*distY + distZ*distZ;

	////Find distance from second cube to camera
	//cubePos = XMVectorZero();

	//cubePos = XMVector3TransformCoord(cubePos, cube2World);

	//distX = XMVectorGetX(cubePos) - XMVectorGetX(camPosition);
	//distY = XMVectorGetY(cubePos) - XMVectorGetY(camPosition);
	//distZ = XMVectorGetZ(cubePos) - XMVectorGetZ(camPosition);

	//float cube2Dist = distX*distX + distY*distY + distZ*distZ;

	////If the first cubes distance is less than the second cubes
	//if (cube1Dist < cube2Dist)
	//{
	//	//Switch the order in which the cubes are drawn
	//	XMMATRIX tempMatrix = cube1World;
	//	cube1World = cube2World;
	//	cube2World = tempMatrix;
	//}

	//Set the WVP matrix and send it to the constant buffer in effect file
	WVP = cube1World * camView * camProjection;
	cbPerObj.WVP = XMMatrixTranspose(WVP);
	d3d11DevCon->UpdateSubresource(cbPerObjectBuffer, 0, NULL, &cbPerObj, 0, 0);
	d3d11DevCon->VSSetConstantBuffers(0, 1, &cbPerObjectBuffer);
	//d3d11DevCon->RSSetState(WireFrame);
	d3d11DevCon->PSSetShaderResources(0, 1, &CubesTexture);
	d3d11DevCon->PSSetSamplers(0, 1, &CubesTexSamplerState);

	//Counter clockwise culling first because we need the back side of
	//the cube to be rendered first, so the front side can blend with it
	//d3d11DevCon->RSSetState(CCWcullMode);
	
	//Draw the first cube
	d3d11DevCon->DrawIndexed(36, 0, 0);
	//draw again
	//d3d11DevCon->RSSetState(CWcullMode);
	//d3d11DevCon->DrawIndexed(36, 0, 0);

	WVP = cube2World * camView * camProjection;
	cbPerObj.WVP = XMMatrixTranspose(WVP);
	d3d11DevCon->UpdateSubresource(cbPerObjectBuffer, 0, NULL, &cbPerObj, 0, 0);
	d3d11DevCon->VSSetConstantBuffers(0, 1, &cbPerObjectBuffer);
	//d3d11DevCon->RSSetState(NULL);
	d3d11DevCon->PSSetShaderResources(0, 1, &CubesTexture);
	d3d11DevCon->PSSetSamplers(0, 1, &CubesTexSamplerState);

	//d3d11DevCon->RSSetState(CCWcullMode);
	//Draw the second cube
	d3d11DevCon->DrawIndexed(36, 0, 0);
	////draw again
	//d3d11DevCon->RSSetState(CWcullMode);
	//d3d11DevCon->DrawIndexed(36, 0, 0);

	SwapChain->Present(0, 0);
}


int messageloop()
{
	MSG msg = { 0 };

	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);

			//Send the message to default windows procedure
			DispatchMessage(&msg);
		}
		else
		{
			//run game code
			UpdateScene();
			DrawScene();
		}
	}

	return (int)msg.wParam;
}



LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			if (MessageBox(0, L"Are you sure you want to exit?", L"Really?", MB_YESNO | MB_ICONQUESTION) == IDYES)
				DestroyWindow(hwnd);
		}
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}