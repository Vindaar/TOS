#ifndef _WAITCONDITIONS_HPP
#define _WAITCONDITIONS_HPP

#include <thread>
#include "helper_functions.hpp"
#include "pc.hpp"
#include <QThread>


class PC;


class Producer : public QThread
{
public:
    // Producer is passed PC object pointer as well as a reference to an
    // AtomicTemps object (see mcp2210/temps_helpers.hpp)
    Producer( PC* par, AtomicTemps *temps ) : parent(par), _temps(temps){}
    ~Producer() {};
    
    void run();

protected:
    PC* parent;
    AtomicTemps *_temps;
};


class Consumer : public QThread
{
public:
    Consumer (PC* par ) : parent(par) {}
    ~Consumer() {};

    void run();

protected:
    PC* parent;
};

#endif
