#version 460

layout(std430, binding = 0) buffer _colBuff
{
   vec4 colBuff[]; 
};

out vec4 FragCol;

#define MAX_BOUNCE 5

#define MAX_FLOAT 9999999.

uniform int select_fractal;
uniform int select_renderer;

uniform bool moved;
uniform int frame;

const uint MANDELBULB = 0;
const uint MANDELBOX = 1;
const uint SIERPINSKI = 2;
const uint MENGER = 3;

const uint PATH_TRACING = 0;
const uint BLINN_PHONG = 1;
const uint PREVIEW = 2;

struct Camera{
    vec3 origin;
    vec3 direction;
    float vfov;
    float defocusAngle;
    float focusDistance;
};
uniform Camera camera;

struct RendererOptions{
    int maxIterations;
    float minDist;
    float maxDist;
};
uniform RendererOptions rendererOptions;


struct FractalOptions {
    int maxIterations;

    vec3 color;
    float frequency;
    float shift;

    float angleA;
    float angleB;
};  
uniform FractalOptions fractalOptions;

struct MandelboxOptions{
    float fixedRadius2;
    float minRadius2;
    float foldingLimit;
    float scale;
};
uniform MandelboxOptions mandelboxOptions;

struct SierpinskiOptions{
    float scale;
    vec3 c;
};
uniform SierpinskiOptions sierpinskiOptions;

struct MengerOptions{
    float scale;
    vec3 c;
};
uniform MengerOptions mengerOptions;

struct HitRecord{
    vec3 position;
    vec3 normal;
    vec3 color;
};


struct Ray{
    vec3 origin;
    vec3 direction;
};

// hash

float g_seed = 0.;

uint base_hash(uvec2 p) {
    p = 1103515245U*((p >> 1U)^(p.yx));
    uint h32 = 1103515245U*((p.x)^(p.y>>3U));
    return h32^(h32 >> 16);
}

vec2 hash2(inout float seed) {
    uint n = base_hash(floatBitsToUint(vec2(seed+=.1,seed+=.1)));
    uvec2 rz = uvec2(n, n*48271U);
    return vec2(rz.xy & uvec2(0x7fffffffU))/float(0x7fffffff);
}

vec3 hash3(inout float seed) {
    uint n = base_hash(floatBitsToUint(vec2(seed+=.1,seed+=.1)));
    uvec3 rz = uvec3(n, n*16807U, n*48271U);
    return vec3(rz & uvec3(0x7fffffffU))/float(0x7fffffff);
}

vec2 random_in_unit_disk(inout float seed) {
    vec2 h = hash2(seed) * vec2(1.,6.28318530718);
    float phi = h.y;
    float r = sqrt(h.x);
	return r * vec2(sin(phi),cos(phi));
}

vec3 random_in_unit_sphere(inout float seed) {
    vec3 h = hash3(seed) * vec3(2.,6.28318530718,1.)-vec3(1,0,0);
    float phi = h.y;
    float r = pow(h.z, 1./3.);
	return r * vec3(sqrt(1.-h.x*h.x)*vec2(sin(phi),cos(phi)),h.x);
}



// sdf
const float ang = 1.;


mat3 rotateX(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(1, 0, 0),
        vec3(0, c, -s),
        vec3(0, s, c)
    );
}

mat3 rotateY(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(c, 0, s),
        vec3(0, 1, 0),
        vec3(-s, 0, c)
    );
}




void sphere_fold(inout vec3 z, inout float dz) {
    float r2 = dot(z, z);
    if(r2 < mandelboxOptions.minRadius2) {
        float temp = (mandelboxOptions.fixedRadius2 / mandelboxOptions.minRadius2);
        z *= temp;
        dz *= temp;
    }else if(r2 < mandelboxOptions.fixedRadius2) {
        float temp = (mandelboxOptions.fixedRadius2 / r2);
        z *= temp;
        dz *= temp;
    }
}

void box_fold(inout vec3 z, inout float dz) {
    z = clamp(z, -mandelboxOptions.foldingLimit, mandelboxOptions.foldingLimit) * 2.0 - z;
}

float sdMandelbox(vec3 z, inout float t0) {
	//z.z = mod(z.z + 1.0, 2.0) - 1.0;
    vec3 offset = z;
    float dr = 1.0;
    for(int n = 0; n < fractalOptions.maxIterations; n++) {
		//z.xy = rot*z.xy;
        z*=rotateY(fractalOptions.angleA);
        
        box_fold(z, dr);
        sphere_fold(z, dr);

        z = mandelboxOptions.scale * z + offset;
        dr = dr * abs(mandelboxOptions.scale) + 1.;

        z*=rotateX(fractalOptions.angleB);

		t0 = min(t0, max(max(z.x,z.z),z.y));
    }
    float r = length(z);
    return r / abs(dr);
}

float sdMandelbulb(vec3 pos, float power, inout float t0) {
	vec3 z = pos;
	float dr = 1.0;
	float r = 0.0;
	for (int i = 0; i < fractalOptions.maxIterations ; i++) {
		r = length(z);
		if(r > 1.5) break;
		
        z*=rotateY(fractalOptions.angleA);

		// convert to polar coordinates
		float theta = acos(z.z/r);
		float phi = atan(z.y,z.x);
		dr =  pow( r, power-1.0)*power*dr + 1.0;

		// scale and rotate the point
		float r = pow( r,power);
		theta = theta*power;
		phi = phi*power;
		
        

		// convert back to cartesian coordinates
		z = r*vec3(sin(theta)*cos(phi), sin(phi)*sin(theta), cos(theta))+pos;
        
        z*=rotateX(fractalOptions.angleB);

        t0 = min(t0, r);
	}
	return 0.5*log(r)*r/dr;
}

float sdBox( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

float sdSierpinski(vec3 z, inout float t0)
{
    float r;
    int n = 0;
    while (n < fractalOptions.maxIterations) {
        z*=rotateX(fractalOptions.angleA);
        if(z.x+z.y<0) z.xy = -z.yx;
        if(z.x+z.z<0) z.xz = -z.zx;
        if(z.y+z.z<0) z.zy = -z.yz;
        z*=rotateX(fractalOptions.angleB);
        z = z*sierpinskiOptions.scale - sierpinskiOptions.c*(sierpinskiOptions.scale-1.0);
        n++;
    }
    return (length(z) ) * pow(sierpinskiOptions.scale, -float(n));
}

float sdMenger(vec3 z, inout float t0){

    int n = 0;
   

    for (n = 0; n < fractalOptions.maxIterations; n++) {
      z = z*rotateY(fractalOptions.angleA);
      z = z*rotateX(fractalOptions.angleB);
      //folding
      z = abs(z);
      if(z.x<z.y) z.xy = z.yx;
      if(z.x<z.z) z.xz = z.zx;
      if(z.y<z.z) z.zy = z.yz;
   
    
      z.x=mengerOptions.scale*z.x-mengerOptions.c.x*(mengerOptions.scale-1);
      z.y=mengerOptions.scale*z.y-mengerOptions.c.y*(mengerOptions.scale-1);
      z.z=mengerOptions.scale*z.z;
   
      if(z.z>0.5*mengerOptions.c.z*(mengerOptions.scale-1)) z.z-=mengerOptions.c.z*(mengerOptions.scale-1);
   
      t0 = min(t0, length(z));
   }

   return sdBox(z, mengerOptions.c)  * pow(mengerOptions.scale, -float(n));

}

float sdSphere(vec3 p, float r){
    float displacement = sin(5.0 * p.x) * sin(5.0 * p.y) * sin(5.0 * p.z) * 0.00;
    return length(p) - r + displacement;
}

float sdPlane(vec3 p, float h){
    return p.y-h;
}

float sdScene(vec3 p, inout float t0){
    float power = 8.0;
    float h = MAX_FLOAT;

    switch (select_fractal){
    case MANDELBULB:
        h=min(h,sdMandelbulb(p, power, t0));
        break;
    case MANDELBOX:
        h=min(h,sdMandelbox(p, t0));
        break;
    case SIERPINSKI:
        h=min(h,sdSierpinski(p, t0));
        break;
    case MENGER:
        h=min(h,sdMenger(p, t0));
        break;
    }
    return h;
}

vec3 calcNormal(in vec3 p) {
    vec2 e = vec2(1.0, -1.0) * 0.000005;
    float tmp;
    return normalize(
      e.xyy * sdScene(p + e.xyy,tmp) +
      e.yyx * sdScene(p + e.yyx,tmp) +
      e.yxy * sdScene(p + e.yxy,tmp) +
      e.xxx * sdScene(p + e.xxx,tmp));
}

vec3 getSkyColor(vec3 direction){
    vec3 dir = normalize(direction);
    vec3 sunDir = normalize(vec3(1,1,1));
    float v = max(0.,dot(dir, sunDir));
    // background
    vec3 color = mix(vec3(0.4,0.7,1),vec3(0.7,0.9,1), direction.y);
    // sun bloom
    color += vec3(v)*0.2;
    // sun
    color += pow(v,400.);
    return color;
}

bool rayMarch(Ray ray, inout HitRecord hit){
    float totalDistance = 0.;
    int i;
    vec3 p;
    float t0 = MAX_FLOAT;
    for(i=0; i<rendererOptions.maxIterations; i++){
        p = ray.origin + totalDistance*ray.direction;
        float currentDistance = sdScene(p, t0);
        totalDistance += currentDistance;
        
        if(currentDistance < rendererOptions.minDist || totalDistance > rendererOptions.maxDist){
            totalDistance-=rendererOptions.minDist;
            break;
        }
    }
    
    if(totalDistance>rendererOptions.maxDist){
        return false;
    }
    p = ray.origin + totalDistance*ray.direction;
    hit.position = ray.origin + 0.99*totalDistance*ray.direction;
    hit.normal = calcNormal(p);
    hit.color =  0.5 + 0.5 * sin(fractalOptions.shift + fractalOptions.frequency * t0 + fractalOptions.color);
    return true;
}

vec3 pathTrace(Ray ray){
    vec3 backgroundColor = vec3(0.8,0.9,1.);
    Ray currentRay = ray;
    HitRecord hit;
    vec3 attenuation  = vec3(1);
    
    for(int i = 0; i<MAX_BOUNCE; i++){
        if(!rayMarch(currentRay, hit)){
            return attenuation*getSkyColor(currentRay.direction); //texture(iChannel0, -currentRay.direction).rgb;
        }
        currentRay.origin = hit.position;
        currentRay.direction = hit.normal+random_in_unit_sphere(g_seed);
        //attenuation *= vec3(1.+hit.normal)*0.5;
        attenuation *= hit.color;
    }
    return vec3(0);
}

vec3 blinnPhong(Ray ray){
    HitRecord hit;
    vec3 color = vec3(0);
    vec3 lightDirection = normalize(vec3(1,1,1));
    if(rayMarch(ray, hit)){
        float ambient = 0.01;
        float diffuse = max(dot(hit.normal, lightDirection),0.)*0.5;
        vec3 halfwayDir = normalize(lightDirection + (ray.origin - hit.position));  
        float spec = pow(max(dot(hit.normal, halfwayDir), 0.0), 32.0);
        color = (ambient+diffuse)*hit.color + spec;
    }
    return color;
}

vec3 preview(Ray ray){
    HitRecord hit;
    vec3 color = vec3(0);
    if(rayMarch(ray, hit)){
        color = vec3(1.+hit.normal)*0.5;
    }
    return color;
}

Ray getRay(vec2 uv, vec3 origin, vec3 direction){
    float defocusAngle =  camera.defocusAngle;
    float focusDistance = camera.focusDistance;
    
    float vfov = camera.vfov; //vertical field of view
    
    float h = focusDistance * tan(radians(vfov)/2.); // half of vertical sensor size
    
    vec3 fixedUp = vec3(0,1,0);
    vec3 front = normalize(direction);
    vec3 right = -normalize(cross(front, fixedUp));
    vec3 up = cross(front, right);  
    
    float defocusRadius = focusDistance * tan(radians(defocusAngle/2.));
    
    vec2 rd = random_in_unit_disk(g_seed)*defocusRadius;
    vec3 offset = rd.x * right + rd.y * up;
         
    vec3 dir = normalize(h*uv.x*right + h*uv.y*up + front*focusDistance - offset);
    return Ray(origin+offset, dir);

}



void main(){
	vec2 uv = 2.*gl_FragCoord.xy/1280.-1.;
    g_seed = float(base_hash(floatBitsToUint(gl_FragCoord.xy)))/float(0xffffffffU) + frame;
    int pixelIndex = int(gl_FragCoord.x+1280.*gl_FragCoord.y);

    vec3 color = vec3(0,0,0);
    
    Ray ray = getRay(uv, camera.origin, camera.direction);
    
    switch (select_renderer){
    case PATH_TRACING:
        color += pathTrace(ray);
        break;
    case BLINN_PHONG:
        color += blinnPhong(ray);
        break;
    case PREVIEW:
        color += preview(ray);
        break;
    }
    

    //color = pow(color, vec3(1.0/2.2)); // Gamma correction
    if(moved)
        colBuff[pixelIndex] =  vec4(color, 1);
    else
        colBuff[pixelIndex] += vec4(color, 1);
    
    FragCol = vec4(pow(colBuff[pixelIndex].xyz/colBuff[pixelIndex].w, vec3(1.0/2.2)),1.0); // couleur finale	
}