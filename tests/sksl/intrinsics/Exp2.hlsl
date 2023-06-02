cbuffer _UniformBuffer : register(b0, space0)
{
    float4 _10_inputVal : packoffset(c0);
    float4 _10_expected : packoffset(c1);
    float4 _10_colorGreen : packoffset(c2);
    float4 _10_colorRed : packoffset(c3);
};


static float4 sk_FragColor;

struct SPIRV_Cross_Output
{
    float4 sk_FragColor : SV_Target0;
};

float4 main(float2 _24)
{
    bool _51 = false;
    if (exp2(_10_inputVal.x) == _10_expected.x)
    {
        float2 _41 = exp2(_10_inputVal.xy);
        _51 = all(bool2(_41.x == _10_expected.xy.x, _41.y == _10_expected.xy.y));
    }
    else
    {
        _51 = false;
    }
    bool _65 = false;
    if (_51)
    {
        float3 _54 = exp2(_10_inputVal.xyz);
        _65 = all(bool3(_54.x == _10_expected.xyz.x, _54.y == _10_expected.xyz.y, _54.z == _10_expected.xyz.z));
    }
    else
    {
        _65 = false;
    }
    bool _76 = false;
    if (_65)
    {
        float4 _68 = exp2(_10_inputVal);
        _76 = all(bool4(_68.x == _10_expected.x, _68.y == _10_expected.y, _68.z == _10_expected.z, _68.w == _10_expected.w));
    }
    else
    {
        _76 = false;
    }
    bool _84 = false;
    if (_76)
    {
        _84 = 1.0f == _10_expected.x;
    }
    else
    {
        _84 = false;
    }
    bool _94 = false;
    if (_84)
    {
        _94 = all(bool2(float2(1.0f, 2.0f).x == _10_expected.xy.x, float2(1.0f, 2.0f).y == _10_expected.xy.y));
    }
    else
    {
        _94 = false;
    }
    bool _104 = false;
    if (_94)
    {
        _104 = all(bool3(float3(1.0f, 2.0f, 4.0f).x == _10_expected.xyz.x, float3(1.0f, 2.0f, 4.0f).y == _10_expected.xyz.y, float3(1.0f, 2.0f, 4.0f).z == _10_expected.xyz.z));
    }
    else
    {
        _104 = false;
    }
    bool _113 = false;
    if (_104)
    {
        _113 = all(bool4(float4(1.0f, 2.0f, 4.0f, 8.0f).x == _10_expected.x, float4(1.0f, 2.0f, 4.0f, 8.0f).y == _10_expected.y, float4(1.0f, 2.0f, 4.0f, 8.0f).z == _10_expected.z, float4(1.0f, 2.0f, 4.0f, 8.0f).w == _10_expected.w));
    }
    else
    {
        _113 = false;
    }
    bool4 _114 = _113.xxxx;
    return float4(_114.x ? _10_colorGreen.x : _10_colorRed.x, _114.y ? _10_colorGreen.y : _10_colorRed.y, _114.z ? _10_colorGreen.z : _10_colorRed.z, _114.w ? _10_colorGreen.w : _10_colorRed.w);
}

void frag_main()
{
    float2 _20 = 0.0f.xx;
    sk_FragColor = main(_20);
}

SPIRV_Cross_Output main()
{
    frag_main();
    SPIRV_Cross_Output stage_output;
    stage_output.sk_FragColor = sk_FragColor;
    return stage_output;
}
