uniform vec3 sunDirection;
varying float sunAngle;

void main(void) {
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    gl_TexCoord[0].xyz = gl_MultiTexCoord0.xyz;

	float cosine = dot (-sunDirection, gl_Normal);
	sunAngle = -cosine;
}
