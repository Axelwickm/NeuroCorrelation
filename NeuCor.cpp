#include "NeuCor.h"

#include <vector>
#include <boost/container/stable_vector.hpp>
#include <map>
#include <queue>
#include <stdlib.h>
#include <iostream>

NeuCor::NeuCor(int n_neurons) {
    runSpeed = 1.0;
    runAll = false;
    totalGenNeurons = 0;

    learningRate = 0.6;
    presynapticTraceDecay = 0.5;
    postsynapticTraceDecay = 0.9;

    totalGenNeurons = n_neurons;
    for (int n = 0; n<n_neurons; n++){
        coord3 d;
        d.setNAN();
        createNeuron(d);
    }
    totalGenNeurons = 0;
    std::cout<<"Making connections for: ";
    for (int n = 0; n<n_neurons; n++){
        if (n != 0) std::cout<<", ";
        std::cout<<n;
        neurons.at(n).makeConnections();
    }
    std::cout<<'\n';
}

NeuCor::~NeuCor(){}

void NeuCor::setInputRateArray(float inputs[], unsigned inputCount, coord3 inputPositions[], float inputRadius[]){
    inputArray = inputs;
    inputArraySize = inputCount;

    int inputHanderSizeChange = inputArraySize - inputHandler.size();
    if (0 < inputHanderSizeChange){
        for (unsigned i = 0; i<inputHanderSizeChange; i++){
            if (inputPositions != NULL)
                inputHandler.emplace_back(this, inputPositions[i], inputRadius[i]);
            else
                inputHandler.emplace_back(this);
        }
    }
    else if (inputHanderSizeChange < 0){
        for (unsigned i = 0; i<-inputHanderSizeChange; i++){
            inputHandler.pop_back();
        }
    }
}

void NeuCor::makeConnections(){
    for (int n = 0; n<neurons.size(); n++){
        neurons.at(n).makeConnections();
    }
}

void NeuCor::createNeuron(coord3 position){
    #define SPAWN_DENSITY 6
    #define SPAWN_SIZE 2.0
    #define SPAWN_SPHERE true

    float spawnSize = SPAWN_SIZE;

    //std::cout<<"Free neurons: "<<freeNeuronIDs.size()<<'\n';
    if (position.x != position.x && SPAWN_SPHERE){
        if (totalGenNeurons != 0 ) spawnSize = powf(totalGenNeurons/(1.3333*3.1459*SPAWN_DENSITY),0.33333)*2.0;
        do {
            position.x = ((float) rand()/RAND_MAX-0.5)*spawnSize;
            position.y = ((float) rand()/RAND_MAX-0.5)*spawnSize;
            position.z = ((float) rand()/RAND_MAX-0.5)*spawnSize;
        } while ( pow(position.x,2) + pow(position.y,2) + pow(position.z,2) > pow(spawnSize/2.0,2.0) );

    }
    else if (position.x != position.x){
        if (totalGenNeurons != 0 ) spawnSize = powf(totalGenNeurons/SPAWN_DENSITY,0.33333);
        position.x = ((float) rand()/RAND_MAX-0.5)*spawnSize;
        position.y = ((float) rand()/RAND_MAX-0.5)*spawnSize;
        position.z = ((float) rand()/RAND_MAX-0.5)*spawnSize;
    }

    if (freeNeuronIDs.size() == 0 || false){
        neurons.emplace_back(this, position);
    }
    else {
        Neuron newNeuron(this, position);
        neurons.at(freeNeuronIDs.back()) = newNeuron;
        freeNeuronIDs.pop_back();
    }
}
void NeuCor::createSynapse(std::size_t toID, std::size_t fromID, float weight){

    for (auto &t: neurons.at(fromID).outSynapses) // Don't allow if synapse already exists
        if (t.tN == toID)
            return;

    neurons.at(fromID).outSynapses.emplace_back(this, fromID, toID);
    neurons.at(fromID).outSynapses.back().setWeight(weight);
}

std::tuple<std::size_t, std::size_t> NeuCor::registerNeuron(coord3 pos, float potential, float activity){
    std::size_t posIndx;
    std::size_t PTIndx;
    if (freeNeuronIDs.size() == 0){
        posIndx = positions.size();
        PTIndx = potAct.size();

        positions.push_back(pos);
        potAct.push_back(potential);
        potAct.push_back(activity);
    }
    else {
        posIndx = neurons.at(freeNeuronIDs.back()).pos;
        PTIndx = posIndx*2;

        positions.at(posIndx) = pos;
        potAct.at(PTIndx) = potential;
        potAct.at(PTIndx+1) = activity;
    }


    return std::make_tuple(posIndx, PTIndx);
}
std::size_t NeuCor::getFreeID() const {
    if (freeNeuronIDs.size() == 0) return neurons.size()-1;
    else return freeNeuronIDs.back();
}

float NeuCor::getTime() const {return currentTime;}

void NeuCor::queueSimulation(simulator* s, const float time){
    simulationQueue.emplace(s, currentTime + time);
}
void NeuCor::queFlip(std::pair<std::size_t, std::size_t> ID){
    synapseFlippingQueue.push_back(ID);
}
void NeuCor::resetActivities(){
    for (auto &neu: neurons) neu.resetActivity();
}

Neuron* NeuCor::getNeuron(std::size_t ID) {
    return &neurons.at(ID);
}

Synapse* NeuCor::getSynapse(std::size_t fromID, std::size_t toID){
    for (size_t i = 0; i<getNeuron(fromID)->outSynapses.size(); i++)
        if (getNeuron(fromID)->outSynapses.at(i).tN == toID)
            return &getNeuron(fromID)->outSynapses.at(i);
}
Synapse* NeuCor::getSynapse(std::pair<std::size_t, std::size_t> ID){
    return getSynapse(ID.first, ID.second);
}

void NeuCor::deleteNeuron(std::size_t ID){
    Neuron* n = getNeuron(ID);

    coord3 emptyPos;
    emptyPos.setNAN();
    n->setPosition(emptyPos);

    //Delete owned synapses backwards iterating for loop
    for (std::vector<Synapse>::reverse_iterator syn = n->outSynapses.rbegin(); syn != n->outSynapses.rend(); ++syn){
        deleteSynapse(syn->pN, syn->tN);
    }
    n->outSynapses.clear();

    //Delete input owned synapses using backwards iterating for loop
    for (auto syn = n->inSynapses.rbegin(); syn != n->inSynapses.rend();){
        deleteSynapse(syn->first, syn->second);
    }
    n->inSynapses.clear();

    freeNeuronIDs.push_back(ID);
}

void NeuCor::deleteSynapse(std::size_t fromID, std::size_t toID){
    for (int i = 0; i<getNeuron(fromID)->outSynapses.size(); i++){
        if (getNeuron(fromID)->outSynapses.at(i).tN == toID){

            getNeuron(fromID)->outSynapses.erase(getNeuron(fromID)->outSynapses.begin()+i);
            getNeuron(toID)->removeInSyn(fromID);
            return;
        }
    }
}

simulator::simulator(NeuCor* p){
    parentNet = p;
    lastRan = parentNet->getTime();
}

deletedSimulator::deletedSimulator(NeuCor* p): simulator(p) {};

InputFirer::InputFirer(NeuCor* p, coord3 position, float radius)
:simulator(p), radius(radius) {
    if (position.x == position.x) a = position; // If x isn't NAN
    else a = {(float) rand()/RAND_MAX,(float) rand()/RAND_MAX,(float) rand()/RAND_MAX};

    //  Make b a random point within a given distance of a
    /*float longitude = 2.0*3.1459*(float) rand()/RAND_MAX;
    float latitude = acos(2.0*(float) rand()/RAND_MAX-1.0);
    b.x = a.x+sqrt(1.0-pow(cos(longitude),2.0))*cos(latitude);
    b.y = a.y+sqrt(1.0-pow(cos(longitude),2.0))*sin(latitude);
    b.z = a.z+cos(longitude);*/

    for (auto &neu: parentNet->neurons){
        if (neu.position().getDist(a) < radius){
            near.push_back(neu.getID());
        }
    }
}

void InputFirer::run(){
    for (auto neuID: near)
        parentNet->getNeuron(neuID)->fire();
}

void InputFirer::schedule(float deltaT, float frequency){
    if (frequency == 0) return;

    if (lastFire != lastFire) lastFire = 0;
    float currentT = parentNet->getTime();

    for (float fireTime = lastFire + 1000.0/frequency; fireTime < currentT + deltaT; fireTime += 1000.0/frequency){
        if (currentT < fireTime){
            parentNet->queueSimulation(this, fireTime-currentT);
            lastFire = fireTime;
        }
    }
}

Neuron::Neuron(NeuCor* p, coord3 position)
:simulator(p), ownID(p->getFreeID()), traceDecayRate(p->postsynapticTraceDecay) {
    auto registration = p->registerNeuron(position, 0.0, 1.0);
    pos = std::get<0>(registration);
    PA = std::get<1>(registration);

    outSynapses.reserve(5);

    baselevel = -70.0;
    threshold = -55.0;
    recharge = 0.5;

    //reuptake = (float) rand()/RAND_MAX;
    reuptake = 0.5;
    buffer = 5.0;

    AP_h = 100.0, AP_depolW = 0.3, AP_polW = 0.6, AP_deltaPol = 1.16, AP_depolFac = 0.2, AP_deltaStart = 1.0;
    AP_cutoff = 2.0;

    activityStartTime = parentNet->getTime();
    firings = 0;
    setActivity(0);

    vesicles = buffer * 0.75;
    lastFire = NAN;

    setPotential(baselevel);
}
Neuron::~Neuron(){

}
Neuron& Neuron::operator=(const Neuron& other){
    pos = other.pos;
    PA = other.PA;

    for (auto &syn: outSynapses){
        parentNet->deleteSynapse(syn.pN, syn.tN);
    }

    outSynapses = other.outSynapses;
    inSynapses = other.inSynapses;

    lastFire = other.lastFire;

    activityStartTime = other.activityStartTime;
    firings = other.firings;
    setActivity(0);
}
void Neuron::makeConnections(){
    coord3 nPos = position();
    for (size_t i = 0; i<parentNet->neurons.size(); i++){
        coord3 otherPos = parentNet->neurons.at(i).position();
        float distance = nPos.getDist(otherPos);
        if (distance<1.0 and i != ownID){
            bool allowed = true;
            for (size_t j = 0; j < parentNet->neurons.at(i).outSynapses.size(); j++){
                break;
                if (parentNet->getNeuron(parentNet->neurons.at(i).outSynapses.at(j).tN) == this){
                    allowed = false;
                    break;
                }
            }
            for (size_t j = 0; j<outSynapses.size(); j++){
                if (outSynapses.at(j).tN == i){
                    allowed = false;
                    break;
                }
            }
            if (allowed){
                outSynapses.emplace_back(parentNet, ownID, i);
                //if (rand()%2 == 0) outSynapses.back().flipDirection();
            }
        }
    }
}
void Neuron::removeOutSyn(std::size_t synTo){
    for (size_t i = 0; i<outSynapses.size(); i++){
        if (outSynapses.at(i).tN == synTo) parentNet->deleteSynapse(ownID, synTo);
    }
}
void Neuron::removeInSyn(std::size_t synFrom){
    inSynapses.erase(synFrom);
}

coord3 Neuron::position() const { return parentNet->positions.at(pos); }
void Neuron::setPosition(coord3 newPos){parentNet->positions.at(pos) = newPos;}
float Neuron::potential() const { return parentNet->potAct.at(PA); }
void Neuron::setPotential(float p){parentNet->potAct.at(PA) = p;}
float Neuron::activity() const { return parentNet->potAct.at(PA + 1); }
void Neuron::setActivity(float a){parentNet->potAct.at(PA+1) = a;}
void Neuron::resetActivity(){firings = 0; activityStartTime = parentNet->getTime(); setActivity(0.0);}
std::size_t Neuron::getID() const { return ownID;}

Synapse::Synapse(NeuCor* p, std::size_t parent, std::size_t target)
:simulator(p), traceDecayRate(p->presynapticTraceDecay) {
    pN = parent;
    tN = target;
    parentNet->getNeuron(target)->inSynapses.emplace(parent, target);

    lastSpikeArrival = -INFINITY;

    weight = (float) rand()/RAND_MAX*0.8 + 0.2;

    if ((float) rand()/RAND_MAX < 0.2) weight = -weight;

    coord3 n1 = parentNet->getNeuron(target)->position();
    coord3 n2 = parentNet->getNeuron(parent)->position();
    length = n2.getDist(n1);

    AP_polW = 0, AP_depolFac = 0, AP_deltaStart = 0, AP_fireTime = 0;
    AP_speed = 2.0;
}
Synapse::Synapse(const Synapse &other):simulator(other.parentNet), traceDecayRate(other.traceDecayRate){
    // Simulator member update
    lastRan = other.lastRan;

    //Synapse shallow copy
    pN = other.pN;
    tN = other.tN;
    lastSpikeArrival = other.lastSpikeArrival;

    length = other.length;
    weight = other.weight;

    AP_polW = 0, AP_depolFac = 0, AP_deltaStart = 0, AP_fireTime = 0;
    AP_speed = 2.0;
}
Synapse& Synapse::operator= (const Synapse &other){
    // Simulator member update
    lastRan = other.lastRan;

    //Synapse shallow copy
    pN = other.pN;
    tN = other.tN;
    lastSpikeArrival = other.lastSpikeArrival;

    length = other.length;
    weight = other.weight;

    AP_polW = 0, AP_depolFac = 0, AP_deltaStart = 0, AP_fireTime = 0;
    AP_speed = 2.0;
}
Synapse::~Synapse(){

}
void Synapse::flipDirection(){
    Neuron* neuP = parentNet->getNeuron(pN);
    Neuron* neuT = parentNet->getNeuron(tN);

    auto inSyn = neuT->inSynapses.find(pN);
    neuP->inSynapses.emplace(tN, pN);
    neuT->inSynapses.erase(inSyn);

    for (auto itr = neuP->outSynapses.begin(); itr != neuP->outSynapses.end(); itr++){
        if (itr->tN == tN) {
            std::swap(tN, pN);
            neuT->outSynapses.push_back(std::move(*itr));
            neuP->outSynapses.erase(itr);
            break;
        }
    }
}


#define AP_RENDER_BEHAVIOUR;                                \
    val = fmin(fmax(val,0.0),0.7);                           \
    if (val < 0.5) val = 8.0*1000.0*powf(val/5.0,3.0);        \
    else val = 8.0*powf(3.5-5*val,2.0);
float Synapse::getPrePot() const {
    if (AP_fireTime != 0.0){
        float val = float(parentNet->getTime() - lastSpikeStart)/(length*AP_speed);
        AP_RENDER_BEHAVIOUR;
        return val*weight;
    }
    else return 0.0;
}

float Synapse::getPostPot() const {
    if (AP_fireTime != 0.0 && parentNet->getTime() < AP_fireTime){
        float val = float(AP_fireTime - parentNet->getTime())/(length*AP_speed);
        AP_RENDER_BEHAVIOUR;
        return val*weight;
    }
    else return 0.0;
}


#undef AP_RENDER_BEHAVIOUR

float Synapse::getWeight() const {
    return weight;
}

void Synapse::setWeight(float w) {
    weight = w;
}

/* Simulation related methods */

void NeuCor::run(){
    assert(0 <= runSpeed);

    for (auto it = synapseFlippingQueue.begin(); it != synapseFlippingQueue.end(); it++)
        getSynapse(*it)->flipDirection();
    synapseFlippingQueue.clear();

    if (runAll){
        for (auto &neu: neurons) queueSimulation(&neu, 0.0);
    }

    for (int i = 0; i<inputHandler.size(); i++){
        inputHandler.at(i).schedule(runSpeed, inputArray[i]);
    }
    float const targetTime = currentTime + runSpeed;
    while (simulationQueue.size() != 0){
        currentTime = simulationQueue.top().stime;
        if (currentTime < simulationQueue.top().stime || targetTime < currentTime) break;
        simulationQueue.top().addr->run();
        simulationQueue.pop();
    }
    currentTime = targetTime;
}

void Neuron::run(){
    // Determine time
    float currentT = parentNet->getTime();
    float deltaT = currentT - lastRan;
    lastRan = currentT;

    // Exit function if no time has passed
    if (deltaT == 0) return;

    // Integrates the potentials of the input synapses
    charge_insynapses(deltaT, currentT);
    // Exponentially decays/grows neuron potential towards base level
    charge_passive(deltaT, currentT);
    // Checks if neuron potential is above threshold, and if so fires neuron
    charge_thresholdCheck(deltaT, currentT);
    // If neuron is firing, shapes the action potential by setting the potential
    AP(currentT);
    // Increases vesicles amount
    vesicles_uptake(deltaT);

    // Update activity
    setActivity(firings/(((float) currentT-activityStartTime)/10.0));
}

void Neuron::fire(){
    lastFire = parentNet->getTime();
    firings++;

    for (size_t s = 0; s < outSynapses.size(); s++){
        outSynapses.at(s).fire(AP_polW, AP_depolFac, AP_deltaStart);
    }

    for (auto syn = inSynapses.begin(); syn != inSynapses.end(); syn++){
        parentNet->getSynapse(syn->first, syn->second)->synapticPlasticity();
    }

    //vesicles -= 5.0;
}
void Neuron::transfer(){
    parentNet->queueSimulation(this, AP_cutoff);
}
void Neuron::givePotential(float pot){
    setPotential(potential()+pot);
}

float Neuron::getTrace() const {
    float t = powf(traceDecayRate, parentNet->getTime()-lastFire);
    if (t == t) return t;
    else return 0;
}

void Neuron::charge_passive(float deltaT, float currentT){
    float newPot = ((float) potential()-baselevel) * powf(recharge, deltaT) + baselevel;
    setPotential(newPot);
}

void Neuron::charge_thresholdCheck(float deltaT, float currentT){
    if (threshold < potential() && (lastFire != lastFire || AP_cutoff < (float) currentT-lastFire) && 0.0 < vesicles) fire();
}

void Neuron::charge_insynapses(float deltaT, float currentT){
    float newPot = potential();
    for (auto syn: inSynapses){
        auto s = parentNet->getSynapse(syn.first, syn.second);
        float timeOffset = currentT - s->AP_fireTime;
        if (timeOffset <= 0.0 || s->AP_fireTime == 0) continue;

        newPot += deltaT*s->AP_depolFac;

        if (AP_cutoff < timeOffset) s->AP_fireTime = 0;
    }
    setPotential(newPot);
}

void Neuron::vesicles_uptake(float deltaT){
    vesicles = fmin(buffer, vesicles + reuptake * deltaT);
}

void Neuron::AP(float currentT){
    if (lastFire != lastFire || AP_cutoff < (float) currentT-lastFire) return;

    float currentAP = AP_h
        * (exp(-powf(((float) currentT-lastFire) - AP_deltaStart,               2.0)/(2.0*AP_depolW*AP_depolW))
        -  exp(-powf(((float) currentT-lastFire) - AP_deltaStart - AP_deltaPol, 2.0)/(2.0*AP_polW*AP_polW)) * AP_depolFac ) + baselevel
        + (threshold - baselevel)*fmax(1.0+lastFire-currentT, 0.0);
    setPotential(currentAP);
}



void Synapse::run(){
    if (AP_fireTime < parentNet->getTime()) return;

    parentNet->getNeuron(tN)->transfer();
    parentNet->queueSimulation(parentNet->getNeuron(tN), 0.1);

    lastSpikeArrival = parentNet->getTime();

    synapticPlasticity();
}
void Synapse::fire(float polW, float depolFac, float deltaStart){
    if (AP_fireTime != 0) return;
    AP_polW = polW, AP_depolFac = depolFac, AP_deltaStart = deltaStart;
    AP_depolFac *= 80.0;
    AP_depolFac *= weight;

    AP_fireTime = length*AP_speed;
    parentNet->queueSimulation(this, AP_fireTime);
    AP_fireTime += parentNet->getTime();

    lastSpikeStart = parentNet->getTime();
}

void Synapse::synapticPlasticity(){
    float traceS = powf(traceDecayRate, parentNet->getTime()-lastSpikeArrival); // Synapse trace (presynaptic)
    float traceT = parentNet->getNeuron(tN)->getTrace(); // Target trace (postsynaptic)
    bool inhibitory = weight < 0;
    if (traceT == 1) traceT = 0;
    if (traceS == 1) traceS = 0;

    if (weight == 0 && !inhibitory && rand()%120 == 0) weight += 1.0; // 1/120 chance of weight being boosted (this allows for signal expansion and reservation throughout the brain)

    float weightChange = traceS - traceT;
    weight += weightChange*parentNet->learningRate;
    if (!inhibitory) weight = fmax(fmin(weight, 1.0), 0.0);
    else weight = fmax(fmin(weight, 0.0), -1.0);
    //std::cout<<"Delta w = "<<weightChange<<'\n';

    //if (weight < 0) parentNet->queFlip(std::pair<std::size_t, std::size_t>(pN, tN)); // Que flipping of synapse if weight is 0
}
