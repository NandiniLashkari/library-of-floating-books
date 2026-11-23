// Model.cpp
#include "Model.h"
#include "Texture.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <limits>
#include <algorithm>

namespace {

// glTF files keep their images INSIDE the file. Assimp names them "*0", "*1"...
// This finds the image for a texture slot and uploads it to the GPU.
GLuint loadEmbedded(const aiScene* scene, const aiMaterial* mat,
                    std::initializer_list<aiTextureType> slots,
                    Texture::Kind kind) {
    for (aiTextureType slot : slots) {
        aiString path;
        if (mat->GetTexture(slot, 0, &path) != AI_SUCCESS) continue;

        const aiTexture* tex = scene->GetEmbeddedTexture(path.C_Str());
        if (!tex) {
            std::cerr << "  texture " << path.C_Str() << " is not embedded\n";
            continue;
        }
        // mHeight == 0 means pcData holds a compressed PNG/JPG of mWidth bytes.
        if (tex->mHeight == 0) {
            return Texture::load2DFromMemory(
                reinterpret_cast<const unsigned char*>(tex->pcData),
                tex->mWidth, kind);
        }
    }
    return 0;
}

}  // namespace

bool Model::load(const std::string& path, float targetSize) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |            // every face becomes a triangle
        aiProcess_GenSmoothNormals |       // make normals if the file has none
        aiProcess_CalcTangentSpace |       // tangents for normal mapping
        aiProcess_JoinIdenticalVertices |  // share repeated corners
        aiProcess_PreTransformVertices);   // bake the file's node transforms in

    if (!scene || !scene->mRootNode || scene->mNumMeshes == 0) {
        std::cerr << "Model loading failed: " << path << " : "
                  << importer.GetErrorString() << '\n';
        return false;
    }
    name = path;

    // ---- materials: the textures embedded in the file ----
    for (unsigned int m = 0; m < scene->mNumMaterials; ++m) {
        const aiMaterial* mat = scene->mMaterials[m];
        Material out;
        out.albedo = loadEmbedded(scene, mat,
            {aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE},
            Texture::Kind::Color);
        out.normal = loadEmbedded(scene, mat, {aiTextureType_NORMALS},
                                  Texture::Kind::Data);
        out.orm = loadEmbedded(scene, mat,
            {aiTextureType_METALNESS, aiTextureType_DIFFUSE_ROUGHNESS,
             aiTextureType_UNKNOWN},
            Texture::Kind::Data);

        // Fallbacks so the shader always has something to sample.
        if (!out.albedo) out.albedo = Texture::solid(200, 200, 200, Texture::Kind::Color);
        out.hasNormalMap = out.normal != 0;
        if (!out.normal) out.normal = Texture::solid(128, 128, 255, Texture::Kind::Data);
        out.hasOrmMap = out.orm != 0;
        if (!out.orm) out.orm = Texture::solid(255, 180, 0, Texture::Kind::Data);
        materials.push_back(out);
    }

    // ---- meshes ----
    glm::vec3 minimum(std::numeric_limits<float>::max());
    glm::vec3 maximum(std::numeric_limits<float>::lowest());

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        std::vector<Vertex> vertices(mesh->mNumVertices);
        std::vector<unsigned int> indices;

        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            Vertex& v = vertices[i];
            const aiVector3D& p = mesh->mVertices[i];
            v.position = glm::vec3(p.x, p.y, p.z);
            v.normal = mesh->HasNormals()
                ? glm::normalize(glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y,
                                           mesh->mNormals[i].z))
                : glm::vec3(0, 1, 0);

            // Assimp flips V for glTF; flip it back so image row 0 = top.
            v.uv = mesh->HasTextureCoords(0)
                ? glm::vec2(mesh->mTextureCoords[0][i].x,
                            1.0f - mesh->mTextureCoords[0][i].y)
                : glm::vec2(0.0f);

            // Tangent + handedness: is Assimp's bitangent cross(N,T) or -cross(N,T)?
            if (mesh->HasTangentsAndBitangents()) {
                glm::vec3 t(mesh->mTangents[i].x, mesh->mTangents[i].y,
                            mesh->mTangents[i].z);
                glm::vec3 b(mesh->mBitangents[i].x, mesh->mBitangents[i].y,
                            mesh->mBitangents[i].z);
                float w = glm::dot(glm::cross(v.normal, t), b) < 0.0f ? -1.0f : 1.0f;
                v.tangent = glm::vec4(t, w);
            } else {
                v.tangent = glm::vec4(1, 0, 0, 1);
            }

            minimum = glm::min(minimum, v.position);
            maximum = glm::max(maximum, v.position);
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            for (unsigned int j = 0; j < face.mNumIndices; ++j)
                indices.push_back(face.mIndices[j]);
        }
        if (indices.empty()) continue;

        Mesh gpu;
        gpu.upload(vertices, indices);
        meshes.push_back(gpu);
        meshMaterial.push_back(static_cast<int>(mesh->mMaterialIndex));
        triangleCount += indices.size() / 3;
    }

    // ---- normalise position and size ----
    glm::vec3 centre = (minimum + maximum) * 0.5f;
    glm::vec3 dims = maximum - minimum;
    float largest = std::max(dims.x, std::max(dims.y, dims.z));
    float s = targetSize / largest;
    baseMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(s)) *
                 glm::translate(glm::mat4(1.0f), -centre);
    size = dims * s;

    std::cout << "Loaded model " << path << ": " << meshes.size() << " mesh(es), "
              << triangleCount << " triangles\n";
    return true;
}

void Model::draw(const Shader& shader, const glm::mat4& model) const {
    shader.setMat4("model", model * baseMatrix);
    for (size_t i = 0; i < meshes.size(); ++i) {
        const Material& mat = materials[meshMaterial[i]];
        glActiveTexture(GL_TEXTURE0);  glBindTexture(GL_TEXTURE_2D, mat.albedo);
        glActiveTexture(GL_TEXTURE1);  glBindTexture(GL_TEXTURE_2D, mat.normal);
        glActiveTexture(GL_TEXTURE2);  glBindTexture(GL_TEXTURE_2D, mat.orm);
        shader.setInt("hasNormalMap", mat.hasNormalMap ? 1 : 0);
        shader.setInt("hasOrmMap", mat.hasOrmMap ? 1 : 0);
        meshes[i].draw();
    }
}

void Model::drawDepth(const Shader& shader, const glm::mat4& model) const {
    shader.setMat4("model", model * baseMatrix);
    for (const Mesh& m : meshes) m.draw();
}
