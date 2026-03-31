#include "NeuCor.h"
#include "NeuCor_Renderer.h"

#include <algorithm>
#include <exception>
#include <functional>
#include <memory>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif


bool windowDestroyed = false;
void windowDestroy() {
    windowDestroyed = true;
}

#ifdef __EMSCRIPTEN__
extern "C" {
EMSCRIPTEN_KEEPALIVE void neurocorrelation_request_shutdown() {
    windowDestroy();
}
}
#endif

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
    float randomRate() {
        return (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 75.0f;
    }

    struct SimulationSession {
        std::unique_ptr<NeuCor> brain;
        std::unique_ptr<NeuCor_Renderer> renderer;
        std::vector<float> inputs;
        std::vector<float> inputRadius;
        std::vector<coord3> inputPositions;
        std::function<void(SimulationSession&)> onFrame;

        void tick() {
            if (onFrame) onFrame(*this);
            renderer->pollWindow();
            if (!windowDestroyed) renderer->updateView();
        }
    };

    std::unique_ptr<SimulationSession> createSession(std::unique_ptr<NeuCor> brain) {
        windowDestroyed = false;

        std::unique_ptr<SimulationSession> session(new SimulationSession());
        session->brain = std::move(brain);
        session->renderer.reset(new NeuCor_Renderer(session->brain.get()));
        session->renderer->setDestructCallback(windowDestroy);
        return session;
    }

    void runSession(std::unique_ptr<SimulationSession> session) {
        std::cout<<"Starting program loop\n";
        while (!windowDestroyed) {
            session->tick();
        }
    }

    std::unique_ptr<SimulationSession> buildStandard() {
        std::unique_ptr<SimulationSession> session = createSession(std::unique_ptr<NeuCor>(new NeuCor(750)));
        session->renderer->runBrainOnUpdate = true;
        session->renderer->realRunspeed = true;
        session->brain->runSpeed = 4;

        session->inputs = {randomRate(), randomRate(), randomRate()};
        session->inputRadius = {0.8f, 0.8f, 0.8f};
        session->inputPositions = {
            {cosf(0.0f) * 2.0f, sinf(0.0f) * 2.0f, 0.0f},
            {cosf(2.0944f) * 2.0f, sinf(2.0944f) * 2.0f, 0.0f},
            {cosf(4.1888f) * 2.0f, sinf(4.1888f) * 2.0f, 0.0f},
        };
        session->brain->setInputRateArray(
            session->inputs.data(),
            session->inputs.size(),
            session->inputPositions.data(),
            session->inputRadius.data()
        );

        session->onFrame = [](SimulationSession& state) {
            for (float& input : state.inputs) {
                input += ((static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 0.5f) * 2.0f;
                input = std::clamp(input, 0.0f, 75.0f);
            }
            state.inputs[1] = state.inputs[0];

            if (10000.0f < state.brain->getTime()) {
                state.brain->learningRate = 0;
                state.inputs[0] = 0.0f;
                state.inputs[1] = 0.0f;
                state.inputs[2] = 0.0f;

                if (10800.0f < state.brain->getTime()) {
                    state.inputs[0] = 50.0f;
                    state.inputs[1] = 50.0f;
                }
                else if (10200.0f < state.brain->getTime() && state.brain->getTime() <= 10600.0f) {
                    state.inputs[2] = 50.0f;
                }
            }
        };

        return session;
    }

    std::unique_ptr<SimulationSession> buildUserInput() {
        int n_neurons = 100;
        int n_inputs = 1;
        int inputLinks = 0;
        { // Getting user input
            std::cout<<"Number of neurons:\n";
            std::cin>>n_neurons;
            std::cout<<"Number of inputs:\n";
            std::cin>>n_inputs;
            std::cout<<"Number of input links (how many inputs have the same frequency):\n";
            std::cin>>inputLinks;
            inputLinks = std::min(n_inputs / 2, inputLinks);
        }

        std::unique_ptr<SimulationSession> session = createSession(std::unique_ptr<NeuCor>(new NeuCor(n_neurons)));
        session->brain->runAll = true;
        session->brain->runSpeed = 0.02f;
        session->renderer->realRunspeed = false;
        session->renderer->runBrainOnUpdate = true;

        session->inputs.resize(n_inputs);
        for (float& value : session->inputs){
            value = static_cast<float>(rand() % 60);
        }
        session->brain->setInputRateArray(session->inputs.data(), session->inputs.size());
        session->brain->setDetectors(1);

        session->onFrame = [inputLinks](SimulationSession& state) {
            for (int i = 0; i < inputLinks; ++i) {
                state.inputs[i * 2 + 1] = state.inputs[i * 2];
            }
        };

        return session;
    }

    std::unique_ptr<SimulationSession> buildFewNeurons() {
        std::unique_ptr<SimulationSession> session = createSession(std::unique_ptr<NeuCor>(new NeuCor(0)));
        session->brain->runAll = true;
        session->brain->runSpeed = 0.02f;
        session->renderer->realRunspeed = false;
        session->renderer->runBrainOnUpdate = true;

        const std::vector<coord3> neuronPositions = {{0, 0, 0}, {0.3f, 0.3f, 0}, {0.3f, -0.3f, 0}};
        for (const coord3& neuronPosition : neuronPositions) {
            session->brain->createNeuron(neuronPosition);
        }
        session->brain->createSynapse(1, 0, 0.5f);
        session->brain->createSynapse(2, 0, 0.5f);

        session->inputs = {50.0f, 50.0f, 50.0f};
        session->inputPositions.assign(neuronPositions.begin(), neuronPositions.end());
        session->inputRadius = {0.1f, 0.1f, 0.1f};
        session->brain->setInputRateArray(
            session->inputs.data(),
            session->inputs.size(),
            session->inputPositions.data(),
            session->inputRadius.data()
        );
        session->brain->addInputOffset(1, 2.0f);
        session->brain->addInputOffset(2, -2.0f);

        return session;
    }

    std::unique_ptr<SimulationSession> buildOneInput() {
        std::unique_ptr<SimulationSession> session = createSession(std::unique_ptr<NeuCor>(new NeuCor(750)));
        session->renderer->runBrainOnUpdate = true;
        session->renderer->realRunspeed = true;

        session->brain->runAll = false;
        session->brain->runSpeed = 4.0f;

        session->inputs = {35.0f};
        session->inputRadius = {0.8f};
        session->inputPositions = {{2.0f, 0.0f, 0.0f}};
        session->brain->setInputRateArray(
            session->inputs.data(),
            session->inputs.size(),
            session->inputPositions.data(),
            session->inputRadius.data()
        );

        return session;
    }

    void standard(){
        runSession(buildStandard());
    }

    void user_input(){
#ifdef __EMSCRIPTEN__
        std::cerr<<"USER_INPUT is not supported in the browser build. Falling back to STANDARD.\n";
        runSession(buildStandard());
#else
        runSession(buildUserInput());
#endif
    }

    void few_neurons(){
        runSession(buildFewNeurons());
    }

    void one_input(){
        runSession(buildOneInput());
    }

}

#ifdef __EMSCRIPTEN__
namespace {
    std::unique_ptr<SIMULATIONS::SimulationSession> g_browserSession;

    void browserFrame() {
        if (!g_browserSession || windowDestroyed) {
            emscripten_cancel_main_loop();
            g_browserSession.reset();
            return;
        }
        try {
            g_browserSession->tick();
        }
        catch (const std::exception& error) {
            std::cerr << "Browser frame exception: " << error.what() << '\n';
            windowDestroyed = true;
        }
    }
}
#endif

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
#ifdef __EMSCRIPTEN__
    if (simulation == "USER_INPUT") {
        std::cerr<<"USER_INPUT is not supported in the browser build. Falling back to STANDARD.\n";
        simulation = "STANDARD";
    }

    if (simulation == "STANDARD"){
        g_browserSession = SIMULATIONS::buildStandard();
    }
    else if (simulation == "FEW_NEURONS"){
        g_browserSession = SIMULATIONS::buildFewNeurons();
    }
    else if (simulation == "ONE_INPUT"){
        g_browserSession = SIMULATIONS::buildOneInput();
    }
    else {
        fprintf(stderr, "Simulation not found\n");
        return 1;
    }

    std::cout<<"Starting program loop\n";
    emscripten_set_main_loop(browserFrame, 0, true);
    return 0;
#else
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
#endif
}
