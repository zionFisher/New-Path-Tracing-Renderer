#include "Utility.hpp"
#include "CornellBox.hpp"

// #include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>

using Global::WindowWidth;
using Global::WindowHeight;
using Global::ImageName;
using Global::ImageFileType;
using Global::spp;
using Global::RussianRoulette;
using Global::IndirLightContributionRate;

int main()
{
	GLFWwindow *window = Utility::SetupGlfwAndGlad();

	if (window == nullptr)
		return 0;

	Shader pathTracingShader("SimplePathTracing.vs", "SimplePathTracing.fs");

	Camera &camera = Utility::camera;
	camera.GenerateRay();

	Model floor(Global::ModelName, Global::FloorPath, true, Global::CornellMaterialPath);
	Model left(Global::ModelName, Global::LeftPath, true, Global::CornellMaterialPath);
	Model light(Global::ModelName, Global::LightPath, true, Global::CornellMaterialPath, true);
	Model right(Global::ModelName, Global::RightPath, true, Global::CornellMaterialPath);
	Model shortbox(Global::ModelName, Global::ShortboxPath, true, Global::CornellMaterialPath);
	Model tallbox(Global::ModelName, Global::TallboxPath, true, Global::CornellMaterialPath);
    floor.Load();
	left.Load();
	light.Load();
	right.Load();
	shortbox.Load();
	tallbox.Load();

	floor.Link(left);
	floor.Link(light);
	floor.Link(right);
	floor.Link(shortbox);
	floor.Link(tallbox);

	ModelData modelData(floor);
	modelData.GenerateModelTexture();
	modelData.GenerateMaterialTexture();

	auto tuple = Utility::SetVAOVBO(camera.vertices);
	unsigned int VAO = std::get<0>(tuple);
	// unsigned int VBO = std::get<1>(tuple); // uncomment if necessary.

	pathTracingShader.use();
	pathTracingShader.setArray("DefaultMat", 12, const_cast<float *>(Global::DefaultMat));
	pathTracingShader.setInt("TriData", 0);
	pathTracingShader.setInt("MatData", 1);
	pathTracingShader.setInt("spp", 1); // high spp **real time** rendering is not supported(cuz path-tracing is not a realtime rt algorithm and FPS is very low).
	pathTracingShader.setVec2("Screen", WindowWidth, WindowHeight);
	// pathTracingShader.setArray("Triangles", sizeof(triangleVertices), const_cast<float *>(triangleVertices));
	pathTracingShader.setFloat("RussianRoulette", RussianRoulette);
	pathTracingShader.setFloat("IndirLightContriRate", IndirLightContributionRate);

	srand(time(NULL));

	glm::mat4 rayRotateMatrix = glm::identity<glm::mat4>();

	// render loop=================================================================================
	while (!glfwWindowShouldClose(window))
	{
		Utility::ProcessTime();
		Utility::ProcessInput(window);

		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float seed[4] = {(float)rand() / RAND_MAX, (float)rand() / RAND_MAX,
						 (float)rand() / RAND_MAX, (float)rand() / RAND_MAX};

		rayRotateMatrix = camera.GetRotateMatrix();

		pathTracingShader.use();
		pathTracingShader.setArray("rdSeed", 4, seed);
		pathTracingShader.setMat4("RayRotateMatrix", rayRotateMatrix);
		pathTracingShader.setVec3("Eye", camera.Position.x, camera.Position.y, camera.Position.z);

		modelData.UseModelTexture();
		modelData.UseMaterialTexture();

		glBindVertexArray(VAO);
		glDrawArrays(GL_POINTS, 0, WindowWidth * WindowHeight);

		glfwSwapBuffers(window);
		glfwPollEvents();

		Utility::PostProcess();
	}
	//=============================================================================================

	Utility::image.SaveImage(ImageName.c_str(), ImageFileType);

	glfwTerminate();
	return 0;
}