uniform sampler2D grassTex;

uniform vec3 lightDir; // Should be pre-normalized. Or else the world will BURN IN RIGHTEOUS FIRE!!

varying vec2 texCoord;
varying vec3 normal;

void main(){
    vec4 grass = texture2D(grassTex, texCoord);

    // if it's the transparent part of the grass texture then discard
    // it. This way no z-sorting is needed.
    if (grass.a < 0.8)
        discard;

    // Calculate diffuse
    float ndotl = dot(normal, lightDir);
    float diffuse = clamp(ndotl, 0.0, 1.0);

    grass = grass * (gl_LightSource[0].ambient + gl_LightSource[0].diffuse * diffuse);

    gl_FragColor = grass;
}
