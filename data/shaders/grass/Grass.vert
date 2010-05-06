const vec2 HEIGHTMAP_DIMS = vec2(400.0, 400.0);
const vec3 offset = vec3(0.0, -10.75, 0.0);

uniform vec3 viewPos;

uniform sampler2D heightmap;
uniform sampler2D normalmap;
uniform vec3 scale; // heightmap scaling

uniform float time;

varying vec2 texCoord;
varying vec3 normal;

void main() {
    texCoord = gl_MultiTexCoord0.xy;
    //vec2 noise = gl_MultiTexCoord1.xy;
    
    vec3 vertex = gl_Vertex.xyz + vec3(viewPos.x, 0.0, viewPos.z);
    vec3 center = gl_Normal.xyz + vec3(viewPos.x, 0.0, viewPos.z);
    vertex = gl_Vertex.xyz + vec3(90.0, 0.0, 90.0);
    center = gl_Normal.xyz + vec3(90.0, 0.0, 90.0);

    vec2 mapCoord = (vertex.xz + 1.0)  / (HEIGHTMAP_DIMS * scale.xz);
    vec2 centerCoord = (center.xz + 1.0)  / (HEIGHTMAP_DIMS * scale.xz);

    float height = texture2D(heightmap, mapCoord).x * scale.y;
    float centerHeight = texture2D(heightmap, centerCoord).x * scale.y;
    normal = texture2D(normalmap, 1.0 - centerCoord).xyz;

    vertex.y += height;
    vertex += offset;
    
    center.y += centerHeight;
    center += offset;

    // Discard the foliage if the slope is too steep
    if (/*normal.y < 0.8 || */center.y < 6.0){
        vertex.xyz = center;
    }

    // Let the grass wave
    if (texCoord.y > 0.9){
        vec2 wave = vec2(cos(time / 1000.0 + center.x), sin(time / 1000.0 + center.z));
        wave *= 0.5;
        vertex.xz += wave;
    }

    gl_Position = gl_ModelViewProjectionMatrix * vec4(vertex, 1.0);
}
