uniform sampler2D grassTex;

varying vec2 texCoord;
varying float diffuse;

void main(){
    vec4 grass = texture2D(grassTex, texCoord);

    // if it's the transparent part of the grass texture then discard
    // it. This way no z-sorting is needed.
    if (grass.a < 0.6)
        discard;

    grass *= (gl_LightSource[0].ambient + gl_LightSource[0].diffuse * diffuse);

    gl_FragColor = grass;
}
