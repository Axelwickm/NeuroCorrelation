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
    brainRenderer.runBrainOnUpdate = true;
    brainRenderer.realRunspeed = false;
    brainRenderer.setDestructCallback(windowDestroy);

    brain.runAll = true;
    brain.runSpeed = 0.05;


    float inputs[] = {250, 250};
    coord3 inputPositions[] = {{2,0,0},{-2,0,0}};
    brain.setInputRateArray(inputs, sizeof(inputs)/sizeof(float), inputPositions);

    std::cout<<"Starting program loop\n";
    unsigned t = 0;
    do {
        t++;
        if (false) brain.run();

        if (brain.getTime() > 20){inputs[0] = 0; inputs[1] = 0;}
        else if (sin(brain.getTime()/10.0) > 0.0) inputs[0] = 250;
        else if (sin(brain.getTime()/10.0) > 0.0) inputs[1] = 250;
        else inputs[0] = 0;
        //std::cout<<inputs[0]<<std::endl;

        brainRenderer.pollWindow();
        brainRenderer.updateView();
    } while (!windowDestroyed);

    return 0;
}
