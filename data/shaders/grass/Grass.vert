uniform vec2 viewPos; // the xz position that the grass is centered around.

uniform sampler2D heightmap;
uniform sampler2D normalmap;
uniform vec2 invHmapDimsScale; // 1.0 / (heightmap dimensions * scale)
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

    float height = texture2D(heightmap, mapCoord).x;
    float centerHeight = texture2D(heightmap, centerCoord).x;
    // The normal map is translated, i'd find out why, but it's going
    // to be replaced by clipmaps anyways, so just swizzle them.
    normal = texture2D(normalmap, mapCoord.yx).xyz;

    center.y += centerHeight;
    center.xz += hmapOffset;

    // Discard the foliage if the slope is too steep
    if (normal.y < 0.6 || center.y < 8.0 || 60.0 < center.y){
        gl_Position = vec4(0.0,0.0,0.0,0.0);
    }else {
        vertex.y += height;
        vertex.xz += hmapOffset;
        
        // Let the grass wave
        vec2 wave = vec2(cos(time + center.xz * 1000.0));
        vertex.xz += 0.5 * texCoord.y * wave;
        gl_Position = gl_ModelViewProjectionMatrix * vec4(vertex, 1.0);
    }

}
