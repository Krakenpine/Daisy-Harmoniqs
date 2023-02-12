#ifndef HARMONIQS_ENGINE_H_
#define HARMONIQS_ENGINE_H_

#include <cmath>
#include "daisysp.h"
#include "daisy_pod.h"

using namespace daisysp;
using namespace daisy;

#define POLYPHONY 10
#define AVERAGE_WINDOW_LENGTH 1024
#define WAVETABLE_LENGTH 1024
#define WAVETABLE_LENGTH_HALF 512
#define WAVETABLE_LENGTH_HALF_MINUS_1 511


struct adsrValueStruct {
    float attack;
    float decay;
    float sustain;
    float release;
};

struct harmVoice {
    Adsr envelopeVol;
    Adsr envelope;
    Oscillator oscillator;
    Decimator decim;
    float harmonics[8];
    float oscFreq;
    float amp;
    float generalEnvelopeValue;
    int age;
    uint8_t note;
    bool on;
    float internalGain;
    bool isPlaying;
    int lastUpdated;
};

class HarmoniqsEngine
{
    public:
        HarmoniqsEngine() { }
        ~HarmoniqsEngine() { }

        void Init(float sample_rate);
        void UpdateHarmonicValues(int i);
        float Process();
        void SetAttack(float attack);
        void SetDecay(float decay);
        void SetSustain(float sustain);
        void SetRelease(float release);
        void SetADSR(float attack, float decay, float sustain, float release);
        void SetAttack(float attack, int voiceIndex);
        void SetDecay(float decay, int voiceIndex);
        void SetSustain(float sustain, int voiceIndex);
        void SetRelease(float release, int voiceIndex);
        void SetADSR(float attack, float decay, float sustain, float release, int voiceIndex);

        bool envToVol;
        bool envToOddEven;
        bool envToLowHigh;
        bool envToHarmLevel;
        bool envToBitcrush;

        float pitchCV;
        float harmOddEvenCV;
        float harmLevelCV;
        float harmLowHighCV;
        float bitcrushCV;

        Oscillator lfo;

        MoogLadder endFilter;

        float distortionCV;
        float filterFreqCV;
        float filterRezCV;
        float inharmonism;

        bool bitcrusherIsSmooth;

        float pitchBend;

        float modwheel;

        int age;

        float lfoSpeed;

        float lfoToVol;
        float lfoToHarmLevel;
        float lfoToOddEven;
        float lfoToHarmLowHigh;
        float lfoToBitcrush;

        int encoderLfoSelection;

        bool lfoToVolSelection;
        bool lfoToHarmLevelSelection;
        bool lfoToOddEvenSelection;
        bool lfoToHarmLowHighSelection;
        bool lfoToBitcrushSelection;

        bool modwheelToVol;
        bool modwheelToHarmLevel;
        bool modwheelToOddEven;
        bool modwheelToHarmLowHigh;
        bool modwheelToBitcrush;

        float lfoCurrentValue;

        float sineWavetable[WAVETABLE_LENGTH];

        adsrValueStruct adsrValues;

        Adsr combinedEnvelope;

        bool combinedEnvelopeOn = false;

        bool adjustEnvelopeToFilter = false;

        float envelopeToFilterAmount = 0.0f;

        harmVoice voices[POLYPHONY];

        int harmonicsTestRound = 0;


        const float hlhLimits[7] = { 0.00f, 0.05f, 0.20f, 0.35f, 0.50f, 0.75f, 0.90f };

        float avrg_sum_pre;
        float avrg_sum_post;

        float avrg_samples_pre[AVERAGE_WINDOW_LENGTH] = { 0.0f };
        float avrg_samples_post[AVERAGE_WINDOW_LENGTH] = { 0.0f };

        int avrg_cursor;

        float maximumOutputValue;

        int optCounter;

        float inharm_max;

        int updateCounter;
};

#endif
