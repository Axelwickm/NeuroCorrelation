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
    brainRenderer.setDestructCallback(windowDestroy);

    brain.runSpeed = 0.3;
    brain.runAll = true;

    std::cout<<"Starting program loop\n";
    unsigned t = 0;
    do {
        if (t%1 == 0)
            brain.run();
        t++;
        brainRenderer.pollWindow();
        brainRenderer.updateView();
    } while (!windowDestroyed);

    return 0;
}
