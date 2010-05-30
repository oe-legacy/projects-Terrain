varying vec2 texCoord;

void main(void)
{
    texCoord = gl_Vertex.xy * 0.5 + 0.5;

    gl_Position = gl_Vertex;
}
