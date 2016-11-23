#include <NeuCor.h>
#include <NeuCor_Renderer.h>

#include <stdlib.h>
#include <time.h>

#include <iostream>

bool windowDestroyed = false;
void windowDestroy() {
    windowDestroyed = true;
}

int main(int argc, char* args[]){
    #define USE_RUNSPEED true
    #define SEED 6542

    srand(time(NULL));
    if (SEED != -1) srand(SEED);

    NeuCor brain(500);/*
    coord3 d;
    d.x = 0;
    d.y = 0;
    d.z = 0;
    brain.createNeuron(d);
    d.x = 1;
    d.y = 0;
    d.z = 0;
    brain.createNeuron(d);
    brain.makeConnections();*/

    NeuCor_Renderer brainRenderer(&brain);
    if (!USE_RUNSPEED) brainRenderer.setRunRate(0.005);
    brainRenderer.setDestructCallback(windowDestroy);

    brain.runAll = true;
    if (USE_RUNSPEED) brain.runSpeed = 0.06;


    float inputs[] = {2.0,8.0};
    brain.setInputRateArray(inputs, 2);

    std::cout<<"Starting program loop\n";
    unsigned t = 0;
    do {
        t++;
        if (USE_RUNSPEED) brain.run();
        brainRenderer.pollWindow();
        brainRenderer.updateView();
    } while (!windowDestroyed);

    return 0;
}
