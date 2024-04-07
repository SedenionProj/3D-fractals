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

	std::vector<glm::vec4> colBuf(width * height, glm::vec4(0.5));
	GLuint ssbo = createSSBO(colBuf);

	float aspect = 16 / 9.f;

	Seden::PerspectiveCamera cam(aspect);

	GLuint triangleVAO, triangleVBO;
	glGenVertexArrays(1, &triangleVAO);
	glBindVertexArray(triangleVAO);

	float var1 = 1;

	int frame = 0;
	while (Seden::win::isRunning()) {
		frame++;
		glm::vec3 oldRot = cam.getFront();
		glm::vec3 oldPos = cam.getPosition();
		processInput(cam, Seden::win::getWindowRef());
		Seden::win::clear();
		Seden::win::clearGui();

		ImGui::Begin("Hello, world!");
		ImGui::Text("This is some useful text.");
		if (ImGui::Button("press")) {

		}
		ImGui::SliderFloat("var1", &var1, 0, 10);
		ImGui::End();

		// render
		sh.Bind();
		sh.setVec3("rayOrigin", cam.getPosition());
		sh.setVec3("rayDirection", cam.getFront());
		sh.setInt("frame", frame);
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

void processInput(Seden::PerspectiveCamera& camera, GLFWwindow* window)
{
	const float cameraSpeed = 3.f * Seden::win::getDeltaTime();
	const float sensitivity = 2.f * Seden::win::getDeltaTime();

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