# Terrain shader resource.

# Vertext shader program.
vert: shaders/default.vert

# Fragment shader program.
frag: shaders/glow.frag

# Uniform values

# The coefficients for the original image and the blurred image
unif: coefficients = 0.6 0.6
unif: halfSamples = 5.0
unif: offset = 0.003
