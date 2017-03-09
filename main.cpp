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

    enum currentSimulationEnum {SIM_OTHER, SIM_FEW_NEURONS, SIM_300_NEURONS};
    const currentSimulationEnum currentSimulation = SIM_FEW_NEURONS;

    switch (currentSimulation){
        case SIM_FEW_NEURONS:{
            NeuCor brain(0);


            brain.runAll = true;
            brain.runSpeed = 0.01;

            coord3 neuronPositions[] = {{0,0,0}, {-0.55,-0.5,0}, {0.52,-0.5,0}};
            for (auto &n: neuronPositions) brain.createNeuron(n);

            brain.createSynapse(0, 1, 0.5f); brain.createSynapse(0, 2, 0.5f);
            brain.createSynapse(1, 0, -0.5f); brain.createSynapse(2, 0, 0.5f);

            coord3 inputPositions[] = {neuronPositions[1], neuronPositions[2]};
            float inputRadius[] = {0.1, 0.1};
            float inputs[] = {200, 200};
            brain.setInputRateArray(inputs, sizeof(inputs)/sizeof(float), inputPositions, inputRadius);

            NeuCor_Renderer brainRenderer(&brain);
            brainRenderer.realRunspeed = false;
            brainRenderer.runBrainOnUpdate = true;
            brainRenderer.setDestructCallback(windowDestroy);

            std::cout<<"Starting program loop\n";
            while (!windowDestroyed) {
                brainRenderer.pollWindow();
                brainRenderer.updateView();
            };
            break;
        }

        case SIM_300_NEURONS:{
            NeuCor brain(300);
            NeuCor_Renderer brainRenderer(&brain);
            brainRenderer.runBrainOnUpdate = true;
            brainRenderer.realRunspeed = false;
            brainRenderer.setDestructCallback(windowDestroy);

            brain.runAll = true;
            brain.runSpeed = 0.025;


            float inputs[] = {(float) rand()/RAND_MAX*200.f, (float) rand()/RAND_MAX*200.f, (float) rand()/RAND_MAX*200.f};
            brain.setInputRateArray(inputs, sizeof(inputs)/sizeof(float));

            std::cout<<"Starting program loop\n";
            while (!windowDestroyed) {
                for (auto &i: inputs){ // Constantly change inputs
                    i += ((float) rand()/RAND_MAX-0.5f)*2.f;
                    i = std::min(std::max((double) i, 0.0), 220.0);
                }

                brainRenderer.pollWindow();
                brainRenderer.updateView();
            };

            break;
        }

        default: {

            NeuCor brain(500);
            NeuCor_Renderer brainRenderer(&brain);
            brainRenderer.runBrainOnUpdate = true;
            brainRenderer.realRunspeed = false;
            brainRenderer.setDestructCallback(windowDestroy);

            brain.runAll = true;
            brain.runSpeed = 0.05;


            float inputs[] = {0, 0, 200};
            coord3 inputPositions[] = {{1,0,0},{-2,0,0}, {-0.5, 1.5, 0}};
            float inputRadius[] = {0.75, 1.0, 0.75};
            brain.setInputRateArray(inputs, sizeof(inputs)/sizeof(float), inputPositions, inputRadius);

            std::cout<<"Starting program loop\n";
            unsigned t = 0;
            while (!windowDestroyed) {
                t++;
                if (false) brain.run();

                if (sin(brain.getTime()/50.0) > 0.0){
                    inputs[2] = 200;
                }
                else {
                    inputs[2] = 0;
                }

                brainRenderer.pollWindow();
                brainRenderer.updateView();
            };

            break;
        }
    }
    return 0;
}
