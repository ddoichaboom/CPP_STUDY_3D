// float2, float3, float4 == Vector
// float1x3, float2x3, float4x4 == Matrix

// HLSL 코드 내에서 사용할 전역 변수 선언
float4x4 g_WorldMatrix, g_ViewMatrix, g_ProjMatrix;

// 셰이더의 입력 구조체는 C++ 측 정점 구조체 VTXTEX와 1:1 대응 해야 함.
struct VS_IN
{
    float3 vPosition;
    float2 vTexcoord;
};

struct VS_OUT
{
    float4 vPosition;
    float2 vTexcoord;
};

// 정점 셰이더
VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;
    
    // 1단계 - 월드 변환 : 로컬 -> 월드 (오브젝트를 세계에 배치)
    float4 vPosition = mul(float4(In.vPosition, 1.f), g_WorldMatrix);
    
    // 2단계 - 뷰 변환 : 월드 -> 뷰 (카메라 기준으로 변환)
    vPosition       = mul(vPosition, g_ViewMatrix);
    
    // 3단계 - 투영 변환 : 뷰 -> 투영 (원근감 적용, 클리핑 준비
    vPosition       = mul(vPosition, g_ProjMatrix);
    
    // 클립 좌표
    Out.vPosition = vPosition;
    Out.vTexcoord = In.vTexcoord;
    
    return Out;
}

// 픽셀 셰이더
//PS_MAIN()
//{

//}