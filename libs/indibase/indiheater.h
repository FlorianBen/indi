/*******************************************************************************
 Copyright (C) 2020 Florian Benedetti. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#pragma once

#include "defaultdevice.h"
#include "indiheaterinterface.h"

namespace Connection {
class Serial;
class TCP;
} // namespace Connection
/**
 * \class Heater
   \brief Class to provide general functionality of a heating device.

   Heater must be able to heat to a specific temperature. Other capabilities are optional.

   This class is designed for pure heating devices. To utilize Heater Interface
in another type of device, inherit from HeaterInterface.

\author Florian Benedetti
*/
namespace INDI {

class Heater : public DefaultDevice, public HeaterInterface {
public:
  Heater();
  virtual ~Heater();

  /** \struct RotatorConnection
          \brief Holds the connection mode of the Rotator.
      */
  enum {
    CONNECTION_NONE = 1 << 0, /** Do not use any connection plugin */
    CONNECTION_SERIAL =
        1 << 1,             /** For regular serial and bluetooth connections */
    CONNECTION_TCP = 1 << 2 /** For Wired and WiFI connections */
  } RotatorConnection;

  virtual bool initProperties();
  virtual void ISGetProperties(const char *dev);
  virtual bool updateProperties();
  virtual bool ISNewNumber(const char *dev, const char *name, double values[],
                           char *names[], int n);
  virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states,
                           char *names[], int n);

  /**
   * @brief setRotatorConnection Set Rotator connection mode. Child class should
   * call this in the constructor before Rotator registers any connection
   * interfaces
   * @param value ORed combination of RotatorConnection values.
   */
  void setHeaterConnection(const uint8_t &value);

  /**
   * @return Get current Rotator connection mode
   */
  uint8_t getHeaterConnection() const;

protected:
  /**
   * @brief saveConfigItems Saves the reverse direction property in the
   * configuration file
   * @param fp pointer to configuration file
   * @return true if successful, false otherwise.
   */
  virtual bool saveConfigItems(FILE *fp);

  /** \brief perform handshake with device to check communication */
  virtual bool Handshake();

  INumber PresetN[3];
  INumberVectorProperty PresetNP;
  ISwitch PresetGotoS[3];
  ISwitchVectorProperty PresetGotoSP;

  Connection::Serial *serialConnection = NULL;
  Connection::TCP *tcpConnection = NULL;

  int PortFD = -1;

private:
  bool callHandshake();
  uint8_t heaterConnection = CONNECTION_SERIAL | CONNECTION_TCP;
};
} // namespace INDI