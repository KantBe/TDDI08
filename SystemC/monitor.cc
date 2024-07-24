#include <cassert>
#include "monitor.h"

Monitor::Monitor(sc_module_name name) : sc_module(name) {
  SC_THREAD(count_waves_thread);

  SC_METHOD(monitor_method);
  dont_initialize();
  sensitive << cars[0] << cars[1] << cars[2] << cars[3] << lights[0] << lights[1] << lights[2] << lights[3];
}

void Monitor::count_waves_thread() {
  int i(0);
  while (true) {                   // Loop infinitely.
    wait(1, SC_SEC);               // Wait one second.
    i++;
    cout << "Wave " << i << "\n";
  }
}

void Monitor::monitor_method() {
  cout << "Cars  : ";
  for (int i{}; i < 4; i++) {
      cout << cars[i] << " ";
  }

  cout << "\nLights: ";
  for (int i{}; i < 4; i++) {
      cout << lights[i] << " ";
  }

  cout << "\n" << std::endl;
}
