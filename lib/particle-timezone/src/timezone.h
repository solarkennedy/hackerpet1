#pragma once

/* Particle-timezone library by Jelmer Tiete<jelmer@tiete.be>
 */

// This will load the definition for common Particle variable types
#include "Particle.h"

// This is your main class that users will import into their application
class Timezone
{
public:
  float utcOffset;
  float rawOffset;
  float dstOffset;

  /**
   * Constructor
   */
  Timezone();

  /**
   * The default event name is "timezone". Use this method to change it. Note that this
   * name is also configured in your server and must be changed in both
   * locations or timezone requests will not work
   */
  Timezone &withEventName(const char *name);

  /**
   * Timezone constructor, must be run first
   */
  void begin();

  /**
   * Request a timezone update
   */
  bool request();

  /**
   * return true if a timezone was received or if no connection to particle cloud
   */
  bool requestDone();

  /**
   * return true if a timezone request is pending
   */
  bool requestPending();

  /**
   * Get timestamp for last request
   */
  unsigned long requestedLast();

  /**
   * return true if timezone was set
   */
  bool isValid();

protected:
  bool publishTimezoneLocation();
  const char *scanWifiNetworks();
  void subscriptionHandler(const char *event, const char *data);

  static const unsigned long REQUEST_TIMEOUT = 300000; // 5 min request timeout

  bool timezoneSet;
  String eventName;
  unsigned long lastRequest;
  unsigned long lastUpdate;
};
