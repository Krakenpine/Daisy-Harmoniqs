# Daisy Harmoniqs
 Additive harmonics synth for Daisy platform

## Version 0.5.0
Works on Daisy Pod.

Six voice additive oscillator synth. Every voice has is comprised of 8 sine wave with frequencies of 1x, 2x, 3x, 4x, 5x, 6x, 7x, and 8x of the fundamental frequency.
- *Harmonics level* sets how much frequencies above the fundamental is added
- *Even - odd harmonics* sets mix of even and odd harmonics. 0 is only even, 1 is only odd and 0.5 has equal amount of everything.
- *Low - high harmonics* sets different ratios of low and high harmonics. Near 0 is low harmonics, near 1 is high harmonics and 0.5 has all harmonics, but between 0 to 0.5 and 0.5 to 1 there are different mixes of them in (hopefully) ear-pleasing combinations.
- *Bitcrusher* is separate for every voice.
- *Envelope* is also per voice, except there is also general envelope which is on when any note is playing and starts attack cycle with every new note (meaning it is retriggerd), and which is currently used only for main filter frequency (if set so)
- *Distortion* based on approximation of tanh() -function, is applied to mix of all voices
- *Filter* After distortion, so common for all voices. Has controls for cut-off frequency and amount of resonance
- *LFO* Triangle low frequency oscillator, currently speed can be set from 0.1 Hz to 10 Hz. Daisy Pod potentiometer 1 controls the speed, potentiometer 2 controls the amount of LFO applied, see Midi implementation.

## Notes

Requires my addition to Decimator effect that adds smoother bitcrush: https://github.com/electro-smith/DaisySP/pull/179
In current (22.1.2023) version of DaisySP library Moog filter is also broken, replace function **float MoogLadder::my_tanh(float x)** in **moogladder.cpp** with for example with this:

    float MoogLadder::my_tanh(float x)
    {
        if (x < -3) {
            return -1.0f;
        } else if (x > 3) {
            return 1.0f;
        } else {
            float x2 = x*x;
            return x * (27 + x2) / (27 + x2 * 9);
        }
    }

Change Library locations in Makefile to where you have DaisySP and libDaisy. Current version of libDaisy, 5.something seems to have problems with midi stability, so I used version 4.0.0. For all purposes, this code seems to be compatible with versions 4 and 5, so when that midi problem is patched, there's no reason to use version 4.0.0.

Currently most of the serial logging messages are commented out.

## To do
- Fix bugs
- Fix code style and remove unnecessary things and make everything generally clearer
- More comments
- Optimize
- Make sound better
- Design schematic for Daisy Seed
- Make PCB layout of that
- Make all settings be adjustable without Midi control commands
- Add Gate and V/Oct inputs for monophonic modular use
- Add possibility to save presets
- Add check if physical controls have changed after value was set with Midi or loading a preset and don't everwrite that until physical control is moved

## Midi implementation:

### Value larger than 0 is on, value of 0 is off:  
CC 16: Envelope to harmonics level  
CC 17: Envelope to even - odd harmonics mix  
CC 18: Envelope to low - high harmonics selection  
CC 19: Envelope to bitcrusher value  
CC 20: Modwheel to harmonics level  
CC 21: Modwheel to even - odd harmonics mix  
CC 22: Modwheel to low - high harmonics selection  
CC 23: Modwheel to bitcrusher value  
CC 28: Envelope to main volume  
CC 29: Modwheel to main volume  

### When these are on, the pot 2 on Daisy Pod sets how strongly the LFO affects that setting.
The set LFO level stays after value is off, so you can set different amount of LFO to other settings.  
CC 24: LFO to harmonics level  
CC 26: LFO to even - odd harmonics mix  
CC 27: LFO to low - high harmonics selection  
CC 28: LFO to bitcrusher value  
CC 30: LFO to main volume  

### When these are on, the CC 77 sets value of that setting.
CC 52: Envelope to filter  
CC 53: Amount of inharmonity (LFO changes pitch of harmonics other than the lowest one)  
CC 54: Set cut-off frequency of output filter  
CC 55: Set resonance of output filter  
CC 56: Set amount of distortion  

CC 62: ADSR envelope attack 0 - 5 seconds  
CC 63: ADSR envelope decay 0 - 5 seconds  
CC 75: ADSR envelope sustain level  
CC 76: ADSR envelope release 0 - 5 seconds  

CC 35: Set harmonics level  
CC 41: Set even - odd harmonics mix  
CC 44: Set low - high harmonics setting  
CC 45: Set bitcrushing amount  

If you happen to have M-Audio Code25 midi keyboard, file harmoniqs1.Code25PresetEditor is a preset of those.