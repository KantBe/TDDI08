#include "input_gen.h"
#include <cassert>

Generator::Generator(sc_module_name name, char *datafile) : sc_module(name) {
  assert(datafile != 0);       // An input file should be given.

  in = new ifstream(datafile); // Open the input file.
  assert(*in);                 // Check that everything is OK.

  for(int i; i<4; i++) {
    cars[i].initialize(0);
  }

  SC_THREAD(generate_thread);

  SC_METHOD(update_cars);
  dont_initialize();
  sensitive << cars_progress[0] << cars_progress[1] << cars_progress[2] << cars_progress[3];
  sensitive << trigger_event;
}

Generator::~Generator() {
  delete in;
}

void Generator::generate_thread() {
  while (!in->eof()) {
    wait(1, SC_SEC);     // Generate new cars every second.
    *in >> cars_in[0] >> cars_in[1] >> cars_in[2] >> cars_in[3]; // Read from the input file.
    trigger_event.notify();
  }

  exit(0);
}

void Generator::update_cars() {
  if (cars_progress[0]+cars_progress[1]+cars_progress[2]+cars_progress[3] > 0 || cars[0]+cars[1]+cars[2]+cars[3] > 0) {
    // for(int i; i<4; i++) {
    //   cars[i]=cars_progress[i];
    // }
    // Workaround monitor bug.
    cars[0]=cars_progress[0];
    cars[1]=cars_progress[1];
    cars[2]=cars_progress[2];
    cars[3]=cars_progress[3];
  } else if (!in->eof()) {
    for(int i; i<4; i++) {
      cars[i]=cars_in[i];
    }
  }
}
