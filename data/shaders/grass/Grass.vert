const vec2 gridDim = vec2(128.0, 128.0);
//const vec2 gridDim = vec2(256.0, 256.0);

uniform vec3 viewPos;

uniform sampler2D heightmap;
uniform sampler2D normalmap;
uniform vec3 scale; // heightmap scaling
uniform vec2 heightmapDims;
uniform vec2 normalmapDims;
uniform vec3 offset;

uniform float time;

varying vec2 texCoord;
varying vec3 normal;

void main() {
    texCoord = gl_MultiTexCoord0.xy;

    vec3 vertex = gl_Vertex.xyz;
    vec3 center = gl_Normal.xyz;

    // division can be precomputed? Would require 2 center coords
    vec2 n = floor((viewPos.xz - center.xz) / gridDim - 0.5) + 1.0;
    vec2 translate = gridDim * n;

    vertex.xz += translate;
    center.xz += translate;

    vec2 mapCoord = (vertex.xz + 1.0)  / (heightmapDims * scale.xz);
    vec2 centerCoord = (center.xz + 1.0)  / (heightmapDims * scale.xz);
    vec2 normalmapCoord = (center.xz+0.5)  / (normalmapDims * scale.xz);
    normalmapCoord = normalmapCoord.yx;

    float height = texture2D(heightmap, mapCoord).x * scale.y;
    float centerHeight = texture2D(heightmap, centerCoord).x * scale.y;
    normal = texture2D(normalmap, normalmapCoord).xyz;

    vertex.y += height;
    vertex += offset;
    
    center.y += centerHeight;
    center += offset;

    // Discard the foliage if the slope is too steep
    if (normal.y < 0.6 || center.y < 8.0 || 60.0 < center.y){
        vertex.xyz = center;
    }else if (texCoord.y > 0.9){
        // Let the grass wave
        vec2 wave = vec2(cos(time / 1000.0 + center));
        wave *= 0.5;
        vertex.xz += wave;
    }

    gl_Position = gl_ModelViewProjectionMatrix * vec4(vertex, 1.0);
}
