#ifndef TRAFFIC_CTRL_H
#define TRAFFIC_CTRL_H

#include <systemc.h>

SC_MODULE(Control) {
  sc_in<bool> cars[4];
  sc_out<bool> cars_progress[4];
  sc_out<bool> lights[4];
  bool NS;

  SC_HAS_PROCESS(Control);
  Control(sc_module_name name);

  void update();
};

#endif
