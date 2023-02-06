#pragma once

#include "daisy_pod.h"

#include "Controller.h"

class PodController: public Controller {
  public:
    DaisyPod   pod;
    float oldk1, oldk2, k1, k2;

    PodController(){};
    void TypeInit(size_t block_size) {
        // Set global variables
        oldk1 = oldk2 = 0;
        k1 = k2   = 0;
        pod.Init();
        pod.SetAudioBlockSize(block_size);
        sample_rate = pod.AudioSampleRate();
        pod.seed.usb_handle.Init(UsbHandle::FS_INTERNAL);
        Controller::InitEngine(sample_rate);
    }
    void Start(AudioHandle::InterleavingAudioCallback audioCallback) {
        // start callback
        pod.StartAdc();
        pod.StartAudio(audioCallback);
        pod.midi.StartReceive();
    }
    void Controls() {
        pod.ProcessAnalogControls();
        pod.ProcessDigitalControls();

        UpdateEncoder();

        UpdateKnobs();

        UpdateLeds();

        UpdateButtons();
    }
    void MainLoop() {
        pod.midi.Listen();
        // Handle MIDI Events
        while(pod.midi.HasEvents())
        {
            HandleMidiMessage(pod.midi.PopEvent());
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

        engine.lfo.SetFreq((1.0f + k1 * 99.f) / 10.f);

        if (engine.lfoToVolSelection) {
            engine.lfoToVol = k2;
        }

        if (engine.lfoToHarmLevelSelection) {
            engine.lfoToHarmLevel = k2;
        }
        
        if (engine.lfoToOddEvenSelection) {
            engine.lfoToOddEven = k2;
        }

        if (engine.lfoToHarmLowHighSelection) {
            engine.lfoToHarmLowHigh = k2;
        }

        if (engine.lfoToBitcrushSelection) {
            engine.lfoToBitcrush = k2;
        }

    }

    void UpdateLeds()
    {
        if (engine.maximumOutputValue < 0.8f) {
            pod.led1.Set(0.0f, engine.maximumOutputValue / 0.8f, 0.f);
        } else if (engine.maximumOutputValue < 1.f) {
            pod.led1.Set((engine.maximumOutputValue - 0.8f) / 0.2f, (engine.maximumOutputValue - 0.8f) / 0.2f, 0.f);
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
                        "maximumOuputValue: %d\r\n"
                        "CPU load min max avrg: %d %d %d\r\n",
                        (int)(engine.modwheel*1000),
                        (int)(engine.envToHarmLevel*1000),
                        (int)(engine.envToOddEven*1000),
                        (int)(engine.envToLowHigh*1000),
                        (int)(engine.envToBitcrush*1000),
                        (int)(engine.modwheelToHarmLevel*1000),
                        (int)(engine.modwheelToOddEven*1000),
                        (int)(engine.modwheelToHarmLowHigh*1000),
                        (int)(engine.modwheelToBitcrush*1000),
                        (int)(engine.lfoToHarmLevelSelection*1000),
                        (int)(engine.lfoToOddEvenSelection*1000),
                        (int)(engine.lfoToHarmLowHighSelection*1000),
                        (int)(engine.lfoToBitcrushSelection*1000),
                        (int)(engine.envToVol*1000),
                        (int)(engine.modwheelToVol*1000),
                        (int)(engine.lfoToVolSelection*1000),
                        (int)(engine.bitcrusherIsSmooth*1000),
                        (int)(adjustInharmonism*1000),
                        (int)(adjustFilterFreq*1000),
                        (int)(adjustFilterRez*1000),
                        (int)(adjustDistortion*1000),
                        (int)(engine.harmLevelCV*1000),
                        (int)(engine.harmOddEvenCV*1000),
                        (int)(engine.harmLowHighCV*1000),
                        (int)(engine.bitcrushCV*1000),
                        (int)(engine.adsrValues.attack*1000),
                        (int)(engine.adsrValues.decay*1000),
                        (int)(engine.adsrValues.sustain*1000),
                        (int)(engine.adsrValues.release*1000),
                        (int)(engine.maximumOutputValue*1000),
                        (int)(meter.GetMinCpuLoad()*1000),
                        (int)(meter.GetMaxCpuLoad()*1000),
                        (int)(meter.GetAvgCpuLoad()*1000)
                        );

                pod.seed.usb_handle.TransmitInternal((uint8_t *)buff, strlen(buff));
        }

        if (pod.button2.RisingEdge())  {

            for (int i = 0; i < POLYPHONY; i++) {
                engine.voices[i].on = false;
            }

            engine.maximumOutputValue = 0.f;
            noteOnAmount = 0;
            noteOffAmount = 0;
            meter.Reset();

            pod.led1.Set(0, 0, 0);
            pod.led2.Set(0, 0, 0);
            pod.UpdateLeds();
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
                StartNote(Note(noteOn));
                
                if (loggingOn) {
                    char        buff[512];
                    sprintf(buff,
                            "Note Received:\t\t%d\t%d\t%d\tNow playing: %d %d %d %d %d %d %d %d\r\n",
                            m.channel,
                            m.data[0],
                            m.data[1],
                            engine.voices[0].on ? engine.voices[0].note : 0,
                            engine.voices[1].on ? engine.voices[1].note : 0,
                            engine.voices[2].on ? engine.voices[2].note : 0,
                            engine.voices[3].on ? engine.voices[3].note : 0,
                            engine.voices[4].on ? engine.voices[4].note : 0,
                            engine.voices[5].on ? engine.voices[5].note : 0,
                            engine.voices[6].on ? engine.voices[6].note : 0,
                            engine.voices[7].on ? engine.voices[7].note : 0);
                    
                    pod.seed.usb_handle.TransmitInternal((uint8_t *)buff, strlen(buff));
                }
            }
            break;
            case NoteOff:
            {
                NoteOffEvent noteOff = m.AsNoteOff();
                StopNote(Note(noteOff));

                if (loggingOn) {
                    char        buff[512];
                    sprintf(buff,
                            "Note Off Received:\t%d\t%d\t%d\r\n",
                            m.channel,
                            m.data[0],
                            m.data[1]);
                    pod.seed.usb_handle.TransmitInternal((uint8_t *)buff, strlen(buff));
                }
            }
            break;
            case ControlChange:
            {
                ControlChangeEvent cc = m.AsControlChange();
                
                if (loggingOn) {
                    char        buff[512];
                    sprintf(buff,
                            "CC Received:\t%d\t%d\t%d\r\n",
                            m.channel,
                            m.data[0],
                            m.data[1]);
                    pod.seed.usb_handle.TransmitInternal((uint8_t *)buff, strlen(buff));
                }

                switch(cc.control_number)
                {
                    case 1:  engine.modwheel = (float)(cc.value) / 127.0f; break;
                    case 16: engine.envToHarmLevel = cc.value > 0; break;
                    case 17: engine.envToOddEven = cc.value > 0; break;
                    case 18: engine.envToLowHigh = cc.value > 0; break;
                    case 19: engine.envToBitcrush = cc.value > 0; break;
                    case 20: engine.modwheelToHarmLevel = cc.value > 0; break;
                    case 21: engine.modwheelToOddEven = cc.value > 0; break;
                    case 22: engine.modwheelToHarmLowHigh = cc.value > 0; break;
                    case 23: engine.modwheelToBitcrush = cc.value > 0; break;
                    case 24: engine.lfoToHarmLevelSelection = cc.value > 0; break;
                    case 25: engine.lfoToOddEvenSelection = cc.value > 0; break;
                    case 26: engine.lfoToHarmLowHighSelection = cc.value > 0; break;
                    case 27: engine.lfoToBitcrushSelection = cc.value > 0; break;
                    case 28: engine.envToVol = cc.value > 0; break;
                    case 29: engine.modwheelToVol = cc.value > 0; break;
                    case 30: engine.lfoToVolSelection = cc.value > 0; break;
                    case 52: adjustEnvelopeToFilter = cc.value > 0; break;
                    case 53: adjustInharmonism = cc.value > 0; break;
                    case 54: adjustFilterFreq = cc.value > 0; break;
                    case 55: adjustFilterRez = cc.value > 0; break;
                    case 56: adjustDistortion = cc.value > 0; break;
                    case 35: engine.harmLevelCV = (float)(cc.value) / 127.f;  break; // E1
                    case 41: engine.harmOddEvenCV = (float)(cc.value) / 127.f; break; // E2
                    case 44: engine.harmLowHighCV = (float)(cc.value) / 127.f; break; // E3
                    case 45: {
                        if (cc.value < 32) {  // E4
                            engine.bitcrushCV = 0.75f * (float)(cc.value) / 32.f;
                        } else {
                            engine.bitcrushCV = 0.75f + 0.25f * (float)(cc.value - 32) / 95.f; break;
                        }
                    }
                    break;
                    case 62: engine.adsrValues.attack = (float)(cc.value) / 127.f + 0.001f; break; // F1
                    case 63: engine.adsrValues.decay = (float)(cc.value) / 127.f + 0.001f; break; // F2
                    case 75: engine.adsrValues.sustain = (float)(cc.value) / 127.f; break; // F3
                    case 76: engine.adsrValues.release = (float)(cc.value) / 127.f + 0.001f; break; // F4
                    case 77: // F5
                        {
                            if (adjustDistortion) {
                                engine.distortionCV = (float)(cc.value) / 127.f;
                            }
                            if (adjustFilterFreq) {
                                engine.filterFreqCV = (float)(cc.value) / 127.f;
                            }
                            if (adjustFilterRez) {
                                engine.filterRezCV = (float)(cc.value) / 127.f;
                                if (engine.filterRezCV > 0.8f) {
                                    engine.filterRezCV = 0.8f;
                                }
                            }
                            if (adjustInharmonism) {
                                engine.inharmonism = (float)(cc.value) / 127.f;
                            }
                            if (adjustEnvelopeToFilter) {
                                engine.envelopeToFilterAmount = (float)(cc.value) / 127.f;
                            }
                        }
                        break;
                    case 31:
                        {
                            for ( int i = 0; i < POLYPHONY; i++) {
                                engine.voices[i].on = false;
                            }
                            engine.envToVol = false;
                            engine.envToOddEven = false;
                            engine.envToLowHigh = false;
                            engine.envToHarmLevel = false;
                            engine.envToBitcrush = false;
                            engine.lfoToVolSelection = false;
                            engine.lfoToHarmLevelSelection = false;
                            engine.lfoToOddEvenSelection = false;
                            engine.lfoToHarmLowHighSelection = false;
                            engine.lfoToBitcrushSelection = false;
                            engine.modwheelToVol = false;
                            engine.modwheelToHarmLevel = false;
                            engine.modwheelToOddEven = false;
                            engine.modwheelToHarmLowHigh = false;
                            engine.modwheelToBitcrush = false;
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

                if (pb.value >= 0) {
                    engine.pitchBend = 1.f + ((float)(pb.value) / 8191.f);
                } else {
                    engine.pitchBend = 1.f + (((float)(pb.value) / 8192.f) * 0.5f); 
                }
                
            }
            break;
            default:
            break;
        }
    }
};