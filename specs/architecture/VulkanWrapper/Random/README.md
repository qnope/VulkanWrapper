# Random Module

`vw` namespace · `Random/` directory · Tier 5

GPU-side random number generation for stochastic rendering effects.

## NoiseTexture

GPU texture filled with random values. Used as seed for per-pixel random sampling in shaders.

## RandomSamplingBuffer

Buffer of precomputed random sampling patterns. Provides stratified or uniform random samples for GPU shaders.

Used by AmbientOcclusionPass and IndirectLightPass for stochastic sampling (progressive accumulation of 1 sample per pixel per frame).
