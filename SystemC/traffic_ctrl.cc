#include "traffic_ctrl.h"

Control::Control(sc_module_name name) : sc_module(name) {
  for(int i; i<4; i++) {
    lights[i].initialize(0);
    cars_progress[i].initialize(0);
  }

  SC_METHOD(update);
  dont_initialize();
  sensitive << cars[0] << cars[1] << cars[2] << cars[3];
}

void Control::update() {
  bool lightN, lightS, lightW, lightE;

  lightN = cars[0];
  lightS = cars[1];
  lightW = cars[2];
  lightE = cars[3];

  if ((lightN || lightS) && (lightW || lightE)) {
    if (NS) {
      lightW = false;
      lightE = false;
      NS = false;
    } else {
      lightN = false;
      lightS = false;
      NS = true;
    }
  }

  lights[0] = lightN;
  lights[1] = lightS;
  lights[2] = lightW;
  lights[3] = lightE;

  cars_progress[0] = cars[0]-lightN;
  cars_progress[1] = cars[1]-lightS;
  cars_progress[2] = cars[2]-lightW;
  cars_progress[3] = cars[3]-lightE;
}