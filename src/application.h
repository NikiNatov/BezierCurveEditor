#pragma once

#include "directx11.h"

#include <glm/glm.hpp>

#define MAX_CONTROL_POINTS 5

struct GraphicsContext
{
    HWND WindowHandle;
    uint32_t WindowWidth;
    uint32_t WindowHeight;
    ComPtr<ID3D11Device> Device;
    ComPtr<ID3D11DeviceContext> DeviceContext;
    ComPtr<IDXGISwapChain> SwapChain;
    ComPtr<ID3D11Texture2D> SwapChainBackBuffer;
    ComPtr<ID3D11RenderTargetView> SwapChainBackBufferRTV;
    D3D11_VIEWPORT SwapChainViewport;
    D3D11_RECT SwapChainScissorRect;
    ComPtr<ID3D11ComputeShader> BezierCurveShader;
    ComPtr<ID3D11Buffer> BezierCurveConstantBuffer;
    ComPtr<ID3D11Texture2D> ViewportTexture;
    ComPtr<ID3D11ShaderResourceView> ViewportTextureSRV;
    ComPtr<ID3D11UnorderedAccessView> ViewportTextureUAV;
};

struct GlobalSettings
{
    bool DrawBezierCurve = true;
    bool DrawPolar = true;
    int NumSamples = 50;
    float T1 = 0.5f;
};

struct BezierCurveShaderConstants
{
    glm::vec3 BezierColor = glm::vec3(1.0f);
    float BezierThickness = 1.0f;
    glm::vec3 PolarColor = glm::vec3(1.0f);
    float PolarThickness = 1.0f;
    int NumControlPoints = 0;
    int NumSamples = 100;
    float T1 = 0.5f;
    int DrawBezierCurve = 1;
    int DrawPolar = 1;
};

struct BezierControlPoint
{
    glm::vec2 Position = glm::vec2(0.0f);
    glm::vec3 Color = glm::vec3(1.0f, 0.0, 0.0f);
};

enum BezierCurveType
{
    Original = 0,
    Polar = 1,
    NumTypes
};

struct BezierCurve
{
    glm::vec3 Color = glm::vec3(1.0f);
    float Thickness = 1.0f;
    std::vector<BezierControlPoint> ControlPoints;
    bool NeedsControlPointsBufferUpdate = false;

    ComPtr<ID3D11Buffer> ControlPointsBuffer;
    ComPtr<ID3D11ShaderResourceView> ControlPointsBufferSRV;
};

class Application
{
public:
    Application(uint32_t windowWidth, uint32_t windowHeight);
    ~Application();

    void Run();
private:
    void InitializeGraphicsContext();
    void InitializeBezierCurves();
    void RecreateSwapChainRenderTarget();
    void RecreateViewportTexture();
    void RecalculateBezierCurvePolar();

    void InitializeImGui();
    void ShutdownImGui();
    void RenderImGui();
    void RenderBezierCurves();

    void OnEvent();
    void OnUpdate();
    void OnRender();

    bool IsKeyPressed(int key);
    glm::vec2 GetMousePosition();
private:
    static LRESULT WINAPI WindowProcSetup(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static LRESULT WINAPI WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
private:
    bool m_Running = false;
    glm::vec2 m_ViewportSize = glm::vec2(1.0f);
    bool m_NeedsResize = false;
    bool m_NeedsConstantBufferUpdate = false;
    GlobalSettings m_Settings;
    BezierCurve m_BezierCurves[BezierCurveType::NumTypes];
    GraphicsContext m_GfxContext;
};