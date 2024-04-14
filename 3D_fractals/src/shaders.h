#pragma once
#include <seden.h>
#include <fstream>
#include <string>
#include <tuple>

void createShader(Shader& shader) {
	std::string optionsCode;
	std::string fragmentCode;

	auto sh = shader.getShaderCode("assets/fractalVert.glsl", "assets/fractalFrag.glsl");

	std::ifstream file("assets/options.h");
	std::ifstream fragment("assets/fractalFrag.glsl");

	if (!fragment.is_open()) {
		std::cerr << "Error opening options file." << std::endl;
		return;
	}

	if (!file.is_open()) {
		std::cerr << "Error opening fragment file." << std::endl;
		return;
	}

	std::string line;
	// parse options
	while (std::getline(file, line)) {
		if (line.find("// begin options") != std::string::npos) {
			break;
		}
	}

	while (std::getline(file, line)) {
		if (line.find("// creation") != std::string::npos) {
			break;
		}
		optionsCode += line + "\n";
	}

	while (std::getline(file, line)) {
		if (line.find("// end options") != std::string::npos) {
			break;
		}
		if (!line.empty())
			optionsCode += "uniform " + line + "\n";
	}

	// fragment code

	while (std::getline(fragment, line)) {
		if (line.find("// (options)") != std::string::npos) {
			fragmentCode += optionsCode + "\n";
		}
		fragmentCode += line + "\n";
	}

	//std::cout << fragmentCode << "\n";

	shader.createShader(std::get<0>(sh).c_str(), fragmentCode.c_str());

	file.close();
}