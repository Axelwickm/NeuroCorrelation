#ifndef NEUCOR_H
#define NEUCOR_H

#include <vector>
#include <boost/container/stable_vector.hpp>
#include <map>
#include <queue>
#include <array>
#include <math.h>
#include <tuple>

// Simple 3D coordinate structure with distance to other coordinate function.
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

// Prototypes.
struct simulation;
class simulator;
class InputFirer;
class VoltageDetector;
class Neuron;
class Synapse;

// The main network class.
// This owns all the simulated objects, and is used to run them.
class NeuCor {
    public:
        NeuCor(int n_neurons);               // Number of initial neurons (n_neurons)
        ~NeuCor();

        void run();                          // Runs the whole simulation
        float runSpeed;                      // What timestep (in ms) is used when run() is called
        bool runAll;                         // If all the neurons should be updated, instead of only the necessary ones. Useful when rendering
        float getTime() const;               // The amount of time (in ms) that has been simulated
        float learningRate;                  // Used as a factor when synapse weight is changed
        float presynapticTraceDecay, postsynapticTraceDecay; // When a synapse (presynaptic) or neuron (postsynaptic) is fired a trace is left. This trace decays exponentially by these rates
        float presynapticFactor, postsynapticFactor;         // How much the trace variables are factored into the plasticity function

        // This is how input signals interface with the brain. Every given input has a position in the brain, which is used to determined what nearby neurons are fired.
        // Input rate is defined in Hz as floats. Since the memory address of the array (inputs) is what's stored, the brain will use the updated values automatically.
        // If the inputs positions are NULL, they are generated randomly and input radius is 1.0
        void setInputRateArray(float inputs[], unsigned inputCount, coord3 inputPositions[] = {NULL}, float inputRadius[] = {NULL});
        void addInputOffset(unsigned inputID, float t);    // Adds time offset (in ms) to a given input

        void setDetectors(unsigned detectorNumber, coord3 detectorPositions[] = {NULL}, float detectorRadius[] = {NULL});
        float getDetectorVoltage(unsigned ID);
        std::vector<float> getDetectorVoltages();

        void createNeuron(coord3 position);  // Creates neuron at given coordinates
        void createSynapse(std::size_t toID, std::size_t fromID, float weight);
        void makeConnections();              // Connects all neurons closer than 1 unit to each other.
    protected:
        friend class simulator;
        friend class Neuron;
        friend class Synapse;
        friend class InputFirer;
        friend class VoltageDetector;
        friend class NeuCor_Renderer;

        void queueSimulation(simulator* s, const float time); // Schedules calling run() of simulator s a given number of ms in the future

        // These are containers used to allocate important values next to each other in a vector,
        // thus making hardware buffering more efficient in the rendering engine.
        std::vector<coord3> positions;      // Stores all the positions of the neurons in order of IDs
        std::vector<float> potAct;          // Stores all neurons potentials and activities in order of ID's (potential, activity, potential, ...)
        void resetActivities();             // Resets the firings count and sets initial time to current time for all neurons

        std::tuple<std::size_t, std::size_t> registerNeuron(coord3 pos, float potential, float activity); // Registers neuron's coordinates, potential and activities in previous positions and potAct vectors
        std::size_t getFreeID() const;      // Returns ID which is isn't used any more because previous neurons with this ID was deleted

        float* inputArray;                   // Holds defined input rate array
        unsigned inputArraySize;
        std::vector<InputFirer> inputHandler;
        std::vector<VoltageDetector> voltageDetectors;

        boost::container::stable_vector<Neuron> neurons;                // Where all neurons are stored
        Neuron* getNeuron(std::size_t ID);
        Synapse* getSynapse(std::size_t toID, std::size_t fromID);
        Synapse* getSynapse(std::pair<std::size_t, std::size_t> ID);
        void queFlip(std::pair<std::size_t, std::size_t>);              // Queues an synapse to be flipped when possible

        void deleteSynapse(std::size_t toID, std::size_t fromID);
        void deleteNeuron(std::size_t ID);
    private:
        float currentTime = 0.0;             // Amount of simulated time

        // Holds vector of simulations, which store memory addresses of simulators and the times when they should be simulated (by calling their run() function).
        // The container is sorted so that the earliest upcoming run() call is first.
        // This assures that everything is simulated in the right order
        std::priority_queue<simulation, std::vector<simulation>, std::greater<simulation>> simulationQueue;

        unsigned totalGenNeurons;                                                // Used for determining neuron density at initial generation
        std::vector<std::size_t> freeNeuronIDs;                                  // Deleted neurons, where IDs can be reused
        std::vector<std::pair<std::size_t, std::size_t> > synapseFlippingQueue;  // Synapses which are to should be flipped
};

// Every timed a future run called is scheduled (queueSimulation()), and instance of this class is stored in the simulationQueue.
struct simulation {
    simulation(simulator* sim, float simTime): addr(sim), stime(simTime){};             // Initialises values in member initializer list
    simulator* addr;                                                                    // Memory address of simulator about to be run
    float stime;                                                                        // Scheduled time for simulator to be ran
    bool operator>(const simulation &otherSim) const {return stime > otherSim.stime;};  // Lets the simulationQueue sort elements with earliest time first
};

// Abstract class which can have future calls to run() function scheduled in simulation queue
class simulator {
    public:
        simulator(NeuCor* p);                // Needs pointer to parent network object (NeuCor)
        NeuCor* parentNet;

        virtual void run() = 0;              // Run-function which updates the simulator to the current time of parent network
        float lastRan;                       // Time of the current network when last ran
};

// UNUSED
// Empty simulator which does nothing when run is called
struct deletedSimulator: public simulator {
    deletedSimulator(NeuCor* p);
    void run(){}; // Run-function which does nothing
};


// Every input value in the parent network's input rate array is given an input firer
// Works to interface between user and neurons
// Schedule is called every time the brain's run function is called
struct InputFirer: public simulator {
    InputFirer(NeuCor* p, coord3 position = {NAN,NAN,NAN}, float radius = 1.0); // If positions are NAN, a random position is assigned
    bool enabled;
    coord3 a;                                               // Position
    float radius;
    std::vector<std::size_t> near;                          // IDs of all neurons closer than radius

    void schedule(float deltaT, float frequency);           // Schedules itself to be run at even intervals (frequency) for the next given ms (deltaT)
    float lastFire;                                         // The last scheduled time
    void run();                                             // Runs all neurons in near vector
};

struct VoltageDetector {
    NeuCor* parentNet;
    VoltageDetector(NeuCor* p, coord3 position = {NAN,NAN,NAN}, float radius = 1.0); // If positions are NAN, a random position is assigned

    coord3 a;                                               // Position
    float radius;
    std::vector<std::size_t> near;                          // IDs of all neurons closer than radius

    float getVoltage();
};

// Implements Neurons as simulator objects
class Neuron: public simulator {
    public:
        Neuron(NeuCor* p, coord3 position);  // Parent network pointer and position coordinate (random if NAN)
        ~Neuron();
        Neuron& operator=(const Neuron& other);

        void makeConnections();              // Creates connections to all neurons closer than 1 unit
        void run();                          // Updates the neuron to current simulation time
        void fire();                         // Initiates neuron firing sequence, increases activity, updates weight of both incoming and outgoing synapses
        void transfer();                     // Schedule simulation at maximum input voltage from synapses, thus firing if needed. Called by in-synapses
        void givePotential(float pot);       // Instantly adds given amount of potential
        void scheduleFire(float const time);    // Time in future that the neuron will fire

        float getTrace() const;              // Returns the amount of trace left after firing. Used for calculating synapse weight change

        std::size_t pos;
        std::size_t PA;
        std::vector<Synapse> outSynapses;               // Owns synapses
        std::map<std::size_t, std::size_t> inSynapses;  // Contains coordinates to synapse in order {from neuron, to neuron}

        void removeOutSyn(std::size_t synTo);           // Removes given synapse from outSynapses
        void removeInSyn(std::size_t synFrom);          // Removes given synapse from inSynapses

        coord3 position() const;
        void setPosition(coord3 newPos);
        float potential() const;
        void setPotential(float p);
        float activity() const;
        void setActivity(float a);
        void resetActivity();                           // Resets the firings count and sets initial time to current time
        std::size_t getID() const;

        float lastFire;                                 // Simulation time when neuron last fired

    private:
        std::size_t const ownID;                        // Global ID (index) in container

        // Neuron simulation variables:
        float baselevel, threshold, recharge;           // Base level standard voltage of neuron, voltage threshold needed to fire, exponential growth rate to base level
        float vesicles, reuptake, buffer;               // Vesicle amount, linear uptake, amount buffered. UNUSED.
        float AP_h, AP_depolW, AP_polW, AP_deltaPol, AP_depolFac, AP_deltaStart; // Defines form of action potential spike
        float AP_cutoff;                                // How long after last spike until next is allowed
        const float traceDecayRate;                     // Rate at which the trace variable decays after spike

        float activityStartTime;                        // ms simulation time
        unsigned firings;                               // Number of firings since activity start time started

        float scheduledFireTime;

        void charge_passive(float deltaT, float currentT);          // Exponential decay/growth towards base level
        void charge_thresholdCheck(float deltaT, float currentT);   // Checks if neuron should fire, and if so calls fire()
        void charge_insynapses(float deltaT, float currentT);       // Transfers in-synapses voltages to neurons voltage over time

        void vesicles_uptake(float deltaT);                         // Increases vesicle amount over time

        void AP(float currentT);                                    // Action potential sequence, which is used when to determine voltage when firing
};

// Implements Synapses as simulator objects.
// Named synapse instead of axon because it does the processing done by the synapse (and synaptic cleft) en vivo.
// Are stored in containers in their parent neurons.
class Synapse: public simulator {
    public:
        Synapse(NeuCor* p, std::size_t parent, std::size_t target); // ID of neuron where synapse comes from (parent), and where it goes (target)
        Synapse(const Synapse &other);
        Synapse& operator=(const Synapse &other);
        ~Synapse();

        void run();                                                 // Delivers voltage to target neuron at right time
        void fire(float polW, float depolFac, float deltaStart);    // Gets spike shape information (polarization width, depolarization factor, spike time offset). Schedules itself to run at delivery time

        float getWeight() const;
        void setWeight(float w);
    protected:
        friend class NeuCor;
        friend class Neuron;
        friend class NeuCor_Renderer;

        void synapticPlasticity();                      // Called when spike is delivered, and when parent neuron fires. Changes the weight of the synapse
        float averageSynapseTrace, averageNeuronTrace;
        unsigned synapticPlasticityCalls;

        float getPrePot() const;                        // Used by renderer to show parent end voltage
        float getPostPot() const;                       // Used by renderer to show target end voltage


        void flipDirection();                           // Flips direction of the synapse. This moves this object ownership, changing the memory addresses of itself and some of the other synapses owned by the same parent neuron

        std::size_t pN;                                 // Parent neuron ID
        std::size_t tN;                                 // Target neuron ID
        float lastSpikeStart;                           // Simulation time when synapse receives spike
        float lastSpikeArrival;                         // Simulation time when last spike arrived

        float AP_polW, AP_depolFac, AP_deltaStart, AP_fireTime; // Shape of the action potential (spike). Mostly inherited from parent neuron
        float AP_speed;                                         // ms/unit of spike
        const float traceDecayRate;
        bool inhibitory;
    private:
        float length;                                   // Length between parent and target neuron
        float weight;                                   // A factor to the action potential's strength. This value is changed by plasticity, and is where most of the learning in the brain happens
};
#endif // NEUCOR_H
