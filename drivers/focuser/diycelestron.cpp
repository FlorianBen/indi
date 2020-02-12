#include <cmath>
#include <memory>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <iostream>

#include "indicom.h"
#include "diycelestron.h"

static std::unique_ptr<DIYCelestron> myDIYCelestron(new DIYCelestron());

void ISGetProperties(const char *dev) { myDIYCelestron->ISGetProperties(dev); }

void ISNewSwitch(const char *dev, const char *name, ISState *states,
                 char *names[], int n) {
  myDIYCelestron->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[],
               int n) {
  myDIYCelestron->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[],
                 char *names[], int n) {
  myDIYCelestron->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[],
               char *blobs[], char *formats[], char *names[], int n) {
  INDI_UNUSED(dev);
  INDI_UNUSED(name);
  INDI_UNUSED(sizes);
  INDI_UNUSED(blobsizes);
  INDI_UNUSED(blobs);
  INDI_UNUSED(formats);
  INDI_UNUSED(names);
  INDI_UNUSED(n);
}

void ISSnoopDevice(XMLEle *root) { myDIYCelestron->ISSnoopDevice(root); }

DIYCelestron::DIYCelestron(/* args */) {
  FI::SetCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE |
                    FOCUSER_CAN_ABORT);
}

DIYCelestron::~DIYCelestron() {}

bool DIYCelestron::initProperties() {
  INDI::Focuser::initProperties();
  // SetZero
  IUFillSwitch(&SetZeroS[0], "SETZERO", "Set zero", ISS_OFF);
  IUFillSwitchVector(&SetZeroSP, SetZeroS, 1, getDeviceName(), "FOCUS_ZERO_POS",
                     "Set position to zero", MAIN_CONTROL_TAB, IP_RW,
                     ISR_1OFMANY, 0, IPS_IDLE);

  // Motor acceleration
  IUFillNumber(&FocusAccN[0], "MOTOR_ACC", "Steps/s^2", "%4.2f", 1.0, 32.0, 1.0,
               16.0);
  IUFillNumberVector(&FocusAccNP, FocusAccN, 1, getDeviceName(),
                     "MOTOR_ACC_SETTING", "Acceleration", MAIN_CONTROL_TAB,
                     IP_RW, 60, IPS_IDLE);
  FocusAccN[0].min = 1.0;
  FocusAccN[0].max = 32.0;
  FocusAccN[0].value = 16.0;
  FocusAccN[0].step = 1.0;

  // Max speed
  IUFillNumber(&FocusMaxSpeedN[0], "MOTOR_MAXSPEED", "Steps/s", "%4.2f", 1.0,
               128.0, 1.0, 128.0);
  IUFillNumberVector(&FocusMaxSpeedNP, FocusMaxSpeedN, 1, getDeviceName(),
                     "MOTOR_MAXSPEED_SETTING", "Max. Speed", MAIN_CONTROL_TAB,
                     IP_RW, 60, IPS_IDLE);
  FocusMaxSpeedN[0].min = 1.0;
  FocusMaxSpeedN[0].max = 128.0;
  FocusMaxSpeedN[0].value = 128.0;
  FocusMaxSpeedN[0].step = 16.0;

  /* Relative and absolute movement */
  FocusRelPosN[0].min = 0.;
  FocusRelPosN[0].max = 2048.;
  FocusRelPosN[0].value = 512.;
  FocusRelPosN[0].step = 32.;

  FocusAbsPosN[0].min = 0.;
  FocusAbsPosN[0].max = 2048. * 10;
  FocusAbsPosN[0].value = 0.;
  FocusAbsPosN[0].step = 512.;

  // FocusMaxPosN[0].min = 0.0;
  // FocusMaxPosN[0].max = 2048. * 10;
  // FocusMaxPosN[0].step = 2048.;

  // Poll every s
  setDefaultPollingPeriod(1000);
  //addDebugControl();

  return true;
}

bool DIYCelestron::updateProperties() {
  INDI::Focuser::updateProperties();
  if (isConnected()) {
    defineNumber(&FocusAccNP);
    defineNumber(&FocusMaxSpeedNP);
    defineSwitch(&SetZeroSP);
    // GetFocusParams();
    loadConfig(true);

  } else {
    deleteProperty(FocusMaxSpeedNP.name);
    deleteProperty(FocusAccNP.name);
    deleteProperty(SetZeroSP.name);
  }

  return true;
}

IPState DIYCelestron::MoveAbsFocuser(uint32_t targetTicks) {
  if (targetTicks > FocusMaxPosN[0].value || targetTicks < 0) {
    LOGF_ERROR("Move to %i not allowed because it is out of range",
               targetTicks);
    return IPS_ALERT;
  }
  char cmd[DRIVER_LEN] = {0}, res[DRIVER_LEN] = {0};
  snprintf(cmd, DRIVER_LEN, "FOCus:STEPper:GOto %u\n", targetTicks);
  if (sendCommand(cmd, res)) {
    return (!strncmp(res, "ACK#000\n", 3) ? IPS_BUSY : IPS_ALERT);
  }
  return IPS_ALERT;
}

IPState DIYCelestron::MoveRelFocuser(FocusDirection dir, uint32_t ticks) {
  return MoveAbsFocuser(dir == FOCUS_INWARD ? FocusAbsPosN[0].value - ticks
                                            : FocusAbsPosN[0].value + ticks);
}

bool DIYCelestron::AbortFocuser() {
  char cmd[DRIVER_LEN] = {0}, res[DRIVER_LEN] = {0};
  snprintf(cmd, DRIVER_LEN, "FOCus:STEPper:ABort\n");
  if (sendCommand(cmd, res)) {
    return (!strncmp(res, "ACK#000\n", 3) ? true : false);
  }
  return false;
}

void DIYCelestron::TimerHit() {
  if (!isConnected()) {
    SetTimer(POLLMS);
    return;
  }
  auto lastPosition = FocusAbsPosN[0].value;
  bool rc = readPosition();
  if (rc) {
    // Only update if there is actual change
    if (std::fabs(lastPosition - FocusAbsPosN[0].value) > 1)
      IDSetNumber(&FocusAbsPosNP, nullptr);
  }

  if (FocusAbsPosNP.s == IPS_BUSY || FocusRelPosNP.s == IPS_BUSY) {
    if (isMoving()) {
      FocusAbsPosNP.s = IPS_OK;
      FocusRelPosNP.s = IPS_OK;
      IDSetNumber(&FocusAbsPosNP, nullptr);
      IDSetNumber(&FocusRelPosNP, nullptr);
      LOG_INFO("Focuser reached requested position.");
    }
  }
  SetTimer(POLLMS);
}

bool DIYCelestron::ISNewNumber(const char *dev, const char *name,
                               double values[], char *names[], int n) {
  if (dev != nullptr && strcmp(dev, getDeviceName()) == 0) {
    if (strcmp(name, FocusAccNP.name) == 0) {
      IUUpdateNumber(&FocusAccNP, values, names, n);
      if (!setAcc(FocusAccN[0].value)) {
        FocusAccNP.s = IPS_ALERT;
        return false;
      }
      FocusAccNP.s = IPS_OK;
      IDSetNumber(&FocusAccNP, nullptr);
      return true;
    }
    if (strcmp(name, FocusMaxSpeedNP.name) == 0) {
      IUUpdateNumber(&FocusMaxSpeedNP, values, names, n);
      if (!setMaxSpeed(FocusMaxSpeedN[0].value)) {
        FocusMaxSpeedNP.s = IPS_ALERT;
        return false;
      }
      FocusMaxSpeedNP.s = IPS_OK;
      IDSetNumber(&FocusMaxSpeedNP, nullptr);
      return true;
    }
    // if (strcmp(name, FocusMaxPosNP.name)) {
    //   IUUpdateNumber(&FocusMaxPosNP, values, names, n);
    //   if (!setMaxPos(FocusMaxPosN[0].value)) {
    //     FocusMaxPosNP.s = IPS_ALERT;
    //     return false;
    //   }
    //   FocusMaxPosNP.s = IPS_OK;
    //   IDSetNumber(&FocusMaxPosNP, nullptr);
    //   return true;
    // }
  }

  // Let INDI::Focuser handle any other number properties
  return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

bool DIYCelestron::ISNewSwitch(const char *dev, const char *name,
                               ISState *states, char *names[], int n) {
  if (dev != nullptr && strcmp(dev, getDeviceName()) == 0) {
    if (!strcmp(name, SetZeroSP.name)) {
      IUResetSwitch(&SetZeroSP);
      SetZeroSP.s = (setZero() ? IPS_OK : IPS_ALERT);
      IDSetSwitch(&SetZeroSP, nullptr);
      return true;
    }
  }
  return INDI::Focuser::ISNewSwitch(dev, name, states, names, n);
}

bool DIYCelestron::Handshake() {
  if (Ack()) {
    return true;
  }
  return false;
}

bool DIYCelestron::Ack() {
  char cmd[DRIVER_LEN] = {0}, res[DRIVER_LEN] = {0};
  snprintf(cmd, DRIVER_LEN, "*IDN?\n");
  if (sendCommand(cmd, res)) {
    return (!strncmp(res, "FlorianBen,Celestron Focusser,#00,v0.1\n", 38)
                ? true
                : false);
  }
  return false;
}

bool DIYCelestron::readPosition() {
  char cmd[DRIVER_LEN] = {0}, res[DRIVER_LEN] = {0};
  auto pos = -1;
  snprintf(cmd, DRIVER_LEN, "FOCus:STEPper:POSition?\n");
  if (sendCommand(cmd, res)) {
    FocusAbsPosN[0].value = atoi(res);
    return true;
  }
  return false;
}

bool DIYCelestron::isMoving() {
  char cmd[DRIVER_LEN] = {0}, res[DRIVER_LEN] = {0};
  snprintf(cmd, DRIVER_LEN, "FOCus:STEPper:Go?\n");
  if (sendCommand(cmd, res)) {
    return (strncmp(res, "BUSY", 4) ? true : false);
  }
  return false;
}

bool DIYCelestron::setZero() {
  char cmd[DRIVER_LEN] = {0}, res[DRIVER_LEN] = {0};
  snprintf(cmd, DRIVER_LEN, "FOCus:STEPper:RAZPOSition\n");
  if (sendCommand(cmd, res)) {
    return (!strncmp(res, "ACK#000\n", 3) ? true : false);
  }
  return false;
}

bool DIYCelestron::setMaxPos(uint32_t steps_max) {
  char cmd[DRIVER_LEN] = {0}, res[DRIVER_LEN] = {0};
  snprintf(cmd, DRIVER_LEN, "FOCus:STEPper:MAXpos %u\n", steps_max);
  if (sendCommand(cmd, res)) {
    return (!strncmp(res, "ACK#000\n", 3) ? true : false);
  }
  return false;
}

bool DIYCelestron::setAcc(double speed) {
  char cmd[DRIVER_LEN] = {0}, res[DRIVER_LEN] = {0};
  snprintf(cmd, DRIVER_LEN, "FOCus:STEPper:ACcel %f\n", speed);
  if (sendCommand(cmd, res)) {
    return (!strncmp(res, "ACK#000\n", 3) ? true : false);
  }
  return false;
}

bool DIYCelestron::readAcc() { return false; }

bool DIYCelestron::setMaxSpeed(int speed) {
  char cmd[DRIVER_LEN] = {0}, res[DRIVER_LEN] = {0};
  snprintf(cmd, DRIVER_LEN, "FOCus:STEPper:RPMspeed %u\n", speed);
  if (sendCommand(cmd, res)) {
    return (!strncmp(res, "ACK#000\n", 3) ? true : false);
  }
  return false;
}

bool DIYCelestron::readMaxSpeed() { return false; }

bool DIYCelestron::sendCommand(const char *cmd, char *res, int cmd_len,
                               int res_len) {
  int nbytes_written = 0, nbytes_read = 0, rc = -1;

  tcflush(PortFD, TCIOFLUSH);

  LOGF_DEBUG("CMD <%s>", cmd);
  rc = tty_write_string(PortFD, cmd, &nbytes_written);
  if (rc != TTY_OK) {
    char errstr[MAXRBUF] = {0};
    tty_error_msg(rc, errstr, MAXRBUF);
    LOGF_ERROR("Serial write error: %s.", errstr);
    return false;
  }
  rc = tty_nread_section(PortFD, res, DRIVER_LEN, DRIVER_STOP_CHAR,
                         DRIVER_TIMEOUT, &nbytes_read);

  if (rc != TTY_OK) {
    char errstr[MAXRBUF] = {0};
    tty_error_msg(rc, errstr, MAXRBUF);
    LOGF_ERROR("Serial read error: %s.", errstr);
    return false;
  }

  LOGF_DEBUG("RES <%s>", res);

  tcflush(PortFD, TCIOFLUSH);

  return true;
}

const char *DIYCelestron::getDefaultName() { return "DIYCelestron Focuser"; }