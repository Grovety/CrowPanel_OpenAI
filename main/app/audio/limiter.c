#include "limiter.h"
#include "math.h"

// Initialize the limiter state
void initLimiter(LimiterState *st, float thresholdDb, float attackMs,
                 float releaseMs, float sampleRate) {
    st->threshold = powf(10.0f, thresholdDb / 20.0f); // convert dB to linear
    float attackSec = attackMs / 1000.0f;
    float releaseSec = releaseMs / 1000.0f;
    // compute coefficients based on sample rate
    st->attackCoeff = expf(-1.0f / (attackSec * sampleRate));
    st->releaseCoeff = expf(-1.0f / (releaseSec * sampleRate));
    st->currentGain = 1.0f; // start at unity gain
}

// Process a single sample
float processSample(LimiterState *st, float input) {
    // absolute value for peak detection
    float absIn = fabsf(input);

    // if input is above threshold/gain, we reduce the currentGain
    float desiredGain = (absIn > (st->threshold / st->currentGain))
                            ? (st->threshold / absIn)
                            : 1.0f;

    // choose attack or release coefficient
    float coeff =
        (desiredGain < st->currentGain) ? st->attackCoeff : st->releaseCoeff;

    // smooth the gain
    st->currentGain =
        (coeff * st->currentGain) + ((1.0f - coeff) * desiredGain);

    // apply the gain
    return input * st->currentGain;
}

// Process an entire buffer
void processBuffer(LimiterState *st, float *buffer, int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        buffer[i] = processSample(st, buffer[i]);
    }
}
