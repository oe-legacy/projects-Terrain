uniform vec2 viewPos; // the xz position of the camera
uniform vec2 patchCenter; // the xz position that the grass is centered around.

uniform sampler2D heightmap;
uniform sampler2D normalmap;
uniform vec2 invHmapDimsScale; // 1.0 / (heightmap dimensions * scale)
uniform vec2 hmapOffset; // The offset of the heightmap in xz.

uniform float gridDim;
uniform float invGridDim;

uniform float time;

uniform vec3 lightDir; // Should be pre-normalized. Or else the world will BURN IN RIGHTEOUS FIRE!!

varying vec2 texCoord;
varying float diffuse;

void main() {
    texCoord = gl_MultiTexCoord0.xy;

    vec3 vertex = gl_Vertex.xyz;
    vec3 center = gl_Normal.xyz;

    // Translate the vertex and center to the camera
    vec2 n = ceil((patchCenter - center.xz) * invGridDim - 0.5);
    vec2 translate = gridDim * n;
    vertex.xz += translate;
    center.xz += translate;

    // Offset by half the heightmaps gridcell width, which is 1.0
    vec2 mapCoord = (vertex.xz + 1.0) * invHmapDimsScale;
    vec2 centerCoord = (center.xz + 1.0) * invHmapDimsScale;

    float centerHeight = texture2DLod(heightmap, centerCoord, 1.0).x;
    // The normal map is transposed, i'd find out why, but it's going
    // to be replaced by clipmaps anyways, so just swizzle the tex coords.
    vec3 normal = texture2DLod(normalmap, mapCoord.yx, 1.0).xyz;

    center.y += centerHeight;

    // Discard the foliage if the slope is too steep
    if (normal.y < 0.7 || center.y < 8.0 || 60.0 < center.y){
        gl_Position = vec4(0.0,0.0,0.0,0.0);
    }else {
        // Let the grass "slide" into the ground if far away.
        float centerFactor = (viewPos.x - center.x) * invGridDim * 2.0;
        centerFactor *= (viewPos.y - center.z) * invGridDim * 2.0;
        float viewFactor = (patchCenter.x - center.x) * invGridDim * 2.0;
        viewFactor *= (patchCenter.y - center.z) * invGridDim * 2.0;
        float fadeFactor = abs(centerFactor) + abs(viewFactor);
        fadeFactor = clamp(fadeFactor, 0.0, 1.0);
        fadeFactor *= fadeFactor;
        vertex.y *= (1.0 - fadeFactor);

        float height = texture2DLod(heightmap, mapCoord, 1.0).x+0.25;
        vertex.y += height;
        vertex.xz += hmapOffset;
        
        // Let the grass wave
        vec2 wave = vec2(cos(time + center.xz * 1000.0));
        vertex.xz += 0.5 * texCoord.y * wave;

        diffuse = clamp(dot(normal, lightDir), 0.0, 1.0);
        // Simulate 40% light passing through the grass
        diffuse = clamp(diffuse * 1.4, 0.0, 1.0);

        gl_Position = gl_ModelViewProjectionMatrix * vec4(vertex, 1.0);
    }

}
