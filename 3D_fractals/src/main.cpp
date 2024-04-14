#include <seden.h>
#include <vector>
#include "../assets/options.h"
#include "shaders.h"

const float PI = 3.141592;

void processInput(Seden::PerspectiveCamera& camera, GLFWwindow* window, float speed);

GLuint createSSBO(const std::vector<glm::vec4>& varray)
{
	GLuint ssbo;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, varray.size() * sizeof(*varray.data()), varray.data(), GL_STATIC_DRAW);
	return ssbo;
}

const int width = 1280;
const int height = 720;

void defaultPresset() {
	// presets are hard coded for now
	camera.origin;
	camera.direction;
	camera.vfov = 90.f;
	camera.defocusAngle = 0.f;
	camera.focusDistance = 1.f;

	rendererOptions.maxIterations = 255;
	rendererOptions.maxBounce = 3;
	rendererOptions.minDist = 0.001f;
	rendererOptions.maxDist = 100.f;
	rendererOptions.fogDist = 0.01f;
	rendererOptions.bounceBlack = true;

	fractalOptions.maxIterations = 12;
	fractalOptions.color = vec3(1);
	fractalOptions.frequency = 0.;
	fractalOptions.shift = 0.;
	fractalOptions.angleA = 0;
	fractalOptions.angleB = 0;

	mandelboxOptions.fixedRadius2 = 1.0;
	mandelboxOptions.minRadius2 = 0.5;
	mandelboxOptions.foldingLimit = 1.;
	mandelboxOptions.scale = 2.;

	sierpinskiOptions.scale = 2.f;
	sierpinskiOptions.c = vec3(1);

	mengerOptions.scale = 3.f;
	mengerOptions.c = vec3(1);

	materialOptions.roughness = 0.5;
	materialOptions.refractionRatio = 0.5;
}

int main() {
	Seden::win::init(1280, 720, "3D fractals");
	Seden::win::initGui();

	defaultPresset();

	Shader sh;
	createShader(sh);

	std::vector<glm::vec4> colBuf(width * height, glm::vec4(0));
	GLuint ssbo = createSSBO(colBuf);

	float aspect = 16 / 9.f;

	Seden::PerspectiveCamera cam(aspect);

	GLuint triangleVAO, triangleVBO;
	glGenVertexArrays(1, &triangleVAO);
	glBindVertexArray(triangleVAO);

	int frame = 0;

	bool optionsWindow;
	bool timeLine;

	float speed = 1;
	

	while (Seden::win::isRunning()) {
		frame++;
		glm::vec3 oldRot = cam.getFront();
		glm::vec3 oldPos = cam.getPosition();
		processInput(cam, Seden::win::getWindowRef(), speed);
		Seden::win::clear();
		Seden::win::clearGui();

		// GUI
		ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
		ImGui::Begin("Options", &optionsWindow, ImGuiWindowFlags_MenuBar);
		if (glfwGetMouseButton(Seden::win::getWindowRef(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && ImGui::IsAnyItemHovered()) {
			itemChanged = true;
		}
		else itemChanged = false;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("renderer")) {
				if (ImGui::MenuItem("path tracing")) { select_renderer = PATH_TRACING; }
				if (ImGui::MenuItem("blinn phong"))  { select_renderer = BLINN_PHONG; }
				if (ImGui::MenuItem("preview"))    { select_renderer = PREVIEW; }
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("fractal")) {
				if (ImGui::MenuItem("Mandelbulb")) { select_fractal = MANDELBULB; }
				if (ImGui::MenuItem("Mandelbox")) { select_fractal = MANDELBOX; }
				if (ImGui::MenuItem("Sierpinski")) { select_fractal = SIERPINSKI ; }
				if (ImGui::MenuItem("Menger")) { select_fractal = MENGER; }
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("material")) {
				if (ImGui::MenuItem("Lambertian")) { select_material = LAMBERTIAN; }
				if (ImGui::MenuItem("Metal")) { select_material = METAL; }
				if (ImGui::MenuItem("Dielectric")) { select_material = DIELECTRIC; }
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		
		}
		// camera
		if (ImGui::CollapsingHeader("camera")) {
			ImGui::Text(("pos : " + std::to_string(oldPos.x) + " " + std::to_string(oldPos.y) + " " + std::to_string(oldPos.z)).c_str());
			ImGui::Text(("rot : " + std::to_string(oldRot.x) + " " + std::to_string(oldRot.y) + " " + std::to_string(oldRot.z)).c_str());
			ImGui::SliderFloat("vertical FOV", &camera.vfov, 0, 180);
			ImGui::SliderFloat("defocus angle", &camera.defocusAngle, 0, 20);
			ImGui::SliderFloat("focus distance", &camera.focusDistance, 0, 3, "%.10f");
			ImGui::SliderFloat("speed", &speed, 0, 10, "%.10f");
		}
		
		
		// Renderer options
		if (ImGui::CollapsingHeader("Renderer options")) {
			ImGui::SeparatorText("Ray marching");
			ImGui::SliderInt("maximum iterations", &rendererOptions.maxIterations, 0, 255);
			ImGui::SliderFloat("minimum distance", &rendererOptions.minDist, .0f, .001f,"%.10f");
			ImGui::SliderFloat("maximum distance", &rendererOptions.maxDist, 10, 1000);
			ImGui::SliderFloat("fog distance", &rendererOptions.fogDist, 0, 1., "%.5f");
			switch (select_renderer)
			{
			case PATH_TRACING:
				ImGui::SeparatorText("path tracing");
				ImGui::SliderInt("maximum bounces", &rendererOptions.maxBounce, 1, 25);
				ImGui::Checkbox("bounce black", &rendererOptions.bounceBlack);
				break;
			case BLINN_PHONG:
				ImGui::SeparatorText("blinn phong");
				break;
			case PREVIEW:
				ImGui::SeparatorText("preview");
				break;
			default:
				break;
			}
		}
		// Fractal options
		if (ImGui::CollapsingHeader("Fractal options")) {
			ImGui::SliderInt("maximum iterations2", &fractalOptions.maxIterations, 0, 100);
			ImGui::ColorEdit3 ("color", value_ptr(fractalOptions.color));
			ImGui::SliderFloat("frequency", &fractalOptions.frequency, .0f, 50.f, "%.5f");
			ImGui::SliderFloat("shift", &fractalOptions.shift, .0f, 50.f, "%.5f");

			ImGui::SliderFloat("rotation A", &fractalOptions.angleA, -PI, PI, "%.5f");
			ImGui::SliderFloat("rotation B", &fractalOptions.angleB, -PI, PI, "%.5f");
			switch (select_fractal)
			{
			case MANDELBULB:
				ImGui::SeparatorText("mandelbulb");
				break;
			case MANDELBOX:
				ImGui::SeparatorText("mandelbox");
				ImGui::SliderFloat("fixedRadius2",&mandelboxOptions.fixedRadius2, .0f, 10.f, "%.5f");
				ImGui::SliderFloat("minRadius2",&mandelboxOptions.minRadius2, .0f, 10.f, "%.5f");
				ImGui::SliderFloat("foldingLimit",&mandelboxOptions.foldingLimit, .0f, 10.f, "%.5f");
				ImGui::SliderFloat("scale",&mandelboxOptions.scale, .0f, 10.f, "%.5f");
				break;
			case SIERPINSKI:
				ImGui::SeparatorText("sierpinski");
				ImGui::SliderFloat("scale", &sierpinskiOptions.scale, .0f, 10.f, "%.5f");
				ImGui::SliderFloat3("center", glm::value_ptr(sierpinskiOptions.c) , .0f, 10.f, "%.5f");
				break;
			case MENGER:
				ImGui::SeparatorText("menger");
				ImGui::SliderFloat("scale", &mengerOptions.scale, .0f, 5.f, "%.5f");
				ImGui::SliderFloat3("center", glm::value_ptr(mengerOptions.c), .0f, 5.f, "%.5f");
				break;
			default:
				break;
			}
			
		}

		if (ImGui::CollapsingHeader("Material options")) {
			switch (select_material)
			{
			case LAMBERTIAN:
				ImGui::Text("no options available");
				break;
			case METAL:
				ImGui::SliderFloat("roughness", &materialOptions.roughness, 0, 1, "%.5f");
				break;
			default:
				ImGui::SliderFloat("refraction ratio", &materialOptions.refractionRatio, 0, 1, "%.5f");
				break;
			}
		}
		ImGui::End();


		ImGui::Begin("timeLine", &timeLine, ImGuiWindowFlags_MenuBar);

		ImGui::End();

		// render
		sh.Bind();
		
		sh.setInt("frame", frame);
		sh.setInt("select_fractal", select_fractal);
		sh.setInt("select_renderer", select_renderer);
		sh.setInt("select_material", select_material);
		sh.setVec2("resolution", glm::vec2(width, height));

		sh.setVec3("camera.origin", cam.getPosition());
		sh.setVec3("camera.direction", cam.getFront());
		sh.setFloat("camera.vfov", camera.vfov);
		sh.setFloat("camera.defocusAngle", camera.defocusAngle);
		sh.setFloat("camera.focusDistance", camera.focusDistance);

		sh.setInt("rendererOptions.maxIterations",rendererOptions.maxIterations);
		sh.setFloat("rendererOptions.minDist", rendererOptions.minDist);
		sh.setFloat("rendererOptions.maxDist", rendererOptions.maxDist);
		sh.setFloat("rendererOptions.fogDist", rendererOptions.fogDist);
		sh.setInt("rendererOptions.maxBounce", rendererOptions.maxBounce);
		sh.setBool("rendererOptions.bounceBlack", rendererOptions.bounceBlack);

		sh.setInt("fractalOptions.maxIterations", fractalOptions.maxIterations);
		sh.setVec3("fractalOptions.color", fractalOptions.color);
		sh.setFloat("fractalOptions.frequency", fractalOptions.frequency);
		sh.setFloat("fractalOptions.shift", fractalOptions.shift);
		sh.setFloat("fractalOptions.angleA", fractalOptions.angleA);
		sh.setFloat("fractalOptions.angleB", fractalOptions.angleB);

		sh.setFloat("mandelboxOptions.fixedRadius2", mandelboxOptions.fixedRadius2);
		sh.setFloat("mandelboxOptions.minRadius2",   mandelboxOptions.minRadius2);
		sh.setFloat("mandelboxOptions.foldingLimit", mandelboxOptions.foldingLimit);
		sh.setFloat("mandelboxOptions.scale",		 mandelboxOptions.scale);

		sh.setFloat("sierpinskiOptions.scale", sierpinskiOptions.scale);
		sh.setVec3("sierpinskiOptions.c", sierpinskiOptions.c);

		sh.setFloat("mengerOptions.scale", mengerOptions.scale);
		sh.setVec3("mengerOptions.c", mengerOptions.c);
		
		sh.setFloat("materialOptions.roughness", materialOptions.roughness);
		sh.setFloat("materialOptions.refractionRatio", materialOptions.refractionRatio);

		if (cam.getPosition() != oldPos || oldRot != cam.getFront() || itemChanged) {
			sh.setBool("moved", true);
		}
		else {
			sh.setBool("moved", false);
		};
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
		glBindVertexArray(triangleVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		Seden::win::drawGui();
		Seden::win::display();
	}
	
	Seden::win::terminate();
	Seden::win::terminateGui();
}
// todo : bloom lod , tonemaping

void processInput(Seden::PerspectiveCamera& camera, GLFWwindow* window, float speed)
{
	const float cameraSpeed = speed * Seden::win::getDeltaTime();
	const float sensitivity = 1.5f * Seden::win::getDeltaTime();

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.moveFront(cameraSpeed);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.moveFront(-cameraSpeed);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.moveRight(cameraSpeed);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.moveRight(-cameraSpeed);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		camera.moveUp(cameraSpeed);
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		camera.moveUp(-cameraSpeed);

	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		camera.rotate(0.0, sensitivity);
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		camera.rotate(0.0, -sensitivity);
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		camera.rotate(sensitivity, 0.0);
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		camera.rotate(-sensitivity, 0.0);
}