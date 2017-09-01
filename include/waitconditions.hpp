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
    // Producer is passed PC object pointer
    Producer( PC* par ) : parent(par) {}
    ~Producer() {};
    
    void run();

protected:
    PC* parent;
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
