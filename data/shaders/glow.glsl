# Glow shader resource.

# Vertext shader program.
vert: shaders/default.vert

# Fragment shader program.
frag: shaders/glow.frag

# Uniform values

# The coefficients for the original image and the blurred image
unif: coefficients = 0.75 0.3
#unif: coefficients = 0.0 1.0
unif: halfSamples = 5
unif: offset = 0.003
