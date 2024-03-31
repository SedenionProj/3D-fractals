#include <seden.h>

void processInput(Seden::PerspectiveCamera& camera, GLFWwindow* window);

const char* vert = R"(#version 330

layout(location = 0) in vec3 aPos;

uniform mat4 viewProjection;

out vec3 pos;

void main()
{
	pos = aPos;
	gl_Position = viewProjection * vec4(aPos, 1);
}
)";

const char* frag = R"(#version 330

out vec4 fragPos;

in vec3 pos;

void main()
{
	fragPos = vec4(1, 0, 0, 1);
}
)";

int main() {
	Seden::win::init(1920, 1080, "projName");
	Seden::win::initGui();

	Shader sh = Shader();
	sh.createShader(vert, frag);

	float aspect = 16 / 9.f;

	Seden::PerspectiveCamera cam(aspect);

	float vertices[] = {
		 -0.5, -0.5,  0,
		 -0.5,  0.5,  0,
		  0.5, -0.5,  0
	};
	GLuint triangleVAO, triangleVBO;
	glGenVertexArrays(1, &triangleVAO);
	glBindVertexArray(triangleVAO);

	glGenBuffers(1, &triangleVBO);
	glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	float var1 = 1;
	while (Seden::win::isRunning()) {
		processInput(cam, Seden::win::getWindowRef());
		Seden::win::clear();
		Seden::win::clearGui();

		ImGui::Begin("Hello, world!");
		ImGui::Text("This is some useful text.");
		if (ImGui::Button("press")) {

		}
		ImGui::SliderFloat("var1", &var1, 0, 10);
		ImGui::End();


		sh.Bind();
		sh.setMat4("viewProjection", cam.getProjectionMatrix() * cam.getViewMatrix());
		glBindVertexArray(triangleVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

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
		camera.moveRight(-cameraSpeed);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.moveRight(cameraSpeed);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		camera.moveUp(cameraSpeed);
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		camera.moveUp(-cameraSpeed);

	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		camera.rotate(0.0, -sensitivity);
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		camera.rotate(0.0, sensitivity);
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		camera.rotate(sensitivity, 0.0);
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		camera.rotate(-sensitivity, 0.0);
}