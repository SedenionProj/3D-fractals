#pragma  once
#include <seden.h>
using namespace glm;

// begin options

struct Camera {
	vec3 origin;
	vec3 direction;
	float vfov;
	float defocusAngle;
	float focusDistance;
};

struct RendererOptions {
	int maxIterations;
	int maxBounce;
	float minDist;
	float maxDist;
	float fogDist;
	bool bounceBlack;
};

struct FractalOptions {
	int maxIterations;
	bool enableGound;

	vec3 color;
	float frequency;
	float shift;
	float angleA;
	float angleB;
};

struct MandelboxOptions {
	float fixedRadius2;
	float minRadius2;
	float foldingLimit;
	float scale;
};

struct SierpinskiOptions {
	float scale;
	vec3 c;
};

struct MengerOptions {
	float scale;
	vec3 c;
};

struct MaterialOptions {
	float roughness;
	float refractionRatio;
};

// creation

MaterialOptions materialOptions;
MengerOptions mengerOptions;
SierpinskiOptions sierpinskiOptions;
MandelboxOptions mandelboxOptions;
FractalOptions fractalOptions;
RendererOptions rendererOptions;
Camera camera;

// end options

struct Render {
	vec3 from;
	vec3 direction;
	int frame;
	int maxFrame;
	int maxSample;
	int Sample;
	bool rendering;
	bool preview;
	char filePath[30];
};
Render render;

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

enum MATERIAL
{
	LAMBERTIAN = 0,
	METAL,
	DIELECTRIC,
};

int select_renderer = BLINN_PHONG;
int select_fractal = MANDELBULB;
int select_material = LAMBERTIAN;
bool itemChanged = false;