#include "NeuCor.h"

#include <vector>
#include <boost/container/stable_vector.hpp>
#include <map>
#include <queue>
#include <nanoflann.hpp>
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <memory>

NeuCor::NeuCor(int n_neurons) {
    runSpeed = 1.0;

    for (int n = 0; n<n_neurons; n++){
        coord3 d;
        d.setNAN();
        createNeuron(d);
    }
    for (int n = 0; n<n_neurons; n++){
        std::cout<<"Making connections for "<<n<<std::endl;
        neurons.at(n).makeConnections();
    }
    //Init kd-tree
    positionData.neuronPositions = positions;
    posTree = new posTree_type(3, positionData, nanoflann::KDTreeSingleIndexAdaptorParams(10));
    posTree->buildIndex();
}

NeuCor::~NeuCor(){
    delete posTree;
}

inline size_t NeuCor::positionData_type::kdtree_get_point_count() const {return neuronPositions.size();}
inline float NeuCor::positionData_type::kdtree_distance(const float *p1, const size_t idx_p2, size_t) const {
    const float d0=p1[0]-neuronPositions[idx_p2].x;
    const float d1=p1[1]-neuronPositions[idx_p2].y;
    const float d2=p1[2]-neuronPositions[idx_p2].z;
    return d0*d0+d1*d1+d2*d2;
}
inline float NeuCor::positionData_type::kdtree_get_pt(const size_t idx, int dim) const {
    if (dim==0) return neuronPositions[idx].x;
    else if (dim==1) return neuronPositions[idx].y;
    else return neuronPositions[idx].z;
}

void NeuCor::makeConnections(){
    for (int n = 0; n<neurons.size(); n++){
        neurons.at(n).makeConnections();
    }
}

void NeuCor::createNeuron(coord3 position){
    //std::cout<<"Free neurons: "<<freeNeuronIDs.size()<<std::endl;
    if (position.x != position.x){
        position.x = ((float) rand()/RAND_MAX-0.5)*3.0;
        position.y = ((float) rand()/RAND_MAX-0.5)*3.0;
        position.z = ((float) rand()/RAND_MAX-0.5)*3.0;
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
std::size_t NeuCor::getFreeID(){
    if (freeNeuronIDs.size() == 0) return neurons.size()-1;
    else return freeNeuronIDs.back();
}

float NeuCor::getTime(){return currentTime;}

void NeuCor::queSimulation(simulator* s, float time){
    simulationQue.emplace(s, currentTime+time);
}

Neuron* NeuCor::getNeuron(std::size_t ID){
    return &neurons.at(ID);
}

Synapse* NeuCor::getSynapse(std::size_t fromID, std::size_t toID){
    for (size_t i = 0; i<getNeuron(fromID)->outSynapses.size(); i++)
        if (getNeuron(fromID)->outSynapses.at(i).tN == toID)
            return &getNeuron(fromID)->outSynapses.at(i);
}

void NeuCor::deleteNeuron(std::size_t ID){
    Neuron* n = getNeuron(ID);
    n->exterminate();

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
    deleted = false;

    parentNet = p;
    lastRan = parentNet->getTime();
}
bool simulator::exists() const {
    return !deleted;
}
void simulator::exterminate(){
    deleted = true;
}

Neuron::Neuron(NeuCor* p, coord3 position)
:simulator(p), ownID(p->getFreeID()) {
    auto registration = p->registerNeuron(position, 0.5, 1.0);
    pos = std::get<0>(registration);
    PA = std::get<1>(registration);

    outSynapses.reserve(5);

    //reuptake = (float) rand()/RAND_MAX;
    reuptake = 0.5;
    buffer = 5.0;
    vesicles = buffer * 0.75;

    trace = 0.0;

    baselevel = 0.3;
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

    trace = other.trace;
    lastFire = other.lastFire;

    deleted = other.deleted;
}
void Neuron::makeConnections(){
    if (!exists()) return;
    coord3 nPos = position();
    for (size_t i = 0; i<parentNet->neurons.size(); i++){
        if (not parentNet->neurons.at(i).exists()) continue;

        coord3 otherPos = parentNet->neurons.at(i).position();
        float distance = nPos.getDist(otherPos);
        if (distance<1.0 and i != ownID){
            bool allowed = true;
            for (size_t j = 0; j < parentNet->neurons.at(i).outSynapses.size(); j++){
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

Synapse::Synapse(NeuCor* p, std::size_t parent, std::size_t target)
:simulator(p) {
    pN = parent;
    tN = target;
    parentNet->getNeuron(target)->inSynapses.emplace(parent, target);

    potential = 0.0;
    lastSpikeArrival = 0.0;

    strength = (float) rand()/RAND_MAX*2.0 - 0.0;

    coord3 n1 = parentNet->getNeuron(target)->position();
    coord3 n2 = parentNet->getNeuron(parent)->position();
    length = n2.getDist(n1);
}
Synapse::Synapse(const Synapse &other):simulator(other.parentNet){
    // Simulator member update
    lastRan = other.lastRan;

    //Synapse shallow copy
    pN = other.pN;
    tN = other.tN;
    lastSpikeArrival = other.lastSpikeArrival;

    length = other.length;
    strength = other.strength;

    potential = other.potential;
}
Synapse& Synapse::operator= (const Synapse &other){
    // Simulator member update
    lastRan = other.lastRan;

    //Synapse shallow copy
    pN = other.pN;
    tN = other.tN;
    lastSpikeArrival = other.lastSpikeArrival;

    length = other.length;
    strength = other.strength;

    potential = other.potential;
}
Synapse::~Synapse(){

}
float Synapse::getPotential(){return potential;}


/* Simulation related methods */

void NeuCor::run(){
    if ((float) rand()/RAND_MAX < 0.5 and true) {
        std::cout<<"Creating neuron\n";
        coord3 d;
        d.setNAN();
        createNeuron(d);
        makeConnections();
    }
    if ((float) rand()/RAND_MAX < 0.5 and true){
        size_t delNeu;
        while (true){
            delNeu = rand()%neurons.size();
            if (neurons.at(delNeu).exists()) break;
        }
        std::cout<<"Deleting neuron : "<<delNeu<<std::endl;
        deleteNeuron(delNeu);
    }
}

void Neuron::run(){
    if (!exists()) return;

    float timeC = parentNet->getTime();
    float deltaT = timeC - lastRan;
    lastRan = timeC;

    if (deltaT == 0) return;

    trace *= powf(0.6, deltaT);

    charge_passive(deltaT);
    charge_thresholdCheck(deltaT);

    vesicles_uptake(deltaT);
}

void Neuron::fire(){
    lastFire = parentNet->getTime();
    trace = fmin(trace + potential(), 5.0);

    for (size_t s = 0; s < outSynapses.size(); s++){
        outSynapses.at(s).fire();
    }
    for (synCoordMap::iterator syn = inSynapses.begin(); syn != inSynapses.end(); syn++){
        parentNet->getSynapse(syn->first, syn->second)->targetFire();
    }

    vesicles -=  potential();
    setPotential(0.0);
}
void Neuron::transmission(float pot){
    setPotential(potential()+pot);
}

void Neuron::charge_passive(float deltaT){
    float newPot = potential();
    newPot = (newPot-baselevel) * powf(0.995, deltaT) + baselevel;

    setPotential(newPot);
}
void Neuron::charge_thresholdCheck(float deltaT){
    if (1.0 < potential() and 0.0 < vesicles) fire();
}

void Neuron::vesicles_uptake(float deltaT){
    vesicles = fmin(buffer, vesicles + reuptake * deltaT);
}



void Synapse::run(){
    if (!exists()) return;

    parentNet->getNeuron(tN)->transmission(potential*strength);
    parentNet->queSimulation(parentNet->getNeuron(tN), 0.1);

    lastSpikeArrival = parentNet->getTime();

    potential = 0;
}
void Synapse::fire(){
    potential = powf(parentNet->getNeuron(pN)->potential(), 0.5);


    float time = length*8.0;
    parentNet->queSimulation(this, time);
}

void Synapse::targetFire(){
    Neuron* parent = parentNet->getNeuron(pN);
    Neuron* target = parentNet->getNeuron(tN);

    float traceP =  parent->trace * powf(0.6, parentNet->getTime() - parentNet->getNeuron(pN)->lastRan);
    float traceT = parentNet->getNeuron(tN)->trace;
    float traceS = powf(0.75,parentNet->getTime()-lastSpikeArrival);

    if (false){

        /*
        if (deltaTime != 0)
            strength += (float) deltaTime/powf(deltaTime, 2.0)/0.5;
        else
            strength += -0.5;*/
        float pFire = traceP*(parentNet->getTime() - parent->lastFire);
        float tFire = traceT*(parentNet->getTime() - target->lastFire);
        std::cout<<strength<<"   "<<pN<<" ---->  "<<tN<<"   ";
        //strength += strength*pFire - strength*tFire;
        std::cout<<strength<<std::endl;
    }
    else if (false){
         //std::cout<<traceS<<std::endl;
         if (0.02<traceS) strength += 0.1;
         else strength -= 0.1;
    }
    else {
        strength += 0.1;
    }
    strength = fmax(fmin(strength, 5.0), 0.0);
    //std::cout<<(parentNet->getTime() - target->lastFire)<<std::endl;
}
