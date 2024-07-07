#include "application.h"

#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.cpp>
#include <backends/imgui_impl_dx11.cpp>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
static HINSTANCE s_hInstance;

static bool DrawVec2Control(const char* label, glm::vec2& values, float columnWidth = 150.0f)
{
    ImGuiIO& io = ImGui::GetIO();
    auto boldFont = io.Fonts->Fonts[0];

    ImGui::PushID(label);

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text(label);
    ImGui::NextColumn();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

    float lineHeight = ImGui::GetFont()->FontSize + ImGui::GetStyle().FramePadding.y * 2.0f;
    ImVec2 buttonSize = { lineHeight, lineHeight };

    float itemWidth = ImGui::GetColumnWidth() * 0.4f;

    bool edited = false;

    // "X" reset button
    ImGui::PushItemWidth(itemWidth);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });

    ImGui::PushFont(boldFont);
    if (ImGui::Button("X", buttonSize))
    {
        values.x = 0.0f;
        edited = true;
    }
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    edited |= ImGui::DragFloat("##X", &values.x, 0.01f, -1.0f, 1.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // "Y" reset button
    ImGui::PushItemWidth(itemWidth);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Y", buttonSize))
    {
        values.y = 0.0f;
        edited = true;
    }
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    edited |= ImGui::DragFloat("##Y", &values.y, 0.01f, -1.0f, 1.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PopStyleVar();

    ImGui::Columns(1);

    ImGui::PopID();

    return edited;
}

static bool DrawColorEdit(const char* label, glm::vec3& values, float columnWidth = 150.0f)
{
    ImGui::PushID(label);

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text(label);
    ImGui::NextColumn();
    
    bool edited = ImGui::ColorEdit3("##Color", glm::value_ptr(values));

    ImGui::Columns(1);

    ImGui::PopID();

    return edited;
}

Application::Application(uint32_t windowWidth, uint32_t windowHeight)
{
    m_GfxContext.WindowWidth = windowWidth;
    m_GfxContext.WindowHeight = windowHeight;

    InitializeGraphicsContext();
    InitializeBezierCurves();
    InitializeImGui();
}

Application::~Application()
{
    ShutdownImGui();
}

void Application::Run()
{
    m_Running = true;
    while (m_Running)
    {
        OnEvent();
        OnUpdate();
        OnRender();
    }
}

void Application::InitializeGraphicsContext()
{
    s_hInstance = (HINSTANCE)&__ImageBase;

    // Initialize window
    WNDCLASSEXA wndClass = {};
    wndClass.cbSize = sizeof(wndClass);
    wndClass.style = CS_OWNDC;
    wndClass.lpfnWndProc = WindowProcSetup;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = s_hInstance;
    wndClass.hCursor = nullptr;
    wndClass.hbrBackground = nullptr;
    wndClass.lpszMenuName = nullptr;
    wndClass.lpszClassName = "Bezier Curve Editor Window";
    RegisterClassExA(&wndClass);

    RECT wndRect = {};
    wndRect.left = 100;
    wndRect.right = m_GfxContext.WindowWidth + wndRect.left;
    wndRect.top = 100;
    wndRect.bottom = m_GfxContext.WindowHeight + wndRect.top;

    AdjustWindowRect(&wndRect, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_SIZEBOX, FALSE);

    m_GfxContext.WindowHandle = CreateWindowExA(0, wndClass.lpszClassName, "Bezier Curve Editor", WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_MAXIMIZEBOX | WS_SIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, wndRect.right - wndRect.left, wndRect.bottom - wndRect.top, nullptr, nullptr, s_hInstance, this);

    ShowWindow(m_GfxContext.WindowHandle, SW_NORMAL);

    // Initialize D3D11
    UINT deviceCreateFlags = 0;
#if defined(_DEBUG)
    deviceCreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = m_GfxContext.WindowWidth;
    swapChainDesc.BufferDesc.Height = m_GfxContext.WindowHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 144;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = 0;
    swapChainDesc.OutputWindow = m_GfxContext.WindowHandle;
    swapChainDesc.Windowed = true;

    DXCall(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceCreateFlags, NULL, 0, D3D11_SDK_VERSION, &swapChainDesc, &m_GfxContext.SwapChain, &m_GfxContext.Device, NULL, &m_GfxContext.DeviceContext));

    // Create render targets
    RecreateSwapChainRenderTarget();
    RecreateViewportTexture();

    // Compile shader
    std::ifstream shaderFile("shaders/beziercurve.hlsl", std::ios::in);
    if (!shaderFile.is_open())
    {
        std::cout << "Failed to open shader file" << std::endl;
        return;
    }

    std::stringstream ss;
    ss << shaderFile.rdbuf();
    std::string shaderSource = ss.str();

    ComPtr<ID3DBlob> errorBlob = nullptr;
    ComPtr<ID3DBlob> csDataBlob = nullptr;
    D3DCompile(shaderSource.c_str(), shaderSource.size(), nullptr, nullptr, nullptr, "CSMain", "cs_5_0", D3DCOMPILE_DEBUG, 0, &csDataBlob, &errorBlob);
    if (errorBlob && errorBlob->GetBufferSize())
    {
        std::cout << "Failed compiling shader: " << (char*)errorBlob->GetBufferPointer() << std::endl;
        return;
    }

    DXCall(m_GfxContext.Device->CreateComputeShader(csDataBlob->GetBufferPointer(), csDataBlob->GetBufferSize(), nullptr, &m_GfxContext.BezierCurveShader));

    // Create constant buffer
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = Align(sizeof(BezierCurveShaderConstants), 16);
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;

    DXCall(m_GfxContext.Device->CreateBuffer(&cbDesc, nullptr, &m_GfxContext.BezierCurveConstantBuffer));
}

void Application::InitializeBezierCurves()
{
    for (uint32_t i = 0; i < BezierCurveType::NumTypes; i++)
    {
        D3D11_BUFFER_DESC sbDesc = {};
        sbDesc.ByteWidth = MAX_CONTROL_POINTS * sizeof(BezierControlPoint);
        sbDesc.StructureByteStride = sizeof(BezierControlPoint);
        sbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        sbDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        sbDesc.Usage = D3D11_USAGE_DYNAMIC;
        sbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

        DXCall(m_GfxContext.Device->CreateBuffer(&sbDesc, nullptr, &m_BezierCurves[i].ControlPointsBuffer));

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.ElementOffset = 0;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.ElementWidth = sizeof(BezierControlPoint);
        srvDesc.Buffer.NumElements = MAX_CONTROL_POINTS;

        DXCall(m_GfxContext.Device->CreateShaderResourceView(m_BezierCurves[i].ControlPointsBuffer.Get(), &srvDesc, &m_BezierCurves[i].ControlPointsBufferSRV));
    }
}

void Application::RecreateSwapChainRenderTarget()
{
    // Recreate RTV
    DXCall(m_GfxContext.SwapChain->GetBuffer(0, IID_PPV_ARGS(&m_GfxContext.SwapChainBackBuffer)));
    DXCall(m_GfxContext.Device->CreateRenderTargetView(m_GfxContext.SwapChainBackBuffer.Get(), nullptr, &m_GfxContext.SwapChainBackBufferRTV));

    // Recreate viewport
    m_GfxContext.SwapChainViewport.TopLeftX = 0.0f;
    m_GfxContext.SwapChainViewport.TopLeftY = 0.0f;
    m_GfxContext.SwapChainViewport.Width = m_GfxContext.WindowWidth;
    m_GfxContext.SwapChainViewport.Height = m_GfxContext.WindowHeight;
    m_GfxContext.SwapChainViewport.MinDepth = 0.0f;
    m_GfxContext.SwapChainViewport.MaxDepth = 1.0f;

    // Recreate scissor rect
    m_GfxContext.SwapChainScissorRect = { 0, 0, (int)m_GfxContext.WindowWidth, (int)m_GfxContext.WindowHeight };
}

void Application::RecreateViewportTexture()
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = m_ViewportSize.x;
    desc.Height = m_ViewportSize.y;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    desc.CPUAccessFlags = 0;

    DXCall(m_GfxContext.Device->CreateTexture2D(&desc, nullptr, &m_GfxContext.ViewportTexture));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;

    DXCall(m_GfxContext.Device->CreateShaderResourceView(m_GfxContext.ViewportTexture.Get(), &srvDesc, &m_GfxContext.ViewportTextureSRV));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = desc.Format;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    DXCall(m_GfxContext.Device->CreateUnorderedAccessView(m_GfxContext.ViewportTexture.Get(), &uavDesc, &m_GfxContext.ViewportTextureUAV));
}

void Application::RecalculateBezierCurvePolar()
{
    const BezierCurve& originalCurve = m_BezierCurves[BezierCurveType::Original];
    BezierCurve& polarCurve = m_BezierCurves[BezierCurveType::Polar];

    polarCurve.ControlPoints.clear();
    for (int i = 0; i < originalCurve.ControlPoints.size() - 1; i++)
    {
        glm::vec2 direction = originalCurve.ControlPoints[i + 1].Position - originalCurve.ControlPoints[i].Position;

        BezierControlPoint& p = polarCurve.ControlPoints.emplace_back();
        p.Position = originalCurve.ControlPoints[i].Position + direction * m_Settings.T1;
        p.Color = { 0.1f, 0.2f, 0.8f };
    }

    polarCurve.NeedsControlPointsBufferUpdate = true;
}

void Application::InitializeImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Set dark theme
    auto& colors = ImGui::GetStyle().Colors;

    colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Buttons
    colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 0.6f };
    colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Frame BG
    colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
    colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
    colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    io.FontDefault = io.Fonts->AddFontFromFileTTF("fonts/opensans/OpenSans-Regular.ttf", 18);

    ImGui_ImplWin32_Init(m_GfxContext.WindowHandle);
    ImGui_ImplDX11_Init(m_GfxContext.Device.Get(), m_GfxContext.DeviceContext.Get());
}

void Application::ShutdownImGui()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Application::RenderImGui()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(m_GfxContext.WindowWidth, m_GfxContext.WindowHeight);

    // Dockspace
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", 0, windowFlags);
    ImGui::PopStyleVar();
    
    ImGui::PopStyleVar(2);
    
    ImGui::DockSpace(ImGui::GetID("BezierCurveEditorDockspace"), ImVec2(0.0f, 0.0f));

    ImGui::Begin("Properties");

    if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 100.0f);
        ImGui::Text("Draw Bezier");
        ImGui::NextColumn();
        m_NeedsConstantBufferUpdate |= ImGui::Checkbox("##DrawBezierCurve", &m_Settings.DrawBezierCurve);
        ImGui::Columns(1);

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 100.0f);
        ImGui::Text("Draw Polar");
        ImGui::NextColumn();
        m_NeedsConstantBufferUpdate |= ImGui::Checkbox("##DrawPolar", &m_Settings.DrawPolar);
        ImGui::Columns(1);

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 100.0f);
        ImGui::Text("Num Samples");
        ImGui::NextColumn();
        m_NeedsConstantBufferUpdate |= ImGui::DragInt("##NumSamples", &m_Settings.NumSamples, 1, 25, 100);
        ImGui::Columns(1);

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 100.0f);
        ImGui::Text("t1");
        ImGui::NextColumn();
        if (ImGui::DragFloat("##t1", &m_Settings.T1, 0.01f, 0.0f, 1.0f))
            RecalculateBezierCurvePolar();
        ImGui::Columns(1);
    }

    if (ImGui::CollapsingHeader("Bezier Curve", ImGuiTreeNodeFlags_DefaultOpen))
    {
        BezierCurve& originalCurve = m_BezierCurves[BezierCurveType::Original];

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 100.0f);
        ImGui::Text("Color");
        ImGui::NextColumn();
        m_NeedsConstantBufferUpdate |= ImGui::ColorEdit3("##ColorBezierCurve", glm::value_ptr(originalCurve.Color));
        ImGui::Columns(1);

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 100.0f);
        ImGui::Text("Thickness");
        ImGui::NextColumn();
        m_NeedsConstantBufferUpdate |= ImGui::DragFloat("##ThicknessBezierCurve", &originalCurve.Thickness, 0.1f, 1.0f, 3.0f);
        ImGui::Columns(1);
    }

    if (ImGui::CollapsingHeader("Bezier Curve Polar", ImGuiTreeNodeFlags_DefaultOpen))
    {
        BezierCurve& polarCurve = m_BezierCurves[BezierCurveType::Polar];

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 100.0f);
        ImGui::Text("Color");
        ImGui::NextColumn();
        m_NeedsConstantBufferUpdate |= ImGui::ColorEdit3("##ColorPolar", glm::value_ptr(polarCurve.Color));
        ImGui::Columns(1);

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 100.0f);
        ImGui::Text("Thickness");
        ImGui::NextColumn();
        m_NeedsConstantBufferUpdate |= ImGui::DragFloat("##ThicknessPolar", &polarCurve.Thickness, 0.1f, 1.0f, 3.0f);
        ImGui::Columns(1);
    }

    if (ImGui::CollapsingHeader("Control Points", ImGuiTreeNodeFlags_DefaultOpen))
    {
        BezierCurve& originalCurve = m_BezierCurves[BezierCurveType::Original];

        if (ImGui::Button("Add") && originalCurve.ControlPoints.size() < MAX_CONTROL_POINTS)
        {
            originalCurve.ControlPoints.emplace_back();
            originalCurve.NeedsControlPointsBufferUpdate = true;
            m_NeedsConstantBufferUpdate = true;

            RecalculateBezierCurvePolar();
        }

        std::vector<uint32_t> pointsToRemove;
        for (uint32_t i = 0; i < originalCurve.ControlPoints.size(); i++)
        {
            ImGui::Separator();
            ImGui::PushID(i);
            if (ImGui::Button("X") && originalCurve.ControlPoints.size())
            {
                pointsToRemove.push_back(i);
            }
            if (DrawVec2Control("Position", originalCurve.ControlPoints[i].Position, 100.0f))
            {
                originalCurve.NeedsControlPointsBufferUpdate = true;
                RecalculateBezierCurvePolar();
            }
            originalCurve.NeedsControlPointsBufferUpdate |= DrawColorEdit("Color", originalCurve.ControlPoints[i].Color, 100.0f);
            ImGui::PopID();
        }

        if (!pointsToRemove.empty())
        {
            for (uint32_t i : pointsToRemove)
                originalCurve.ControlPoints.erase(originalCurve.ControlPoints.begin() + i);

            m_NeedsConstantBufferUpdate = true;
            originalCurve.NeedsControlPointsBufferUpdate = true;

            if (!originalCurve.ControlPoints.empty())
                RecalculateBezierCurvePolar();
        }
    }

    ImGui::End();

    // Scene viewport
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport", 0, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
    ImVec2 panelSize = ImGui::GetContentRegionAvail();
    
    if (m_ViewportSize.x != panelSize.x || m_ViewportSize.y != panelSize.y)
    {
        m_ViewportSize = { panelSize.x, panelSize.y };
        m_NeedsResize = true;
    }
    
    ImGui::Image((ImTextureID)m_GfxContext.ViewportTextureSRV.Get(), { m_ViewportSize.x, m_ViewportSize.y });
    ImGui::End();
    ImGui::PopStyleVar();
    
    ImGui::End();

    ImGui::Render();

    m_GfxContext.DeviceContext->RSSetViewports(1, &m_GfxContext.SwapChainViewport);
    m_GfxContext.DeviceContext->RSSetScissorRects(1, &m_GfxContext.SwapChainScissorRect);
    m_GfxContext.DeviceContext->OMSetRenderTargets(1, m_GfxContext.SwapChainBackBufferRTV.GetAddressOf(), nullptr);

    float color[4] = { 1.0f, 0.0f, 1.0f, 1.0f };
    m_GfxContext.DeviceContext->ClearRenderTargetView(m_GfxContext.SwapChainBackBufferRTV.Get(), color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Application::RenderBezierCurves()
{
    const uint32_t THREAD_COUNT_X = 8;
    const uint32_t THREAD_COUNT_Y = 8;

    uint32_t threadGroupCountX = (m_ViewportSize.x + THREAD_COUNT_X - 1) / THREAD_COUNT_X;
    uint32_t threadGroupCountY = (m_ViewportSize.y + THREAD_COUNT_Y - 1) / THREAD_COUNT_Y;

    m_GfxContext.DeviceContext->CSSetShader(m_GfxContext.BezierCurveShader.Get(), nullptr, 0);
    m_GfxContext.DeviceContext->CSSetUnorderedAccessViews(0, 1, m_GfxContext.ViewportTextureUAV.GetAddressOf(), nullptr);
    m_GfxContext.DeviceContext->CSSetConstantBuffers(0, 1, m_GfxContext.BezierCurveConstantBuffer.GetAddressOf());

    ID3D11ShaderResourceView* controlPointsBuffers[] = { 
        m_BezierCurves[BezierCurveType::Original].ControlPointsBufferSRV.Get(),
        m_BezierCurves[BezierCurveType::Polar].ControlPointsBufferSRV.Get()
    };
    m_GfxContext.DeviceContext->CSSetShaderResources(0, 2, controlPointsBuffers);

    m_GfxContext.DeviceContext->Dispatch(threadGroupCountX, threadGroupCountY, 1);
    
    ID3D11UnorderedAccessView* nullUAV = { nullptr };
    m_GfxContext.DeviceContext->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
}

void Application::OnEvent()
{
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            break;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Application::OnUpdate()
{
    if (m_NeedsResize)
    {
        RecreateViewportTexture();
        m_NeedsResize = false;
    }

    if (m_NeedsConstantBufferUpdate)
    {
        BezierCurveShaderConstants constants;
        constants.BezierColor = m_BezierCurves[BezierCurveType::Original].Color;
        constants.BezierThickness = m_BezierCurves[BezierCurveType::Original].Thickness;
        constants.PolarColor = m_BezierCurves[BezierCurveType::Polar].Color;
        constants.PolarThickness = m_BezierCurves[BezierCurveType::Polar].Thickness;
        constants.NumControlPoints = m_BezierCurves[BezierCurveType::Original].ControlPoints.size();
        constants.NumSamples = m_Settings.NumSamples;
        constants.T1 = m_Settings.T1;
        constants.DrawBezierCurve = m_Settings.DrawBezierCurve;
        constants.DrawPolar = m_Settings.DrawPolar;

        D3D11_MAPPED_SUBRESOURCE msr = {};
        m_GfxContext.DeviceContext->Map(m_GfxContext.BezierCurveConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
        memcpy(msr.pData, &constants, sizeof(BezierCurveShaderConstants));
        m_GfxContext.DeviceContext->Unmap(m_GfxContext.BezierCurveConstantBuffer.Get(), 0);

        m_NeedsConstantBufferUpdate = false;
    }

    for (uint32_t i = 0; i < BezierCurveType::NumTypes; i++)
    {
        if (m_BezierCurves[i].NeedsControlPointsBufferUpdate)
        {
            D3D11_MAPPED_SUBRESOURCE msr = {};
            m_GfxContext.DeviceContext->Map(m_BezierCurves[i].ControlPointsBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
            memcpy(msr.pData, m_BezierCurves[i].ControlPoints.data(), sizeof(BezierControlPoint) * m_BezierCurves[i].ControlPoints.size());
            m_GfxContext.DeviceContext->Unmap(m_BezierCurves[i].ControlPointsBuffer.Get(), 0);

            m_BezierCurves[i].NeedsControlPointsBufferUpdate = false;
        }
    }
}

void Application::OnRender()
{
    RenderBezierCurves();
    RenderImGui();
    DXCall(m_GfxContext.SwapChain->Present(1, 0));
}

bool Application::IsKeyPressed(int key)
{
    if (GetAsyncKeyState(key) & (1 << 15))
        return true;

    return false;
}

glm::vec2 Application::GetMousePosition()
{
    POINT point;
    GetCursorPos(&point);
    ScreenToClient(m_GfxContext.WindowHandle, &point);
    return { point.x, point.y };
}

LRESULT WINAPI Application::WindowProcSetup(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg == WM_CREATE)
    {
        const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        Application* const app = static_cast<Application*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Application::WindowProc));
        return WindowProc(hWnd, Msg, wParam, lParam);
    }

    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

LRESULT WINAPI Application::WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    Application* app = reinterpret_cast<Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam))
        return true;

    switch (Msg)
    {
        case WM_CLOSE:
        {
            app->m_Running = false;
            PostQuitMessage(0);
            return 0;
        }
        case WM_SIZE:
        {
            uint32_t newWidth = LOWORD(lParam);
            uint32_t newHeight = HIWORD(lParam);

            if (app->m_GfxContext.WindowWidth != newWidth || app->m_GfxContext.WindowHeight != newHeight)
            {
                // Resize swapchain
                app->m_GfxContext.WindowWidth = newWidth;
                app->m_GfxContext.WindowHeight = newHeight;
                app->m_GfxContext.SwapChainBackBuffer = nullptr;
                app->m_GfxContext.SwapChainBackBufferRTV = nullptr;

                DXCall(app->m_GfxContext.SwapChain->ResizeBuffers(2, app->m_GfxContext.WindowWidth, app->m_GfxContext.WindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
                app->RecreateSwapChainRenderTarget();
            }
            break;
        }
    }

    return DefWindowProc(hWnd, Msg, wParam, lParam);
}