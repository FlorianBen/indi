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

#pragma once

#include "indibase.h"
#include <stdint.h>

using HI = INDI::HeaterInterface;

/**
 * \class HeaterInterface
 * \brief Provides interface to implement Heater functionality.
 * \author Florian Benedetti
 */
namespace INDI {

class HeaterInterface {
public:
  /**
   * \struct HeaterCapability
   * \brief Holds the capabilities of a Heater.
   */
  enum {
    HEATER_CAN_FEEDBACK =
        1 << 1, /** Can the Heater read the actual temperature? */
    HEATER_CAN_AMBIENT =
        1 << 2,              /** Can the Heater read the ambient temperature? */
    HEATER_CAN_HR = 1 << 3,  /** Can the Heater read the relative humidity? */
    HEATER_CAN_PID = 1 << 4, /** Can the Heater support PID control? */
  } HeaterCapability;

  /**
   * @brief GetHeaterCapability returns the capability of the Heater
   */
  uint32_t GetCapability() const { return heaterCapability; }

  /**
   * @brief SetHeaterCapability sets the Heater capabilities. All capabilities
   * must be initialized.
   * @param cap pointer to Heater capability struct.
   */
  void SetCapability(uint32_t cap) { heaterCapability = cap; }

  /**
   * @return Whether Heater can measure the feedback temperature.
   */
  bool CanFeedback() { return heaterCapability & HEATER_CAN_PID; }

  /**
   * @return Whether Heater can measure the ambient temperature.
   */
  bool CanAmbient() { return heaterCapability & HEATER_CAN_PID; }

  /**
   * @return Whether Heater can measure the humidity.
   */
  bool CanHR() { return heaterCapability & HEATER_CAN_HR; }

  /**
   * @return Whether Heater can be controlled by a PID.
   */
  bool CanPID() { return heaterCapability & HEATER_CAN_PID; }

protected:
  explicit HeaterInterface(DefaultDevice *defaultDevice);

  /**
   * \brief Initilize Heater properties. It is recommended to call this
   * function within initProperties() of your primary device \param groupName
   * Group or tab name to be used to define Heater properties.
   */
  void initProperties(const char *groupName);

  /**
   * @brief updateProperties Define or Delete Heater properties based on the
   * connection status of the base device
   * @return True if successful, false otherwise.
   */
  bool updateProperties();

  /** \brief Process Heater number properties */
  bool processNumber(const char *dev, const char *name, double values[],
                     char *names[], int n);

  /** \brief Process Heater switch properties */
  bool processSwitch(const char *dev, const char *name, ISState *states,
                     char *names[], int n);

  /**
   * @brief Heat Heat to specific temperature
   * @param temp Temperature set point in Celsius.
   * @return State of operation: IPS_OK is motion is completed, IPS_BUSY if
   * heating in progress, IPS_ALERT on error.
   */
  virtual IPState Heat(double temp) = 0;

  /**
   * @brief AbortHeater Abort all motion
   * @return True if successful, false otherwise.
   */
  virtual bool AbortHeater() = 0;

  ISwitch HeaterS[2]; 
  ISwitchVectorProperty HeaterSP;
  enum {HEAT, STOP};

  INumber SetPointN[1];
  INumberVectorProperty SetPointNP;

  ISwitch AbortHeaterS[1];
  ISwitchVectorProperty AbortHeaterSP;

  INumber PIDparamN[3]; /** PID parameter*/
  INumberVectorProperty PIDparamNP;

  uint32_t heaterCapability = 0;
  DefaultDevice *m_defaultDevice{nullptr};
};

} // namespace INDI