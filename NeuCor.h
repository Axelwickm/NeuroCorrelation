#ifndef NEUCOR_H
#define NEUCOR_H

#include <vector>
#include <array>
#include <math.h>
#include <tuple>
#include <memory>


/*
    List of pointers to objects in vectors:
        simulationQueAdresses
*/
struct coord3 {
    float x,y,z;

   float getDist(coord3 c){
        return sqrtf(powf(x-c.x,2)+powf(y-c.y,2)+powf(z-c.z,2));
    }
    void setNAN(){
        x = NAN;
        y = NAN;
        z = NAN;
    }
};

typedef std::array<std::size_t, 2> synapseID;

class simulator;
class Neuron;
class Synapse;

class NeuCor {
    public:
        NeuCor(int n_neurons);
        ~NeuCor();

        void run();

        std::vector<coord3> positions;
        std::vector<float> potAct;


        void createNeuron(coord3 position);
        void makeConnections();
    protected:
        friend class simulator;
        friend class Neuron;
        friend class Synapse;
        friend class NeuCor_Renderer;

        void queSimulation(simulator* s, float time);
        float getTime();

        std::tuple<std::size_t, std::size_t> registerNeuron(coord3 pos, float potential, float activity);
        std::size_t getFreeID();

        std::vector<Neuron> neurons;
        Neuron* getNeuron(std::size_t ID);
        Synapse* getSynapse(synapseID);
        Synapse* getSynapse(std::size_t fromID, std::size_t toID);

        void deleteSynapse(std::size_t fromID, std::size_t toID);
        void deleteNeuron(std::size_t ID);

        //bool __banSynDereg; // Really ugly way to make sure Synapse data isn't de-registered when vector data is moved.
    private:
        std::vector<float> simulationQueTime;
        std::vector<simulator*> simulationQueAdresses;

        std::vector<std::size_t> freeNeuronIDs;

        float currentTime = 0.0;

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
        std::vector<synapseID> inSynapses; // Contains id of post synaptic neuron.

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
        float getPotential();

        std::size_t pN;
        std::size_t tN;
        float lastSpikeArrival;
    private:
        float length;
        float strength;

        float potential;
};

#endif // NEUCOR_H
