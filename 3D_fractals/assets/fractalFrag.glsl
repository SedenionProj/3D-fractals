#version 460

layout(std430, binding = 0) buffer _colBuff
{
   vec4 colBuff[]; 
};

out vec4 FragCol;

#define MAX_STEP 255
#define MIN_DIST 0.01
#define MAX_DIST 100.

uniform vec3 rayOrigin;
uniform vec3 rayDirection;
uniform bool moved;
uniform int frame;

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

vec2 random_in_unit_disk(inout float seed) {
    vec2 h = hash2(seed) * vec2(1.,6.28318530718);
    float phi = h.y;
    float r = sqrt(h.x);
	return r * vec2(sin(phi),cos(phi));
}

struct Ray{
    vec3 origin;
    vec3 direction;
};

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


float smin(float a, float b, float k) {
  float h = clamp(0.5+0.5*(b-a)/k, 0.0, 1.0);
  return mix(b, a, h) - k*h*(1.0-h);
}

float sdRoundBox( vec3 p, vec3 b, float r )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0) - r;
}

float sdPyramid( vec3 p, float h )
{
  float m2 = h*h + 0.25;
    
  p.xz = abs(p.xz);
  p.xz = (p.z>p.x) ? p.zx : p.xz;
  p.xz -= 0.5;

  vec3 q = vec3( p.z, h*p.y - 0.5*p.x, h*p.x + 0.5*p.y);
   
  float s = max(-q.x,0.0);
  float t = clamp( (q.y-0.5*p.z)/(m2+0.25), 0.0, 1.0 );
    
  float a = m2*(q.x+s)*(q.x+s) + q.y*q.y;
  float b = m2*(q.x+0.5*t)*(q.x+0.5*t) + (q.y-m2*t)*(q.y-m2*t);
    
  float d2 = min(q.y,-q.x*m2-q.y*0.5) > 0.0 ? 0.0 : min(a,b);
    
  return sqrt( (d2+q.z*q.z)/m2 ) * sign(max(q.z,-p.y));
}

float sdPlane( vec3 p, float h )
{
  return p.y-h;
}  

float sdSphere(vec3 p, float r)
{
    return length(p)-r;
}

float sdf(vec3 p)
{
    float d = min(sdSphere(p,1.),sdPlane(p,-1.));
    d = min(d,sdRoundBox(p-vec3(4,0,0),vec3(1,1,1),0.1));
    d = min(d,sdPyramid(p-vec3(-4,-1,0),3.));
    return d;
}

float AmbientOcclusion(vec3 p, vec3 normal, float step_dist, float step_nbr)
{
    float occlusion = 1.f;
    while(step_nbr > 0.0)
    {
	occlusion -= pow(step_nbr * step_dist - (sdf( p + normal * step_nbr * step_dist)),2.) / step_nbr;
	step_nbr--;
    }

    return occlusion;
}

vec3 calcNormal(in vec3 p) {
    vec2 e = vec2(1.0, -1.0) * 0.0005;
    return normalize(
      e.xyy * sdf(p + e.xyy) +
      e.yyx * sdf(p + e.yyx) +
      e.yxy * sdf(p + e.yxy) +
      e.xxx * sdf(p + e.xxx));
}

float rayMarch(vec3 origin, vec3 direction) {
	float totalDistance = 0.0;
	for (int i = 0; i < MAX_STEP; i++)
    {
		vec3 p = origin + totalDistance * direction;
		float distance = sdf(p);
		totalDistance += distance;
		if (abs(distance) < MIN_DIST || totalDistance > MAX_DIST) break;
	}
	return totalDistance;
}

float softShadow(vec3 ro,vec3 rd, float mint, float maxt, float w )
{
    float res = 1.0;
    float t = mint;
    for( int i=0; i<256 && t<maxt; i++ )
    {
        float h = sdf(ro + t*rd);
        res = min( res, h/(w*t) );
        t += clamp(h, 0.005, 0.50);
        if( res<-1.0 || t>maxt ) break;
    }
    res = max(res,-1.0);
    return 0.25*(1.0+res)*(1.0+res)*(2.0-res);
}

Ray getRay(vec2 uv, vec3 origin, vec3 direction){
    float defocusAngle =  3.;
    float focusDistance = distance(origin, direction);
    
    float vfov = 90.; //vertical field of view
    
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

    vec3 lightPosition = vec3(5., 5., 1.0);
    vec3 color = vec3(0);
    
    
    Ray ray = getRay(uv, rayOrigin, rayDirection);
    
    float dist = rayMarch(ray.origin,ray.direction); // Ray march
    
    if(dist > MAX_DIST){
        color = vec3(0); // Couleur arrière plan
    }
    else{
        vec3 p = ray.origin + dist * ray.direction; // position objet touché
        vec3 normal = calcNormal(p);
        vec3 lightDirection = normalize(lightPosition - p);
        
        float ambiant = 0.1;
        float diffuse = max(dot(normal, lightDirection),0.)*0.5;
        float specular = pow(max(dot(reflect(-lightDirection,normal),normalize(rayOrigin-p)),0.), 50.);
    
        float light = ambiant+diffuse+specular;
        
        float ambOccl = clamp(pow(AmbientOcclusion(p,normal,0.015,20.),32.),0.1,1.);
        float sh = clamp(softShadow(p, lightDirection, 0.02, 20.5, 0.5), 0.1, 1.);
        
        color = vec3(1)*light*ambOccl*sh;
    }
    color = pow(color, vec3(1.0/2.2)); // Gamma correction
    if(moved)
        colBuff[pixelIndex] = vec4(color, 1);
    else
        colBuff[pixelIndex] += vec4(color, 1);
    
    FragCol = vec4(colBuff[pixelIndex].xyz/colBuff[pixelIndex].w,1.0); // couleur finale	
}