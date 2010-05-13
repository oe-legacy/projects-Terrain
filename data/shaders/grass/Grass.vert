uniform vec2 viewPos; // the xz position that the grass is centered around.

uniform sampler2D heightmap;
uniform sampler2D normalmap;
uniform vec3 scale; // heightmap scaling
uniform vec2 hmapDims;
uniform vec2 nmapDims;
uniform vec3 hmapOffset; // The offset of the heightmap, mostly used to place it under the water.
uniform float gridDim;
uniform float invGridDim;

uniform float time;

varying vec2 texCoord;
varying vec3 normal;

void main() {
    texCoord = gl_MultiTexCoord0.xy;

    vec3 vertex = gl_Vertex.xyz;
    vec3 center = gl_Normal.xyz;

    // division can be precomputed? Would require 2 center coords
    vec2 n = floor((viewPos - center.xz) * invGridDim - 0.5) + 1.0;
    vec2 translate = gridDim * n;

    vertex.xz += translate;
    center.xz += translate;

    vec2 mapCoord = (vertex.xz + 1.0)  / (hmapDims * scale.xz);
    vec2 centerCoord = (center.xz + 1.0)  / (hmapDims * scale.xz);
    vec2 normalmapCoord = (center.xz+0.5)  / (nmapDims * scale.xz);
    normalmapCoord = normalmapCoord.yx;

    float height = texture2D(heightmap, mapCoord).x * scale.y;
    float centerHeight = texture2D(heightmap, centerCoord).x * scale.y;
    normal = texture2D(normalmap, normalmapCoord).xyz;

    center.y += centerHeight;
    center += hmapOffset;

    // Discard the foliage if the slope is too steep
    if (normal.y < 0.6 || center.y < 8.0 || 60.0 < center.y){
        gl_Position = vec4(0.0,0.0,0.0,0.0);
    }else {
        vertex.y += height;
        vertex += hmapOffset;
        
        // Let the grass wave
        if (texCoord.y > 0.9){
            vec2 wave = vec2(cos(time / 1000.0 + center.xz * 1000.0));
            wave *= 0.5;
            vertex.xz += wave;
        }
        gl_Position = gl_ModelViewProjectionMatrix * vec4(vertex, 1.0);
    }

}
