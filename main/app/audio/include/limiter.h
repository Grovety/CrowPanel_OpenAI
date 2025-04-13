/* Audio limiter header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _LIMITER_H_
#define _LIMITER_H_

typedef struct {
    float threshold;    // linear amplitude threshold, e.g., 0.5
    float attackCoeff;  // how fast to reduce gain when above threshold
    float releaseCoeff; // how fast to restore gain when below threshold
    float currentGain;  // smoothed gain value
} LimiterState;

void initLimiter(LimiterState *st, float thresholdDb, float attackMs,
                 float releaseMs, float sampleRate);
void processBuffer(LimiterState *st, float *buffer, int numSamples);

#endif
