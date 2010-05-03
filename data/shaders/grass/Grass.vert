const vec2 HEIGHTMAP_DIMS = vec2(400.0, 400.0);

uniform vec3 viewPos;

uniform sampler2D heightmap;
uniform vec3 scale; // heightmap scaling

varying vec2 texCoord;

void main() {
    texCoord = gl_MultiTexCoord0.xy;
    vec4 vertex = gl_Vertex + vec4(viewPos.x + 100.0, 0.0, viewPos.z + 100.0, 0.0);

    vec2 mapCoord = vertex.xz * scale.xz / HEIGHTMAP_DIMS;
    float height = texture2D(heightmap, mapCoord).x * scale.y;

    vertex.y += height;

    gl_Position = gl_ModelViewProjectionMatrix * vertex;
}
