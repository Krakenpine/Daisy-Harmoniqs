#include "daisy.h"
#include "daisysp.h"
#include <cmath>

#include "Controller.h"
#include "PodController.h"

using namespace daisysp;
using namespace daisy;

#define BLOCK_SIZE 32

CpuLoadMeter meter;

Controller& controller = *(new PodController());

bool loggingOn = false;
bool meterOn = true;

void ConditionalParameter(float  oldVal,
                          float  newVal,
                          float &param,
                          float  update);

void Controls();

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    Controls();

    for(size_t i = 0; i < size; i += 2)
    {
        if (meterOn) { meter.OnBlockStart(); }

        // left out
        out[i] = controller.Process();

        // right out
        out[i + 1] = out[i];

        if (meterOn) { meter.OnBlockEnd(); }
    }
}


int main(void)
{
    //Init everything
    controller.Init((size_t) BLOCK_SIZE, loggingOn);

    System::Delay(250);

    controller.Start(AudioCallback);
    while(1) {
        controller.MainLoop();
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



void Controls()
{
    controller.Controls();
    controller.UpdateValues();
}
