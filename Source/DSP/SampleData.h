#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

namespace bdsp
{
    // A mono one-shot (e.g. an 808) loaded by the user. Immutable once
    // handed to the audio thread.
    struct SampleData
    {
        std::vector<float> samples;
        double sampleRate = 44100.0;
        std::vector<float> peaks;          // max |x| per display column, for the GUI
        float detectedNote = -1.0f;        // fractional MIDI note, -1 if unknown

        int length() const noexcept { return (int) samples.size(); }

        // 4-point Hermite interpolation; returns 0 outside the sample.
        inline float read (double pos) const noexcept
        {
            const int n = length();
            const int i = (int) pos;
            if (i < 0 || i >= n) return 0.0f;
            const float f = (float) (pos - i);
            auto at = [&] (int k) { return (k >= 0 && k < n) ? samples[(size_t) k] : 0.0f; };
            const float xm1 = at (i - 1), x0 = at (i), x1 = at (i + 1), x2 = at (i + 2);
            const float c1 = 0.5f * (x1 - xm1);
            const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
            const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
            return ((c3 * f + c2) * f + c1) * f + x0;
        }

        void computePeaks (int columns = 300)
        {
            peaks.assign ((size_t) columns, 0.0f);
            const int n = length();
            if (n == 0) return;
            for (int i = 0; i < n; ++i)
            {
                auto& p = peaks[(size_t) std::min (columns - 1, (int) ((long long) i * columns / n))];
                p = std::max (p, std::abs (samples[(size_t) i]));
            }
        }

        // Pitch detection (McLeod normalised square difference) on a window
        // after the attack, where an 808 has settled on its fundamental.
        void detectPitch()
        {
            detectedNote = -1.0f;
            const int n = length();
            const int window = (int) (0.15 * sampleRate);
            int start = (int) (0.12 * sampleRate);
            if (start + window * 2 > n)
                start = std::max (0, n / 2 - window);
            if (start + window * 2 > n)
                return;

            const int minLag = std::max (2, (int) (sampleRate / 600.0));
            const int maxLag = std::min (window - 1, (int) (sampleRate / 25.0));
            const float* x = samples.data() + start;

            std::vector<double> nsdf ((size_t) maxLag + 2, 0.0);
            for (int lag = minLag; lag <= maxLag + 1; ++lag)
            {
                double acf = 0.0, m = 0.0;
                for (int i = 0; i < window; ++i)
                {
                    acf += (double) x[i] * x[i + lag];
                    m   += (double) x[i] * x[i] + (double) x[i + lag] * x[i + lag];
                }
                nsdf[(size_t) lag] = m > 1.0e-12 ? 2.0 * acf / m : 0.0;
            }

            // key maxima between positive-going and negative-going zero crossings
            double best = 0.0;
            std::vector<std::pair<int, double>> peaksFound;
            for (int lag = minLag + 1; lag <= maxLag; ++lag)
            {
                const double a = nsdf[(size_t) lag - 1], b = nsdf[(size_t) lag], c = nsdf[(size_t) lag + 1];
                if (b > 0.0 && b >= a && b > c)
                {
                    peaksFound.push_back ({ lag, b });
                    best = std::max (best, b);
                }
            }
            if (best < 0.5)
                return;

            for (auto [lag, v] : peaksFound)
            {
                if (v >= 0.9 * best)
                {
                    // parabolic interpolation of the peak
                    const double a = nsdf[(size_t) lag - 1], b = nsdf[(size_t) lag], c = nsdf[(size_t) lag + 1];
                    const double denom = a - 2.0 * b + c;
                    const double shift = std::abs (denom) > 1.0e-12 ? 0.5 * (a - c) / denom : 0.0;
                    const double period = lag + shift;
                    const double freq = sampleRate / period;
                    detectedNote = (float) (69.0 + 12.0 * std::log2 (freq / 440.0));
                    return;
                }
            }
        }
    };
}
