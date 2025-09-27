#pragma once

#include "IRHIHelper.h"
#include "../../World/WWorld.h"
#include "../../UI/UI.h"

#include <GL/glew.h>
#include <gl/GL.h>
#include <iostream>



struct VFShaderProgramm
{
    GLuint shaderProgram;
    VFShaderProgramm() { shaderProgram = 0; }
    VFShaderProgramm(const char* vertexShaderSource, const char* fragmentShaderSource)
    {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        // Проверка компиляции вершинного шейдера
        GLint  success;
        GLchar infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cerr << "Vertex shader compilation failed:\n" << infoLog << std::endl;
        }

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        // Проверка компиляции фрагментного шейдера
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cerr << "Fragment shader compilation failed:\n" << infoLog << std::endl;
        }

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        // Проверка линковки шейдерной программы
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            std::cerr << "Shader program linking failed:\n" << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void Use() { glUseProgram(shaderProgram); }

    void Unuse() { glUseProgram(0); }

    operator bool() { return shaderProgram; }
    bool operator !() { return !shaderProgram; }
    
    void GetLoc(const char* name)
    {
        return glGetUniformLocation(shaderProgram, name);
    }

    template<typename T>
    void setUniform(const char* name, const T& v)
    {
        GLint loc = glGetUniformLocation(shaderProgram, name);
        setUniform(loc, v);
    }

    void setUniform(GLint loc, const int& f)
    {
        if (loc != -1)
        {
            glUniform1i(loc, f);
        }
    }

    void setUniform(GLint loc, const float& f)
    {
        if (loc != -1)
        {
            glUniform1f(loc, f);
        }
    }

    void setUniform(GLint loc, const MVector2f& vec)
    {
        if (loc != -1)
        {
            glUniform2f(loc, vec[0], vec[1]);
        }
    }

    void setUniform(GLint loc, const MVector3f& vec)
    {
        if (loc != -1)
        {
            glUniform3f(loc, vec[0], vec[1], vec[2]);
        }
    }

    void setUniform(GLint loc, const MVector4f& vec)
    {
        if (loc != -1)
        {
            glUniform4f(loc, vec[0], vec[1], vec[2], vec[3]);
        }
    }

    void setUniform(GLint loc, const MMatrix2f& matrix)
    {
        if (loc != -1)
        {
            glUniformMatrix2fv(loc, 1, GL_TRUE, &matrix.m[0][0]);
        }
    }

    void setUniform(GLint loc, const MMatrix3f& matrix)
    {
        if (loc != -1)
        {
            glUniformMatrix3fv(loc, 1, GL_TRUE, &matrix.m[0][0]);
        }
    }

    void setUniform(GLint loc, const MMatrix4f& matrix)
    {
        if (loc != -1)
        {
            glUniformMatrix4fv(loc, 1, GL_TRUE, &matrix.m[0][0]);
        }
    }
};


namespace shaders
{

const char* Render3DVertexShaderSource = R"GLSL(#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aTex;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in ivec4 aBoneIDs2;
layout (location = 5) in vec4 aWeights;
layout (location = 6) in vec4 aWeights2;

out vec3 FragPos;
out vec3 Normal;
out vec3 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout(std140, binding = 0) readonly buffer BoneMatrices {
    mat4 bones[];
};


void main() {
vec4 totalPosition = vec4(aPos, 1.0);
    vec3 totalNormal = aNormal;
    
    int bones_length = bones.length();
    
    if (bones_length > 0 && (
        aWeights.x > 0.0 || aWeights.y > 0.0 || aWeights.z > 0.0 || aWeights.w > 0.0 ||
        aWeights2.x > 0.0 || aWeights2.y > 0.0 || aWeights2.z > 0.0 || aWeights2.w > 0.0)) {
        
        totalPosition = vec4(0.0);
        totalNormal = vec3(0.0);

        int bones_length = bones.length();
        float tot_weight = 0.0;

        for(int i = 0; i < 4; i++) {
            if(aBoneIDs[i] >= 0 && aBoneIDs[i] <= bones_length)
                tot_weight += aWeights[i];
            if(aBoneIDs2[i] >= 0 && aBoneIDs2[i] <= bones_length)
                tot_weight += aWeights2[i];
        }
        
        for(int i = 0; i < 4; i++) {
            int boneIndex = aBoneIDs[i];
            if(boneIndex >= 0 && boneIndex <= bones_length)
            {
                mat4 boneTransform = bones[boneIndex];
                float weight = aWeights[i] / tot_weight;
                
                totalPosition += boneTransform * vec4(aPos, 1.0) * weight;
                totalNormal += mat3(boneTransform) * aNormal * weight;
            }
            boneIndex = aBoneIDs2[i];
            if(boneIndex >= 0 && boneIndex <= bones_length)
            {
                mat4 boneTransform = bones[boneIndex];
                float weight = aWeights2[i] / tot_weight;
                
                totalPosition += boneTransform * vec4(aPos, 1.0) * weight;
                totalNormal += mat3(boneTransform) * aNormal * weight;
            }
        }
    }

    FragPos = vec3(model * totalPosition);
    Normal = mat3(transpose(inverse(model))) * totalNormal;
    TexCoord = aTex;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)GLSL";

const char* Render3DFragmentShaderSource = R"GLSL(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec3 TexCoord;

out vec4 FragColor;

uniform vec3 viewPos;
uniform vec3 objectColor;
uniform float shininess = 6;

uniform float specularStrength = 0.3;
uniform sampler2D texture_diffuse;
uniform sampler2D texture_specular;

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    
    float power;
};  
#define NR_POINT_LIGHTS 16  
uniform PointLight pointLights[NR_POINT_LIGHTS];

vec3 CalcPointLight(PointLight light, vec3 norm, vec3 fragPos, vec3 viewDir, float viewNorm, float mshininess, float mspec, vec3 mdiffuse, vec3 mspecular)
{
    if(light.power == 0)
        return vec3(0.0, 0.0, 0.0);

    // Ambient
    vec3 ambient = light.ambient * mdiffuse;

    // Diffuse
    vec3 lightDelta = light.position - FragPos;
    vec3 lightDir = normalize(lightDelta);
    float lightDist = length(lightDelta);

    float power = sqrt(light.power);
    float attenuation = clamp(light.power / (lightDist * lightDist), 0.0, 1.0);
    
    float diff = dot(norm, lightDir);
    if(viewNorm * diff > 0)
    {
        diff = abs(diff);
    }
    else
        return vec3(0.0, 0.0, 0.0);

    vec3 diffuse = diff * light.diffuse * mdiffuse;

    // Specular
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), mshininess);
    vec3 specular = mspec * spec * light.diffuse * mspecular;

    return (ambient + diffuse + specular) * attenuation;
}

void main() 
{
    vec4 texColor = texture(texture_diffuse, TexCoord.xy);
    vec3 materialDiffuse = objectColor;
    if(length(materialDiffuse) == 0)
        materialDiffuse = texColor.rgb;
    
    float materialSpecular = specularStrength;
    if(materialSpecular == 0)
        materialSpecular = specularStrength * texture(texture_specular, TexCoord.xy).r;

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    float viewNorm = dot(norm, viewDir);
    vec3 result = vec3(0.0, 0.0, 0.0);

    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, viewNorm, 
                                shininess, materialSpecular, materialDiffuse, vec3(1.0));

    FragColor = vec4(result, texColor.a);  // Сохраняем альфа-канал
}
)GLSL";

};


class OpenglRHIMesh : public IRHIMesh {
public:
    GLuint    VBO, VAO, EBO, SIZE;

    Ref<Mesh> mesh;
    Ref<SkeletalMesh> smesh;
    bool need_update;

    virtual void Update(Ref<Mesh> mesh)
    {
        if(this->mesh != mesh)
        {
            this->mesh = mesh;
        }
        if(!glGenVertexArrays || !GetIRHIHelper()->Inited())
        {
            need_update =true;
            return;
        }
        SIZE = mesh->f.size();
        if(!VAO)
            glGenVertexArrays(1, &VAO);
        if(!VBO)
            glGenBuffers(1, &VBO);
        if(!EBO)
            glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, mesh->v.size() * sizeof(Mesh::Vertex), mesh->v.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->f.size() * sizeof(uint32_t), mesh->f.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex, v));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex, vt));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex, vn));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
        need_update = false;
    }

    virtual void Update(Ref<SkeletalMesh> mesh)
    {
        if(this->smesh != mesh)
        {
            this->smesh = mesh;
        }
        if(!glGenVertexArrays || !GetIRHIHelper()->Inited())
        {
            need_update =true;
            return;
        }
        SIZE = mesh->f.size();
        if(!VAO)
            glGenVertexArrays(1, &VAO);
        if(!VBO)
            glGenBuffers(1, &VBO);
        if(!EBO)
            glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, mesh->v.size() * sizeof(SkeletalMesh::Vertex), mesh->v.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->f.size() * sizeof(uint32_t), mesh->f.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkeletalMesh::Vertex), (void*)offsetof(SkeletalMesh::Vertex, v));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SkeletalMesh::Vertex), (void*)offsetof(SkeletalMesh::Vertex, vt));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(SkeletalMesh::Vertex), (void*)offsetof(SkeletalMesh::Vertex, vn));
        glEnableVertexAttribArray(2);

        glVertexAttribIPointer(3, 4, GL_INT, sizeof(SkeletalMesh::Vertex), (void*)offsetof(SkeletalMesh::Vertex, bi));
        glEnableVertexAttribArray(3);

        glVertexAttribIPointer(4, 4, GL_INT, sizeof(SkeletalMesh::Vertex), (void*)(offsetof(SkeletalMesh::Vertex, bi) + sizeof(int) * 4));
        glEnableVertexAttribArray(4);

        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(SkeletalMesh::Vertex), (void*)(offsetof(SkeletalMesh::Vertex, bv)));
        glEnableVertexAttribArray(5);

        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(SkeletalMesh::Vertex), (void*)(offsetof(SkeletalMesh::Vertex, bv) + sizeof(float) * 4));
        glEnableVertexAttribArray(6);

        glBindVertexArray(0);
        need_update = false;
    }

    
};

class OpenglRHISkeleton : public IRHISkeleton {
public:
    GLuint SSBO = 0;
    Ref<Skeleton> skeleton;
    bool need_update;

    virtual void Update(Ref<Skeleton> skeleton)
    {
        if(this->skeleton != skeleton)
        {
            this->skeleton = skeleton;
        }
        if(!glGenVertexArrays || !GetIRHIHelper()->Inited())
        {
            need_update = true;
            return;
        }
        if (!SSBO)
            glGenBuffers(1, &SSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
        
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 
                     skeleton->boneMatrices.size() * sizeof(skeleton->boneMatrices[0]),
                     skeleton->boneMatrices.data(), 
                     GL_DYNAMIC_DRAW);
    }

    
};

GLenum PixelFormat2GLformat(const PixelFormat& m)
{
    switch (m)
    {
        case PixelFormat::RED_8 :
        case PixelFormat::RED_F : return GL_RED;

        case PixelFormat::RG_8 :
        case PixelFormat::RG_F : return GL_RG;

        case PixelFormat::RGB_8 : 
        case PixelFormat::RGB_F : return GL_RGB;

        case PixelFormat::RGBA_8 :
        case PixelFormat::RGBA_F : return GL_RGBA;

        case PixelFormat::RED_16 :
        case PixelFormat::RED_32 : return GL_RED_INTEGER;

        case PixelFormat::RG_16 :
        case PixelFormat::RG_32 : return GL_RG_INTEGER;

        case PixelFormat::RGB_16 :
        case PixelFormat::RGB_32 : return GL_RGB_INTEGER;

        case PixelFormat::RGBA_16 :
        case PixelFormat::RGBA_32 : return GL_RGBA_INTEGER;
    
    default:
        break;
    }
}

GLenum PixelFormat2GLiternal(const PixelFormat& m)
{
    switch (m)
    {
        case PixelFormat::RED_8 : return GL_R8;
        case PixelFormat::RED_16 : return GL_R16UI;
        case PixelFormat::RED_32 : return GL_R32UI;
        case PixelFormat::RED_F : return GL_R32F;

        case PixelFormat::RG_8 : return GL_RG8;
        case PixelFormat::RG_16 : return GL_RG16UI;
        case PixelFormat::RG_32 : return GL_RG32UI;
        case PixelFormat::RG_F : return GL_RG32F;

        case PixelFormat::RGB_8 : return GL_RGB8;
        case PixelFormat::RGB_16 : return GL_RGB16UI;
        case PixelFormat::RGB_32 : return GL_RGB32UI;
        case PixelFormat::RGB_F : return GL_RGB32F;

        case PixelFormat::RGBA_8 : return GL_RGBA8;
        case PixelFormat::RGBA_16 : return GL_RGBA16UI;
        case PixelFormat::RGBA_32 : return GL_RGBA32UI;
        case PixelFormat::RGBA_F : return GL_RGBA32F;
    
    default:
        break;
    }
}

GLenum PixelFormat2GLType(const PixelFormat& m)
{
    switch (m)
    {
        case PixelFormat::RED_8 : return GL_UNSIGNED_BYTE;
        case PixelFormat::RG_8 : return GL_UNSIGNED_BYTE;
        case PixelFormat::RGB_8 : return GL_UNSIGNED_BYTE;
        case PixelFormat::RGBA_8 : return GL_UNSIGNED_BYTE;

        case PixelFormat::RED_16 : return GL_UNSIGNED_SHORT;
        case PixelFormat::RG_16 : return GL_UNSIGNED_SHORT;
        case PixelFormat::RGB_16 : return GL_UNSIGNED_SHORT;
        case PixelFormat::RGBA_16 : return GL_UNSIGNED_SHORT;

        case PixelFormat::RED_32 : return GL_UNSIGNED_INT;
        case PixelFormat::RG_32 : return GL_UNSIGNED_INT;
        case PixelFormat::RGB_32 : return GL_UNSIGNED_INT;
        case PixelFormat::RGBA_32 : return GL_UNSIGNED_INT;
        
        case PixelFormat::RED_F : return GL_FLOAT;
        case PixelFormat::RG_F : return GL_FLOAT;
        case PixelFormat::RGB_F : return GL_FLOAT;
        case PixelFormat::RGBA_F : return GL_FLOAT;
    
    default:
        break;
    }
}


class OpenglRHITexture : public IRHITexture {
public:
    
    GLuint Tex = 0;
    PixelFormat mt;
    virtual void Update(std::string path)
    {
        throw std::runtime_error("TODO");
    }

    virtual void Update(void* data, PixelFormat type, size_t w, size_t h)
    {
        mt = type;
        if (!Tex)
            glGenTextures(1, &Tex);
        glDisable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, Tex);
        glTexImage2D(
            GL_TEXTURE_2D, 
            0, 
            PixelFormat2GLiternal(mt),
            w, 
            h, 
            0, 
            PixelFormat2GLformat(mt),
            PixelFormat2GLType(mt),
            data
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    
    virtual PixelFormat GetPixelFormat() const { return mt; };
};

class OpenglRHIHelper : public IRHIHelper {
public: 
    VFShaderProgramm Render3DShaderProgram;
    bool shader3dinitialized = false;
    bool inited = false;
    int WIDTH, HEIGHT;

    virtual bool Inited() { return inited; };

    OpenglRHIHelper()
    {
        
    }

    virtual Ref<IRHIMesh> GenMesh()
    {
        return new OpenglRHIMesh;
    }
    virtual Ref<IRHISkeleton> GenSkeleton()
    {
        return new OpenglRHISkeleton;
    }
    virtual IRHITexture* GenTexture()
    {
        return new OpenglRHITexture();
    }

    virtual int RenderWorld(WWorld* w)
    {

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (!shader3dinitialized)
        {
            Render3DShaderProgram = VFShaderProgramm(shaders::Render3DVertexShaderSource, shaders::Render3DFragmentShaderSource);

            shader3dinitialized = true;
        }

        Render3DShaderProgram.Use();


        MMatrix4f view       = lookAt(w->camera->cameraPos, w->camera->target, w->camera->up);
        MMatrix4f projection = perspective(w->camera->fov * 3.1415926535 / 180.0,
                                        (float)WIDTH / (float)HEIGHT, // соотношение сторон
                                        0.1f,
                                        100.0f // ближняя и дальняя плоскости
        );
        glEnable(GL_DEPTH_TEST);

        Render3DShaderProgram.setUniform("view", view);
        Render3DShaderProgram.setUniform("projection", projection);

        Render3DShaderProgram.setUniform("viewPos", w->camera->cameraPos);
        auto lights = w->lights;
        int s = lights.size();
        if(s > 16)
            s = 16;
        char buff[128];
        for(int i = 0; i < s; ++i)
        {
            sprintf(buff, "pointLights[%d].position", i);
            Render3DShaderProgram.setUniform(buff, lights[i]->pos);
            sprintf(buff, "pointLights[%d].ambient", i);
            Render3DShaderProgram.setUniform(buff, lights[i]->ambient);
            sprintf(buff, "pointLights[%d].diffuse", i);
            Render3DShaderProgram.setUniform(buff, lights[i]->color);
            sprintf(buff, "pointLights[%d].power", i);
            Render3DShaderProgram.setUniform(buff, lights[i]->pow);
        }

        for(auto& obj : w->objects)
        {
            Render3DShaderProgram.setUniform("model", obj->model);
            if(obj->mesh->MeshMaterial)
            {
                glActiveTexture(GL_TEXTURE0);
                if(obj->mesh->MeshMaterial->DiffuseMap && 0)
                {
                    glBindTexture(GL_TEXTURE_2D, /*((OpenglRHITexture*)(obj->mesh->MeshMaterial->DiffuseMap))->Tex*/ 0);
                }
                else
                {
                    Render3DShaderProgram.setUniform("objectColor", obj->mesh->MeshMaterial->DiffuseColor);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }        
                glActiveTexture(GL_TEXTURE1);
                if(obj->mesh->MeshMaterial->SpecularMap && 0)
                {
                    glBindTexture(GL_TEXTURE_2D, /*((OpenglRHITexture*)(obj->mesh->MeshMaterial->SpecularMap))->Tex*/ 0);
                }
                else
                {
                    Render3DShaderProgram.setUniform("specularStrength", obj->mesh->MeshMaterial->SpecularColor[0]);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            }
            else
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, 0);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, 0);
                Render3DShaderProgram.setUniform("objectColor", MVector3f{1.0f, 1.0f, 1.0f});
                Render3DShaderProgram.setUniform("specularStrength", 0.3);
            }
            OpenglRHIMesh* m = RefCast<OpenglRHIMesh>(obj->rhimesh);
            OpenglRHISkeleton* s = RefCast<OpenglRHISkeleton>(obj->rhiskeleton);
            if(m)
            {
                if(m->need_update)
                    if(m->mesh)
                        m->Update(m->mesh);
                    else if(m->smesh)
                        m->Update(m->smesh);
                if(m->smesh && s)
                {
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s->SSBO);
                }
                else
                {
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
                }
                glBindVertexArray(m->VAO);
                glDrawElements(GL_TRIANGLES, m->SIZE, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }

        }
        return 0;
    }

    virtual int RenderUI(UI* w)
    {
        
        return 0;
    }
    
    virtual int RenderMesh(Ref<IRHIMesh> _m)
    {
        OpenglRHIMesh* m = RefCast<OpenglRHIMesh>(_m);
        if(!m)
            return 1;
        glBindVertexArray(m->VAO);
        glDrawElements(GL_TRIANGLES, m->SIZE, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        return 0;
    }
};


/*    void DrawUi()
    {

        static GLuint shader_program = 0;
        if (!shader_program)
        {
            const char* vertex_shader_code =
              R"VS_CODE(
#version 330 core

layout (location = 0) in vec3 position;

void main()
{
    gl_Position = vec4(position.x, position.y, position.z, 1.0);
}
)VS_CODE";
            const char* frag_shader_code =
              R"FS_CODE(
#version 330 core
out vec4 color;
uniform sampler2D TTT;
uniform float H;
uniform float W;
void main()
{
	color = texture(TTT, vec2(gl_FragCoord.x / W, gl_FragCoord.y / H));
}  

)FS_CODE";

            GLint  success;
            GLchar infoLog[512];
            auto   vertex = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertex, 1, &vertex_shader_code, NULL);
            glCompileShader(vertex);
            glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(vertex, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
            };

            auto fragment = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragment, 1, &frag_shader_code, NULL);
            glCompileShader(fragment);
            glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(fragment, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
            };

            shader_program = glCreateProgram();
            glAttachShader(shader_program, vertex);
            glAttachShader(shader_program, fragment);
            glLinkProgram(shader_program);
            glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
            }

            glDeleteShader(vertex);
            glDeleteShader(fragment);
        }

        static GLuint VBO = 0;
        if (!VBO)
        {
            GLfloat vertices[] = {-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f};

            glGenBuffers(1, &VBO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        }

        static GLuint VAO = 0;
        if (!VAO)
        {
            GLfloat vertices[] = {
              -1.0f,
              -1.0f,
              0.0f,
              1.0f,
              -1.0f,
              0.0f,
              1.0f,
              1.0f,
              0.0f,
              -1.0f,
              -1.0f,
              0.0f,
              -1.0f,
              1.0f,
              0.0f,
              1.0f,
              1.0f,
              0.0f,
            };

            glGenVertexArrays(1, &VAO);
            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
            glEnableVertexAttribArray(0);

            glBindVertexArray(0);
        }

        static GLuint Tex = 0;

        int                  K = 20;
        int                  X = WIDTH;
        int                  Y = HEIGHT;
        std::vector<uint8_t> data(X * Y * 4);
        for (int i = 0; i < data.size(); ++i)
        {
            auto t  = rand();
            data[i] = (t & 0xff) ^ ((t << 8) & 0xff);
        }
        if (!Tex)
            glGenTextures(1, &Tex);
        glDisable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, Tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, X, Y, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);

        glUseProgram(shader_program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Tex);
        glUniform1i(glGetUniformLocation(shader_program, "TTT"), 0);
        glUniform1f(glGetUniformLocation(shader_program, "H"), HEIGHT);
        glUniform1f(glGetUniformLocation(shader_program, "W"), WIDTH);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
*/


IRHIHelper* GetIRHIHelper() { static OpenglRHIHelper _; return &_; }
