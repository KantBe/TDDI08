#ifndef INPUT_GEN_H
#define INPUT_GEN_H

#include <systemc.h>
#include <fstream>

using std::ifstream;

SC_MODULE(Generator) {
  sc_out<bool> cars[4];
  sc_in<bool> cars_progress[4];
  sc_event trigger_update;

  SC_HAS_PROCESS(Generator);
  Generator(sc_module_name name, char *datafile);
  ~Generator();

  void generate_thread();
  void update_cars_method();

  ifstream *in;
  bool cars_in[4];
};

#endif
