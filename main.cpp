#include "NeuCor.h"
#include "NeuCor_Renderer.h"

#include <stdlib.h>
#include <time.h>
#include <iostream>


bool windowDestroyed = false;
void windowDestroy() {
    windowDestroyed = true;
}

void showUsage(){
    printf("Neuro Correlation usage: "
           "[--help] [--seed <value>] <simulation preset>"
           "\nThe following are the preset simulations:\n"
           "\tSTANDARD - (default) Creates 750 neurons, 3 inputs (2 of them linked)\n"
           "\tUSER_INPUT - Prompts about number of neurons, inputs, and linked inputs\n"
           "\tFEW_NEURONS - Creates only a few connected neurons\n"
           "\tONE_INPUT - Creates 750 neurons, and 1 input\n"
           );
}


namespace SIMULATIONS {

    void standard(){
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
    }

    void user_input(){
        int n_neurons = 100; int n_inputs = 1; int inputLinks = 0;
        { // Getting user input
            std::cout<<"Number of neurons:\n";
            std::cin>>n_neurons;
            std::cout<<"Number of inputs:\n";
            std::cin>>n_inputs;
            std::cout<<"Number of input links (how many inputs have the same frequency):\n";
            std::cin>>inputLinks;
            inputLinks = min(n_inputs/2, inputLinks);
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
            //std::cout<<brain.getDetectorVoltages().at(0)<<std::endl;
        };
    }

    void few_neurons(){
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
    }

    void one_input(){
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

    }

}

int main(int argc, char* argv[]){
    unsigned seed = time(NULL);
    std::string simulation = "STANDARD";
    
    // Interpret arguments
    for (int i = 0; i < argc; i++){
        std::string arg = argv[i];
        if (arg == "--help") {
            showUsage();
            return 0;
        }
        else if (arg == "--seed"){
            if (i+1 == argc){
                fprintf(stderr, "Missing seed value\n");
                return 1;
            }
            seed = std::stoul(argv[i+1], nullptr, 0);
            i++;
        }
        else if (i != 0){
            simulation = arg;
        }

    }

    // Set seed
    srand(seed);
    printf("Using seed %u\n", seed);
    fflush(stdout);

    // Start simulation
    if (simulation == "STANDARD"){
        SIMULATIONS::standard();
    }
    else if (simulation == "USER_INPUT"){
        SIMULATIONS::user_input();
    }
    else if (simulation == "FEW_NEURONS"){
        SIMULATIONS::few_neurons();
    }
    else if (simulation == "ONE_INPUT"){
        SIMULATIONS::one_input();
    }
    else {
        fprintf(stderr, "Simulation not found\n");
        return 1;
    }

    return 0;
}
