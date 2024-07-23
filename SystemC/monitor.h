#ifndef MONITOR_H
#define MONITOR_H

#include <systemc.h>
#include <fstream>

using std::ofstream;

SC_MODULE(Monitor) {
    sc_in<bool> cars[4];
    sc_in<bool> lights[4];

    SC_HAS_PROCESS(Monitor);
    Monitor(sc_module_name name);

    void monitor_method();
    void count_waves();
};

#endif
