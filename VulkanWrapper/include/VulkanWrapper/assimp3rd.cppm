module;

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

export module assimp3rd;

// Assimp types
export using ::aiMaterial;
export using ::aiMesh;
export using ::aiScene;
export using ::aiNode;
export using ::aiString;
export using ::aiColor4D;
export using ::aiColor3D;
export using ::aiFace;
export using ::aiVector3D;
export using ::ai_real;
export using ::aiTextureType;
export using ::aiReturn;
export using ::aiPostProcessSteps;
export using ::aiTextureType_BASE_COLOR;
export using ::aiTextureType_DIFFUSE;
export using ::aiTextureType_EMISSIVE;
export using ::aiReturn_SUCCESS;
export using ::aiProcess_Triangulate;
export using ::aiProcess_FlipUVs;
export using ::aiProcess_GenUVCoords;
export using ::aiProcess_OptimizeGraph;
export using ::aiProcess_SplitLargeMeshes;
export using ::aiProcess_CalcTangentSpace;
export using ::aiProcess_GenSmoothNormals;
export using ::aiProcess_ImproveCacheLocality;
export using ::aiProcess_JoinIdenticalVertices;
export using ::aiProcess_RemoveRedundantMaterials;

export namespace Assimp {
using Assimp::Importer;
}
