uniform vec2 viewPos; // the xz position that the grass is centered around.

uniform sampler2D heightmap;
uniform sampler2D normalmap;
//uniform vec3 scale; // heightmap scaling
uniform vec2 hmapDims;
uniform vec2 invHmapDimsScale; // 1.0 / (heightmap dimensions * scale)
uniform vec2 invNmapDimsScale; // 1.0 / (normalmap dimensions * scale)
// We have to use both normalmap dimensions and heightmap dimensions
// as the textures often have different sizes.
uniform vec2 hmapOffset; // The offset of the heightmap in xz.

uniform float gridDim;
uniform float invGridDim;

uniform float time;

varying vec2 texCoord;
varying vec3 normal;

void main() {
    texCoord = gl_MultiTexCoord0.xy;

    vec3 vertex = gl_Vertex.xyz;
    vec3 center = gl_Normal.xyz;

    vec2 n = ceil((viewPos - center.xz) * invGridDim - 0.5);
    vec2 translate = gridDim * n;
    vertex.xz += translate;
    center.xz += translate;

    vec2 mapCoord = (vertex.xz + 1.0) * invHmapDimsScale;
    vec2 centerCoord = (center.xz + 1.0) * invHmapDimsScale;
    vec2 normalmapCoord = (center.xz+0.5) * invNmapDimsScale;
    // The normal map is translated, i'd find out why, but it's going
    // to be replaced by clipmaps anyways, so just swizzle them.
    normalmapCoord = normalmapCoord.yx;

    float height = texture2D(heightmap, mapCoord).x;
    float centerHeight = texture2D(heightmap, centerCoord).x;
    normal = texture2D(normalmap, normalmapCoord).xyz;

    center.y += centerHeight;
    center.xz += hmapOffset;

    // Discard the foliage if the slope is too steep
    if (normal.y < 0.6 || center.y < 8.0 || 60.0 < center.y){
        gl_Position = vec4(0.0,0.0,0.0,0.0);
    }else {
        vertex.y += height;
        vertex.xz += hmapOffset;
        
        // Let the grass wave
        if (texCoord.y > 0.9){
            vec2 wave = vec2(cos(time + center.xz * 1000.0));
            wave *= 0.5;
            vertex.xz += wave;
        }
        gl_Position = gl_ModelViewProjectionMatrix * vec4(vertex, 1.0);
    }

}
