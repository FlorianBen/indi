/**
 Poor Man's Celestron Focuser
 Copyright (C) 2020 F. Benedetti
 
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA */

#pragma once

#include "indifocuser.h"

// TODO: Documentation
// TODO: Implement Sync?
// TODO: Read motor parameters from the devices?

class DIYCelestron : public INDI::Focuser {
public:
  DIYCelestron(/* args */);
  ~DIYCelestron();

  virtual bool Handshake() override;
  const char *getDefaultName();

  bool initProperties() override;
  bool updateProperties() override;

  bool ISNewNumber(const char *dev, const char *name, double values[],
                   char *names[], int n) override;
  bool ISNewSwitch(const char *dev, const char *name, ISState *states,
                   char *names[], int n) override;

protected:
  virtual void TimerHit() override;
  virtual IPState MoveAbsFocuser(uint32_t targetTicks) override;
  virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks) override;
  virtual bool AbortFocuser() override;

private:
  /**
   * Communication commands as internal const.
   **/
  static constexpr const char *MYARDUICMD_ACK = "ACK";
  static constexpr const char *MYARDUICMD_ERRPAR = "ERR#001";
  static constexpr const char *MYARDUICMD_WARSTATE = "WAR#001";
  static constexpr const char *MYARDUICMD_WARPOS = "WAR#001";
  static const char DRIVER_STOP_CHAR{0x0a};
  static constexpr const uint8_t DRIVER_LEN{64};
  static constexpr const uint8_t DRIVER_TIMEOUT{3};

  bool Ack();

  bool readPosition();
  bool isMoving();
  bool setZero();
  bool setMaxPos(uint32_t steps_max);
  bool setAcc(double acc);
  bool readAcc();
  bool setMaxSpeed(int speed);
  bool readMaxSpeed();

  bool sendCommand(const char *cmd, char *res = nullptr, int cmd_len = -1,
                   int res_len = -1);

  INumber FocusAccN[1];
  INumberVectorProperty FocusAccNP;

  INumber FocusMaxSpeedN[1];
  INumberVectorProperty FocusMaxSpeedNP;

  ISwitch SetZeroS[1];
  ISwitchVectorProperty SetZeroSP;
};
