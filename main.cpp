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
    srand(time(NULL));
    srand(6542);

    NeuCor brain(120);/*
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
    brainRenderer.setRunRate(0.01);
    brainRenderer.setDestructCallback(windowDestroy);

    brain.runAll = true;

    float inputs[] = {2.0,8.0};
    brain.setInputRateArray(inputs, 2);

    std::cout<<"Starting program loop\n";
    unsigned t = 0;
    do {
        std::cout<<t<<std::endl;
        t++;
        brainRenderer.pollWindow();
        brainRenderer.updateView();

        if (1000==t){
            inputs[0] = 0;
            inputs[1] = 0;
        }
    } while (!windowDestroyed);

    return 0;
}
