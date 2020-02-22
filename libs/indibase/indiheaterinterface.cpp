/*
    Heater Interface
    Copyright (C) 2020 Florian Benedetti

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
   USA

*/

#include "indiheaterinterface.h"
#include "defaultdevice.h"

#include "indilogger.h"

#include <cstring>

namespace INDI {

HeaterInterface::HeaterInterface(DefaultDevice *defaultDevice)
    : m_defaultDevice(defaultDevice) {}

void HeaterInterface::initProperties(const char *groupName) {
  // Start heating
  IUFillSwitch(&HeaterS[HEAT], "HEAT", "Heat", ISS_OFF);
  IUFillSwitch(&HeaterS[STOP], "STOP", "Stop", ISS_OFF);
  IUFillSwitchVector(&HeaterSP, HeaterS, 2, m_defaultDevice->getDeviceName(),
                     "HEATER", "Heater", groupName, IP_RW, ISR_ATMOST1, 60,
                     IPS_IDLE);

  // Rotator Angle
  IUFillNumber(&SetPointN[0], "SETPOINT", "Setpoint", "%.2f", -10, 40., 1.,
               20.);
  IUFillNumberVector(&SetPointNP, SetPointN, 1,
                     m_defaultDevice->getDeviceName(), "TEMP_SETPOINT",
                     "Process", groupName, IP_RW, 0, IPS_IDLE);

  // Abort Rotator
  IUFillSwitch(&AbortHeaterS[0], "ABORT", "Abort", ISS_OFF);
  IUFillSwitchVector(&AbortHeaterSP, AbortHeaterS, 1,
                     m_defaultDevice->getDeviceName(), "ROTATOR_ABORT_HEATER",
                     "Abort Motion", groupName, IP_RW, ISR_ATMOST1, 0,
                     IPS_IDLE);
}

bool HeaterInterface::processNumber(const char *dev, const char *name,
                                    double values[], char *names[], int n) {
  INDI_UNUSED(names);
  INDI_UNUSED(n);

  if (dev != nullptr && strcmp(dev, m_defaultDevice->getDeviceName()) == 0) {
    ////////////////////////////////////////////
    // Move Absolute Angle
    ////////////////////////////////////////////
    if (strcmp(name, SetPointNP.name) == 0) {
      if (values[0] == SetPointN[0].value) {
        SetPointNP.s = IPS_OK;
        IDSetNumber(&SetPointNP, nullptr);
        return true;
      }
      IDSetNumber(&SetPointNP, nullptr);
      return true;
    }
  }

  return false;
}

bool HeaterInterface::processSwitch(const char *dev, const char *name,
                                    ISState *states, char *names[], int n) {
  INDI_UNUSED(states);
  INDI_UNUSED(names);
  INDI_UNUSED(n);

  if (dev != nullptr && strcmp(dev, m_defaultDevice->getDeviceName()) == 0) {
    ////////////////////////////////////////////
    // Abort
    ////////////////////////////////////////////
    if (strcmp(name, AbortHeaterSP.name) == 0) {
      AbortHeaterSP.s = AbortHeater() ? IPS_OK : IPS_ALERT;
      IDSetSwitch(&AbortHeaterSP, nullptr);
      if (AbortHeaterSP.s == IPS_OK) {
        if (SetPointNP.s != IPS_OK) {
          SetPointNP.s = IPS_OK;
          IDSetNumber(&SetPointNP, nullptr);
        }
      }
      return true;
    }
  }

  return false;
}

bool HeaterInterface::updateProperties() {
  if (m_defaultDevice->isConnected()) {
    m_defaultDevice->defineNumber(&SetPointNP);
    m_defaultDevice->defineSwitch(&AbortHeaterSP);
  } else {
    m_defaultDevice->deleteProperty(SetPointNP.name);
    m_defaultDevice->deleteProperty(AbortHeaterSP.name);
  }

  return true;
}

} // namespace INDI