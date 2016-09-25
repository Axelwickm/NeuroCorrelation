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
class Neuron;
class Synapse;

class NeuCor {
    public:
        NeuCor(int n_neurons);
        ~NeuCor();

        void run();
        float runSpeed;
        bool runAll;

        std::vector<coord3> positions;
        std::vector<float> potAct;


        void createNeuron(coord3 position);
        void makeConnections();
    protected:
        friend class simulator;
        friend class Neuron;
        friend class Synapse;
        friend class NeuCor_Renderer;

        void queSimulation(simulator* s, const float time);
        float getTime() const;

        std::tuple<std::size_t, std::size_t> registerNeuron(coord3 pos, float potential, float activity);
        std::size_t getFreeID() const;

        boost::container::stable_vector<Neuron> neurons;
        Neuron* getNeuron(std::size_t ID);
        Synapse* getSynapse(std::size_t toID, std::size_t fromID);

        void deleteSynapse(std::size_t toID, std::size_t fromID);
        void deleteNeuron(std::size_t ID);
    private:
        std::priority_queue<simulation, std::vector<simulation>, std::greater<simulation>> simulationQue;
        float currentTime = 0.0;

        std::vector<std::size_t> freeNeuronIDs;
};

typedef std::map<std::size_t, std::size_t> synCoordMap;

struct simulation {
    simulation(simulator* sim, float simTime): addr(sim), stime(simTime){};
    simulator* addr;
    float stime;
    bool operator>(const simulation &otherSim) const {return stime > otherSim.stime;};
};

class simulator {
    public:
        simulator(NeuCor* p);

        virtual void run() = 0;

        NeuCor* parentNet;

        bool exists() const;
        void exterminate();
    protected:
        friend class Neuron;
        friend class Synapse;
        float lastRan;
    private:
        bool deleted;
};

class Neuron: public simulator {
    public:
        Neuron(NeuCor* p, coord3 position);
        ~Neuron();
        Neuron& operator=(const Neuron& other);
        void makeConnections();

        void run();
        void fire();
        void transmission(float pot);

        /* These are indexes so that they can be allocated next to each other in a vector,
           thus making hardware buffering more efficient in the rendering engine. */
        std::size_t pos;
        std::size_t PA;
        std::vector<Synapse> outSynapses; //Owns synapses
        synCoordMap inSynapses; // Contains coordinates to synapse in order {from neuron, to neuron}

        void removeOutSyn(std::size_t synTo); // Removes given synapse from outSynapses
        void removeInSyn(std::size_t synFrom); // Removes given synapse from inSynapses

        coord3 position() const;
        void setPosition(coord3 newPos);
        float potential() const;
        void setPotential(float p);
        float activity() const;

        float trace, lastFire;

    private:
        std::size_t const ownID;
        // Neuron function variables:

        float baselevel;
        float vesicles, reuptake, buffer;

        void charge_passive(float deltaT);
        void charge_thresholdCheck(float deltaT);

        void vesicles_uptake(float deltaT);
};

class Synapse: public simulator {
    public:
        Synapse(NeuCor* p, std::size_t parent, std::size_t target);
        Synapse(const Synapse &other);
        Synapse& operator=(const Synapse &other);
        ~Synapse();

        void run();
        void fire();
    protected:
        friend class NeuCor;
        friend class Neuron;
        friend class NeuCor_Renderer;

        void targetFire();
        float getPotential() const;

        std::size_t pN;
        std::size_t tN;
        float lastSpikeArrival;
    private:
        float length;
        float strength;

        float potential;
};
#endif // NEUCOR_H
