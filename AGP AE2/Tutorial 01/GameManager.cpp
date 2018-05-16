#include "GameManager.h"



struct POS_COL_TEX_NORM_VERTEX
{
	XMFLOAT3 Pos;
	XMFLOAT4 Col;
	XMFLOAT2 Texture0;
	XMFLOAT3 Normal;
};



GameManager::GameManager()
{
	m_pPlayerInput = new Input;
}


GameManager::~GameManager()
{
}

//Cleanup function
void GameManager::ShutdownD3D()
{

	m_pRasterSolid->Release();
	m_pRasterSkyBox->Release();
    m_pDepthWriteSolid->Release();
	m_pDepthWriteSkyBox->Release();
	m_pMenu->~MenuSystem();
	if (m_pCamera) m_pCamera->~Camera();
	if (m_pZBuffer) m_pZBuffer->Release();
	if (m_pVertexBuffer) m_pVertexBuffer->Release();
	if (m_pInputLayout) m_pInputLayout->Release();
	if (m_pVertexShader) m_pVertexShader->Release();
	if (m_pPixelShader) m_pPixelShader->Release();
	if (m_pConstantBuffer0) m_pConstantBuffer0->Release();

	if (m_pPlayerInput) m_pPlayerInput->~Input();

	if (m_pBackBufferRTView) m_pBackBufferRTView->Release();
	if (m_pSwapChain) m_pSwapChain->Release();
	if (m_pImmediateContext) m_pImmediateContext->Release();
	if (m_pD3DDevice) m_pD3DDevice->Release();

}
HINSTANCE GameManager::GetHInstance()
{
	return m_hInst;
}

HWND GameManager::GetHWnd()
{
	return m_hWnd;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
	{
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_CLOSE:
	{
		PostQuitMessage(0);
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}


	return 0;
}

HRESULT GameManager::InitaliseWindow(HINSTANCE hInstance, int nCmdShow)
{
	char Name[100] = "DUUM";

	//Register class
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	//	wcex.hbrBackground = (HBRUSH) ( COLOR_WINDOW + 1);
	wcex.lpszClassName = Name;

	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	//Create window
	m_hInst = hInstance;
	RECT rc = { 0,0,1280,800 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	m_hWnd = CreateWindow(Name, Name, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left,
		rc.bottom - rc.top, NULL, NULL, hInstance, NULL);

	if (!m_hWnd)
		return E_FAIL;

	ShowWindow(m_hWnd, nCmdShow);

	return S_OK;
}



//Create D3D device and swap chain

HRESULT GameManager::InitialiseD3D()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(m_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;

#ifdef _DEBUG
	createDeviceFlags != D3D11_CREATE_DEVICE_DEBUG;
#endif
	
	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE, 
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};

	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	//creates an array of feature levels that can be used, with the first item being the
	//one attempted first
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	//The swap chain description
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = m_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = true;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		m_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(NULL, m_driverType, NULL,
			createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, &m_pSwapChain,
			&m_pD3DDevice, &m_featureLevel, &m_pImmediateContext);
		if (SUCCEEDED(hr))
			break;

	}

	if (FAILED(hr))
		return hr;

	// Get pointer to back buffer texture
	ID3D11Texture2D *pBackBufferTexture;
	hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
		(LPVOID*)&pBackBufferTexture);

	if (FAILED(hr)) return hr;

	// Use the back buffer texture pointer to create the render target view
	hr = m_pD3DDevice->CreateRenderTargetView(pBackBufferTexture, NULL,
		&m_pBackBufferRTView);

	pBackBufferTexture->Release();



	if (FAILED(hr)) return hr;

	//Creates a Z Buffer texture

	D3D11_TEXTURE2D_DESC tex2dDesc;
	ZeroMemory(&tex2dDesc, sizeof(tex2dDesc));

	tex2dDesc.Width = width;
	tex2dDesc.Height = height;
	tex2dDesc.ArraySize = 1;
	tex2dDesc.MipLevels = 1;
	tex2dDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	tex2dDesc.SampleDesc.Count = sd.SampleDesc.Count;
	tex2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	tex2dDesc.Usage = D3D11_USAGE_DEFAULT;

	ID3D11Texture2D *pZBufferTexture;
	hr = m_pD3DDevice->CreateTexture2D(&tex2dDesc, NULL, &pZBufferTexture);

	if (FAILED(hr)) return hr;

	//Creates the actual Z Buffer
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));

	dsvDesc.Format = tex2dDesc.Format;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

	hr = m_pD3DDevice->CreateDepthStencilView(pZBufferTexture, &dsvDesc, &m_pZBuffer);
	pZBufferTexture->Release();


	// Set the render target view
	m_pImmediateContext->OMSetRenderTargets(1, &m_pBackBufferRTView, m_pZBuffer);

	// Set the viewport
	D3D11_VIEWPORT viewport;

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	m_pImmediateContext->RSSetViewports(1, &viewport);

	m_p2DText = new Text2D("assets/font1.png", m_pD3DDevice, m_pImmediateContext);
	m_pUISprite = new Sprite("assets/UIpng.png", m_pD3DDevice, m_pImmediateContext);

	//Creation of the Alpha Blend description
	D3D11_BLEND_DESC b;
	ZeroMemory(&b, sizeof(b));

	b.RenderTarget[0].BlendEnable = TRUE;
	b.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	b.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	b.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	b.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	b.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	b.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	b.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	b.IndependentBlendEnable = FALSE;
	b.AlphaToCoverageEnable = FALSE;

	m_pD3DDevice->CreateBlendState(&b, &m_pBlendAlphaEnable);

	//Rasteriser for creating the skybox and normal states
	D3D11_RASTERIZER_DESC d;
	ZeroMemory(&d, sizeof(d));
	d.FillMode = D3D11_FILL_SOLID;
	d.CullMode = D3D11_CULL_NONE;
	d.DepthClipEnable = false;
	d.FrontCounterClockwise = false;
	d.MultisampleEnable = false;

	hr = m_pD3DDevice->CreateRasterizerState(&d, &m_pRasterSolid);
	d.CullMode = D3D11_CULL_FRONT;
	hr = m_pD3DDevice->CreateRasterizerState(&d, &m_pRasterSkyBox);

	//Depth Stencil to allow the skybox to not be drawn over objects
	D3D11_DEPTH_STENCIL_DESC ds;
	ZeroMemory(&ds, sizeof(ds));
	ds.StencilEnable = false;
	ds.DepthEnable = true;
	ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	ds.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	

	m_pD3DDevice->CreateDepthStencilState(&ds, &m_pDepthWriteSolid);

	ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	m_pD3DDevice->CreateDepthStencilState(&ds, &m_pDepthWriteSkyBox);

	//Audio stuff

	//ZeroMemory(&m_pAudioEngine, sizeof(m_pAudioEngine));

	//m_pAudioEngine->

	return S_OK;
}

HRESULT GameManager::InitialiseGraphics()
{
	HRESULT hr = S_OK;

	//Creates the level
	CreateLevel();


	XMFLOAT3 vertices[] =
	{
		XMFLOAT3(1.0f, -1.0f, 0.0f),
		XMFLOAT3(1.0f, 1.0f, 0.0f),
		XMFLOAT3(-1.0f, 1.0f, 0.0f),
		XMFLOAT3(-1.0f, -1.0f, 0.0f),
		XMFLOAT3(1.0f, -1.0f, 0.0f),
		XMFLOAT3(1.0f, 1.0f, 0.0f)

	};

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));

	bufferDesc.Usage = D3D11_USAGE_DYNAMIC; // GPU and CPU
	bufferDesc.ByteWidth = sizeof(vertices);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hr = m_pD3DDevice->CreateBuffer(&bufferDesc, NULL, &m_pVertexBuffer);



	if (FAILED(hr))
	{
		return hr;
	}

	//Create constant buffer

	D3D11_BUFFER_DESC constant_buffer_desc;
	ZeroMemory(&constant_buffer_desc, sizeof(constant_buffer_desc));

	constant_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	constant_buffer_desc.ByteWidth = 208;
	constant_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	hr = m_pD3DDevice->CreateBuffer(&constant_buffer_desc, NULL, &m_pConstantBuffer0);

	if (FAILED(hr))
	{
		return hr;
	}

	//Copy the certices into the buffer
	D3D11_MAPPED_SUBRESOURCE ms;

	//Lock the buffer to allow writing
	m_pImmediateContext->Map(m_pVertexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);

	//Copy the data
	memcpy(ms.pData, vertices, sizeof(vertices));

	//Unlock of Buffer
	m_pImmediateContext->Unmap(m_pVertexBuffer, NULL);

	//Load and compile pixel and vertex shaders
	ID3DBlob *VS, *PS, *error;
	hr = D3DX11CompileFromFile("model_shaders.hlsl", 0, 0, "ModelVS", "vs_4_0", 0, 0, 0, &VS, &error, 0);

	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
		{
			return hr;
		}
	}

	hr = D3DX11CompileFromFile("model_shaders.hlsl", 0, 0, "ModelPS", "ps_4_0", 0, 0, 0, &PS, &error, 0);

	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
		{
			return hr;
		}
	}

	hr = m_pD3DDevice->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &m_pVertexShader);

	if (FAILED(hr))
	{
		return 0;
	}

	hr = m_pD3DDevice->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &m_pPixelShader);

	if (FAILED(hr))
	{
		return 0;
	}

	m_pImmediateContext->VSSetShader(m_pVertexShader, 0, 0);

	m_pImmediateContext->PSSetShader(m_pPixelShader, 0, 0);


	D3D11_INPUT_ELEMENT_DESC iedesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = m_pD3DDevice->CreateInputLayout(iedesc, ARRAYSIZE(iedesc), VS->GetBufferPointer(), VS->GetBufferSize(), &m_pInputLayout);

	if (FAILED(hr))
	{
		return hr;
	}

	m_pImmediateContext->IASetInputLayout(m_pInputLayout);

	return S_OK;

}

//Creates the levels items and sets the values relevent for the,
void GameManager::CreateLevel()
{
	//This is really inefficient and can be cleaned up, needed for larger levels.

	//Initialization of the games models -
#pragma region Environment Creation



	m_pEnemy = new Model(m_pD3DDevice, m_pImmediateContext);
	m_pEnemy->LoadObjModel("assets/wall.obj");
	m_pEnemy->LoadShader("model_shaders.hlsl");
	m_pEnemy->AddTexture("assets/UIpng.png");


	m_pReflectiveCube = new Model(m_pD3DDevice, m_pImmediateContext);
	m_pReflectiveCube->LoadObjModel("assets/cube.obj");
	m_pReflectiveCube->LoadShader("reflect_shader.hlsl");
	m_pReflectiveCube->AddTexture("assets/skybox02.dds");

	m_pSkybox = new Model(m_pD3DDevice, m_pImmediateContext);
	m_pSkybox->LoadObjModel("assets/cube.obj");
	m_pSkybox->LoadShader("SkyBox_shader.hlsl");
	m_pSkybox->AddTexture("assets/skybox02.dds");

	m_pPresent = new Model(m_pD3DDevice, m_pImmediateContext);
	m_pPresent->LoadObjModel("assets/cube.obj");
	m_pPresent->LoadShader("model_shaders.hlsl");
	m_pPresent->AddTexture("assets/present.bmp");

	m_pFloor = new Model(m_pD3DDevice, m_pImmediateContext);
	m_pFloor->LoadObjModel("assets/wall.obj");
	m_pFloor->LoadShader("model_shaders.hlsl");
	m_pFloor->AddTexture("assets/texture.bmp");

	m_pLeftWall = new Model(m_pD3DDevice, m_pImmediateContext);
	m_pLeftWall->LoadObjModel("assets/wall.obj");
	m_pLeftWall->LoadShader("model_shaders.hlsl");
	m_pLeftWall->AddTexture("assets/texture.bmp");

	m_pRightWall = new Model(m_pD3DDevice, m_pImmediateContext);
	m_pRightWall->LoadObjModel("assets/wall.obj");
	m_pRightWall->LoadShader("model_shaders.hlsl");
	m_pRightWall->AddTexture("assets/texture.bmp");

	m_pFrontWall = new Model(m_pD3DDevice, m_pImmediateContext);
	m_pFrontWall->LoadObjModel("assets/wall.obj");
	m_pFrontWall->LoadShader("model_shaders.hlsl");
	m_pFrontWall->AddTexture("assets/texture.bmp");


	//Back wall

	m_pBackWall = new Model(m_pD3DDevice, m_pImmediateContext);
	m_pBackWall->LoadObjModel("assets/wall.obj");
	m_pBackWall->LoadShader("model_shaders.hlsl");
	m_pBackWall->AddTexture("assets/texture.bmp");

	m_pBackWall2 = new Model(m_pD3DDevice, m_pImmediateContext);
	m_pBackWall2->LoadObjModel("assets/wall.obj");
	m_pBackWall2->LoadShader("model_shaders.hlsl");
	m_pBackWall2->AddTexture("assets/texture.bmp");

	m_pObstacle1 = new Model(m_pD3DDevice, m_pImmediateContext);
	m_pObstacle1->LoadObjModel("assets/cube.obj");
	m_pObstacle1->LoadShader("model_shaders.hlsl");
	m_pObstacle1->AddTexture("assets/texture.bmp");

	m_pObstacle2 = new Model(m_pD3DDevice, m_pImmediateContext);
	m_pObstacle2->LoadObjModel("assets/cube.obj");
	m_pObstacle2->LoadShader("model_shaders.hlsl");
	m_pObstacle2->AddTexture("assets/texture.bmp");

	m_pObstacle3 = new Model(m_pD3DDevice, m_pImmediateContext);
	m_pObstacle3->LoadObjModel("assets/cube.obj");
	m_pObstacle3->LoadShader("model_shaders.hlsl");
	m_pObstacle3->AddTexture("assets/texture.bmp");

	m_pObstacle4 = new Model(m_pD3DDevice, m_pImmediateContext);
	m_pObstacle4->LoadObjModel("assets/cube.obj");
	m_pObstacle4->LoadShader("model_shaders.hlsl");
	m_pObstacle4->AddTexture("assets/texture.bmp");



	//Initialization of the games SceneNodes
	m_pRootNode = new SceneNode();
	m_pEnemyNode = new SceneNode();
	m_pReflectiveCubeNode = new SceneNode();
	m_pSkyboxNode = new SceneNode();
	m_pCameraNode = new SceneNode();
	m_pPresentNode = new SceneNode();
	m_pFloorNode = new SceneNode(); 
	m_pLeftWallNode = new SceneNode();
	m_pRightWallNode = new SceneNode();
	m_pFrontWallNode = new SceneNode();

    m_pBackWallNode1 = new SceneNode();
	//m_pBackWallNode2 = new SceneNode();
	//m_pBackWallNode3 = new SceneNode();
	//m_pBackWallNode4 = new SceneNode();
	//m_pBackWallNode5 = new SceneNode();
	//m_pBackWallNode6 = new SceneNode();
	//m_pBackWallNode7 = new SceneNode();
	//m_pBackWallNode8 = new SceneNode();
	//m_pBackWallNode9 = new SceneNode();



	m_pObstacle1Node = new SceneNode();
	m_pObstacle2Node = new SceneNode();
	m_pObstacle3Node = new SceneNode();
	m_pObstacle4Node = new SceneNode();

	//Adding all of the relevent Nodes to the RootNode for Scene management
	m_pRootNode->AddChildNode(m_pEnemyNode);
	m_pRootNode->AddChildNode(m_pReflectiveCubeNode);
	m_pRootNode->AddChildNode(m_pPresentNode);
	m_pRootNode->AddChildNode(m_pFloorNode);
	m_pRootNode->AddChildNode(m_pRightWallNode);
	m_pRootNode->AddChildNode(m_pLeftWallNode);
	m_pRootNode->AddChildNode(m_pFrontWallNode);

	m_pRootNode->AddChildNode(m_pBackWallNode1);
	//m_pRootNode->AddChildNode(m_pBackWallNode2);
	//m_pRootNode->AddChildNode(m_pBackWallNode3);
	//m_pRootNode->AddChildNode(m_pBackWallNode4);
	//m_pRootNode->AddChildNode(m_pBackWallNode5);
	//m_pRootNode->AddChildNode(m_pBackWallNode6);
	//m_pRootNode->AddChildNode(m_pBackWallNode7);
	//m_pRootNode->AddChildNode(m_pBackWallNode8);
	//m_pRootNode->AddChildNode(m_pBackWallNode9);

	m_pRootNode->AddChildNode(m_pObstacle1Node);
	m_pRootNode->AddChildNode(m_pObstacle2Node);
	m_pRootNode->AddChildNode(m_pObstacle3Node);
	m_pRootNode->AddChildNode(m_pObstacle4Node);

	//Wall/floor code
	m_pFloorNode->AddModel(m_pFloor);
	m_pFloorNode->SetRotationX(-90, m_pRootNode);
	m_pFloorNode->SetYPos(-51, m_pRootNode);
	m_pFloorNode->SetScale(50, m_pRootNode);
	m_pFloorNode->SetCanObjectCollide(false);

	m_pFrontWallNode->AddModel(m_pFrontWall);
	m_pFrontWallNode->SetYPos(-45, m_pRootNode);
	m_pFrontWallNode->SetScale(50, m_pRootNode);
	//m_pFrontWallNode->SetCanObjectCollide(false);

	//Back wall of the arena
	m_pBackWallNode1->AddModel(m_pBackWall);
	m_pBackWallNode1->SetRotationX(-180, m_pRootNode);
	m_pBackWallNode1->SetZPos(-10.0f, m_pRootNode);
	m_pBackWallNode1->SetXPos(0.0f, m_pRootNode);
	m_pBackWallNode1->SetScale(2, m_pRootNode);

	//m_pBackWallNode2->AddModel(m_pBackWall2);
	//m_pBackWallNode2->SetRotationX(-180, m_pRootNode);
	//m_pBackWallNode2->SetZPos(-20.0f, m_pRootNode);
	//m_pBackWallNode1->SetXPos(-2.0f, m_pRootNode);

	m_pRightWallNode->AddModel(m_pRightWall);
	m_pRightWallNode->SetRotationY(-90, m_pRootNode);
	m_pRightWallNode->SetYPos(-45, m_pRootNode);
	m_pRightWallNode->SetXPos(100, m_pRootNode);
	m_pRightWallNode->SetScale(50, m_pRootNode);
	m_pRightWallNode->SetCanObjectCollide(false);

	m_pLeftWallNode->AddModel(m_pLeftWall);
	m_pLeftWallNode->SetRotationY(90, m_pRootNode);
	m_pLeftWallNode->SetYPos(-45, m_pRootNode);
	m_pLeftWallNode->SetXPos(-100, m_pRootNode);
	m_pLeftWallNode->SetScale(50, m_pRootNode);
	m_pLeftWallNode->SetCanObjectCollide(false);

	//Interactables
	m_pEnemyNode->AddModel(m_pEnemy);
	m_pReflectiveCubeNode->AddModel(m_pReflectiveCube);
	m_pPresentNode->AddModel(m_pPresent);

	m_pEnemyNode->SetZPos(30.0f, m_pRootNode);
	m_pReflectiveCubeNode->SetZPos(10.0f, m_pRootNode);
	m_pReflectiveCubeNode->SetXPos(0.0f, m_pRootNode);
	m_pReflectiveCubeNode->SetYPos(3.0f, m_pRootNode);


	m_pPresentNode->SetZPos(5.0f, m_pRootNode);
	m_pPresentNode->SetXPos(5.0f, m_pRootNode);
	m_pPresentNode->SetScale(0.6f, m_pRootNode);
	

	m_pSkyboxNode->AddModel(m_pSkybox);
	m_pSkyboxNode->SetCanObjectCollide(false);
	m_pSkyboxNode->SetScale(4.0f, m_pRootNode);

	//Static Objects
	m_pObstacle1Node->AddModel(m_pObstacle1);
	m_pObstacle1Node->SetXPos(-10, m_pRootNode);
	m_pObstacle1Node->SetZPos(10, m_pRootNode);
	m_pObstacle1Node->SetRotationY(45, m_pRootNode);

	m_pObstacle2Node->AddModel(m_pObstacle1);
	m_pObstacle2Node->SetXPos(-10, m_pRootNode);
	m_pObstacle2Node->SetZPos(-10, m_pRootNode);
	m_pObstacle2Node->SetRotationY(45, m_pRootNode);

	m_pObstacle3Node->AddModel(m_pObstacle1);
	m_pObstacle3Node->SetXPos(10, m_pRootNode);
	m_pObstacle3Node->SetZPos(10, m_pRootNode);
	m_pObstacle3Node->SetRotationY(45, m_pRootNode);

	m_pObstacle4Node->AddModel(m_pObstacle1);
	m_pObstacle4Node->SetXPos(10, m_pRootNode);
	m_pObstacle4Node->SetZPos(-10, m_pRootNode);
	m_pObstacle4Node->SetRotationY(45, m_pRootNode);

#pragma endregion

	m_pCamera = new Camera(0.0f, 0.0f, 0.0, 0.0f);
	m_pThirdPerson = new Camera(-3.0f, 1.0f, 0.0f, 0.0f);
	
	//Creates a menu instance and sets the main menu up
	m_pMenu = new MenuSystem(m_pD3DDevice, m_pImmediateContext);
	m_pMenu->SetupMainMenu();

	
}
void GameManager::RenderFrame(void)
{

	float rgba_clear_colour[4] = { 0.1f, 0.2f,0.6f, 1.0f };
	//Clear the back buffer
	m_pImmediateContext->ClearRenderTargetView(m_pBackBufferRTView, rgba_clear_colour);

	//clear the Z Buffer
	m_pImmediateContext->ClearDepthStencilView(m_pZBuffer, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//Render here
	m_p2DText->AddText("Score" + std::to_string(m_Score), -1.0f, +1.0f, 0.1f);



	//Select primitive type
	m_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	XMMATRIX world, projection, view;

	world = XMMatrixIdentity();

	//Matrix that represents the field of view
	projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(60.0f), 640.0f / 480.0f, 0.01f, 100.0f);

	//Camera view point
	view = m_pCamera->GetViewMatrix();

	//Lighting for the world and objects
	m_pEnemy->SetDirectionalLight(0.0f, 0.0f, -1.0f, 0.0f);
	m_pReflectiveCube->SetDirectionalLight(0.0f, 0.0f, -1.0f, 0.0f);
	m_pFrontWall->SetDirectionalLight(0.0f, 0.0f, 1.0f, 0.0f);

	m_pObstacle1->SetDirectionalLight(0.0f, 0.0f, -1.0f, 0.0f);
	m_pObstacle2->SetDirectionalLight(0.0f, 0.0f, -1.0f, 0.0f);
	m_pObstacle3->SetDirectionalLight(0.0f, 0.0f, -1.0f, 0.0f);
	m_pObstacle4->SetDirectionalLight(0.0f, 0.0f, -1.0f, 0.0f);

	m_pPresent->SetDirectionalLight(0.0f, 0.0f, -1.0f, 0.0f);
	m_pPresent->SetPointLight(0.0f, 10.0f, 0.0f, 0.0f);
	m_pPresent->SetPointLightColour(0.0f, 1.0f, 0.0f, 0.0f);

	m_pFloor->SetDirectionalLight(0.0f, 0.6f, -1.0f, 0.0f);

	//Sets the raster state and depth stencil for the skybox before setting the default states
	m_pImmediateContext->RSSetState(m_pRasterSkyBox);	
	m_pImmediateContext->OMSetDepthStencilState(m_pDepthWriteSkyBox, 0);

	m_pSkyboxNode->Execute(&world, &view, &projection);

	m_pImmediateContext->OMSetDepthStencilState(m_pDepthWriteSolid, 0);
	m_pImmediateContext->RSSetState(m_pRasterSolid);

	//Draw all of the nodes models
	m_pRootNode->Execute(&world, &view, &projection);

	//Renders text after enabling the alpha channel
	m_pImmediateContext->OMSetBlendState(m_pBlendAlphaEnable, 0, 0xffffffff);
	m_p2DText->RenderText();
	//m_pUISprite->Draw();
	m_pImmediateContext->OMSetBlendState(m_pBlendAlphaDisable, 0, 0xffffffff);



	m_pSwapChain->Present(0, 0);
}

void GameManager::GameLogic()
{
	//Missing XAudio2_8.dll
	//m_pAudioEngine->Update();

	//Reads players input
	m_pPlayerInput->ReadInputStates();

	XMMATRIX identity = XMMatrixIdentity();
	m_pRootNode->UpdateCollisionTree(&identity, 1.0f);

	if (m_pPlayerInput->IsKeyPressed(DIK_A))
	{
		m_pCamera->Strafe(0.002f);

		xyz Lookat = m_pCamera->GetCameraRight();

		Lookat.x *= -0.008f;
		Lookat.y *= -0.008f;
		Lookat.z *= -0.008f;


		if (m_pPresentNode->CheckRaycastCollision(m_pCamera->GetCameraPos(), Lookat, false) == true)
		{
			m_Score += 100;
			m_pPresentNode->SetXPos(Math::GetRandomNumber(10, -10), m_pRootNode);
			m_pPresentNode->SetZPos(Math::GetRandomNumber(10, -10), m_pRootNode);
		}

		if (m_pRootNode->CheckRaycastCollision(m_pCamera->GetCameraPos(), Lookat, true) == true)
		{
			m_pCamera->Strafe(-0.002f);

		}
	
		
	}
	else if (m_pPlayerInput->IsKeyPressed(DIK_D))
	{
		m_pCamera->Strafe(-0.002f);

		xyz Lookat = m_pCamera->GetCameraRight();

		Lookat.x *= 0.008f;
		Lookat.y *= 0.008f;
		Lookat.z *= 0.008f;

		if (m_pPresentNode->CheckRaycastCollision(m_pCamera->GetCameraPos(), Lookat, false) == true)
		{
			m_Score += 100;
			m_pPresentNode->SetXPos(Math::GetRandomNumber(10, -10), m_pRootNode);
			m_pPresentNode->SetZPos(Math::GetRandomNumber(10, -10), m_pRootNode);
		}

		if (m_pRootNode->CheckRaycastCollision(m_pCamera->GetCameraPos(), Lookat, true) == true)
		{
			m_pCamera->Strafe(0.002f);
			
		}

		
	}


	//Player input code
	//If the player presses W, check collisions and interactions moving forward
	if (m_pPlayerInput->IsKeyPressed(DIK_W))
	{
		m_pCamera->Forward(0.002f);
		m_pThirdPerson->Forward(0.002f);

		xyz Lookat = m_pCamera->GetLookAt();

		Lookat.x *= 0.008f;
		Lookat.y *= 0.008f;
		Lookat.z *= 0.008f;

		//Checks for a collision with an object and reverses the movement if so
		if (m_pPresentNode->CheckRaycastCollision(m_pCamera->GetCameraPos(), Lookat, false) == true)
		{
			m_Score += 100;
			m_pPresentNode->SetXPos(Math::GetRandomNumber(10, -10), m_pRootNode);
			m_pPresentNode->SetZPos(Math::GetRandomNumber(10, -10), m_pRootNode);
		}
		//Checks for collision with the present and if so moves the present to a new position
		if (m_pRootNode->CheckRaycastCollision(m_pCamera->GetCameraPos(), Lookat, true) == true)
		{
			m_pThirdPerson->Forward(-0.002f);
			m_pCamera->Forward(-0.002f);
		}

		


	}	
	else if (m_pPlayerInput->IsKeyPressed(DIK_S))
	{
		m_pCamera->Forward(-0.002f);
		m_pThirdPerson->Forward(-0.002f);

		xyz Lookat = m_pCamera->GetLookAt();

		Lookat.x *= -0.002f;
		Lookat.y *= -0.002f;
		Lookat.z *= -0.002f;

		if (m_pPresentNode->CheckRaycastCollision(m_pCamera->GetCameraPos(), Lookat, false) == true)
		{
			m_Score += 100;
			m_pPresentNode->SetXPos(Math::GetRandomNumber(10, -10), m_pRootNode);
			m_pPresentNode->SetZPos(Math::GetRandomNumber(10, -10), m_pRootNode);
		}
		
		if (m_pRootNode->CheckRaycastCollision(m_pCamera->GetCameraPos(), Lookat, true) == true)
		{
			m_pCamera->Forward(0.002f);
			m_pThirdPerson->Forward(0.002f);
		}		 	

	}

	if (m_pPlayerInput->IsKeyPressed(DIK_ESCAPE))
		m_eGameState = ePauseMenu;

	//Mouse input code

	if (m_pPlayerInput->HasMouseMoved())
	{
		m_pCamera->Rotate(m_pPlayerInput->GetMouseX() / 10);
		//Rotate Y as well?
	}

	//TODO - Add proper deadzones for the controller analogues
#pragma region ControllerInput

	


	if (m_pPlayerInput->IsButtonPressed(XINPUT_GAMEPAD_START))
		m_eGameState = ePauseMenu;

	if (m_pPlayerInput->GetControllerLeftAnalogueY() >= 10000.0f
		|| m_pPlayerInput->IsButtonPressed(XINPUT_GAMEPAD_DPAD_UP))
	{
		m_pCamera->Forward(0.002f);
		m_pThirdPerson->Forward(0.002f);

		xyz Lookat = m_pCamera->GetLookAt();

		Lookat.x *= 0.008f;
		Lookat.y *= 0.008f;
		Lookat.z *= 0.008f;

		//Checks for a collision with an object and reverses the movement if so
		if (m_pPresentNode->CheckRaycastCollision(m_pCamera->GetCameraPos(), Lookat, false) == true)
		{
			m_Score += 100;
			m_pPresentNode->SetXPos(Math::GetRandomNumber(10, -10), m_pRootNode);
			m_pPresentNode->SetZPos(Math::GetRandomNumber(10, -10), m_pRootNode);
			
			Thread1 = thread([=] { m_pPlayerInput->SetControllerVibration(20000.0f, 20000.0f, 1.0f); });
			Thread1.detach();

		}
		//Checks for collision with the present and if so moves the present to a new position
		if (m_pRootNode->CheckRaycastCollision(m_pCamera->GetCameraPos(), Lookat, true) == true)
		{
			m_pThirdPerson->Forward(-0.002f);
			m_pCamera->Forward(-0.002f);
		}

	
		
	}
	else if (m_pPlayerInput->GetControllerLeftAnalogueY() <= -10000.0f
		|| m_pPlayerInput->IsButtonPressed(XINPUT_GAMEPAD_DPAD_DOWN))
	{
		m_pCamera->Forward(-0.002f);
		m_pThirdPerson->Forward(-0.002f);

		xyz Lookat = m_pCamera->GetLookAt();

		Lookat.x *= -0.008f;
		Lookat.y *= -0.008f;
		Lookat.z *= -0.008f;

		if (m_pPresentNode->CheckRaycastCollision(m_pCamera->GetCameraPos(), Lookat, false) == true)
		{
			m_Score += 100;
			m_pPresentNode->SetXPos(Math::GetRandomNumber(10, -10), m_pRootNode);
			m_pPresentNode->SetZPos(Math::GetRandomNumber(10, -10), m_pRootNode);

			Thread1 = thread([=] { m_pPlayerInput->SetControllerVibration(20000.0f, 20000.0f, 1.0f); });
			Thread1.detach();
		}

		if (m_pRootNode->CheckRaycastCollision(m_pCamera->GetCameraPos(), Lookat, true) == true)
		{
			m_pCamera->Forward(0.002f);
			m_pThirdPerson->Forward(0.002f);
		}

	}

	if (m_pPlayerInput->GetControllerLeftAnalogueX() >= 10000.0f
		|| m_pPlayerInput->IsButtonPressed(XINPUT_GAMEPAD_DPAD_RIGHT))
	{
		m_pCamera->Strafe(-0.002f);

		xyz Lookat = m_pCamera->GetCameraRight();

		Lookat.x *= 0.008f;
		Lookat.y *= 0.008f;
		Lookat.z *= 0.008f;

		if (m_pPresentNode->CheckRaycastCollision(m_pCamera->GetCameraPos(), Lookat, false) == true)
		{
			m_Score += 100;
			m_pPresentNode->SetXPos(Math::GetRandomNumber(10, -10), m_pRootNode);
			m_pPresentNode->SetZPos(Math::GetRandomNumber(10, -10), m_pRootNode);

			Thread1 = thread([=] { m_pPlayerInput->SetControllerVibration(20000.0f, 20000.0f, 1.0f); });
			Thread1.detach();
		}

		if (m_pRootNode->CheckRaycastCollision(m_pCamera->GetCameraPos(), Lookat, true) == true)
		{
			m_pCamera->Strafe(0.002f);

		}


	}	
	else if (m_pPlayerInput->GetControllerLeftAnalogueX() <= -10000.0f
		|| m_pPlayerInput->IsButtonPressed(XINPUT_GAMEPAD_DPAD_LEFT))
	{	
		m_pCamera->Strafe(0.002f);

		xyz Lookat = m_pCamera->GetCameraRight();

		Lookat.x *= -0.008f;
		Lookat.y *= -0.008f;
		Lookat.z *= -0.008f;


		if (m_pPresentNode->CheckRaycastCollision(m_pCamera->GetCameraPos(), Lookat, false) == true)
		{
			m_Score += 100;
			m_pPresentNode->SetXPos(Math::GetRandomNumber(10, -10), m_pRootNode);
			m_pPresentNode->SetZPos(Math::GetRandomNumber(10, -10), m_pRootNode);

			Thread1 = thread([=] { m_pPlayerInput->SetControllerVibration(20000.0f, 20000.0f, 1.0f); });
			Thread1.detach();
		}

		if (m_pRootNode->CheckRaycastCollision(m_pCamera->GetCameraPos(), Lookat, true) == true)
		{
			m_pCamera->Strafe(-0.002f);

		}
	}

	if (m_pPlayerInput->GetControllerRightAnalogueX() >= 20000.0f)
	{
		m_pCamera->Rotate(0.502f);
	}
	else if (m_pPlayerInput->GetControllerRightAnalogueX() <= -20000.0f)
	{
		m_pCamera->Rotate(-0.502f);
	}

#pragma endregion


	//if (m_pPlayerInput->IsKeyPressed(DIK_I))
	//	m_pReflectiveCubeNode->IncRotX(0.01f, RootNode);

	if (m_pPlayerInput->IsKeyPressed(DIK_K))
		m_pObstacle1Node->IncXPos(0.01f, m_pRootNode);

	if (m_pPlayerInput->IsKeyPressed(DIK_H))
		m_pObstacle1Node->IncXPos(-0.01f, m_pRootNode);

	if (m_pPlayerInput->IsKeyPressed(DIK_U))
		m_pObstacle1Node->IncZPos(0.01f, m_pRootNode);

	if (m_pPlayerInput->IsKeyPressed(DIK_J))
		m_pObstacle1Node->IncZPos(-0.01f, m_pRootNode);

	xyz Lookat = m_pCamera->GetLookAt();

	//Enemy "AI" that follows the player around
	//m_pEnemyNode->LookAtXYZ(m_pCamera->GetX(), m_pCamera->GetY(), m_pCamera->GetZ(), m_pRootNode);
	//m_pEnemyNode->MoveForward(0.001f, m_pRootNode);


	Lookat.x *= 0.002f;
	Lookat.y *= 0.002f;
	Lookat.z *= 0.002f;

	//ends the game if the enemy catches up with the player or the player earns 1000 points
	if (m_pEnemyNode->CheckRaycastCollision(m_pCamera->GetCameraPos(), Lookat, false) == true)
	{

		m_eGameState = eEndGame;
	}
	else if (m_Score >= 1000)
	{
		m_eGameState = eEndGame;
	}

	/*xyz PresentPos(m_pPresentNode->GetXPos(), m_pPresentNode->GetYPos(), m_pPresentNode->GetZPos());

	if (m_pEnemyNode->CheckRaycastCollision(PresentPos, Lookat, false) == true)
	{
		
	}*/

	//Skybox code
	m_pSkyboxNode->SetXPos(m_pCamera->GetX(), m_pRootNode);
	m_pSkyboxNode->SetYPos(m_pCamera->GetY(), m_pRootNode);
	m_pSkyboxNode->SetZPos(m_pCamera->GetZ(), m_pRootNode);

	
}

//Main menu loop
void GameManager::MainMenu()
{
	float rgba_clear_colour[4] = { 0.1f, 0.2f,0.6f, 1.0f };
	m_pImmediateContext->ClearRenderTargetView(m_pBackBufferRTView, rgba_clear_colour);
	m_pPlayerInput->ReadInputStates();

	m_pMenu->MainMenuLoop(m_pPlayerInput);
	m_pSwapChain->Present(0, 0);

	if (m_pMenu->m_ePlayerSelection == eQuit && m_pMenu->GetSelection() == true)
	{
		m_eGameState = eEndGame;
	}
	else if (m_pMenu->m_ePlayerSelection == eStartGame && m_pMenu->GetSelection() == true)
	{
		m_eGameState = eInGame;
	}

}
//Pause menu loop
void GameManager::PauseMenu()
{
	float rgba_clear_colour[4] = { 0.1f, 0.2f,0.6f, 1.0f };

	m_pImmediateContext->ClearRenderTargetView(m_pBackBufferRTView, rgba_clear_colour);
	m_pPlayerInput->ReadInputStates();

	m_pMenu->PauseMenu(m_pPlayerInput);
	m_pSwapChain->Present(0, 0);

	if (m_pMenu->m_ePlayerSelection == eQuit && m_pMenu->GetSelection() == true)
	{
		m_eGameState = eEndGame;
	}
	else if (m_pMenu->m_ePlayerSelection == eStartGame && m_pMenu->GetSelection() == true)
	{
		m_eGameState = eInGame;
	}

}