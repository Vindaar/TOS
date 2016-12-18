#ifndef _WAITCONDITIONS_HPP
#define _WAITCONDITIONS_HPP

#include <QtCore>
#include <stdio.h>
#include <stdlib.h>
#include "header.hpp"
#include <thread>
#include "helper_functions.hpp"

class PC;


class Producer : public QThread
{
public:
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
