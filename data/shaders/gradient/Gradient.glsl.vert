uniform vec3 sunDirection;

varying float sunAngle;
varying vec3 normal;

void main(void) {
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    gl_TexCoord[0].xyz = gl_MultiTexCoord0.xyz;
    
    normal = gl_Normal;

	float cosine = dot (-sunDirection, gl_Normal);
	sunAngle = -cosine;
}
