// shaders/tonemap.frag
// Tone-mapping and gamma-correction pass.
//
// Reads the HDR floating-point accumulation texture produced by path_trace.frag,
// applies Reinhard tone mapping to compress unbounded radiance values into [0, 1],
// then applies gamma correction to convert from linear light to the sRGB perceptual
// encoding expected by the monitor.
//
// This runs as a second fullscreen-quad pass on every frame, reading from the
// write buffer that path_trace.frag just finished writing into.

#version 330 core

// ─── Inputs / outputs ─────────────────────────────────────────────────────────
in  vec2 fragmentTexCoord;
out vec4 outputColor;

// ─── Uniforms ─────────────────────────────────────────────────────────────────
uniform sampler2D uniformAccumTexture;  // HDR accumulation result from path_trace.frag.

// ─── Constants ────────────────────────────────────────────────────────────────
const float DISPLAY_GAMMA = 2.2;  // Standard sRGB display gamma.

void main()
{
    // ── Sample the HDR accumulation buffer ────────────────────────────────
    vec3 hdrRadiance = texture(uniformAccumTexture, fragmentTexCoord).rgb;

    // ── Reinhard tone mapping ──────────────────────────────────────────────
    // Maps [0, ∞) → [0, 1) smoothly.  Bright highlights compress gracefully
    // rather than clipping.  Formula: L_display = L_hdr / (1 + L_hdr).
    vec3 tonemappedColor = hdrRadiance / (hdrRadiance + vec3(1.0));

    // ── Gamma correction (linear → sRGB) ──────────────────────────────────
    // GL framebuffers store perceptual (gamma-encoded) values.  Monitors apply
    // the inverse power (≈ 2.2) to recover linear light, so we pre-apply 1/2.2.
    vec3 gammaCorrectedColor = pow(tonemappedColor, vec3(1.0 / DISPLAY_GAMMA));

    outputColor = vec4(gammaCorrectedColor, 1.0);
}
