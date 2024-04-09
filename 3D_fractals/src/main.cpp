#include <seden.h>
#include <vector>
void processInput(Seden::PerspectiveCamera& camera, GLFWwindow* window);

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

int main() {
	Seden::win::init(1280, 720, "projName");
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

	int maxIterations = 10;

	while (Seden::win::isRunning()) {
		frame++;
		glm::vec3 oldRot = cam.getFront();
		glm::vec3 oldPos = cam.getPosition();
		processInput(cam, Seden::win::getWindowRef());
		Seden::win::clear();
		Seden::win::clearGui();

		ImGui::Begin("Hello, world!", &my_tool_active, ImGuiWindowFlags_MenuBar);
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
		ImGui::Text("camera");
		ImGui::Text(("pos : " + std::to_string(oldPos.x) + " " + std::to_string(oldPos.y) + " " + std::to_string(oldPos.z)).c_str());
		ImGui::Text(("rot : " + std::to_string(oldRot.x) +" " + std::to_string(oldRot.y) + " " + std::to_string(oldRot.z)).c_str());

		ImGui::Text("Renderer options");
		switch (select_renderer)
		{
		case PATH_TRACING:
			ImGui::Text("path tracing");
			break;
		case BLINN_PHONG:
			ImGui::Text("blinn phong");
			break;
		case PREVIEW:
			ImGui::Text("preview");
			break;
		default:
			break;
		}

		ImGui::Text("Fractal options");
		ImGui::SliderInt("max iterations", &maxIterations,0,100);
		switch (select_fractal)
		{
		case MANDELBULB:
			ImGui::Text("mandelbulb");
			break;
		case MANDELBOX:
			ImGui::Text("mandelbox");
			break;
		case SIERPINSKI:
			ImGui::Text("sierpinski");
			break;
		case MENGER:
			ImGui::Text("menger");
			break;
		default:
			break;
		}
		ImGui::End();


		// render
		sh.Bind();
		sh.setVec3("camera.origin", cam.getPosition());
		sh.setVec3("camera.direction", cam.getFront());
		sh.setInt("frame", frame);
		sh.setInt("select_fractal", select_fractal);
		sh.setInt("select_renderer", select_renderer);

		sh.setInt("fractalProperties.maxIterations", maxIterations);

		if (cam.getPosition() != oldPos || oldRot != cam.getFront()) {
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

void processInput(Seden::PerspectiveCamera& camera, GLFWwindow* window)
{
	const float cameraSpeed = 1.f * Seden::win::getDeltaTime();
	const float sensitivity = 1.f * Seden::win::getDeltaTime();

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