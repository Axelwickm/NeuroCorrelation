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
    #define SEED -1
    unsigned seed = SEED;
    if (SEED == -1) seed = time(NULL);
    srand(seed);
    std::cout<<"Current seed: "<<seed<<std::endl;

    NeuCor brain(500);

/*
    for (float xpos = 0; xpos<8; xpos++){
        coord3 d;
        if (xpos == 0) d = {0.5, 0, 0};
        if (xpos == 1) d = {0, 0.5, 0};
        if (xpos == 2) d = {0, 0, 0.5};
        if (xpos == 3) d = {0, 0, 0.6};
        if (xpos == 4) d = {0, 0, 0.8};
        if (xpos == 5) d = {0, 0, 0.7};
        if (xpos == 5) d = {0.6, 0, 0};
        if (xpos == 6) d = {0.4, 0.1, 0};
        if (xpos == 7) d = {0.4, 0.1, 0};
        brain.createNeuron(d);
    }
    brain.makeConnections();
*/

    NeuCor_Renderer brainRenderer(&brain);
    brainRenderer.runBrainOnUpdate = true;
    brainRenderer.realRunspeed = false;
    brainRenderer.setDestructCallback(windowDestroy);

    brain.runAll = true;
    brain.runSpeed = 0.05;


    float inputs[] = {0, 0, 200};
    coord3 inputPositions[] = {{1,0,0},{-2,0,0}, {-0.5, 1.5, 0}};
    brain.setInputRateArray(inputs, sizeof(inputs)/sizeof(float), inputPositions);

    std::cout<<"Starting program loop\n";
    unsigned t = 0;
    do {
        t++;
        if (false) brain.run();

        //if (brain.getTime() > 20){inputs[0] = 0; inputs[1] = 0;}
        /*if (sin(brain.getTime()/30.0) > 0.0){
            inputs[0] = 100; inputs[1] = 100;
        }
        else {
            inputs[0] = 0; inputs[1] = 0;
        }*/
        if (brain.getTime()>2000.0) inputs[0] = 200;
        if (brain.getTime()>2000.0) inputs[2] = 100;
        //if (rand()%100 == 0) inputs[2] = rand()%200;
        //std::cout<<inputs[0]<<std::endl;

        brainRenderer.pollWindow();
        brainRenderer.updateView();
    } while (!windowDestroyed);

    return 0;
}
