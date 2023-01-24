#include "daisysp.h"
#include "daisy_pod.h"
#include <cmath>

using namespace daisysp;
using namespace daisy;

#define POLYPHONY 6
#define AVERAGE_WINDOW_LENGTH 1024
#define WAVETABLE_LENGTH 1024
#define WAVETABLE_LENGTH_HALF 512
#define WAVETABLE_LENGTH_HALF_MINUS_1 511

DaisyPod   pod;
Oscillator lfo;
MoogLadder flt;
Decimator decim;

float oldk1, oldk2, k1, k2;

float avrg_sum_pre = 0.0f;
float avrg_sum_post = 0.0f;

float avrg_samples_pre[AVERAGE_WINDOW_LENGTH] = { 0.0f };
float avrg_samples_post[AVERAGE_WINDOW_LENGTH] = { 0.0f };

int avrg_cursor = 0;

float maximumOutputValue = 0.0f;

int noteOnAmount = 0;
int noteOffAmount = 0;

bool envToVol = false;
bool envToOddEven = false;
bool envToLowHigh = false;
bool envToHarmLevel = false;
bool envToBitcrush = false;

float pitchCV = 0.f;
float harmOddEvenCV = 0.f;
float harmLevelCV = 0.f;
float harmLowHighCV = 0.f;
float bitcrushCV = 0.f;

bool adjustDistortion = false;
float distortionCV = 0.0f;

bool adjustFilterFreq = false;
float filterFreqCV = 1.0f;

bool adjustFilterRez = false;
float filterRezCV = 0.0f;

bool adjustInharmonism = false;
float inharmonism = 0.0f;

bool bitcrusherIsSmooth = true;

float pitchBend = 0.f;

float modwheel = 0.f;

int age = 1;

float lfoSpeed = 1.f;

float lfoToVol = 0.f;
float lfoToHarmLevel = 0.f;
float lfoToOddEven = 0.f;
float lfoToHarmLowHigh = 0.f;
float lfoToBitcrush = 0.f;

int encoderLfoSelection = 0;

bool lfoToVolSelection = false;
bool lfoToHarmLevelSelection = false;
bool lfoToOddEvenSelection = false;
bool lfoToHarmLowHighSelection = false;
bool lfoToBitcrushSelection = false;

bool modwheelToVol = false;
bool modwheelToHarmLevel = false;
bool modwheelToOddEven = false;
bool modwheelToHarmLowHigh = false;
bool modwheelToBitcrush = false;

float lfoCurrentValue = 0.0f;

float sineWavetable[1024];

struct adsrValueStruct {
    float attack;
    float decay;
    float sustain;
    float release;
};

adsrValueStruct adsrValues;

Adsr combinedEnvelope;

bool combinedEnvelopeOn = false;

bool adjustEnvelopeToFilter = false;

float envelopeToFilterAmount = 0.0f;

struct omaVoice {
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
};

omaVoice voices[POLYPHONY];

int harmonicsTestRound = 0;

const float hlhLimits[7] = { 0.00f, 0.05f, 0.20f, 0.35f, 0.50f, 0.75f, 0.90f };

void ConditionalParameter(float  oldVal,
                          float  newVal,
                          float &param,
                          float  update);

void Controls();

void UpdateHarmonicValues()
{
    for (int i = 0; i < POLYPHONY; i++) {
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
    }
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


void NextSamples(float &sig)
{
    sig = 0.f;

    float 🧙 = 2.f;

    for (int e = 0; e < POLYPHONY; e++) {
        voices[e].generalEnvelopeValue = voices[e].envelope.Process(voices[e].on);
    }

    float combiEnveValueTemp = combinedEnvelope.Process(combinedEnvelopeOn);

    lfoCurrentValue = (lfo.Process() + 1.f) * 0.5f;
    
    UpdateHarmonicValues();

    for (int v = 0; v < POLYPHONY; v++) {
        float generalEnvelope = voices[v].envelope.Process(voices[v].on);
        voices[v].oscillator.SetFreq(voices[v].oscFreq * pitchBend);

        int osc0 = int(voices[v].oscillator.Process() * WAVETABLE_LENGTH_HALF + WAVETABLE_LENGTH_HALF_MINUS_1);

        float gain = 1.f / (sqrt( 1.f + 
                voices[v].harmonics[1] + 
                voices[v].harmonics[2] + 
                voices[v].harmonics[3] + 
                voices[v].harmonics[4] + 
                voices[v].harmonics[5] + 
                voices[v].harmonics[6] + 
                voices[v].harmonics[7]) * 🧙);

        float signalTemp = sineWavetable[int(osc0)];

        for (int i = 1; i < 8; i++) {

            int oscPlusHarm = (int)(osc0 * (i+1));

            int inharmTemp = (int)(inharmonism * WAVETABLE_LENGTH * lfoCurrentValue * ((float)i+1.f) * (2.f / 3.f) ) % WAVETABLE_LENGTH;

            if (inharmTemp < 0) {
                inharmTemp = WAVETABLE_LENGTH - inharmTemp;
            }

            float harmTemp = sineWavetable[(oscPlusHarm + inharmTemp) % WAVETABLE_LENGTH]; // Base wavetable
            harmTemp *= voices[v].harmonics[i];                 // Harmonic gain
            harmTemp *= harmLevelCV;                            // General harmonic gain CV

            if (envToHarmLevel) {                               // Apply Envelope to harmonic level if applicable
                harmTemp *= generalEnvelope;
            }

            harmTemp = ((1.0f - lfoToHarmLevel) * harmTemp) + (lfoToHarmLevel * lfoCurrentValue * harmTemp); // Apply right amount of LFO to harmonic level

            if (modwheelToHarmLevel) {                    // Apply Modwheel to harmonic level if applicable
                harmTemp *= modwheel;
            }
            
            if(i%2 == 0) {                                // Switch polarity of every other harmonic to get smaller peak volume
                harmTemp *= -1;
            }

            signalTemp += harmTemp;
            
        }

        signalTemp *= gain;  // Gain to equalize volume when amount of harmonics change

        float bitcrushTemp = bitcrushCV * (envToBitcrush ? voices[v].generalEnvelopeValue : 1.f);

        bitcrushTemp = bitcrushCV;   // Set basic amount of bitcrushing

        if (envToBitcrush) {         // Apply envelope to bitcrushing if applicable
            bitcrushCV = voices[v].generalEnvelopeValue;
        }

        bitcrushTemp = ((1.0f - lfoToBitcrush) * bitcrushTemp) + (lfoToBitcrush * lfoCurrentValue * bitcrushTemp);   // Apply right amount of LFO to bitcrushing

        if (modwheelToBitcrush) {    // Apply Modwheel to bitcrushing if applicable
            bitcrushTemp *= modwheel;
        }

        voices[v].decim.SetBitcrushFactor(bitcrushTemp);

        signalTemp = voices[v].decim.Process(signalTemp) * voices[v].envelopeVol.Process(voices[v].on);

        sig += signalTemp * 0.4f;          // Number derived with Stetson-Harrison method to have right signal levels
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


    flt.SetFreq((filterFreqCVTemp * filterFreqCVTemp * filterFreqCVTemp * 18000.f) + 200.f);
    flt.SetRes(filterRezCV);

    sig = flt.Process(sig) * (1.f + filterRezCV);

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

    if (abs(sig) > maximumOutputValue) {
        maximumOutputValue = abs(sig);
    }

}

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    Controls();

    for(size_t i = 0; i < size; i += 2)
    {
        float sig;
        NextSamples(sig);

        // left out
        out[i] = sig;

        // right out
        out[i + 1] = sig;
    }
}

// Typical Switch case for Message Type.
void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent noteOn = m.AsNoteOn();
            noteOnAmount++;
            combinedEnvelopeOn = true;
            combinedEnvelope.Retrigger(false);

            int sameNoteIndex = -1;
            int silentIndex = -1;
            int oldestNonPlayingInRelease = age;
            int oldestNonPlayingInReleaseIndex = -1;
            int oldestPlaying = age;
            int oldestPlayingIndex = -1;

            for (int i = 0; i < POLYPHONY; i++) {
                if (voices[i].note == noteOn.note) {
                    sameNoteIndex = i;
                    break;
                }
                if (!voices[i].on && voices[i].envelope.GetCurrentSegment() != ADSR_SEG_RELEASE) {
                    silentIndex = i;
                    break;
                }
                if (voices[i].age < oldestNonPlayingInRelease && voices[i].envelope.GetCurrentSegment() == ADSR_SEG_RELEASE) {
                    oldestNonPlayingInRelease = voices[i].age;
                    oldestNonPlayingInReleaseIndex = i;
                }
                if (voices[i].age < oldestPlaying) {
                    oldestPlaying = voices[i].age;
                    oldestPlayingIndex = i;
                }
            }

            int index;

            if (sameNoteIndex >= 0) {
                index = sameNoteIndex;
            } else if (silentIndex >= 0) {
                index = silentIndex;
            } else if (oldestNonPlayingInReleaseIndex >= 0) {
                index = oldestNonPlayingInReleaseIndex;
            } else {
                index = oldestPlayingIndex;
            }

            age++;
            voices[index].oscFreq = mtof(noteOn.note);
            voices[index].amp = noteOn.velocity / 127.0f;
            voices[index].on = true;
            voices[index].note = noteOn.note;
            voices[index].age = age;
            
            /*
            char        buff[512];
            sprintf(buff,
                    "Note Received:\t\t%d\t%d\t%d\tNow playing: %d %d %d %d %d %d\r\n",
                    m.channel,
                    m.data[0],
                    m.data[1],
                    voices[0].on ? voices[0].note : 0,
                    voices[1].on ? voices[1].note : 0,
                    voices[2].on ? voices[2].note : 0,
                    voices[3].on ? voices[3].note : 0,
                    voices[4].on ? voices[4].note : 0,
                    voices[5].on ? voices[5].note : 0);
            
            pod.seed.usb_handle.TransmitInternal((uint8_t *)buff, strlen(buff));
            */
            

        }
        break;
        case NoteOff:
        {
            NoteOffEvent noteOff = m.AsNoteOff();
            noteOffAmount++;

            /*
            char        buff[512];
            sprintf(buff,
                    "Note Off Received:\t%d\t%d\t%d\r\n",
                    m.channel,
                    m.data[0],
                    m.data[1]);
            pod.seed.usb_handle.TransmitInternal((uint8_t *)buff, strlen(buff));
            */
            
            int anyOnTemp = 0;

            for (int i = 0; i < POLYPHONY; i++) {

                if (voices[i].note == noteOff.note) {
                    voices[i].on = false;
                }

                anyOnTemp += voices[i].on;
            }

            combinedEnvelopeOn = anyOnTemp > 0;
            
        }
        break;
        case ControlChange:
        {
            ControlChangeEvent cc = m.AsControlChange();
            /*
            char        buff[512];
            sprintf(buff,
                    "CC Received:\t%d\t%d\t%d\r\n",
                    m.channel,
                    m.data[0],
                    m.data[1]);
            pod.seed.usb_handle.TransmitInternal((uint8_t *)buff, strlen(buff));
            */

            switch(cc.control_number)
            {
                case 1:  modwheel = (float)(cc.value) / 127.0f; break;
                case 16: envToHarmLevel = cc.value > 0; break;
                case 17: envToOddEven = cc.value > 0; break;
                case 18: envToLowHigh = cc.value > 0; break;
                case 19: envToBitcrush = cc.value > 0; break;
                case 20: modwheelToHarmLevel = cc.value > 0; break;
                case 21: modwheelToOddEven = cc.value > 0; break;
                case 22: modwheelToHarmLowHigh = cc.value > 0; break;
                case 23: modwheelToBitcrush = cc.value > 0; break;
                case 24: lfoToHarmLevelSelection = cc.value > 0; break;
                case 25: lfoToOddEvenSelection = cc.value > 0; break;
                case 26: lfoToHarmLowHighSelection = cc.value > 0; break;
                case 27: lfoToBitcrushSelection = cc.value > 0; break;
                case 28: envToVol = cc.value > 0; break;
                case 29: modwheelToVol = cc.value > 0; break;
                case 30: lfoToVolSelection = cc.value > 0; break;
                case 52: adjustEnvelopeToFilter = cc.value > 0; break;
                case 53: adjustInharmonism = cc.value > 0; break;
                case 54: adjustFilterFreq = cc.value > 0; break;
                case 55: adjustFilterRez = cc.value > 0; break;
                case 56: adjustDistortion = cc.value > 0; break;
                case 35: harmLevelCV = (float)(cc.value) / 127.f;  break; // E1
                case 41: harmOddEvenCV = (float)(cc.value) / 127.f; break; // E2
                case 44: harmLowHighCV = (float)(cc.value) / 127.f; break; // E3
                case 45: {
                    if (cc.value < 32) {  // E4
                        bitcrushCV = 0.75f * (float)(cc.value) / 32.f;
                    } else {
                        bitcrushCV = 0.75f + 0.25f * (float)(cc.value - 32) / 95.f; break;
                    }
                }
                break;
                case 62: adsrValues.attack = (float)(cc.value) / 127.f + 0.001f; break; // F1
                case 63: adsrValues.decay = (float)(cc.value) / 127.f + 0.001f; break; // F2
                case 75: adsrValues.sustain = (float)(cc.value) / 127.f; break; // F3
                case 76: adsrValues.release = (float)(cc.value) / 127.f + 0.001f; break; // F4
                case 77: // F5
                    {
                        if (adjustDistortion) {
                            distortionCV = (float)(cc.value) / 127.f;
                        }
                        if (adjustFilterFreq) {
                            filterFreqCV = (float)(cc.value) / 127.f;
                        }
                        if (adjustFilterRez) {
                            filterRezCV = (float)(cc.value) / 127.f;
                            if (filterRezCV > 0.8f) {
                                filterRezCV = 0.8f;
                            }
                        }
                        if (adjustInharmonism) {
                            inharmonism = (float)(cc.value) / 127.f;
                        }
                        if (adjustEnvelopeToFilter) {
                            envelopeToFilterAmount = (float)(cc.value) / 127.f;
                        }
                    }
                    break;
                case 31:
                    {
                        for ( int i = 0; i < POLYPHONY; i++) {
                            voices[i].on = false;
                        }
                        envToVol = false;
                        envToOddEven = false;
                        envToLowHigh = false;
                        envToHarmLevel = false;
                        envToBitcrush = false;
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
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        break;
        case PitchBend:
        {
            PitchBendEvent pb = m.AsPitchBend();
            /*
            char        buff[512];
            sprintf(buff,
                    "CC Received:\t%d\t%d\t%d\t%d\r\n",
                    m.channel,
                    m.data[0],
                    m.data[1],
                    pb.value);
            pod.seed.usb_handle.TransmitInternal((uint8_t *)buff, strlen(buff));
            */
            if (pb.value >= 0) {
                pitchBend = 1.f + ((float)(pb.value) / 8191.f);
            } else {
                pitchBend = 1.f + (((float)(pb.value) / 8192.f) * 0.5f); 
            }
            
        }
        break;
        default:
        break;
    }
}

int main(void)
{
    // Set global variables
    float sample_rate;
    oldk1 = oldk2 = 0;
    k1 = k2   = 0;

    adsrValues.attack = 0.001f;
    adsrValues.decay = 0.001f;
    adsrValues.sustain = 1.0f;
    adsrValues.release = 0.001f;

    pitchCV = 0.0f;
    harmLevelCV = 1.0f;
    harmOddEvenCV = 0.5f;
    harmLowHighCV = 0.3f;
    bitcrushCV = 0.0f;

    pitchBend = 1.0f;

    // Generate sinewavetable
    for (int i = 0; i < WAVETABLE_LENGTH; i++) {
        sineWavetable[i] = sinf(TWOPI_F * ((i * 1.f) / (float)(WAVETABLE_LENGTH)) );
    }

    //Init everything
    pod.Init();
    pod.SetAudioBlockSize(16);
    sample_rate = pod.AudioSampleRate();

    pod.seed.usb_handle.Init(UsbHandle::FS_INTERNAL);
    System::Delay(250);

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

    decim.Init();
    decim.SetSmoothCrushing(true);
    decim.SetDownsampleFactor(0.f);

    flt.Init(sample_rate);
    flt.SetFreq(10000);
    flt.SetRes(0.f);

    // start callback
    pod.StartAdc();
    pod.StartAudio(AudioCallback);
    pod.midi.StartReceive();



    while(1) {
        pod.midi.Listen();
        // Handle MIDI Events
        while(pod.midi.HasEvents())
        {
            HandleMidiMessage(pod.midi.PopEvent());
        }
    }
}

//Updates values if knob had changed
void ConditionalParameter(float  oldVal,
                          float  newVal,
                          float &param,
                          float  update)
{
    if(abs(oldVal - newVal) > 0.00005)
    {
        param = update;
    }
}


//Controls Helpers
void UpdateEncoder()
{

}

void UpdateKnobs()
{
    k1 = pod.knob1.Process();
    k2 = pod.knob2.Process();

    lfo.SetFreq((1.0f + k1 * 99.f) / 10.f);

    if (lfoToVolSelection) {
        lfoToVol = k2;
    }

    if (lfoToHarmLevelSelection) {
        lfoToHarmLevel = k2;
    }
    
    if (lfoToOddEvenSelection) {
        lfoToOddEven = k2;
    }

    if (lfoToHarmLowHighSelection) {
        lfoToHarmLowHigh = k2;
    }

    if (lfoToBitcrushSelection) {
        lfoToBitcrush = k2;
    }

}

void UpdateLeds()
{
    if (maximumOutputValue < 0.8f) {
        pod.led1.Set(0.0f, maximumOutputValue / 0.8f, 0.f);
    } else if (maximumOutputValue < 1.f) {
        pod.led1.Set((maximumOutputValue - 0.8f) / 0.2f, (maximumOutputValue - 0.8f) / 0.2f, 0.f);
    } else {
        pod.led1.Set(1.f, 0.f, 0.f);
    }

    if (noteOnAmount > noteOffAmount) {
        pod.led2.Set(1, 0, 0);
    } else if (noteOnAmount < noteOffAmount) {
        pod.led2.Set(0, 0, 1);
    } else {
        pod.led2.Set(0, 1, 0);
    }

    pod.UpdateLeds();
}

void UpdateButtons()
{
    if (pod.button1.RisingEdge()) {
            char        buff[2048];
            sprintf(buff,
                    "Modwheel: %d\r\n"
                    "envToHarmLevel: %d\r\n"
                    "envToOddEven: %d\r\n"
                    "envToLowHigh: %d\r\n"
                    "envToBitcrush: %d\r\n"
                    "modwheelToHarmLevel: %d\r\n"
                    "modwheelToOddEven: %d\r\n"
                    "modwheelToHarmLowHigh: %d\r\n"
                    "modwheelToBitcrush: %d\r\n"
                    "lfoToHarmLevelSelection: %d\r\n"
                    "lfoToOddEvenSelection: %d\r\n"
                    "lfoToHarmLowHighSelection: %d\r\n"
                    "lfoToBitcrushSelection: %d\r\n"
                    "envToVol: %d\r\n"
                    "modwheelToVol: %d\r\n"
                    "lfoToVolSelection: %d\r\n"
                    "bitcrusherIsSmooth: %d\r\n"
                    "adjustInharmonism: %d\r\n"
                    "adjustFilterFreq: %d\r\n"
                    "adjustFilterRez: %d\r\n"
                    "adjustDistortion: %d\r\n"
                    "harmLevelCV: %d\r\n"
                    "harmOddEvenCV: %d\r\n"
                    "harmLowHighCV: %d\r\n"
                    "bitcrushCV: %d\r\n"
                    "adsrValues.attack: %d\r\n"
                    "adsrValues.decay: %d\r\n"
                    "adsrValues.sustain: %d\r\n"
                    "adsrValues.release: %d\r\n"
                    "maximumOuputValue: %d\r\n",
                    (int)(modwheel*1000),
                    (int)(envToHarmLevel*1000),
                    (int)(envToOddEven*1000),
                    (int)(envToLowHigh*1000),
                    (int)(envToBitcrush*1000),
                    (int)(modwheelToHarmLevel*1000),
                    (int)(modwheelToOddEven*1000),
                    (int)(modwheelToHarmLowHigh*1000),
                    (int)(modwheelToBitcrush*1000),
                    (int)(lfoToHarmLevelSelection*1000),
                    (int)(lfoToOddEvenSelection*1000),
                    (int)(lfoToHarmLowHighSelection*1000),
                    (int)(lfoToBitcrushSelection*1000),
                    (int)(envToVol*1000),
                    (int)(modwheelToVol*1000),
                    (int)(lfoToVolSelection*1000),
                    (int)(bitcrusherIsSmooth*1000),
                    (int)(adjustInharmonism*1000),
                    (int)(adjustFilterFreq*1000),
                    (int)(adjustFilterRez*1000),
                    (int)(adjustDistortion*1000),
                    (int)(harmLevelCV*1000),
                    (int)(harmOddEvenCV*1000),
                    (int)(harmLowHighCV*1000),
                    (int)(bitcrushCV*1000),
                    (int)(adsrValues.attack*1000),
                    (int)(adsrValues.decay*1000),
                    (int)(adsrValues.sustain*1000),
                    (int)(adsrValues.release*1000),
                    (int)(maximumOutputValue*1000)
                    );

            pod.seed.usb_handle.TransmitInternal((uint8_t *)buff, strlen(buff));
    }

    if (pod.button2.RisingEdge())  {

        for (int i = 0; i < POLYPHONY; i++) {
            voices[i].on = false;
        }

        maximumOutputValue = 0.f;
        noteOnAmount = 0;
        noteOffAmount = 0;

        pod.led1.Set(0, 0, 0);
        pod.led2.Set(0, 0, 0);
        pod.UpdateLeds();
    }
}

void UpdateValues()
{
    for (int i = 0; i < POLYPHONY; i++) {
        voices[i].envelope.SetAttackTime(adsrValues.attack * 5.f);
        voices[i].envelope.SetDecayTime(adsrValues.decay * 5.f);
        voices[i].envelope.SetSustainLevel(adsrValues.sustain);
        voices[i].envelope.SetReleaseTime(adsrValues.release * 5.f);

        voices[i].envelopeVol.SetAttackTime(envToVol ? adsrValues.attack * 5.f : 0.001f);
        voices[i].envelopeVol.SetDecayTime(envToVol ? adsrValues.decay * 5.f : 0.001f);
        voices[i].envelopeVol.SetSustainLevel(envToVol ? adsrValues.sustain : 1.f);
        voices[i].envelopeVol.SetReleaseTime(adsrValues.release * 5.f);

        combinedEnvelope.SetAttackTime(adsrValues.attack * 5.f);
        combinedEnvelope.SetDecayTime(adsrValues.decay * 5.f);
        combinedEnvelope.SetSustainLevel(adsrValues.sustain);
        combinedEnvelope.SetReleaseTime(adsrValues.release * 5.f);
    }

}

void Controls()
{
    pod.ProcessAnalogControls();
    pod.ProcessDigitalControls();

    UpdateEncoder();

    UpdateKnobs();

    UpdateLeds();

    UpdateButtons();

    UpdateValues();
}
