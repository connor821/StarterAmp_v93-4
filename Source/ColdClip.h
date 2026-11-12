#pragma once
// b68 (decision 4): the SLO-100's 'cold clipper' between gain stages 2 and 3 (V2B, 39k unbypassed cathode, 100k plate load).
// The cathode resistor feeds back almost all of the signal (small-signal gain about 2.2), so the stage is nearly linear for the half of the
// swing that drives the grid up, and cuts off hard on the other half: a one-sided, level-dependent clip. The curve is
// c * tanh (u / c) on the cutoff side and u on the other, in the engine's polarity (positive = toward cutoff), c = L. First-order anti-aliased.
// amt (0..1) blends it in; a negative L flips the polarity. Run at the oversampled rate, on the amp channel only.
#include <cmath>

struct ColdClip
{
    float amt = 0.0f, L = 1.0f, uPrev = 0.0f;

    void process (float* y, int m)
    {
        const float c = std::abs (L), a = amt, pol = L < 0.0f ? -1.0f : 1.0f;
        if (a <= 0.0f || c < 1.0e-6f) return;
        auto F = [c] (float u) { if (u <= 0.0f) return 0.5f * u * u;  const float t = u / c;  return c * c * (t + std::log1p (std::exp (-2.0f * t)) - 0.69314718f); };      // antiderivative of f
        auto f = [c] (float u) { return u <= 0.0f ? u : c * std::tanh (u / c); };
        float up = uPrev;
        for (int i = 0; i < m; ++i)
        {
            const float u = pol * y[i];
            const float du = u - up;
            float o;
            if (std::abs (du) > 1.0e-4f * c) o = (F (u) - F (up)) / du;
            else                             o = f (0.5f * (u + up));
            up = u;
            y[i] = pol * (u + a * (o - u));
        }
        uPrev = up;
    }
    void reset() { uPrev = 0.0f; }
};
