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

    enum currentSimulationEnum {SIM_OTHER, SIM_USER_INPUT, SIM_FEW_NEURONS, SIM_750_NEURONS_3_IN, SIM_750_NEURONS_1_IN};
    const currentSimulationEnum currentSimulation = SIM_USER_INPUT;

    switch (currentSimulation){
        case SIM_USER_INPUT:{
            int n_neurons = 100; int n_inputs = 1; int inputLinks = 0;
            { // Getting user input
                std::cout<<"Number of neurons:\n";
                std::cin>>n_neurons;
                std::cout<<"Number of inputs:\n";
                std::cin>>n_inputs;
                /*std::cout<<"Number of input links (how many inputs have the same frequency):\n";
                std::cin>>inputLinks;
                inputLinks = min(n_inputs/2, inputLinks);*/
            }

            NeuCor brain(n_neurons);

            brain.runAll = true;
            brain.runSpeed = 0.02;


            float inputs[n_inputs];
            for (auto &val: inputs){
                val = rand()%60;
            }
            brain.setInputRateArray(inputs, sizeof(inputs)/sizeof(float));
            brain.setDetectors(1);

            NeuCor_Renderer brainRenderer(&brain);
            brainRenderer.realRunspeed = false;
            brainRenderer.runBrainOnUpdate = true;
            brainRenderer.setDestructCallback(windowDestroy);

            std::cout<<"Starting program loop\n";
            while (!windowDestroyed) {
                brainRenderer.pollWindow();
                brainRenderer.updateView();
                for (int i = 0; i<inputLinks; i++) inputs[i*2+1] = inputs[i*2];
                std::cout<<brain.getDetectorVoltages().at(0)<<std::endl;
            };
            break;
        }

        case SIM_FEW_NEURONS:{
            NeuCor brain(0);


            brain.runAll = true;
            brain.runSpeed = 0.02;

            coord3 neuronPositions[] = {{0, 0, 0}, {0.3, 0.3, 0}, {0.3, -0.3, 0}};
            for (auto &n: neuronPositions) brain.createNeuron(n);
            brain.createSynapse(1, 0, 0.5f);  brain.createSynapse(2, 0, 0.5);

            float inputs[] = {50, 50, 50};
            coord3 inputPositions[] = {neuronPositions[0], neuronPositions[1], neuronPositions[2]};
            float inputRadius[] = {0.1, 0.1, 0.1};
            brain.setInputRateArray(inputs, sizeof(inputs)/sizeof(float), inputPositions, inputRadius);
            brain.addInputOffset(1, 2.0); brain.addInputOffset(2, -2.0);

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

        case SIM_750_NEURONS_3_IN:{
            NeuCor brain(750);
            NeuCor_Renderer brainRenderer(&brain);
            brainRenderer.runBrainOnUpdate = true;
            brainRenderer.realRunspeed = false;
            brainRenderer.setDestructCallback(windowDestroy);

            brain.runSpeed = 1;

            float inputs[] = {(float) rand()/RAND_MAX*75.f, (float) rand()/RAND_MAX*75.f, (float) rand()/RAND_MAX*75.f}; // 3 inputs with random firing rate between 0 and 75 Hz
            float inputRadius[] = {0.8, 0.8, 0.8};
            coord3 inputPositions[] = {{cosf(0)*2.f,sinf(0)*2.f,0},{cosf(2.0944)*2.f,sinf(2.0944)*2.f,0}, {cosf(2.0944*2.f)*2.f,sinf(2.0944*2.f)*2.f,0}}; // Inputs in triangle
            brain.setInputRateArray(inputs, sizeof(inputs)/sizeof(float), inputPositions, inputRadius);

            std::cout<<"Starting program loop\n";
            while (!windowDestroyed) {
                for (auto &i: inputs){ // Constantly change inputs
                    i += ((float) rand()/RAND_MAX-0.5f)*2.f;
                    i = std::min(std::max((double) i, 0.0), 75.0);
                }
                inputs[1] = inputs[0]; // Input 1 = Input 0, they are correlated and should connect.

                if (10000.f < brain.getTime()){
                    brain.learningRate = 0;

                    inputs[0] = 0; inputs[1] = 0;
                    inputs[2] = 0;

                    if (10800.f < brain.getTime()){
                        inputs[0] = 50; inputs[1] = 50;
                        inputs[2] = 0;
                    }
                    else if (10600.f < brain.getTime()){
                        inputs[0] = 0; inputs[1] = 0;
                        inputs[2] = 0;
                    }
                    else if (10200.f < brain.getTime()){
                        inputs[0] = 0; inputs[1] = 0;
                        inputs[2] = 50;
                    }
                }

                brainRenderer.pollWindow();
                brainRenderer.updateView();
            };

            break;
        }

        case SIM_750_NEURONS_1_IN:{
            NeuCor brain(750);
            NeuCor_Renderer brainRenderer(&brain);
            brainRenderer.runBrainOnUpdate = true;
            brainRenderer.realRunspeed = false;
            brainRenderer.setDestructCallback(windowDestroy);

            brain.runAll = false;
            brain.runSpeed = 0.025;

            float inputs[] = {35.f}; // 1 input with random firing rate between 0 and 75 Hz
            float inputRadius[] = {0.8};
            coord3 inputPositions[] = {{2, 0, 0}};
            brain.setInputRateArray(inputs, sizeof(inputs)/sizeof(float), inputPositions, inputRadius);

            std::cout<<"Starting program loop\n";
            while (!windowDestroyed) {
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


            float inputs[] = {25};//, 0, 200};
            coord3 inputPositions[] = {{1,0,0},{-2,0,0}, {-0.5, 1.5, 0}};
            float inputRadius[] = {0.75, 1.0, 0.75};
            brain.setInputRateArray(inputs, sizeof(inputs)/sizeof(float), inputPositions, inputRadius);
            //brain.setInputRateArray(inputs, 1, inputPositions, inputRadius);

            std::cout<<"Starting program loop\n";
            unsigned t = 0;
            while (!windowDestroyed) {
                t++;
                if (false) brain.run();

                if (sin(brain.getTime()/50.0) > 0.0){
                    inputs[0] = 180;
                }
                else {
                    inputs[0] = 0;
                }

                brainRenderer.pollWindow();
                brainRenderer.updateView();
            };

            break;
        }
    }
    return 0;
}
