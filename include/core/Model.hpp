#ifndef MODEL_HPP
#define MODEL_HPP

#include <glm/glm.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <unordered_set>
#include <unordered_map>

/* Only .obj and .mtl file is supported.
 * and .mtl file will provides material infomation for BRDF.
 */

/* Model model
 * mtl specified the material of all faces in this model.
 * the type of variable faces represents data structure below:
 *                faces
 *           /      |      \
 *          /       |       \
 *        face     face     face  // n faces
 *              /   |   \
 *            vec3 vec3 vec3      // each face 3 vec3
 *                / | \
 *               v  vt vn         // (v, vt, vn)
 */
struct SingleModel
{
    // std::string modelName;
    std::string materialName;
    std::vector<std::vector<glm::vec3>> faces; // faces[0 ~ n][0 ~ 3][v, vt, vn]
    bool isLight;

    SingleModel(bool isLight = false) : materialName("DEFAULT"), isLight(isLight) {}
    SingleModel(std::string materialName, bool isLight = false) : materialName(materialName), isLight(isLight) {}
};

/* Material
 * So far, only Ka, Kd, Ks and Ke is needed for BRDF.
 * Thus, any other keyword will be ignored.
 * Texture mapping will be supported in later version.
 */
struct Material
{
    std::string materialName; // name
    // unsigned int Ns;          // gloss of specular light
    // float Ni;                 // refraction
    // float d;                  // 0.0 opaque ~ 1.0 transparent
    // float Tr;                 // alpha
    // glm::vec3 Tf;             // transmission filter
    // unsigned int illum;       // illumination
    glm::vec3 Ka;             // ambient color
    glm::vec3 Kd;             // diffuse color
    glm::vec3 Ks;             // specular color
    glm::vec3 Ke;             // emissive color
    // std::string  map_Ka;
    // std::string  map_Kd;

    Material() : materialName("DEFAULT") {}
    Material(std::string materialName) : materialName(materialName),
                                         Ka(glm::vec3(0, 0, 0)), Kd(glm::vec3(0, 0, 0)),
                                         Ks(glm::vec3(0, 0, 0)), Ke(glm::vec3(0, 0, 0)) {}
};

std::unordered_set<std::string> objKeyword =
{
    "v", "vt", "vn", "vp",
    "deg", "bmat", "step", "cstype",
    "p", "l", "f", "curv", "curv2", "surf",
    "parm", "trim", "hole", "scrv", "sp", "end", "con",
    "g", "s", "mg", "o",
    "bevel", "c_interp", "d_interp", "lod",
    "usemtl", "mtllib", "shadow_obj", "trace_obj", "ctech", "stech"
};

std::unordered_set<std::string> mtlKeyword =
{
    "newmtl", "Ns", "Ni", "d", "Tr", "Tf",
    "illum", "Ka", "Kd", "Ks", "Ke",
    "map_Ka", "map_Kd"
};

class Model
{
private:
    std::vector<glm::vec3>                    vertices;
    std::vector<glm::vec3>                    textureCoords;
    std::vector<glm::vec3>                    normals;
    std::vector<SingleModel>                  models;
    std::unordered_map<std::string, Material> materials;

    std::string mtlState = "";
    bool isLight;

    bool LoadObj();
    bool LoadMtl();

    // Process line content.
    bool ProcessObjectLineContent   (std::istringstream &strStream, std::string &prefix, unsigned int &lineCounter);
    bool ProcessMaterialLineContent (std::istringstream &strStream, std::string &prefix, unsigned int &lineCounter);

    // Process .obj lines.
    void ProcessVertices      (std::istringstream &strStream);
    void ProcessTextureCoords (std::istringstream &strStream);
    void ProcessNormals       (std::istringstream &strStream);
    void ProcessMtlState      (std::istringstream &strStream);
    void ProcessFaces         (std::istringstream &strStream);
    void ProcessVTN           (std::string subString, int &v, int &t, int &n);

    // Process .mtl lines.
    void ProcessNewMaterial (std::istringstream &strStream);
    void ProcessKa          (std::istringstream &strStream);
    void ProcessKd          (std::istringstream &strStream);
    void ProcessKs          (std::istringstream &strStream);

public:
    const std::string modelName;
    const std::string objPath;
    const std::string mtlPath;
    const bool        hasMtl;

    Model(std::string modelName, std::string objPath, bool hasMtl = false, std::string mtlPath = "", bool isLight = false)
          : modelName(modelName), objPath(objPath), hasMtl(hasMtl), mtlPath(mtlPath), isLight(isLight) {}
    ~Model() {}

    void Load();
    void Link(Model &anotherModel);

    unsigned int CountFileLines(std::string filePath);

    // Return a const reference to reduce copy assignment.
    const std::vector<glm::vec3>&                    GetVertices      () const;
    const std::vector<glm::vec3>&                    GetTextureCoords () const;
    const std::vector<glm::vec3>&                    GetNormals       () const;
    const std::vector<SingleModel>&                  GetModels        () const;
    const std::unordered_map<std::string, Material>& GetMaterial      () const;
};

bool Model::LoadObj()
{
    unsigned int lineCount = CountFileLines(objPath);
    unsigned int lineCounter = 0;
    unsigned int prePercentage = 0;
    unsigned int curPercentage = 0;

    std::ifstream inStream;
    inStream.open(objPath, std::ifstream::in);
    if (!inStream)
    {
        std::cerr << "Error: Unable to open .obj file!" << std::endl;
        return false;
    }

    std::string lineBuf;
    while (getline(inStream, lineBuf))
    {
        if (lineBuf.length() > 255)
        {
            std::cerr << "Error: A line length greater than 255 may cause problems. Line at: " << lineCounter << std::endl;
            return false;
        }

        curPercentage = (int)(100 * lineCounter++ / lineCount);
        if (prePercentage != curPercentage) // cerr flushes iff percentage of completion changes.
        {
            prePercentage = curPercentage;
            std::cerr << "Remaining Lines: " << std::setw(7) << std::right << lineCount - lineCounter
                      << " || Percentage: " << std::setw(3) << std::right << curPercentage << "%\r";
            std::cerr.flush();
        }

        std::istringstream strStream(lineBuf);
        std::string prefix;
        strStream >> prefix;

        if (!ProcessObjectLineContent(strStream, prefix, lineCounter))
        {
            inStream.close();
            return false;
        }
    }

    inStream.close();
    return true;
}

bool Model::LoadMtl()
{
    if (!hasMtl || mtlPath == "")
    {
        if (!hasMtl)
            std::cerr << "Error: value of variable hasMtl is false." << std::endl;
        else
            std::cerr << "Error: never specified .mtl file path." << std::endl;

        return false;
    }

    unsigned int lineCounter = 0;

    std::ifstream inStream;
    inStream.open(mtlPath, std::ifstream::in);
    if (!inStream)
    {
        std::cerr << "Error: Unable to open .mtl file!" << std::endl;
        return false;
    }

    std::string lineBuf;

    while (getline(inStream, lineBuf))
    {
        lineCounter++;
        if (lineBuf.length() > 255)
        {
            std::cerr << "Error: A line length greater than 255 may cause problems. Line at: " << lineCounter << std::endl;
            return 0;
        }

        std::istringstream strStream(lineBuf);
        std::string prefix;
        strStream >> prefix;

        if (!ProcessMaterialLineContent(strStream, prefix, lineCounter))
        {
            inStream.close();
            return false;
        }
    }

    inStream.close();
    return true;
}

bool Model::ProcessObjectLineContent(std::istringstream &strStream, std::string &prefix, unsigned int &lineCounter)
{
    if (prefix == "" || prefix == "#")
        return true;
    if (prefix == "mtllib")
    {
        bool res = LoadMtl();
        if (!res || mtlPath == "")
        {
            if (!res)
                std::cerr << "Error: Unable to open .mtl file." << std::endl;
            else
                std::cerr << "Error: You haven't specified .mtl file path but .obj file requires it. "
                          << "Line at:" << lineCounter << std::endl;
            return false;
        }
        return true;
    }
    if (prefix == "v")
    {
        ProcessVertices(strStream);
        return true;
    }
    if (prefix == "vt")
    {
        ProcessTextureCoords(strStream);
        return true;
    }
    if (prefix == "vn")
    {
        ProcessNormals(strStream);
        return true;
    }
    if (prefix == "o")
    {
        // TODO: o
        return true;
    }
    if (prefix == "g")
    {
        // TODO: g
        return true;
    }
    if (prefix == "s") // Smooth shading only
    {
        return true;
    }
    if (prefix == "usemtl")
    {
        ProcessMtlState(strStream);
        return true;
    }
    if (prefix == "f")
    {
        ProcessFaces(strStream);
        return true;
    }
    if (objKeyword.find(prefix) == objKeyword.end())
    {
        std::cerr << "Error: Format error, check line: " << lineCounter << " of " << objPath << std::endl;
        return false;
    }
    else
    {
        std::cerr << "Error: Keyword: " << prefix << " is not supported for now. I am sorry about that." << std::endl;
        return false;
    }
}

bool Model::ProcessMaterialLineContent(std::istringstream &strStream, std::string &prefix, unsigned int &lineCounter)
{
    if (prefix == "" || prefix == "#")
        return true;
    if (prefix == "Ns" || prefix == "d" || prefix == "Tr" || prefix == "Tf" || prefix == "Ns" || prefix == "illum"||
        prefix == "Ke" || prefix == "map_Ka" || prefix == "map_Kd") // TODO: ...
        return true;
    if (prefix == "newmtl")
    {
        ProcessNewMaterial(strStream);
        return true;
    }
    if (prefix == "Ka")
    {
        ProcessKa(strStream);
        return true;
    }
    if (prefix == "Kd")
    {
        ProcessKd(strStream);
        return true;
    }
    if (prefix == "Ks")
    {
        ProcessKs(strStream);
        return true;
    }
    if (mtlKeyword.find(prefix) == mtlKeyword.end())
    {
        std::cerr << "Error: Format error, check line: " << lineCounter << " of " << mtlPath << std::endl;
        return false;
    }
    else
    {
        std::cerr << "Error: Keyword: " << prefix << " is not supported for now. I am sorry about that." << std::endl;
        return false;
    }
}

void Model::ProcessVertices(std::istringstream &strStream)
{
    float v1, v2, v3;
    strStream >> v1;
    strStream >> v2;
    strStream >> v3;
    this->vertices.push_back(glm::vec3(v1, v2, v3));
}

void Model::ProcessTextureCoords(std::istringstream &strStream)
{
    float t1, t2, t3;
    strStream >> t1;
    strStream >> t2;
    strStream >> t3;
    this->textureCoords.push_back(glm::vec3(t1, t2, t3));
}

void Model::ProcessNormals(std::istringstream &strStream)
{
    float n1, n2, n3;
    strStream >> n1;
    strStream >> n2;
    strStream >> n3;
    this->normals.push_back(glm::vec3(n1, n2, n3));
}

void Model::ProcessMtlState(std::istringstream &strStream)
{
    strStream >> this->mtlState;
    this->models.push_back(SingleModel(this->mtlState, this->isLight));
}

void Model::ProcessFaces(std::istringstream &strStream)
{
    if (this->models.size() == 0)
        this->models.push_back(SingleModel(this->isLight));

    int v1, v2, v3, t1, t2, t3, n1, n2, n3;
    std::string subString1, subString2, subString3;

    strStream >> subString1;
    strStream >> subString2;
    strStream >> subString3;

    ProcessVTN(subString1, v1, t1, n1);
    ProcessVTN(subString2, v2, t2, n2);
    ProcessVTN(subString3, v3, t3, n3);

    auto &faces = this->models.back().faces;
    faces.push_back(std::vector<glm::vec3>{glm::vec3(v1, t1, n1),
                                           glm::vec3(v2, t2, n2),
                                           glm::vec3(v3, t3, n3)});
}

void Model::ProcessVTN(std::string subString, int &v, int &t, int &n)
{
    std::size_t firstSlash = subString.find_first_of('/');
    std::size_t lastSlash = subString.find_last_of('/');

    if (firstSlash == subString.npos)
    {
        v = std::stoi(subString);
        t = -100;
        n = -100;
    }
    else if (firstSlash == lastSlash)
    {
        v = std::stoi(subString);
        t = std::stoi(subString.substr(firstSlash + 1));
        n = -100;
    }
    else if (firstSlash + 1 == lastSlash)
    {
        v = std::stoi(subString);
        t = -100;
        n = std::stoi(subString.substr(lastSlash + 1));
    }
    else
    {
        v = std::stoi(subString);
        t = std::stoi(subString.substr(firstSlash + 1, lastSlash - firstSlash - 1));
        n = std::stoi(subString.substr(lastSlash + 1));
    }
}

void Model::ProcessNewMaterial(std::istringstream &strStream)
{
    strStream >> this->mtlState;
    this->materials.insert({ this->mtlState, Material(this->mtlState) });
}

void Model::ProcessKa(std::istringstream &strStream)
{
    float a1, a2, a3;
    strStream >> a1;
    strStream >> a2;
    strStream >> a3;
    glm::vec3 Ka = glm::vec3(a1, a2, a3);
    this->materials[this->mtlState].Ka = Ka;
}

void Model::ProcessKd(std::istringstream &strStream)
{
    float d1, d2, d3;
    strStream >> d1;
    strStream >> d2;
    strStream >> d3;
    glm::vec3 Kd = glm::vec3(d1, d2, d3);
    this->materials[this->mtlState].Kd = Kd;
}

void Model::ProcessKs(std::istringstream &strStream)
{
    float s1, s2, s3;
    strStream >> s1;
    strStream >> s2;
    strStream >> s3;
    glm::vec3 Ks = glm::vec3(s1, s2, s3);
    this->materials[this->mtlState].Ks = Ks;
}

void Model::Load()
{
    LoadObj();
}

void Model::Link(Model &anotherModel)
{
    int vSize = vertices.size();
    int tSize = textureCoords.size();
    int nSize = normals.size();
    int mSize = models.size();

    this->vertices.insert(vertices.end(), anotherModel.vertices.begin(), anotherModel.vertices.end());
    this->textureCoords.insert(textureCoords.end(), anotherModel.textureCoords.begin(), anotherModel.textureCoords.end());
    this->normals.insert(normals.end(), anotherModel.normals.begin(), anotherModel.normals.end());
    this->models.insert(models.end(), anotherModel.models.begin(), anotherModel.models.end());

    int nmSize = models.size();

    for (; mSize != nmSize; mSize++)
    {
        auto &model = models[mSize];
        for (auto &face : model.faces)
        {
            face[0].x += vSize;
            face[1].x += vSize;
            face[2].x += vSize;
            if (face[0].y != -100.0f)
            {
                face[0].y += tSize;
                face[1].y += tSize;
                face[2].y += tSize;
            }
            if (face[0].z != -100.0f)
            {
                face[0].z += nSize;
                face[1].z += nSize;
                face[2].z += nSize;
            }
        }
    }

    for (auto &mat : anotherModel.materials)
    {
        this->materials[mat.first] = mat.second;
    }
}

unsigned int Model::CountFileLines(std::string filePath)
{
    std::ifstream inStream;
    inStream.open(filePath, std::ifstream::in);
    if (!inStream)
    {
        std::cerr << "Error: Unable to open file!" << std::endl;
        return false;
    }

    unsigned int lineCounter = 0;
    std::string non;
    while (getline(inStream, non))
        lineCounter++;

    inStream.close();

    return lineCounter;
}

const std::vector<glm::vec3>& Model::GetVertices() const
{
    return this->vertices;
}

const std::vector<glm::vec3>& Model::GetTextureCoords() const
{
    return this->textureCoords;
}

const std::vector<glm::vec3>& Model::GetNormals() const
{
    return this->normals;
}

const std::vector<SingleModel>& Model::GetModels() const
{
    return this->models;
}

const std::unordered_map<std::string, Material>& Model::GetMaterial() const
{
    return this->materials;
}

#endif