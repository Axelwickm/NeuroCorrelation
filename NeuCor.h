#ifndef NEUCOR_H
#define NEUCOR_H

#include <vector>
#include <boost/container/stable_vector.hpp>
#include <map>
#include <queue>
#include <array>
#include <math.h>
#include <tuple>

struct coord3 {
    float x,y,z;

   float getDist(coord3 c) const {
        return sqrtf(powf(x-c.x,2)+powf(y-c.y,2)+powf(z-c.z,2));
    }
    void setNAN(){
        x = NAN;
        y = NAN;
        z = NAN;
    }
};

struct simulation;
class simulator;
class InputFirer;
class Neuron;
class Synapse;

class NeuCor {
    public:
        NeuCor(int n_neurons);
        ~NeuCor();

        void run();
        float runSpeed;
        bool runAll;
        float getTime() const;
        float learningRate, preSynapticTraceDecay, postSynapticTraceDecay;

        void setInputRateArray(float inputs[], unsigned inputCount, coord3 inputPositions[] = {NULL});

        void createNeuron(coord3 position);
        void makeConnections();

        void printSynapseWeightDist() const;
    protected:
        friend class simulator;
        friend class Neuron;
        friend class Synapse;
        friend class InputFirer;
        friend class NeuCor_Renderer;

        void queSimulation(simulator* s, const float time);

        std::vector<coord3> positions;
        std::vector<float> potAct;
        void resetActivities();

        std::tuple<std::size_t, std::size_t> registerNeuron(coord3 pos, float potential, float activity);
        std::size_t getFreeID() const;

        float* inputArray;
        std::vector<InputFirer> inputHandler;

        boost::container::stable_vector<Neuron> neurons;
        Neuron* getNeuron(std::size_t ID);
        Synapse* getSynapse(std::size_t toID, std::size_t fromID);
        Synapse* getSynapse(std::pair<std::size_t, std::size_t> ID);
        void queFlip(std::pair<std::size_t, std::size_t>);

        void deleteSynapse(std::size_t toID, std::size_t fromID);
        void deleteNeuron(std::size_t ID);
    private:
        std::priority_queue<simulation, std::vector<simulation>, std::greater<simulation>> simulationQue;
        float currentTime = 0.0;

        unsigned inputArraySize;
        unsigned totalGenNeurons;
        std::vector<std::size_t> freeNeuronIDs;
        std::vector<std::pair<std::size_t, std::size_t> > synapseFlippingQue;
};

struct simulation {
    simulation(simulator* sim, float simTime): addr(sim), stime(simTime){};
    simulator* addr;
    float stime;
    bool operator>(const simulation &otherSim) const {return stime > otherSim.stime;};
};

class simulator {
    public:
        simulator(NeuCor* p);
        NeuCor* parentNet;

        virtual void run() = 0;
    protected:
        friend class Neuron;
        friend class Synapse;
        float lastRan;
};
struct deletedSimulator: public simulator {
    deletedSimulator(NeuCor* p);
    void run(){}; // Run-function which does nothing
};

struct InputFirer: public simulator {
    InputFirer(NeuCor* p, unsigned i, coord3 position = {NAN,NAN,NAN});
    coord3 a;
    coord3 b;

    void run();
    void schedule(float deltaT, float frequency);
    float lastFire;
    const unsigned index;
    std::vector<std::size_t> near;
    float radius;
};

class Neuron: public simulator {
    public:
        Neuron(NeuCor* p, coord3 position);
        ~Neuron();
        Neuron& operator=(const Neuron& other);
        void makeConnections();

        void run();
        void fire();
        void transfer();
        void givePotential(float pot);

        /* These are indexes so that they can be allocated next to each other in a vector,
           thus making hardware buffering more efficient in the rendering engine. */
        std::size_t pos;
        std::size_t PA;
        std::vector<Synapse> outSynapses; //Owns synapses
        std::map<std::size_t, std::size_t> inSynapses; // Contains coordinates to synapse in order {from neuron, to neuron}

        void removeOutSyn(std::size_t synTo); // Removes given synapse from outSynapses
        void removeInSyn(std::size_t synFrom); // Removes given synapse from inSynapses

        coord3 position() const;
        void setPosition(coord3 newPos);
        float potential() const;
        void setPotential(float p);
        float activity() const;
        void setActivity(float a);
        void resetActivity();
        std::size_t getID() const;

        float trace, lastFire;

    private:
        std::size_t const ownID;
        // Neuron function variables:

        float baselevel, threshold, recharge;
        float vesicles, reuptake, buffer;
        float AP_h, AP_depolW, AP_polW, AP_deltaPol, AP_depolFac, AP_deltaStart;
        float AP_cutoff;
        const float traceDecayRate;

        float activityStartTime;
        unsigned firings;

        void charge_passive(float deltaT, float currentT);
        void charge_thresholdCheck(float deltaT, float currentT);

        void vesicles_uptake(float deltaT);

        void AP(float currentT);
};

class Synapse: public simulator {
    public:
        Synapse(NeuCor* p, std::size_t parent, std::size_t target);
        Synapse(const Synapse &other);
        Synapse& operator=(const Synapse &other);
        ~Synapse();

        void run();
        void fire(float polW, float depolFac, float deltaStart);
    protected:
        friend class NeuCor;
        friend class Neuron;
        friend class NeuCor_Renderer;

        void synapticPlasticity();

        float getPrePot() const;
        float getPostPot() const;
        float getWeight() const;

        void flipDirection();

        std::size_t pN;
        std::size_t tN;
        float lastSpikeStart;
        float lastSpikeArrival;

        float AP_polW, AP_depolFac, AP_deltaStart, AP_fireTime;
        float AP_speed;
        const float traceDecayRate;
    private:
        float length;
        float weight;
};
#endif // NEUCOR_H
