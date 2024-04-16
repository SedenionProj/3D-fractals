#version 460
layout(std430, binding = 0) buffer _colBuff
{
   vec4 colBuff[]; 
};

layout(std430, binding = 1) buffer varSsbo
{
    float totalDistCPU; 
};

out vec4 FragCol;

#define MAX_FLOAT 9999999.

uniform int select_fractal;
uniform int select_renderer;
uniform int select_material;

uniform bool moved;
uniform int frame;
uniform vec2 resolution;

const uint MANDELBULB = 0;
const uint MANDELBOX = 1;
const uint SIERPINSKI = 2;
const uint MENGER = 3;

const uint PATH_TRACING = 0;
const uint BLINN_PHONG = 1;
const uint PREVIEW = 2;

const uint LAMBERTIAN = 0;
const uint METAL = 1;
const uint DIELECTRIC = 2;

// (options)

struct HitRecord{
    vec3 position;
    float dist;
    vec3 normal;
    vec3 color;
};


struct Ray{
    vec3 origin;
    vec3 direction;
};

// ----- Hash -----

float g_seed = 0.;

uint base_hash(uvec2 p) {
    p = 1103515245U*((p >> 1U)^(p.yx));
    uint h32 = 1103515245U*((p.x)^(p.y>>3U));
    return h32^(h32 >> 16);
}

float hash1(inout float seed) {
    uint n = base_hash(floatBitsToUint(vec2(seed+=.1,seed+=.1)));
    return float(n)/float(0xffffffffU);
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

const float ang = 1.;

// ----- Math -----

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

float schlick(float cosine, float ref_idx){
    float r0 = (1.-ref_idx)/(1.+ref_idx);
    r0 = r0*r0;
    return r0 +(1.-r0)*pow((1.-cosine),5.);
}

// ----- Signed distance functions -----

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
    t0 = MAX_FLOAT;
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

		t0 = min(t0, length(z));
    }
    return length(z) / abs(dr);
}

float sdMandelbulb(vec3 pos, float power, inout float t0) {
	vec3 z = pos;
	float dr = 1.0;
	float r = 0.0;
    t0 = MAX_FLOAT;
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
    t0 = MAX_FLOAT;
    for (n = 0; n < fractalOptions.maxIterations; n++) {
        z*=rotateX(fractalOptions.angleA);
        if(z.x+z.y<0) z.xy = -z.yx;
        if(z.x+z.z<0) z.xz = -z.zx;
        if(z.y+z.z<0) z.zy = -z.yz;
        z*=rotateX(fractalOptions.angleB);
        z = z*sierpinskiOptions.scale - sierpinskiOptions.c*(sierpinskiOptions.scale-1.0);
        t0 = min(t0, length(z));
    }
    return (length(z) ) * pow(sierpinskiOptions.scale, -float(n));
}

float sdMenger(vec3 z, inout float t0){

    int n = 0;
    t0 = MAX_FLOAT;

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
    if(fractalOptions.enableGound)
        h=min(h,sdPlane(p, -1.));
    return h;
}

// ----- Ray Marching -----

float ambientOcclusion(vec3 point, vec3 normal, float step_dist, float step_nbr)
{
    float occlusion = 1.f;
    float tmp;
    while(step_nbr > 0.0)
    {
	occlusion -= pow(step_nbr * step_dist - (sdScene( point + normal * step_nbr * step_dist,tmp)),2.) / step_nbr;
	step_nbr--;
    }

    return occlusion;
}


float softShadow(vec3 ro,vec3 rd, float mint, float maxt, float w )
{
    float res = 1.0;
    float t = mint;
    float tmp;
    for( int i=0; i<256 && t<maxt; i++ )
    {
        float h = sdScene(ro + t*rd,tmp);
        res = min( res, h/(w*t) );
        t += clamp(h, 0.005, 0.50);
        if( res<-1.0 || t>maxt ) break;
    }
    res = max(res,-1.0);
    return 0.25*(1.0+res)*(1.0+res)*(2.0-res);
    // https://iquilezles.org/articles/rmshadows/
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

vec3 setFrontNormal(vec3 n, vec3 dir){
    return dot(n,dir)<0 ? n : -n;
}

bool rayMarch(Ray ray, inout HitRecord hit){
    float totalDistance = 0.;
    int i;
    vec3 p;
    float t0 = 0.;
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
    hit.dist = totalDistance;
    p = ray.origin + totalDistance*ray.direction;
    hit.position = ray.origin + 0.99*totalDistance*ray.direction;
    hit.normal = setFrontNormal(calcNormal(p),ray.direction);

    hit.color =  0.5 + 0.5 * sin(fractalOptions.shift + fractalOptions.frequency * t0 + fractalOptions.color);
    return true;
}



void scatter(inout Ray ray, HitRecord hit){
    ray.origin = hit.position;
    switch (select_material){
    case LAMBERTIAN:
        ray.direction = hit.normal+random_in_unit_sphere(g_seed);
        break;
    case METAL:
        vec3 reflected = reflect(normalize(ray.direction),hit.normal);
        ray.direction = reflected + materialOptions.roughness*random_in_unit_sphere(g_seed);
        break;
    case DIELECTRIC:
        float refraction_ratio = materialOptions.refractionRatio;
        
        vec3 unit_dir = normalize(ray.direction); 
        float cos_theta = min(dot(-unit_dir, hit.normal),1.0);
        float sin_theta = sqrt(1.0 - cos_theta*cos_theta);
                
        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        if(cannot_refract || schlick(cos_theta,refraction_ratio)>hash1(g_seed)){
            ray.direction =  reflect(unit_dir,hit.normal);

        }else{
            ray.direction =  refract(unit_dir,hit.normal,refraction_ratio);
        }
        break;
    }
    
}

// ----- Renderers -----

vec3 pathTrace(Ray ray){
    vec3 backgroundColor = vec3(0.8,0.9,1.);
    Ray currentRay = ray;
    HitRecord hit;
    vec3 attenuation  = vec3(1);
    
    for(int i = 0; i<rendererOptions.maxBounce; i++){
        if(!rayMarch(currentRay, hit)){
            vec3 col = attenuation*getSkyColor(currentRay.direction);
            // fog is broken on path tracing
            return mix(col, getSkyColor(ray.direction), 1.-exp(-rendererOptions.fogDist * hit.dist * hit.dist));
        }
        scatter(currentRay, hit);
        attenuation *= hit.color;
    }
    if (rendererOptions.bounceBlack)
        return vec3(0);
    else
        return attenuation*getSkyColor(currentRay.direction);
}

vec3 blinnPhong(Ray ray){
    HitRecord hit;
    vec3 color = getSkyColor(ray.direction);
    vec3 lightPosition = vec3(1,1,1)*1000.;
    if(rayMarch(ray, hit)){
        vec3 lightDirection = normalize(lightPosition - hit.position);
        
        float ambient = 0.01;
        float diffuse = max(dot(hit.normal, lightDirection),0.)*0.5;
        vec3 halfwayDir = normalize(lightDirection + (ray.origin - hit.position));  
        float spec = pow(max(dot(hit.normal, halfwayDir), 0.0), 32.0);

        float sh = softShadow(hit.position, lightDirection, 0.01, 3.0, 0.1);

        color = (ambient+diffuse*sh)*hit.color + spec;

        float ambOccl = clamp(pow(ambientOcclusion(hit.position,hit.normal,0.015,20.),32.),0.1,1.);

        color = color*ambOccl;

        color = mix(color, getSkyColor(ray.direction), 1.-exp(-rendererOptions.fogDist * hit.dist * hit.dist));
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
        
    vec2 aa = hash2(g_seed)*0.001; // antialiasing

    vec3 dir = normalize(h*(uv.x+aa.x)*right + h*(uv.y+aa.y)*up + front*focusDistance - offset);
    return Ray(origin+offset, dir);

}

float vignette(vec2 uv){
    // from https://www.shadertoy.com/view/lsKSWR
    uv *=  1.0 - uv.yx;
    return pow(uv.x*uv.y *30.0, 0.07);
}

void main(){
	vec2 uv = 2.*gl_FragCoord.xy/resolution-1.;
    uv.x*=resolution.x/resolution.y;

    g_seed = float(base_hash(floatBitsToUint(gl_FragCoord.xy)))/float(0xffffffffU) + frame;
    int pixelIndex = int(gl_FragCoord.x+resolution.x*gl_FragCoord.y);

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
    
    float tmp;
    if(pixelIndex == resolution.y*resolution.x/2)
        totalDistCPU = sdScene(ray.origin,tmp);

    

    if(moved)
        colBuff[pixelIndex] =  vec4(color, 1);
    else
        colBuff[pixelIndex] += vec4(color, 1);
    
    color = colBuff[pixelIndex].xyz/colBuff[pixelIndex].w;
    color = pow(color, vec3(1.0/2.2)); // Gamma correction
    color *= vignette(gl_FragCoord.xy/resolution);

    FragCol = vec4(color,1.0); // couleur finale	
}