#include <systemc.h>
#include "traffic_ctrl.h"
#include "input_gen.h"
#include "monitor.h"

int sc_main(int argc, char **argv) {
  assert(argc == 2);
  char *infile = argv[1];

  // Create channels.
  sc_signal<bool> cars_sig[4];
  sc_signal<bool> cars_progress_sig[4];
  sc_signal<bool> lights_sig[4];

  // Instantiate modules.
  Control control("Control");
  Generator gen("Generator", infile);
  Monitor monitor("Monitor");

  // Connect the channels to the ports.
  control(cars_sig[0], cars_sig[1], cars_sig[2], cars_sig[3], cars_progress_sig[0], cars_progress_sig[1], cars_progress_sig[2], cars_progress_sig[3], lights_sig[0], lights_sig[1], lights_sig[2], lights_sig[3]);
  gen(cars_sig[0], cars_sig[1], cars_sig[2], cars_sig[3], cars_progress_sig[0], cars_progress_sig[1], cars_progress_sig[2], cars_progress_sig[3], lights_sig[0], lights_sig[1], lights_sig[2], lights_sig[3]);
  monitor(cars_sig[0], cars_sig[1], cars_sig[2], cars_sig[3], lights_sig[0], lights_sig[1], lights_sig[2], lights_sig[3]);

  // Start the simulation.
  sc_start();

  return 0;
}
