# Glow shader resource.

# Vertext shader program.
vert: shaders/default.vert

# Fragment shader program.
frag: shaders/glow.frag

# Uniform values

# The coefficients for the original image and the blurred image
unif: coefficients = 0.7 0.3
unif: halfSamples = 5
unif: samples = 11 # halfSamples * 2 + 1
unif: offset = 0.003
