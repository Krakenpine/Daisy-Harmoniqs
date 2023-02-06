#pragma once
#include "Harmoniqs-engine.h"

class Note {
  public:
    uint8_t note;
    uint8_t velocity;
    Note() {
        note = 0;
        velocity = 0;
    }
    Note(uint8_t _note, uint8_t _velocity) {
        note = _note;
        velocity = _velocity;
    }
    Note(NoteOnEvent noteOn) {
        note = noteOn.note;
        velocity = noteOn.velocity;
    }
    Note(NoteOffEvent noteOff) {
        note = noteOff.note;
        velocity = noteOff.velocity;
    }
};

class Controller {
  public:
    HarmoniqsEngine engine;

    // engine settings
    int noteOnAmount = 0;
    int noteOffAmount = 0;

    bool adjustDistortion = false;
    bool adjustFilterFreq = false;
    bool adjustFilterRez = false;
    bool adjustInharmonism = false;
    bool adjustEnvelopeToFilter = false;

    float sample_rate;
    bool loggingOn;
    CpuLoadMeter meter;

    virtual void MainLoop() = 0;
    virtual void Controls() = 0;
    virtual void Start(AudioHandle::InterleavingAudioCallback audioCallback) = 0;
    virtual void TypeInit(size_t block_size) = 0;

    void Init(size_t block_size, bool _loggingOn) {
        loggingOn = _loggingOn;
        TypeInit(block_size);
        meter.Init(sample_rate, block_size, 1);
    }
    void InitEngine(float _sample_rate) {
        engine.Init(_sample_rate);
        engine.adsrValues.attack = 0.001f;
        engine.adsrValues.decay = 0.001f;
        engine.adsrValues.sustain = 1.0f;
        engine.adsrValues.release = 0.001f;
    }
    void UpdateValues()
    {
        engine.SetADSR(engine.adsrValues.attack, engine.adsrValues.decay, engine.adsrValues.sustain, engine.adsrValues.release);
    }
    float Process() {
        return engine.Process();
    }
    void StartNote(Note noteOn)
    {
        noteOnAmount++;
        engine.combinedEnvelopeOn = true;
        engine.combinedEnvelope.Retrigger(false);

        int sameNoteIndex = -1;
        int silentIndex = -1;
        int oldestNonPlayingInRelease = engine.age;
        int oldestNonPlayingInReleaseIndex = -1;
        int oldestPlaying = engine.age;
        int oldestPlayingIndex = -1;

        for (int i = 0; i < POLYPHONY; i++)
        {
            if (engine.voices[i].note == noteOn.note)
            {
                sameNoteIndex = i;
                break;
            }
            if (!engine.voices[i].on && engine.voices[i].envelope.GetCurrentSegment() != ADSR_SEG_RELEASE)
            {
                silentIndex = i;
                break;
            }
            if (engine.voices[i].age < oldestNonPlayingInRelease && engine.voices[i].envelope.GetCurrentSegment() == ADSR_SEG_RELEASE)
            {
                oldestNonPlayingInRelease = engine.voices[i].age;
                oldestNonPlayingInReleaseIndex = i;
            }
            if (engine.voices[i].age < oldestPlaying)
            {
                oldestPlaying = engine.voices[i].age;
                oldestPlayingIndex = i;
            }
        }

        int index;

        if (sameNoteIndex >= 0)
        {
            index = sameNoteIndex;
        }
        else if (silentIndex >= 0)
        {
            index = silentIndex;
        }
        else if (oldestNonPlayingInReleaseIndex >= 0)
        {
            index = oldestNonPlayingInReleaseIndex;
        }
        else
        {
            index = oldestPlayingIndex;
        }

        engine.age++;
        engine.voices[index].oscFreq = mtof(noteOn.note);
        engine.voices[index].amp = noteOn.velocity / 127.0f;
        engine.voices[index].on = true;
        engine.voices[index].note = noteOn.note;
        engine.voices[index].age = engine.age;

    }
    void StopNote(Note noteOff)
    {
        noteOffAmount++;

        int anyOnTemp = 0;

        for (int i = 0; i < POLYPHONY; i++)
        {

            if (engine.voices[i].note == noteOff.note)
            {
                engine.voices[i].on = false;
            }

            anyOnTemp += engine.voices[i].on;
        }

        engine.combinedEnvelopeOn = anyOnTemp > 0;
    }
};