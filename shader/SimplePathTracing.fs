#version 330 core

// Variables-------------------------------------------------------------------
#define EPSILON 0.0001                         // Float EPSILON
#define PI      3.1415926535897                // PI

in vec3 rayDirection;                          // Ray Direction
in vec3 eye;                                   // Position of eye
in vec2 screen;                                // Width and Height of Screen(window actually)

out vec4 FragColor;                            // Output Color

uniform sampler2D TriData;                     // Scene Data aka Triangle Data
uniform sampler2D MatData;                     // Material Data
// uniform sampler2D TexData;                  // TODO: Texture Mapping will be supported in later version(Maybe)

uniform int        spp;                        // Samples Per Pixel
uniform float[4]   rdSeed;                     // Random seed
uniform float[12]  DefaultMat;                 // Default Material
uniform float      RussianRoulette;            // Russian Roulette
uniform float      IndirLightContriRate;       // Indirect Light Contribution Rate
uniform mat4       RayRotateMatrix;

float rdCount;                                 // Random counter
float pdfLight;                                // PDF of light
vec3  debugger   = vec3(1.0, 1.0, 1.0);        // Only for debug(it's too hard to debug in GLSL)
vec3  lightColor = vec3(1.0, 1.0, 1.0);        // Default light color

ivec2 triTexSizeVec;
ivec2 matTexSizeVec;
int triTexSize;
int matTexSize;

vec3 emit = 2 * (8.0f  * vec3(0.747f + 0.058f, 0.747f + 0.258f, 0.747f) +
                15.6f * vec3(0.740f + 0.287f, 0.740f + 0.160f, 0.740f) +
                18.4f * vec3(0.737f + 0.642f, 0.737f + 0.159f, 0.737f));

// Struct----------------------------------------------------------------------
struct Ray
{
    vec3 origin;
    vec3 direction;
};

struct Triangle
{
    vec3 v0;
    vec3 v1;
    vec3 v2;
    vec3 t0;
    vec3 t1;
    vec3 t2;
    vec3 n0;
    vec3 n1;
    vec3 n2;
};

struct Intersection
{
    bool happened; // isIntersect
    bool isLight;
    vec3 coords;
    vec3 normal;
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
    vec3 Ke;
    float distance;
};

struct Material
{
    float key;
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
    vec3 Ke;
};

// Declaration-----------------------------------------------------------------

// Main
void main();

// Shading
vec3 Shade (Ray ray);

// Texture
vec3 Texture(sampler2D tex, vec2 coords, ivec2 sizeVec);
vec3 Texture(sampler2D tex, int index, ivec2 sizeVec);

// Material
Material GetDefaultMat();

// Random
float RandXY       (float x, float y);
float Rand         ();
float GetRandFloat ();

// BRDF
vec3 BRDF (vec3 wi, vec3 wo, vec3 N, vec3 Kd);

// Intersection
Intersection IntersectTriangle (Ray ray, Triangle triangle);
Intersection IntersectScene    (Ray ray);

// Triangle Process
float        GetTriangleArea     (Triangle triangle);
float        PDFTriangle         (vec3 wi, vec3 wo, vec3 N);
vec3         SampleTriangle      (vec3 wi, vec3 N);
Intersection SampleTriangleLight (Triangle triangle);
Intersection SampleLight         ();

// Main------------------------------------------------------------------------
void main()
{
    triTexSizeVec = textureSize(TriData, 0);
    matTexSizeVec = textureSize(MatData, 0);
    triTexSize = triTexSizeVec.x * triTexSizeVec.y;
    matTexSize = matTexSizeVec.x * matTexSizeVec.y;

	vec3 color;
    vec4 rayDir = RayRotateMatrix * vec4(rayDirection, 0.0f);

	color = Shade(Ray(eye, vec3(rayDir.x, rayDir.y, rayDir.z)));

	FragColor = vec4(color, 1.0);
}

// Shading---------------------------------------------------------------------
vec3 Shade(Ray ray)
{
    // Special case: outside the scene or is a light.
	Intersection scene = IntersectScene(ray);

    if (scene.happened == false)
		return vec3(0.2, 0.2, 0.2);

    if (scene.isLight)
        return lightColor;  // default light color

    // Iteration Implementation: using Array
    vec3 colorBuffer[20];
    int dirLightIndex = 0, indirLightIndex = 19;

    vec3 color = vec3(0.0f);
    int counter = 0;
    bool isFirst = true;

    for (int i = 0; i < spp; ++i)
    {
        vec3 result = vec3(0.0f);
        bool flag = true;

        Ray curRay = ray;
        Intersection inter = scene;

        while (flag)
        {
            if (!isFirst)
            {
                inter = IntersectScene(curRay);
                isFirst = false;
            }

	        vec3 dirLight = vec3(0.0f, 0.0f, 0.0f);

            int depth = 0;

            vec3 p = inter.coords;
            vec3 N = normalize(inter.normal);
            vec3 wo = normalize(-ray.direction);

            Intersection interLight = SampleLight();

            vec3 x = interLight.coords;
            vec3 ws = normalize(x - p);
            vec3 NN = normalize(interLight.normal);

            bool block = length(IntersectScene(Ray(p, ws)).coords - x) > EPSILON;

            if (!block)
            {
                colorBuffer[dirLightIndex++] = (emit * BRDF(wo, ws, N, inter.Kd) * dot(ws, N) * dot(-ws, NN))
                                               /
                                               ((length(x - p) * length(x - p)) * pdfLight);
            }

            // Ruaaian Roulette test.
            float seed = GetRandFloat();
            if (seed >= RussianRoulette || indirLightIndex - dirLightIndex <= 2)
            {
                colorBuffer[indirLightIndex--] = vec3(0.0f);
                break;
            }

            vec3 wi = normalize(SampleTriangle(wo, N));
            Ray reflectRay = Ray(p, wi);
            Intersection reflectInter = IntersectScene(reflectRay);
            if (reflectInter.happened && !reflectInter.isLight)
            {
                colorBuffer[indirLightIndex--] = IndirLightContriRate * BRDF(wo, wi, N, inter.Kd) * dot(wi, N)
                                                 /
                                                 (PDFTriangle(wo, wi, N) * RussianRoulette);
            }

            curRay = reflectRay;
            flag = reflectInter.happened;
        }

        for (int i = dirLightIndex; i >= 0; i--)
        {
            result += colorBuffer[i] + colorBuffer[19 - i] * result;
        }

        color += result / spp;
        counter++;
    }

	return color;
}

// Texture---------------------------------------------------------------------
vec3 Texture(sampler2D tex, vec2 coords, ivec2 sizeVec2)
{
    vec2 vec = vec2(coords.x / (sizeVec2.x - 1), coords.y / (sizeVec2.y - 1));
    return texture(tex, vec).xyz;
}

vec3 Texture(sampler2D tex, int index, ivec2 sizeVec2)
{
    vec2 coords = ivec2(index % sizeVec2.x, index / sizeVec2.y);
    vec2 vec = vec2(coords.x / (sizeVec2.x - 1), coords.y / (sizeVec2.y - 1));
    return texture(tex, vec).xyz;
}

// Material--------------------------------------------------------------------
Material GetDefaultMat()
{
    Material mat;
    mat.key = 0;
    mat.Ka = vec3(DefaultMat[0], DefaultMat[1], DefaultMat[2]);
    mat.Kd = vec3(DefaultMat[3], DefaultMat[4], DefaultMat[5]);
    mat.Ks = vec3(DefaultMat[6], DefaultMat[7], DefaultMat[8]);
    mat.Ke = vec3(DefaultMat[9], DefaultMat[10], DefaultMat[11]);

    return mat;
}

// Random----------------------------------------------------------------------
float RandXY(float x, float y)
{
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt = dot(vec2(x, y) ,vec2(a,b));
    float sn = mod(dt,3.14);
    return fract(sin(sn) * c);
}

// // optional
// float getNoise(float x, float y)
// {
//     vec2 theta_factor_a = vec2(0.9898, 0.233);
//     vec2 theta_factor_b = vec2(12.0, 78.0);
//
//     float theta_a = dot(vec2(x, y), theta_factor_a);
//     float theta_b = dot(vec2(x, y), theta_factor_b);
//     float theta_c = dot(vec2(y, x), theta_factor_a);
//     float theta_d = dot(vec2(y, x), theta_factor_b);
//
//     float value = cos(theta_a) * sin(theta_b) + sin(theta_c) * cos(theta_d);
//     float temp = mod(197.0 * value, 1.0) + value;
//     float part_a = mod(220.0 * temp, 1.0) + temp;
//     float part_b = value * 0.5453;
//     float part_c = cos(theta_a + theta_b) * 0.43758;
//
//     return fract(part_a + part_b + part_c);
// }

// [0.0f, 1.0f)
float Rand()
{
    float a = RandXY(rayDirection.x, rdSeed[0]);
    float b = RandXY(rdSeed[1], rayDirection.y);
    float c = RandXY(rdCount++, rdSeed[2]);
    float d = RandXY(rdSeed[3], a);
    float e = RandXY(b, c);
    float f = RandXY(d, e);

    return f;
}

// 0 ~ 1
float GetRandFloat()
{
    return Rand();
}

// BRDF------------------------------------------------------------------------
// Currently only diffuse is supported, so wi have been never used.
vec3 BRDF(vec3 wi, vec3 wo, vec3 N, vec3 Kd)
{
	float cosalpha = dot(N, wo);

	if (cosalpha > 0.0f)
	{
	     vec3 diffuse = Kd / PI;
         return diffuse;
	}
    else return vec3(0.0f);
}

// Intersection----------------------------------------------------------------
Intersection IntersectTriangle(Ray ray, Triangle triangle)
{
    Intersection inter;
	inter.happened = false;

	vec3 e1 = triangle.v1 - triangle.v0;
    vec3 e2 = triangle.v2 - triangle.v0;

    vec3 triangleNormal = normalize(cross(e1, e2));
    if (dot(ray.direction, triangleNormal) > 0)
        return inter;

    vec3 pvec = cross(ray.direction, e2);
    float det = dot(e1, pvec);
    if (abs(det) < EPSILON)
        return inter;

    float det_inv = 1.0 / det;
    vec3 tvec = ray.origin - triangle.v0;
    float u = dot(tvec, pvec) * det_inv;
    if (u < 0 || u > 1)
        return inter;

    vec3 qvec = cross(tvec, e1);
    float v = dot(ray.direction, qvec) * det_inv;
    if (v < 0 || u + v > 1)
        return inter;

    float t_tmp = dot(e2, qvec) * det_inv;
    if (t_tmp < 0)
        return inter;

    inter.happened = true;
    inter.coords = ray.origin + ray.direction * t_tmp;
    inter.normal = triangleNormal;
    inter.distance = t_tmp;

    return inter;
}

Intersection IntersectScene(Ray ray)
{
	Intersection inter, temp;
	inter.happened = false;

	float minDistance = -1;

    Material material = GetDefaultMat();
    Triangle triangle;

    float curKey, resKey;
    bool curIsLight = false, resIsLight = false;

    vec3 res;
    vec3 data[9];
    int dataCounter = 0;

    for (int counter = 0; counter < triTexSize; counter++)
    {
        res = Texture(TriData, counter, triTexSizeVec);

        if (res.x == -10086)
        {
            curKey = res.y;
            curIsLight = res.z == 1.0 ? true : false;
            continue;
        }
        if (res.x == -10087)
        {
            continue;
        }
        if (res.xyz == vec3(-500, -140, -400))
            break;

        data[dataCounter++] = res.xyz;

        if (dataCounter == 9)
        {
            dataCounter = 0;

            triangle = Triangle(data[0], data[1], data[2],
                                data[3], data[4], data[5],
                                data[6], data[7], data[8]);

            temp = IntersectTriangle(ray, triangle);
            if (temp.happened && (temp.distance <= minDistance || minDistance < 0))
		    {
                resKey = curKey;
                resIsLight = curIsLight;
		    	inter = temp;
		    	minDistance = temp.distance;
		    }
        }
    }

    for (int counter = 0; counter < matTexSize; counter++)
    {
        res = Texture(MatData, counter, matTexSizeVec);

        if (res.x == -10090 && res.y == resKey)
        {
            vec3 Ka = Texture(MatData, counter + 1, matTexSizeVec);
            vec3 Kd = Texture(MatData, counter + 2, matTexSizeVec);
            vec3 Ks = Texture(MatData, counter + 3, matTexSizeVec);
            vec3 Ke = Texture(MatData, counter + 4, matTexSizeVec);
            material = Material(resKey, Ka, Kd, Ks, Ke);
            break;
        }
        if (res.xyz == vec3(-500, -140, -400))
            break;
    }

    inter.Ka = material.Ka;
    inter.Kd = material.Kd;
    inter.Ks = material.Ks;
    inter.Ke = material.Ke;
    inter.isLight = resIsLight;

	return inter;
}

// Triangle Process------------------------------------------------------------
float GetTriangleArea(Triangle triangle)
{
    return length(cross(triangle.v1 - triangle.v0, triangle.v2 - triangle.v0)) * 0.5;
}

float PDFTriangle(vec3 wi, vec3 wo, vec3 N)
{
    if (dot(wo, N) > 0.0f)
        return 0.5f * PI;
    else
        return 0.0f;
}

vec3 SampleTriangle(vec3 wi, vec3 N)
{
    float x1 = GetRandFloat(), x2 = GetRandFloat();
    float z = abs(1.0f - 2.0f * x1);
    float r = sqrt(1.0f - z * z), phi = 2 * PI * x2;
    vec3 localRay = vec3(r * cos(phi), r * sin(phi), z);

    vec3 B, C;
    if (abs(N.x) > abs(N.y))
    {
        float invLen = 1.0f / sqrt(N.x * N.x + N.z * N.z);
        C = vec3(N.z * invLen, 0.0f, -N.x * invLen);
    }
    else
    {
        float invLen = 1.0f / sqrt(N.y * N.y + N.z * N.z);
        C = vec3(0.0f, N.z * invLen, -N.y * invLen);
    }
    B = cross(C, N);

    return localRay.x * B + localRay.y * C + localRay.z * N;
}

Intersection SampleTriangleLight(Triangle triangle)
{
    Intersection inter;
    float x = sqrt(GetRandFloat());
    float y = GetRandFloat();

    inter.coords = triangle.v0 * (1.0f - x) + triangle.v1 * (x * (1.0f - y)) + triangle.v2 * (x * y);
    inter.normal = normalize(cross(triangle.v1 - triangle.v0, triangle.v2 - triangle.v0));
    pdfLight += 1.0f / GetTriangleArea(triangle);
    return inter;
}

Intersection SampleLight()
{
    Intersection inter;
    float emitAreaSum = 0;

    Triangle triangle;
    bool isLight = false;

    vec3 res;
    vec3 data[9];
    int dataCounter = 0;

    for (int counter = 0; counter < triTexSize; counter++)
    {
        res = Texture(TriData, counter, triTexSizeVec);

        if (res.x == -10086)
        {
            isLight = res.z == 1.0 ? true : false;
            continue;
        }
        if (res.x == -10087)
        {
            continue;
        }
        if (res.xyz == vec3(-500, -140, -400))
            break;

        if (isLight)
        {
            data[dataCounter++] = res.xyz;

            if (dataCounter == 9)
            {
                dataCounter = 0;
                triangle = Triangle(data[0], data[1], data[2],
                                    data[3], data[4], data[5],
                                    data[6], data[7], data[8]);
                emitAreaSum += GetTriangleArea(triangle);
            }
        }
    }

    float p = GetRandFloat() * emitAreaSum;
    emitAreaSum = 0;
    dataCounter = 0;

    for (int counter = 0; counter < triTexSize; counter++)
    {
        res = Texture(TriData, counter, triTexSizeVec);

        if (res.x == -10086)
        {
            isLight = res.z == 1.0 ? true : false;
            continue;
        }
        if (res.x == -10087)
        {
            continue;
        }
        if (res.xyz == vec3(-500, -140, -400))
            break;

        if (isLight)
        {
            data[dataCounter++] = res.xyz;

            if (dataCounter == 9)
            {
                dataCounter = 0;
                triangle = Triangle(data[0], data[1], data[2],
                                    data[3], data[4], data[5],
                                    data[6], data[7], data[8]);
                emitAreaSum += GetTriangleArea(triangle);
                if (p <= emitAreaSum)
                {
                    inter = SampleTriangleLight(triangle);
                    break;
                }
            }
        }
    }

    return inter;
}