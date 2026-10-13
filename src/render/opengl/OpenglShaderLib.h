#ifndef __SHADERLIB_H__
#define __SHADERLIB_H__

#include "../../core/core.h"
#include "OpenglRHIResources.h"

namespace ShaderLib
{

static std::unordered_map<std::string, Ref<OpenglShader>>& GetShaders()
{
    static std::unordered_map<std::string, Ref<OpenglShader>> gShaders;
    return gShaders;
}

static Ref<OpenglShader> GetShader(const std::string& name)
{
    std::unordered_map<std::string, Ref<OpenglShader>>& gShaders = GetShaders();
    auto it = gShaders.find(name);
    if (it != gShaders.end())
        return it->second;
    auto nShader = MakeRef<OpenglShader>();
    gShaders[name] = nShader;
    return nShader;
}

static Ref<OpenglShader> RegShader(const std::string& name, const char* vs, const char* fs)
{
    auto shader = GetShader(name);
    shader->SetSrc(vs, fs);
    return shader;
}

static Ref<OpenglShader> RegShaderVS(const std::string& name, const char* vs)
{
    auto shader = GetShader(name);
    shader->SetVS(vs);
    return shader;
}

static Ref<OpenglShader> RegShaderFS(const std::string& name, const char* fs)
{
    auto shader = GetShader(name);
    shader->SetFS(fs);
    return shader;
}

#define __AS_STR_LIT__HELPER__(X) #X
#define __AS_STR_LIT__(X) __AS_STR_LIT__HELPER__(X)
#define __LINE_PP__ "#line " __AS_STR_LIT__(__LINE__) "\n"
#define REG_SHADER_VS(name, Version, vs) static Ref<OpenglShader> name##_registred_vs = RegShaderVS(#name, "#version " #Version " core\n" vs)
#define REG_SHADER_FS(name, Version, fs) static Ref<OpenglShader> name##_registred_fs = RegShaderFS(#name, "#version " #Version " core\n" fs)

namespace Mesh3D
{
REG_SHADER_VS(Mesh3D, 430, R"GLSL(
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aTex;
layout (location = 2) in vec3 aNormal;

out vec3 TexCoord;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

void main() {
    vec4 totalPosition = vec4(aPos, 1.0);
    FragPos = vec3(model * totalPosition);
    TexCoord = aTex;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)GLSL");

REG_SHADER_FS(Mesh3D, 430, R"GLSL(
in vec3 TexCoord;
in vec3 FragPos;
out vec4 FragColor;
uniform sampler2D texture_diffuse;
// uniform sampler2D texture_specular;

uniform vec3 viewPos;

void main() {
    vec4 texColor = texture(texture_diffuse, TexCoord.xy);
    float d = distance(viewPos, FragPos);
    float v = 1.0 / (d * d * 0.01 + 1.0);
    FragColor = vec4(texColor.rgb * v, 1.0);
}
)GLSL");

} // namespace Mesh3D

namespace InstMesh3D
{
REG_SHADER_VS(InstMesh3D, 430, R"GLSL(
layout(std430, binding = 0) buffer InstanceData { mat4 models[]; };

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aTex;
layout(location = 2) in vec3 aNormal;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

out vec3 FragPos;
out vec3 Normal;
out vec3 TexCoord;

void main()
{
    mat4 model = transpose(models[gl_InstanceID]);
    vec4 totalPosition = vec4(aPos, 1.0);
    FragPos = vec3(model * totalPosition);
    TexCoord = aTex;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)GLSL");

REG_SHADER_FS(InstMesh3D, 430, R"GLSL(
in vec3 TexCoord;
in vec3 FragPos;
out vec4 FragColor;
uniform sampler2D texture_diffuse;
// uniform sampler2D texture_specular;

uniform vec3 viewPos;

void main() {
    vec4 texColor = texture(texture_diffuse, TexCoord.xy);
    float d = distance(viewPos, FragPos);
    float v = 1.0 / (d * d * 0.01 + 1.0);
    FragColor = vec4(texColor.rgb * v, 1.0);
}
)GLSL");
} // namespace InstMesh3D

namespace FullscreenQuad
{

REG_SHADER_VS(FullscreenQuad, 430, R"GLSL(
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTex;
out vec2 TexCoord;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTex;
}
)GLSL");
REG_SHADER_FS(FullscreenQuad, 330, R"GLSL(
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D screenTexture;
void main() {
    FragColor = texture(screenTexture, TexCoord);
}
)GLSL");

} // namespace FullscreenQuad

namespace TestShader
{

REG_SHADER_VS(TestShader, 430, R"GLSL(
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTex;
out vec2 TexCoord;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTex;
}
)GLSL");
REG_SHADER_FS(TestShader, 330, R"GLSL(
in vec2 TexCoord;
out vec4 FragColor;
void main() {
    FragColor = vec4(TexCoord, 0.0, 1.0);
}
)GLSL");

} // namespace TestShader

namespace WidgetBlit
{
REG_SHADER_VS(WidgetBlit, 430, R"GLSL(
layout(location = 0) in vec2 aPos; // unit quad в NDC [-1..1]

out vec2 TexCoord;

uniform vec2 u_targetSize; // размер цели рендера (W, H) в пикселях
uniform vec4 u_rect;       // x, y, w, h в пикселях, origin — левый верх

void main()
{
    vec2 t = vec2(aPos.x * 0.5 + 0.5,
                  0.5 - aPos.y * 0.5);

    vec2 px = u_rect.xy + t * u_rect.zw;

    vec2 ndc = vec2( px.x / u_targetSize.x * 2.0 - 1.0,
                     1.0 - px.y / u_targetSize.y * 2.0 );

    gl_Position = vec4(ndc, 0.0, 1.0);
    // gl_Position = aPos;

    TexCoord = t;
}
)GLSL");

REG_SHADER_FS(WidgetBlit, 430, R"GLSL(
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D screenTexture;
void main()
{
    FragColor = texture(screenTexture, TexCoord);
}
)GLSL");
} // namespace WidgetBlit

namespace TextInstanced
{
REG_SHADER_VS(TextInstanced, 430, R"GLSL(
layout(location = 0) in vec2 aPos;    // unit quad [0..1]
layout(location = 1) in vec2 iPos;    // instance: top-left px
layout(location = 2) in vec2 iSize;   // instance: size px
layout(location = 3) in vec4 iUV;     // instance: u0,v0,u1,v1
layout(location = 4) in vec4 iColor;  // instance: rgba

uniform vec2 u_targetSize;

out vec2 vUV;
out vec4 vColor;

void main()
{
    vec2 px  = iPos + aPos * iSize;
    vec2 ndc = vec2(px.x / u_targetSize.x * 2.0 - 1.0,
                    1.0 - px.y / u_targetSize.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
    vUV    = mix(iUV.xy, iUV.zw, aPos);
    vColor = iColor;
}
)GLSL");

REG_SHADER_FS(TextInstanced, 430, R"GLSL(
in vec2 vUV;
in vec4 vColor;
out vec4 FragColor;
uniform sampler2D u_atlas;

void main()
{
    // Временно для теста геометрии:
    // FragColor = vec4(1.0, 1.0, 1.0, 1.0);

    float a = texture(u_atlas, vUV).r;
    FragColor = vec4(vColor.rgb, vColor.a * a);
}
)GLSL");
} // namespace TextInstanced
} // namespace ShaderLib

#endif