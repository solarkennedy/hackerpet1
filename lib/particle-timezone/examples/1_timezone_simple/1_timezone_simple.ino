// Example usage for timezone library by Jelmer Tiete<jelmer@tiete.be>.

#include "timezone.h"

SerialLogHandler logHandler;

// Initialize the Timezone lib
Timezone timezone;

void setup() {

	// Make sure you call timezone.begin() in your setup() function
    timezone.begin();

}

void loop() {

    // Print the current time
    Log.info("The current time is: %s", Time.format(TIME_FORMAT_ISO8601_FULL).c_str());

    // If we're connected to the Particle cloud and we don't have a valid timezone
    if (Particle.connected() && !timezone.isValid())
        // Request our current timezone
        timezone.request();

    delay(10000);

}
