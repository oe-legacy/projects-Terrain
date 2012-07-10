uniform sampler2D color0;
uniform sampler2DShadow depth;

uniform mat4 viewProjectionInverse;

uniform sampler3D src;

varying vec2 texCoord;

const vec4 center = vec4(.0);
const float radius = 100.0;

const vec3 boxMin = vec3(0.0);
const vec3 boxMax = vec3(500.0);

const float stepSize = 5.0;

uniform float time;

bool repeat = false;

bool RayBoxIntersection(in vec3 origin, in vec3 dir,
                        in vec3 aabbMin, in vec3 aabbMax,
                        out float nearAlpha, out float farAlpha){

    if (repeat){
        nearAlpha -0.0;
        farAlpha = 100000000.0;
        return true;
    }else{
        vec3 minInters = (aabbMin - origin) / dir;
        vec3 maxInters = (aabbMax - origin) / dir;
        
        vec3 minAlphas = min(minInters, maxInters);
        vec3 maxAlphas = max(minInters, maxInters);
        
        nearAlpha = max(0.0, max(minAlphas.x, max(minAlphas.y, minAlphas.z)));
        farAlpha = min(maxAlphas.x, min(maxAlphas.y, maxAlphas.z));
        
        return nearAlpha < farAlpha && 0. < farAlpha;
    }
}

void main () {
    vec4 hat = texture2D(color0, texCoord);

    // Get the depth buffer value at this pixel.  
    float zOverW = shadow2D(depth, vec3(texCoord, 0.0)).x;
    // screenPos is the viewport position at this pixel in the range -1 to 1.  
    vec4 screenPos = vec4(texCoord.x * 2.0 - 1.0, 
                          texCoord.y * 2.0 - 1.0,  
                          zOverW * 2.0 - 1.0, 1.0);

    // Transform by the view-projection inverse.  
    //vec4 currentPos = gl_ModelViewProjectionMatrixInverse * screenPos;
    vec4 currentPos = viewProjectionInverse * screenPos;

    // Ray in world space.
    vec4 origin = gl_ModelViewMatrixInverse[3];
    vec4 worldPos = currentPos / currentPos.w;
    vec4 ray = normalize(worldPos - origin);

    float near, far;
    vec4 color = vec4(.0);
    if (RayBoxIntersection(vec3(origin), vec3(ray), boxMin, boxMax, near, far)){
        
        far = min((worldPos.x - origin.x) / ray.x, far);
        
        for (float t = near; t < far; t+= stepSize){
            vec4 pos = origin + ray * t;
            
            pos.z -= time/20.0;

            vec4 sample = texture3D(src, vec3(pos) / 500.0) / 3.0;
            sample.rgb = vec3(0.1);

            //color = (1.0 - sample.a) * color + sample.a * sample;
            float scl = min(1.0 - color.a, sample.a);
            color += scl * sample;
            if (color.a >= 1.0)
                break;
        }
        
    }
    gl_FragColor = mix(hat, color, color.a);
    gl_FragDepth = zOverW;
}
