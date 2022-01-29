#ifndef MODEL_DATA_HPP
#define MODEL_DATA_HPP

#include "Model.hpp"

class ModelData
{
private:
    Model &model;

    std::vector<float> modelData;
    std::vector<float> materialData;

    unsigned int modelTextureID;
    unsigned int materialTextureID;

    void GenerateModelData();
    void GenerateMaterialData();

    void GenerateBVHModelData();
    void GenerateBVHMaterialData();

    void GenerateTexture(unsigned int &textureID, std::vector<float> &data);

public:
    ModelData(Model &model) : model(model) {}
    ~ModelData() {}

    void GenerateModelTexture();
    void GenerateMaterialTexture();

    void GenerateBVHModelTexture();
    void GenerateBVHMaterialTexture();

    void UseModelTexture();
    void UseMaterialTexture();

    void PrintModelTexture(unsigned int textureSize);
    void PrintMaterialTexture(unsigned int textureSize);
};

void ModelData::GenerateModelData()
{
    // references
    const std::vector<glm::vec3> &vertices = this->model.GetVertices();
    const std::vector<glm::vec3> &textureCoords = this->model.GetTextureCoords();
    const std::vector<glm::vec3> &normals = this->model.GetNormals();
    const std::vector<SingleModel> &models = this->model.GetModels();

    std::hash<std::string> strHash;

    for (auto &it : models)
    {
        modelData.push_back(-10086);
        std::size_t key = strHash(it.materialName) % 100000;
        modelData.push_back((int)key);
        modelData.push_back(it.isLight == true ? 1.0f : 0.0f);

        modelData.push_back(-10087);
        modelData.push_back(0);        // placeholder
        modelData.push_back(0);        // placeholder

        for (auto &face : it.faces)
        {
            glm::vec3 vec1 = face[0];
            glm::vec3 vec2 = face[1];
            glm::vec3 vec3 = face[2];

            // 3 vertices
            modelData.push_back(vertices[vec1.x - 1].x);
            modelData.push_back(vertices[vec1.x - 1].y);
            modelData.push_back(vertices[vec1.x - 1].z);
            modelData.push_back(vertices[vec2.x - 1].x);
            modelData.push_back(vertices[vec2.x - 1].y);
            modelData.push_back(vertices[vec2.x - 1].z);
            modelData.push_back(vertices[vec3.x - 1].x);
            modelData.push_back(vertices[vec3.x - 1].y);
            modelData.push_back(vertices[vec3.x - 1].z);

            // 3 texture coords
            if (vec1.y != -100.0f)
            {
                modelData.push_back(textureCoords[vec1.y - 1].x);
                modelData.push_back(textureCoords[vec1.y - 1].y);
                modelData.push_back(textureCoords[vec1.y - 1].z);
                modelData.push_back(textureCoords[vec2.y - 1].x);
                modelData.push_back(textureCoords[vec2.y - 1].y);
                modelData.push_back(textureCoords[vec2.y - 1].z);
                modelData.push_back(textureCoords[vec3.y - 1].x);
                modelData.push_back(textureCoords[vec3.y - 1].y);
                modelData.push_back(textureCoords[vec3.y - 1].z);
            }
            else
            {
                for (int i = 0; i < 9; i++)
                    modelData.push_back(0.0f);
            }

            // 3 normals
            if (vec1.z != -100.0f)
            {
                modelData.push_back(normals[vec1.z - 1].x);
                modelData.push_back(normals[vec1.z - 1].y);
                modelData.push_back(normals[vec1.z - 1].z);
                modelData.push_back(normals[vec2.z - 1].x);
                modelData.push_back(normals[vec2.z - 1].y);
                modelData.push_back(normals[vec2.z - 1].z);
                modelData.push_back(normals[vec3.z - 1].x);
                modelData.push_back(normals[vec3.z - 1].y);
                modelData.push_back(normals[vec3.z - 1].z);
            }
            else
            {
                for (int i = 0; i < 9; i++)
                    modelData.push_back(0.0f);
            }
        }
    }

    modelData.push_back(-500); // E: 5
    modelData.push_back(-140); // N: 14
    modelData.push_back(-400); // D: 4

    return;
}

void ModelData::GenerateMaterialData()
{
    // reference
    const std::unordered_map<std::string, Material> &materials = this->model.GetMaterial();

    std::hash<std::string> strHash;

    for (auto &it : materials)
    {
        Material material = it.second;
        materialData.push_back(-10090);
        materialData.push_back(strHash(material.materialName) % 100000);
        materialData.push_back(-10091);

        materialData.push_back(material.Ka.x);
        materialData.push_back(material.Ka.y);
        materialData.push_back(material.Ka.z);
        materialData.push_back(material.Kd.x);
        materialData.push_back(material.Kd.y);
        materialData.push_back(material.Kd.z);
        materialData.push_back(material.Ks.x);
        materialData.push_back(material.Ks.y);
        materialData.push_back(material.Ks.z);
        materialData.push_back(material.Ke.x);
        materialData.push_back(material.Ke.y);
        materialData.push_back(material.Ke.z);
    }

    materialData.push_back(-500); // E: 5
    materialData.push_back(-140); // N: 14
    materialData.push_back(-400); // D: 4

    return;
}

void ModelData::GenerateTexture(unsigned int &textureID, std::vector<float> &data)
{
    unsigned int textureSize = std::sqrt(data.size() / 3);
	if (textureSize * textureSize < data.size())
        textureSize++;
    data.resize(textureSize * textureSize * 3);

    // if (textureID == modelTextureID)
    //     PrintModelTexture(textureSize);
    // if (textureID == materialTextureID)
    //     PrintMaterialTexture(textureSize);

	glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, textureSize, textureSize, 0, GL_RGB, GL_FLOAT, data.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void ModelData::GenerateModelTexture()
{
    GenerateModelData();
    GenerateTexture(modelTextureID, modelData);
}

void ModelData::GenerateMaterialTexture()
{
    GenerateMaterialData();
    GenerateTexture(materialTextureID, materialData);
}

void ModelData::UseModelTexture()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, modelTextureID);
}

void ModelData::UseMaterialTexture()
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, materialTextureID);
}

void ModelData::PrintModelTexture(unsigned int textureSize)
{
    std::cout << "   ";
    for (int i = 0; i < textureSize; ++i)
    {
        std::cout << std::setw(21) << std::left << i;
    }

    std::cout << std::endl;

    std::cout << "   ";
    for (int i = 0; i < textureSize * 3; ++i)
    {
        std::cout << std::setw(7) << std::left << i;
    }

    std::cout << std::endl;
    std::cout << std::endl;

    for (int i = 0; i < textureSize; ++i)
    {
        std::cout << std::setw(3) << std::left << i;
        for (int j = 0; j < textureSize * 3; ++j)
        {
            std::cout << std::setw(7) << std::left << modelData[3 * i * textureSize + j];
        }
        std::cout << std::endl;
    }
}

void ModelData::PrintMaterialTexture(unsigned int textureSize)
{
    std::cout << "   ";
    for (int i = 0; i < textureSize; ++i)
    {
        std::cout << std::setw(21) << std::left << i;
    }

    std::cout << std::endl;

    std::cout << "   ";
    for (int i = 0; i < textureSize * 3; ++i)
    {
        std::cout << std::setw(7) << std::left << i;
    }

    std::cout << std::endl;
    std::cout << std::endl;

    for (int i = 0; i < textureSize; ++i)
    {
        std::cout << std::setw(3) << std::left << i;
        for (int j = 0; j < textureSize * 3; ++j)
        {
            std::cout << std::setw(7) << std::left << materialData[3 * i * textureSize + j];
        }
        std::cout << std::endl;
    }

    std::cout << std::endl;
}

#endif