#include "Harmoniqs-engine.h"

void HarmoniqsEngine::Init(float sample_rate)
{
    // Generate sinewavetable
    for (int i = 0; i < WAVETABLE_LENGTH; i++) {
        sineWavetable[i] = sinf(TWOPI_F * ((i * 1.f) / (float)(WAVETABLE_LENGTH)) );
    }

    for (int i = 0; i < POLYPHONY; i++) {
        voices[i].oscFreq = 100.f;
        voices[i].amp = 1.f;
        voices[i].on = false;

        voices[i].generalEnvelopeValue = 1.f;

        voices[i].envelopeVol.Init(sample_rate);
        voices[i].envelopeVol.SetAttackTime(adsrValues.attack);
        voices[i].envelopeVol.SetDecayTime(adsrValues.decay);
        voices[i].envelopeVol.SetSustainLevel(adsrValues.sustain);
        voices[i].envelopeVol.SetReleaseTime(adsrValues.release);

        voices[i].envelope.Init(sample_rate);
        voices[i].envelope.SetAttackTime(adsrValues.attack);
        voices[i].envelope.SetDecayTime(adsrValues.decay);
        voices[i].envelope.SetSustainLevel(adsrValues.sustain);
        voices[i].envelope.SetReleaseTime(adsrValues.release);

        voices[i].oscillator.Init(sample_rate);
        voices[i].oscillator.SetWaveform(voices[i].oscillator.WAVE_RAMP);
        voices[i].oscillator.SetFreq(voices[i].oscFreq);
        voices[i].oscillator.SetAmp(1.0f);

        voices[i].decim.Init();
        voices[i].decim.SetSmoothCrushing(true);
        voices[i].decim.SetDownsampleFactor(0.f);

        for (int j = 0; j < 8; j++) {
            voices[i].harmonics[j] = 1.f;
        }

        voices[i].internalGain = 1.f;
        voices[i].lastUpdated = i;
    }

    combinedEnvelope.Init(sample_rate);
    combinedEnvelope.SetAttackTime(adsrValues.attack);
    combinedEnvelope.SetDecayTime(adsrValues.decay);
    combinedEnvelope.SetSustainLevel(adsrValues.sustain);
    combinedEnvelope.SetReleaseTime(adsrValues.release);
    
    lfo.Init(sample_rate);
    lfo.SetWaveform(lfo.WAVE_TRI);
    lfo.SetFreq(1.f),
    lfo.SetAmp(1.f);
    
    endFilter.Init(sample_rate);
    endFilter.SetFreq(10000);
    endFilter.SetRes(0.f);
 
    envToVol = false;
    envToOddEven = false;
    envToLowHigh = false;
    envToHarmLevel = false;
    envToBitcrush = false;

    pitchCV = 0.f;
    harmOddEvenCV = 0.f;
    harmLevelCV = 0.f;
    harmLowHighCV = 0.f;
    bitcrushCV = 0.f;

    distortionCV = 0.0f;
    filterFreqCV = 1.0f;
    filterRezCV = 0.0f;
    inharmonism = 0.0f;

    bitcrusherIsSmooth = true;

    pitchBend = 1.f;

    modwheel = 0.f;

    age = 1;

    lfoSpeed = 1.f;

    lfoToVol = 0.f;
    lfoToHarmLevel = 0.f;
    lfoToOddEven = 0.f;
    lfoToHarmLowHigh = 0.f;
    lfoToBitcrush = 0.f;

    encoderLfoSelection = 0;

    lfoToVolSelection = false;
    lfoToHarmLevelSelection = false;
    lfoToOddEvenSelection = false;
    lfoToHarmLowHighSelection = false;
    lfoToBitcrushSelection = false;

    modwheelToVol = false;
    modwheelToHarmLevel = false;
    modwheelToOddEven = false;
    modwheelToHarmLowHigh = false;
    modwheelToBitcrush = false;

    lfoCurrentValue = 0.0f;

    avrg_sum_pre = 0.0f;
    avrg_sum_post = 0.0f;

    avrg_cursor = 0;

    maximumOutputValue = 0.0f;

    optCounter = 0;

    inharm_max = 2.f / 3.f;

    updateCounter = 0;
}

void HarmoniqsEngine::UpdateHarmonicValues(int i)
{

        float harmLowHighCV_env = harmLowHighCV * (envToLowHigh ? voices[i].generalEnvelopeValue : 1.0f);
        harmLowHighCV_env = ((1.0f - lfoToHarmLowHigh) * (harmLowHighCV_env) + lfoToHarmLowHigh * lfoCurrentValue * harmLowHighCV_env);
        if (modwheelToHarmLowHigh) {
            harmLowHighCV_env *= modwheel;
        }
        voices[i].harmonics[0] = 1.0f;
        if (harmLowHighCV_env < hlhLimits[1]) {
            voices[i].harmonics[1] = harmLowHighCV_env * (2.0f / hlhLimits[2]) + 0.5f;
            voices[i].harmonics[2] = harmLowHighCV_env * (1.0f / hlhLimits[2]) + 0.25f;
            voices[i].harmonics[3] = harmLowHighCV_env * (1.50f / hlhLimits[2]);
            voices[i].harmonics[4] = 0.0f;
            voices[i].harmonics[5] = 0.0f;
            voices[i].harmonics[6] = 0.0f;
            voices[i].harmonics[7] = 0.0f;
        } else if (harmLowHighCV_env < hlhLimits[2]) {
            float multiplier = 1.0f / (hlhLimits[3] - hlhLimits[2]);
            voices[i].harmonics[1] = 1.0f;
            voices[i].harmonics[2] = 0.666f;
            voices[i].harmonics[3] = 0.333f;
            voices[i].harmonics[4] = 0.0f;
            voices[i].harmonics[5] = 0.333f * (harmLowHighCV_env - hlhLimits[1]) * multiplier;
            voices[i].harmonics[6] = 0.666f * (harmLowHighCV_env - hlhLimits[1]) * multiplier;
            voices[i].harmonics[7] = (harmLowHighCV_env - hlhLimits[1]) * multiplier;
        } else if (harmLowHighCV_env < hlhLimits[3]) {
            float multiplier = 1.0f / (hlhLimits[3] - hlhLimits[2]);
            voices[i].harmonics[1] = 1.0f;
            voices[i].harmonics[2] = 0.666f + 0.333f * (harmLowHighCV_env - hlhLimits[2]) * multiplier;
            voices[i].harmonics[3] = 0.333f + 0.666f * (harmLowHighCV_env - hlhLimits[2]) * multiplier;
            voices[i].harmonics[4] = (harmLowHighCV_env - hlhLimits[2]) *  multiplier;
            voices[i].harmonics[5] = 0.333f + 0.333f * (harmLowHighCV_env - hlhLimits[2]) * multiplier;
            voices[i].harmonics[6] = 0.666f - 0.333f * (harmLowHighCV_env - hlhLimits[2]) * multiplier;
            voices[i].harmonics[7] = 1.0f - (harmLowHighCV_env - hlhLimits[2]) * multiplier;
        } else if (harmLowHighCV_env < hlhLimits[4]) {
            float multiplier = 1.0f / (hlhLimits[4] - hlhLimits[3]);
            voices[i].harmonics[1] = 1.0f;
            voices[i].harmonics[2] = 1.0f;
            voices[i].harmonics[3] = 1.0f;
            voices[i].harmonics[4] = 1.0f;
            voices[i].harmonics[5] = 0.666f + 0.333f * (harmLowHighCV_env - hlhLimits[3]) * multiplier;
            voices[i].harmonics[6] = 0.333f + 0.666f * (harmLowHighCV_env - hlhLimits[3]) * multiplier;
            voices[i].harmonics[7] = (harmLowHighCV_env - hlhLimits[3]) * multiplier;
        } else if (harmLowHighCV_env < hlhLimits[5]) {
            float multiplier = 1.0f / (hlhLimits[5] - hlhLimits[4]);
            voices[i].harmonics[1] = 1 - (harmLowHighCV_env - hlhLimits[4]) * multiplier;
            voices[i].harmonics[2] = 1 - 0.666f * (harmLowHighCV_env - hlhLimits[4]) * multiplier;
            voices[i].harmonics[3] = 1 - 0.333f * (harmLowHighCV_env - hlhLimits[4]) * multiplier;
            voices[i].harmonics[4] = 1.0f;
            voices[i].harmonics[5] = 1 - 0.333f * (harmLowHighCV_env - hlhLimits[4]) * multiplier;
            voices[i].harmonics[6] = 1 - 0.666f * (harmLowHighCV_env - hlhLimits[4]) * multiplier;
            voices[i].harmonics[7] = 1 - (harmLowHighCV_env - hlhLimits[4]) * multiplier;
        } else if (harmLowHighCV_env < hlhLimits[6]) {
            float multiplier = 1.0f / (hlhLimits[6] - hlhLimits[5]);
            voices[i].harmonics[1] = 0.0f;
            voices[i].harmonics[2] = 0.333f - 0.333f * (harmLowHighCV_env - hlhLimits[5]) * multiplier;
            voices[i].harmonics[3] = 0.666f - 0.666f * (harmLowHighCV_env - hlhLimits[5]) * multiplier;
            voices[i].harmonics[4] = 1.0f - (harmLowHighCV_env - hlhLimits[5]) * multiplier;
            voices[i].harmonics[5] = 0.666f - 0.333f * (harmLowHighCV_env - hlhLimits[5]) * multiplier;
            voices[i].harmonics[6] = 0.333f + 0.333f * (harmLowHighCV_env - hlhLimits[5]) * multiplier;
            voices[i].harmonics[7] = (harmLowHighCV_env - hlhLimits[5]) * multiplier;
        } else {
            float multiplier = 1.0f / (1.0f - hlhLimits[6]);
            voices[i].harmonics[1] = 0.0f;
            voices[i].harmonics[2] = 0.0f;
            voices[i].harmonics[3] = 0.0f;
            voices[i].harmonics[4] = 0.0f;
            voices[i].harmonics[5] = 0.333f - 0.333f * (harmLowHighCV_env - hlhLimits[6]) * multiplier;
            voices[i].harmonics[6] = 0.666f - 0.333f * (harmLowHighCV_env - hlhLimits[6]) * multiplier;
            voices[i].harmonics[7] = 1.0f;
        }
        
        float harmOddEvenCV_env = harmOddEvenCV * (envToOddEven ? voices[i].generalEnvelopeValue : 1.0f);

        harmOddEvenCV_env = ((1.0f - lfoToOddEven) * harmOddEvenCV_env) + (lfoToOddEven * lfoCurrentValue * harmOddEvenCV_env);

        if (modwheelToOddEven) {
            harmOddEvenCV_env *= modwheel;
        }

        if (harmOddEvenCV_env < 0.5f) {
            voices[i].harmonics[1] = voices[i].harmonics[1] * harmOddEvenCV_env * 2.0f;
            voices[i].harmonics[3] = voices[i].harmonics[3] * harmOddEvenCV_env * 2.0f;
            voices[i].harmonics[5] = voices[i].harmonics[5] * harmOddEvenCV_env * 2.0f;
            voices[i].harmonics[7] = voices[i].harmonics[7] * harmOddEvenCV_env * 2.0f;
        } else {
            voices[i].harmonics[2] = voices[i].harmonics[2] * (1.0f - harmOddEvenCV_env) * 2.0f;
            voices[i].harmonics[4] = voices[i].harmonics[2] * (1.0f - harmOddEvenCV_env) * 2.0f;
            voices[i].harmonics[6] = voices[i].harmonics[2] * (1.0f - harmOddEvenCV_env) * 2.0f;
        }

        voices[i].internalGain = 1.f / (sqrt( 1.f + 
                voices[i].harmonics[1] + 
                voices[i].harmonics[2] + 
                voices[i].harmonics[3] + 
                voices[i].harmonics[4] + 
                voices[i].harmonics[5] + 
                voices[i].harmonics[6] + 
                voices[i].harmonics[7]) * 2.f);
}

float fastTanh(float in)
{
    if (in < -3.f) {
        return -1.f;
    } else if (in > 3.f) {
        return 1.f;
    } else {
        float in2 = in*in;
        return in * (27.f + in2) / (27.f + in2 * 9.f);
    }
}

void HarmoniqsEngine::SetAttack(float attack)
{
    adsrValues.attack = attack;
    for (int i = 0; i < POLYPHONY; i++) {
        SetAttack(adsrValues.attack, i);
    }
    combinedEnvelope.SetAttackTime(adsrValues.attack * 5.f);
}

void HarmoniqsEngine::SetDecay(float decay)
{
    adsrValues.decay = decay;
    for (int i = 0; i < POLYPHONY; i++) {
        SetDecay(adsrValues.decay, i);
    }
    combinedEnvelope.SetDecayTime(adsrValues.decay * 5.f);
}

void HarmoniqsEngine::SetSustain(float sustain)
{
    adsrValues.sustain = sustain;
    for (int i = 0; i < POLYPHONY; i++) {
        SetSustain(adsrValues.sustain, i);
    }
    combinedEnvelope.SetSustainLevel(adsrValues.sustain);
}

void HarmoniqsEngine::SetRelease(float release)
{
    adsrValues.release = release;
    for (int i = 0; i < POLYPHONY; i++) {
        SetRelease(adsrValues.release, i);
    }
    combinedEnvelope.SetReleaseTime(adsrValues.release * 5.f);
}

void HarmoniqsEngine::SetAttack(float attack, int voiceIndex)
{
    voices[voiceIndex].envelope.SetAttackTime(attack * 5.f);
    voices[voiceIndex].envelopeVol.SetAttackTime(envToVol ? attack * 5.f : 0.001f);
}

void HarmoniqsEngine::SetDecay(float decay, int voiceIndex)
{
    voices[voiceIndex].envelope.SetDecayTime(decay * 5.f);
    voices[voiceIndex].envelopeVol.SetDecayTime(envToVol ? decay * 5.f : 0.001f);
}

void HarmoniqsEngine::SetSustain(float sustain, int voiceIndex)
{
    voices[voiceIndex].envelope.SetSustainLevel(sustain);
    voices[voiceIndex].envelopeVol.SetSustainLevel(envToVol ? sustain : 1.f);
}

void HarmoniqsEngine::SetRelease(float release, int voiceIndex)
{
    voices[voiceIndex].envelope.SetReleaseTime(release * 5.f);
    voices[voiceIndex].envelopeVol.SetReleaseTime(release * 5.f);
}

void HarmoniqsEngine::SetADSR(float attack, float decay, float sustain, float release)
{
    adsrValues.attack = attack;
    adsrValues.decay = decay;
    adsrValues.sustain = sustain;
    adsrValues.release = release;
    for (int i = 0; i < POLYPHONY; i++) {
        SetAttack(adsrValues.attack, i);
        SetDecay(adsrValues.decay, i);
        SetSustain(adsrValues.sustain, i);
        SetRelease(adsrValues.release, i);
    }

    combinedEnvelope.SetAttackTime(adsrValues.attack * 5.f);
    combinedEnvelope.SetDecayTime(adsrValues.decay * 5.f);
    combinedEnvelope.SetSustainLevel(adsrValues.sustain);
    combinedEnvelope.SetReleaseTime(adsrValues.release * 5.f);
}


void HarmoniqsEngine::SetADSR(float attack, float decay, float sustain, float release, int voiceIndex)
{
    SetAttack(attack, voiceIndex);
    SetDecay(decay, voiceIndex);
    SetSustain(sustain, voiceIndex);
    SetRelease(release, voiceIndex);
}


float HarmoniqsEngine::Process()
{
    float sig = 0.f;

    float combiEnveValueTemp = combinedEnvelope.Process(combinedEnvelopeOn);

    lfoCurrentValue = (lfo.Process() + 1.f) * 0.5f;
    
    
    int oldestUpdated = updateCounter;
    int oldestUpdatedIndex = 0;

    for (int uhv = 0; uhv < POLYPHONY; uhv++) {
        if ((voices[uhv].on || voices[uhv].isPlaying) && voices[uhv].lastUpdated < oldestUpdated) {
            oldestUpdated = voices[uhv].lastUpdated;
            oldestUpdatedIndex = uhv;
        }
    }

    updateCounter++;

    voices[oldestUpdatedIndex].lastUpdated = updateCounter;
    
    UpdateHarmonicValues(oldestUpdatedIndex);
    
    /*
    UpdateHarmonicValues(optCounter);

    optCounter++;

    if (optCounter > POLYPHONY - 1) {
        optCounter = 0;
    }
    */

    float oneMinusLfoToHarmLevel = 1.0f - lfoToHarmLevel;
    float lfoToHarmLevelXlfoCurrentValue = lfoToHarmLevel * lfoCurrentValue;

    float oneMinusLfoToBitcrush = 1.0f - lfoToBitcrush;
    float lfoToBitrcrushXlfoCurrentValue = lfoToBitcrush * lfoCurrentValue;

    for (int v = 0; v < POLYPHONY; v++) {
        voices[v].generalEnvelopeValue = voices[v].envelope.Process(voices[v].on);
        voices[v].isPlaying = voices[v].envelope.IsRunning();

        if (voices[v].isPlaying) {
            
            voices[v].oscillator.SetFreq(voices[v].oscFreq * pitchBend);

            int osc0 = int(voices[v].oscillator.Process() * WAVETABLE_LENGTH_HALF + WAVETABLE_LENGTH_HALF_MINUS_1);

            float signalTemp = sineWavetable[int(osc0)];

            float signalHarmonicsTemp = 0;

            for (int i = 1; i < 8; i++) {

                int oscPlusHarm = (int)(osc0 * (i+1));

                int inharmTemp = (int)(inharmonism * WAVETABLE_LENGTH * lfoCurrentValue * ((float)i+1.f) * inharm_max );

                if (inharmTemp < 0) {
                    inharmTemp = WAVETABLE_LENGTH - inharmTemp;
                }

                float harmTemp = sineWavetable[(oscPlusHarm + inharmTemp) % WAVETABLE_LENGTH]; // Base wavetable
                harmTemp *= voices[v].harmonics[i];                 // Harmonic gain
                harmTemp *= harmLevelCV;                            // General harmonic gain CV

                if (envToHarmLevel) {                               // Apply Envelope to harmonic level if applicable
                    harmTemp *= voices[v].generalEnvelopeValue;
                }

                
                if(i%2 == 0) {                                // Switch polarity of every other harmonic to get smaller peak volume
                    harmTemp *= -1;
                }

                signalHarmonicsTemp += harmTemp;
                
            }

            signalHarmonicsTemp = (oneMinusLfoToHarmLevel * signalHarmonicsTemp) + (lfoToHarmLevelXlfoCurrentValue * signalHarmonicsTemp); // Apply right amount of LFO to harmonic level

            if (modwheelToHarmLevel) {                    // Apply Modwheel to harmonic level if applicable
                signalHarmonicsTemp *= modwheel;
            }
            
            signalTemp += signalHarmonicsTemp;

            signalTemp *= voices[v].internalGain;  // Gain to equalize volume when amount of harmonics change

            float bitcrushTemp = bitcrushCV * (envToBitcrush ? voices[v].generalEnvelopeValue : 1.f);

            bitcrushTemp = (oneMinusLfoToBitcrush * bitcrushTemp) + (lfoToBitrcrushXlfoCurrentValue * bitcrushTemp);   // Apply right amount of LFO to bitcrushing

            if (modwheelToBitcrush) {    // Apply Modwheel to bitcrushing if applicable
                bitcrushTemp *= modwheel;
            }

            voices[v].decim.SetBitcrushFactor(bitcrushTemp);

            signalTemp = voices[v].decim.Process(signalTemp) * voices[v].envelopeVol.Process(voices[v].on);

            sig += signalTemp * 0.2f;          // Number derived with Stetson-Harrison method to have right signal levels
        }
    }

    avrg_cursor++;
    if (avrg_cursor > AVERAGE_WINDOW_LENGTH - 1) {
        avrg_cursor = 0;

        /*
        char        buff[512];
            sprintf(buff,
                    "Pre AVRG: %d\tPost AVRG: %d\r\n", (int)((rms_sum_pre/128.0f)*1000.0f), (int)((rms_sum_post/128.0f)*1000.0f));
            
            pod.seed.usb_handle.TransmitInternal((uint8_t *)buff, strlen(buff));
        */
    }

    float oldest_avrg_sample_pre = avrg_samples_pre[avrg_cursor];
    float oldest_avrg_sample_post = avrg_samples_post[avrg_cursor];

    float abs_pre = abs(sig);
    avrg_samples_pre[avrg_cursor] = abs_pre;

    avrg_sum_pre -= oldest_avrg_sample_pre;
    avrg_sum_pre += abs_pre;

    sig = fastTanh(sig * (1.f + (distortionCV * 100.f)));


    float filterFreqCVTemp = filterFreqCV;

    filterFreqCVTemp = (1.f - envelopeToFilterAmount) * filterFreqCVTemp + (envelopeToFilterAmount * combiEnveValueTemp) * filterFreqCVTemp; 


    endFilter.SetFreq((filterFreqCVTemp * filterFreqCVTemp * filterFreqCVTemp * 18000.f) + 200.f);
    endFilter.SetRes(filterRezCV);

    sig = endFilter.Process(sig) * (1.f + filterRezCV);

    float abs_post = abs(sig);
    avrg_samples_post[avrg_cursor] = abs_post;
    avrg_sum_post -= oldest_avrg_sample_post;
    avrg_sum_post += abs_post;

    // Use rolling average to calculate and compensate volume gain/loss from distortion and filter
    // Straight average seems to work better than RMS.
    if (avrg_sum_post > 0 && avrg_sum_post > avrg_sum_pre) {
        float distQuieter = avrg_sum_pre / avrg_sum_post;

        sig *= distQuieter;
    }

    sig = ((1.f - lfoToVol) * sig) + (lfoToVol * lfoCurrentValue * sig);    // Apply right amount of LFO to volume

    if (modwheelToVol) {
        sig *= modwheel;
    }

    // crude limiting as signal manages to get over 1 sometimes with high resonance
    if (sig > 0.9f) {
        sig = 0.9f + (sig - 0.9f) * 0.5f;
    } else if (sig < -0.9f) {
        sig = -0.9f - (sig + 0.9f) * 0.5f;
    }

    if (sig > 1.0f) {
        sig = 1.0f;
    } else if (sig < -0.9f) {
        sig = 1.0f;
    }

    if (abs(sig) > maximumOutputValue) {
        maximumOutputValue = abs(sig);
    }

    return sig;

}