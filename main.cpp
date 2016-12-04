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
    #define SEED 8423
    srand(time(NULL));
    if (SEED != -1) srand(SEED);

    NeuCor brain(500);

    /*
    for (float xpos = 0; xpos<30; xpos += 25.0/30.0){
        break;
        coord3 d;
        d.x = xpos;
        d.y = (float) rand()/RAND_MAX*0.08;
        d.z = 0;
        brain.createNeuron(d);
    }
    brain.makeConnections();
    */

    NeuCor_Renderer brainRenderer(&brain);
    if (!USE_RUNSPEED) brainRenderer.setRunRate(0.1);
    brainRenderer.setDestructCallback(windowDestroy);

    brain.runAll = true;
    if (USE_RUNSPEED) brain.runSpeed = 0.05;


    float inputs[] = {100.0};
    brain.setInputRateArray(inputs, 1);

    std::cout<<"Starting program loop\n";
    unsigned t = 0;
    do {
        t++;
        if (USE_RUNSPEED) brain.run();

        if (sin(brain.getTime()/40.0) > 0.0) inputs[0] = 250;
        else inputs[0] = 0;
        //std::cout<<inputs[0]<<std::endl;

        brainRenderer.pollWindow();
        brainRenderer.updateView();
    } while (!windowDestroyed);

    return 0;
}
