#define MAX_CONTROL_POINTS 5

cbuffer BezierCurveConstants : register(b0)
{
    float3 BezierColor;
    float BezierThickness;
    float3 PolarColor;
    float PolarThickness;
    int NumControlPoints;
    int NumSamples;
    float T1;
    int DrawBezierCurve;
    int DrawPolar;
};

struct BezierControlPoint
{
    float2 Position;
    float3 Color;
};

StructuredBuffer<BezierControlPoint> ControlPointsBezier : register(t0);
StructuredBuffer<BezierControlPoint> ControlPointsPolar : register(t1);
RWTexture2D<float4> RenderTexture : register(u0);

float2 GetBezierPoint(float t, StructuredBuffer<BezierControlPoint> controlPoints, int numControlPoints)
{
    // Returns a Bezier Curve point based on t by using the de Casteljau algorithm
    float2 pointsCopy[MAX_CONTROL_POINTS];
    for (int i = 0; i < numControlPoints; i++)
    {
        pointsCopy[i] = controlPoints[i].Position;
    }
    
    for (int n = 1; n < numControlPoints; n++)
    {
        for (int i = 0; i < numControlPoints - n; i++)
        {
            pointsCopy[i] = lerp(pointsCopy[i], pointsCopy[i + 1], t);
        }
    }
    
    return pointsCopy[0];
}

float3 DrawLine(float2 pixelPos, float2 a, float2 b, float3 color, float thickness)
{
    float2 ap = pixelPos - a;
    float2 ab = b - a;
    float APDotAB = dot(ap, ab);
    
    float lengthAB = length(ab);
    if (APDotAB / lengthAB > lengthAB || APDotAB / lengthAB < 0.0)
        return float3(0.0, 0.0, 0.0);
    
    float t = saturate(APDotAB / dot(ab, ab));
    float2 c = a + ab * t;
    float d = distance(c, pixelPos);
    
    return lerp(color, float3(0.0, 0.0, 0.0), smoothstep(0.0, thickness, d));
}

float3 DrawCircle(float2 pixelPos, float2 center, float radius, float3 color)
{
    float d = distance(center, pixelPos);
    return lerp(color, float3(0.0, 0.0, 0.0), smoothstep(0.0, radius, d));
}

float3 DrawBezier(float2 pixelPos, StructuredBuffer<BezierControlPoint> controlPoints, int numSamples, int numControlPoints, float3 curveColor, float3 polygonEdgeColor, float thickness)
{
    float3 polygonColor = float3(0.0, 0.0, 0.0);
    for (int j = 0; j < numControlPoints; j++)
    {
        polygonColor += DrawCircle(pixelPos, controlPoints[j].Position, 0.05, controlPoints[j].Color);
    }
    
    for (int k = 0; k < numControlPoints - 1; k++)
    {
        polygonColor += DrawLine(pixelPos, controlPoints[k].Position, controlPoints[k + 1].Position, polygonEdgeColor, 0.005);
    }
    
    float3 bezierColor = float3(0.0, 0.0, 0.0);
    float2 currPoint;
    float2 prevPoint = controlPoints[0].Position;
    for (int i = 0; i < numSamples; i++)
    {
        float t = float(i) / float(numSamples - 1);
        currPoint = GetBezierPoint(t, controlPoints, numControlPoints);
        
        bezierColor += DrawLine(pixelPos, currPoint, prevPoint, curveColor, thickness);
        
        prevPoint = currPoint;
    }
    
    return bezierColor + polygonColor;
}

[numthreads(8, 8, 1)]
void CSMain(uint2 threadID : SV_DispatchThreadID)
{
    uint width, height;
    RenderTexture.GetDimensions(width, height);
    
    float2 pixelPos = (float2(threadID.xy) / float2(width, height)) * 2.0f - 1.0f;
    pixelPos.y = -pixelPos.y;
    
    float3 color = float3(0.0, 0.0, 0.0);
    
    if (DrawBezierCurve && NumControlPoints > 0)
    {
        color += DrawBezier(pixelPos, ControlPointsBezier, NumSamples, NumControlPoints, BezierColor, float3(0.8, 0.2, 0.1), BezierThickness * 0.005);
    }
    
    if (DrawPolar && NumControlPoints > 1)
    {
        color += DrawBezier(pixelPos, ControlPointsPolar, NumSamples, NumControlPoints - 1, PolarColor, float3(0.1, 0.2, 0.8), PolarThickness * 0.005);
    }
    
    RenderTexture[threadID] = float4(color, 1.0);
    
}