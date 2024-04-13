#include <seden.h>
#include <vector>

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

struct Camera {
	//glm::vec3 origin;
	//glm::vec3 direction;
	float vfov = 90.f;
	float defocusAngle = 0.f;
	float focusDistance = 1.f;
};
Camera camera;

struct RendererOptions {
	int maxIterations = 255;
	float minDist = 0.001f;
	float maxDist = 100.f;
};
RendererOptions rendererOptions;

struct FractalOptions {
	int maxIterations = 12;

	glm::vec3 color = glm::vec3(1);
	float frequency = 0.;
	float shift = 0.;
	float angleA = 0;
	float angleB = 0;
};
FractalOptions fractalOptions;

struct MandelboxOptions {
	float fixedRadius2 = 1.0;
	float minRadius2 = 0.5;
	float foldingLimit = 1.;
	float scale = 2.;
};
MandelboxOptions mandelboxOptions;

struct SierpinskiOptions {
	float scale = 2.f;
	glm::vec3 c = glm::vec3(1);
};
SierpinskiOptions sierpinskiOptions;

struct MengerOptions {
	float scale = 3.f;
	glm::vec3 c = glm::vec3(1);
};
MengerOptions mengerOptions;

int main() {
	Seden::win::init(1280, 720, "3D fractals");
	Seden::win::initGui();

	Shader sh = Shader("assets/fractalVert.glsl", "assets/fractalFrag.glsl");

	std::vector<glm::vec4> colBuf(width * height, glm::vec4(0));
	GLuint ssbo = createSSBO(colBuf);

	float aspect = 16 / 9.f;

	Seden::PerspectiveCamera cam(aspect);

	GLuint triangleVAO, triangleVBO;
	glGenVertexArrays(1, &triangleVAO);
	glBindVertexArray(triangleVAO);

	float var1 = 1;
	bool tt = 1;

	int frame = 0;
	bool my_tool_active;

	enum RENDERER
	{
		PATH_TRACING = 0,
		BLINN_PHONG,
		PREVIEW,
	};

	enum FRACTAL
	{
		MANDELBULB = 0,
		MANDELBOX,
		SIERPINSKI,
		MENGER,
	};

	int select_renderer = BLINN_PHONG;
	int select_fractal = MANDELBULB;
	bool itemChanged = false;

	float speed = 1;

	while (Seden::win::isRunning()) {
		frame++;
		glm::vec3 oldRot = cam.getFront();
		glm::vec3 oldPos = cam.getPosition();
		processInput(cam, Seden::win::getWindowRef(), speed);
		Seden::win::clear();
		Seden::win::clearGui();

		// GUI
		ImGui::Begin("Options", &my_tool_active, ImGuiWindowFlags_MenuBar);
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
			switch (select_renderer)
			{
			case PATH_TRACING:
				ImGui::SeparatorText("path tracing");
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
		ImGui::End();
		// render
		sh.Bind();
		
		sh.setInt("frame", frame);
		sh.setInt("select_fractal", select_fractal);
		sh.setInt("select_renderer", select_renderer);

		sh.setVec3("camera.origin", cam.getPosition());
		sh.setVec3("camera.direction", cam.getFront());
		sh.setFloat("camera.vfov", camera.vfov);
		sh.setFloat("camera.defocusAngle", camera.defocusAngle);
		sh.setFloat("camera.focusDistance", camera.focusDistance);

		sh.setInt("rendererOptions.maxIterations",rendererOptions.maxIterations);
		sh.setFloat("rendererOptions.minDist", rendererOptions.minDist);
		sh.setFloat("rendererOptions.maxDist", rendererOptions.maxDist);

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
// todo : vignette , bloom lod , tonemaping

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