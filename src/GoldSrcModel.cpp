//
//  GoldSrcModel.cpp
//  hlmv
//
//  Created by Fedor Artemenkov on 09.11.24.
//

#include "GoldSrcModel.h"
#include "studio.h"
#include <span>
#include <math.h>

void Model::loadFromFile(const std::string &filename)
{
    long size;
    void *buffer;
    
    FILE* fp = fopen(filename.c_str(), "rb" );

    if(fp == nullptr) {
        printf("unable to open %s\n", filename.c_str());
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buffer = malloc(size);
    fread(buffer, size, 1, fp);

    m_pin = (byte *)buffer;
    m_pheader = (studiohdr_t *)m_pin;
    
    name = m_pheader->name;
    
    printf("------------ READ HEADER --------------\n");
    printf("filename %s\n", filename.c_str());
    printf("name: %s\n", m_pheader->name);
    printf("version: %i\n", m_pheader->version);
    printf("---------------------------------------\n");
    
    readTextures();
    readBodyparts();
    readSequence();
    
    mstudiobone_t* pbone = (mstudiobone_t *)(m_pin + m_pheader->boneindex);
    bones.resize(m_pheader->numbones);
    
    for (int i = 0; i < m_pheader->numbones; ++i)
    {
        bones[i] = pbone[i].parent;
    }
    
    free(buffer);
    fclose(fp);
}

void makeTexture(byte* pin, mstudiotexture_t& texInfo, Texture& texture);

void Model::readTextures()
{
    if (m_pheader == nullptr) return;
    if (m_pheader->textureindex == 0) return;
    
    mstudiotexture_t* ptextures = (mstudiotexture_t *)(m_pin + m_pheader->textureindex);
    
    for (int i = 0; i < m_pheader->numtextures; ++i)
    {
        Texture texture;
        makeTexture(m_pin, ptextures[i], texture);
        
        textures.push_back(texture);
    }
}

void makeTexture(byte* pin, mstudiotexture_t& texInfo, Texture& texture)
{
    const int RGB_SIZE = 3;
    const int RGBA_SIZE = 4;
    
    int offset = texInfo.index;
    int count = texInfo.width * texInfo.height;
    
    byte* data = (byte*)(pin + texInfo.index);
    byte* palette = data + count;
    
    texture.data.resize(count * RGBA_SIZE);
    
    // Parsing indexed color: every item in texture data is index of color in colors palette
    for (int i = 0; i < count; ++i)
    {
        byte item = data[i];

        int paletteOffset = int(item) * RGB_SIZE;
        int pixelOffset = i * RGBA_SIZE;

        // Just applying to texture image data
        texture.data[pixelOffset + 0] = palette[paletteOffset + 0]; // red
        texture.data[pixelOffset + 1] = palette[paletteOffset + 1]; // green
        texture.data[pixelOffset + 2] = palette[paletteOffset + 2]; // blue
        texture.data[pixelOffset + 3] = 255; // alpha
    }
    
    texture.name = texInfo.name;
    texture.width = texInfo.width;
    texture.height = texInfo.height;
}

struct MeshData
{
    std::span<int16_t>& triverts;
    std::span<float>& vertices;
    std::span<float>& normals;
    mstudiotexture_t* ptexture;
    std::span<uint8_t>& boneIndices;
};

void makeMesh(const MeshData& data, Mesh& mesh);

void Model::readBodyparts()
{
    mstudiobodyparts_t* pbodyparts = (mstudiobodyparts_t *)(m_pin + m_pheader->bodypartindex);
    std::span<mstudiobodyparts_t> bodyparts(pbodyparts, m_pheader->numbodyparts);
    
    for (auto& bodypart : bodyparts)
    {
        mstudiomodel_t* pmodels = (mstudiomodel_t *)(m_pin + bodypart.modelindex);
        std::span<mstudiomodel_t> models(pmodels, bodypart.nummodels);
        
        for (auto& model : models)
        {
            float* pverts = (float *)(m_pin + model.vertindex);
            std::span<float> verts(pverts, model.numverts * 3);
            
            float* pnorms = (float *)(m_pin + model.normindex);
            std::span<float> norms(pnorms, model.numnorms * 3);
            
            uint8_t* pvert_infos = (uint8_t *)(m_pin + model.vertinfoindex);
            std::span<uint8_t> vert_infos(pvert_infos, model.numverts);
            
            mstudiomesh_t* pmeshes = (mstudiomesh_t *)(m_pin + model.meshindex);
            std::span<mstudiomesh_t> meshes(pmeshes, model.nummesh);
            
            for (auto& mesh : meshes)
            {
                int tris_count = floor((m_pheader->length - mesh.triindex) / 2);
                int16_t* ptris = (int16_t *)(m_pin + mesh.triindex);
                std::span<int16_t> tris(ptris, tris_count);
                
                mstudiotexture_t* ptexture = nullptr;
                
                Mesh result;
                
                if (mesh.skinref < m_pheader->numskinref)
                {
                    int16_t* skinrefs = (int16_t *)(m_pin + m_pheader->skinindex);
                    int16_t texture_index = skinrefs[mesh.skinref];
                    
                    mstudiotexture_t* ptextures = (mstudiotexture_t *)(m_pin + m_pheader->textureindex);
                    ptexture = &ptextures[texture_index];
                    
                    result.textureIndex = texture_index;
                }
                
                MeshData data = {
                    .triverts = tris,
                    .vertices = verts,
                    .normals = norms,
                    .ptexture = ptexture,
                    .boneIndices = vert_infos
                };
                
                makeMesh(data, result);
                
                this->meshes.push_back(result);
            }
        }
    }
}

void makeMesh(const MeshData& data, Mesh& mesh)
{
    enum class TrianglesType
    {
        TRIANGLE_FAN,
        TRIANGLE_STRIP
    };
    
    struct vert_t
    {
        glm::vec3 pos;
        glm::vec3 nrm;
        glm::vec2 uv;
        int vindex;
    };
    
    std::vector<vert_t> verticesData;
    std::vector<unsigned int> indicesData;
    
    int textureWidth = 64;
    int textureHeight = 64;
    
    if (data.ptexture != nullptr)
    {
        textureWidth = data.ptexture->width;
        textureHeight = data.ptexture->height;
    }
    
    int trisPos = 0;
    
    int verticesCount = 0;
    
    // Processing triangle series
    while (data.triverts[trisPos])
    {
        // Detecting triangle series type
        TrianglesType trianglesType = data.triverts[trisPos] < 0 ? TrianglesType::TRIANGLE_FAN : TrianglesType::TRIANGLE_STRIP;
        
        // Starting vertex for triangle fan
        int startVertIndex = -1;

        // Number of following triangles
        int trianglesNum = abs(data.triverts[trisPos]);
        
        // This index is no longer needed,
        // we proceed to the following
        trisPos += 1;

        // For counting we will make steps for 4 array items:
        // 0 — index of the vertex origin in vertices buffer
        // 1 — index of the normal
        // 2 — first uv coordinate
        // 3 — second uv coordinate
        for (int j = 0; j < trianglesNum; ++j)
        {
            int vertIndex = data.triverts[trisPos];
            int vert = data.triverts[trisPos] * 3;
            int norm = data.triverts[trisPos + 1] * 3;
            
            float u_offset = (float)data.triverts[trisPos + 2] / textureWidth;
            float v_offset = (float)data.triverts[trisPos + 3] / textureHeight;
            
            // Vertex data
            vert_t vertexData = {
                .pos = {data.vertices[vert + 0], data.vertices[vert + 1], data.vertices[vert + 2]},
                .nrm = {data.normals[norm + 0], data.normals[norm + 1], data.normals[norm + 2]},
                .uv = { u_offset, v_offset},
                .vindex = vertIndex
            };
            
            trisPos += 4;

            // Unpacking triangle strip. Each next vertex, beginning with the third,
            // forms a triangle with the last and the penultimate vertex.
            //       1 ________3 ________ 5
            //       ╱╲        ╱╲        ╱╲
            //     ╱    ╲    ╱    ╲    ╱    ╲
            //   ╱________╲╱________╲╱________╲
            // 0          2         4          6
            if (trianglesType == TrianglesType::TRIANGLE_STRIP)
            {
                if (j > 2)
                {
                    if (j % 2 == 0)
                    {
                        // even
                        indicesData.insert(indicesData.end(), {
                            indicesData[indicesData.size() - 3], // previously first one
                            indicesData[indicesData.size() - 1]  // last one
                        });
                    }
                    else
                    {
                        // odd
                        indicesData.insert(indicesData.end(), {
                            indicesData[indicesData.size() - 1], // last one
                            indicesData[indicesData.size() - 2]  // second to last
                        });
                    }
                }
            }

            // Unpacking triangle fan. Each next vertex, beginning with the third,
            // forms a triangle with the last and first vertex.
            //       2 ____3 ____ 4
            //       ╱╲    |    ╱╲
            //     ╱    ╲  |  ╱    ╲
            //   ╱________╲|╱________╲
            // 1          0            5
            if (trianglesType == TrianglesType::TRIANGLE_FAN)
            {
                if (startVertIndex == -1)
                {
                    startVertIndex = verticesCount;
                }

                if (j > 2)
                {
                    indicesData.insert(indicesData.end(), {
                        (unsigned int)startVertIndex,
                        indicesData[indicesData.size() - 1]
                    });
                }
            }

            // New one
            indicesData.push_back(verticesCount);
            verticesData.push_back(vertexData);
            
            verticesCount++;
        }
    }
    
    std::vector<MeshVertex> meshVerts;
    meshVerts.resize(verticesCount);

    for (int i = 0; i < verticesCount; ++i)
    {
        int vertIndex = verticesData[i].vindex;
        int boneIndex = data.boneIndices[vertIndex];
        
        meshVerts[i].position = verticesData[i].pos;
        meshVerts[i].normal = verticesData[i].nrm;
        meshVerts[i].texCoord = verticesData[i].uv;
        meshVerts[i].boneIndex = boneIndex;
    }
    
    mesh.vertexBuffer = meshVerts;
    mesh.indexBuffer = indicesData;
}

void calcBoneRotation(int frame, mstudiobone_t *pbone, mstudioanim_t *panim, float *angle);
void calcBonePosition(int frame, mstudiobone_t *pbone, mstudioanim_t *panim, float *pos);

void Model::readSequence()
{
    mstudioseqdesc_t* psequences = (mstudioseqdesc_t *)(m_pin + m_pheader->seqindex);
    std::span<mstudioseqdesc_t> sequences(psequences, m_pheader->numseq);
    
    for (auto& sequence : sequences)
    {
        int fps = sequence.fps;
        int numframes = sequence.numframes;
        
//        float groundSpeed = sqrt(sequence.linearmovement[0] * sequence.linearmovement[0] +
//                                 sequence.linearmovement[1] * sequence.linearmovement[1] +
//                                 sequence.linearmovement[2] * sequence.linearmovement[2]) * fps / (float(numframes) - 1);
        
        Sequence seq;
        seq.name = sequence.label;
        seq.fps = sequence.fps;
        seq.groundSpeed = 0;
        
        for (int frame_idx = 0; frame_idx < numframes; ++frame_idx)
        {
            mstudiobone_t* pbone = (mstudiobone_t *)(m_pin + m_pheader->boneindex);
            mstudioanim_t* panim = (mstudioanim_t *)(m_pin + sequence.animindex);
            
            vec3_t frame_pos[MAXSTUDIOBONES];
            vec3_t frame_rot_euler[MAXSTUDIOBONES];
            
            for (int i = 0; i < m_pheader->numbones; i++, pbone++, panim++)
            {
                calcBoneRotation(frame_idx, pbone, panim, frame_rot_euler[i]);
                calcBonePosition(frame_idx, pbone, panim, frame_pos[i]);
            }
            
            Frame frame;
            frame.rotationPerBone.resize(m_pheader->numbones);
            frame.positionPerBone.resize(m_pheader->numbones);
            
            for (int i = 0; i < m_pheader->numbones; ++i)
            {
                glm::vec3 angle = { frame_rot_euler[i][0], frame_rot_euler[i][1], frame_rot_euler[i][2] };
                
                frame.rotationPerBone[i] = glm::quat(angle);
                frame.positionPerBone[i] = { frame_pos[i][0], frame_pos[i][1], frame_pos[i][2] };
            }
            
            seq.frames.push_back(frame);
        }
        
        this->sequences.push_back(seq);
    }
}

void calcBoneRotation(int frame, mstudiobone_t *pbone, mstudioanim_t *panim, float *angle)
{
    int                    j, k;
    mstudioanimvalue_t    *panimvalue;

    for (j = 0; j < 3; j++)
    {
        if (panim->offset[j + 3] == 0)
        {
            angle[j] = pbone->value[j + 3]; // default;
        }
        else
        {
            panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j + 3]);
            k = frame;
            while (panimvalue->num.total <= k)
            {
                k -= panimvalue->num.total;
                panimvalue += panimvalue->num.valid + 1;
            }
            // Bah, missing blend!
            if (panimvalue->num.valid > k)
            {
                angle[j] = panimvalue[k + 1].value;
            }
            else
            {
                angle[j] = panimvalue[panimvalue->num.valid].value;
            }
            
            angle[j] = pbone->value[j + 3] + angle[j] * pbone->scale[j + 3];
        }
    }
}

void calcBonePosition(int frame, mstudiobone_t *pbone, mstudioanim_t *panim, float *pos)
{
    int                    j, k;
    mstudioanimvalue_t    *panimvalue;
    
    float s = 0.0;

    for (j = 0; j < 3; j++)
    {
        pos[j] = pbone->value[j]; // default;
        
        if (panim->offset[j] != 0)
        {
            panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j]);

            k = frame;
            // find span of values that includes the frame we want
            while (panimvalue->num.total <= k)
            {
                k -= panimvalue->num.total;
                panimvalue += panimvalue->num.valid + 1;
            }
            // if we're inside the span
            if (panimvalue->num.valid > k)
            {
                // and there's more data in the span
                if (panimvalue->num.valid > k + 1)
                {
                    pos[j] += (panimvalue[k + 1].value * (1.0 - s) + s * panimvalue[k + 2].value) * pbone->scale[j];
                }
                else
                {
                    pos[j] += panimvalue[k + 1].value * pbone->scale[j];
                }
            }
            else
            {
                // are we at the end of the repeating values section and there's another section with data?
                if (panimvalue->num.total <= k + 1)
                {
                    pos[j] += (panimvalue[panimvalue->num.valid].value * (1.0 - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
                }
                else
                {
                    pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
                }
            }
        }

//        if (pbone->bonecontroller[j] != -1)
//        {
//            pos[j] += m_adj[pbone->bonecontroller[j]];
//        }
    }
}
