// Example usage for timezone library by Jelmer Tiete<jelmer@tiete.be>.

#include "timezone.h"

SerialLogHandler logHandler;

// Initialize the Timezone lib
Timezone timezone;

bool done = false;

void setup() {

	// Make sure you call timezone.begin() in your setup() function
    // timezone.begin();

    // Here you can define a custom event name
    // Make sure to change the event name in your webhook and server too
    timezone.withEventName("test/custom_timezone_event").begin();

}

void loop() {
    if (done)
        return;

    if (timezone.requestedLast() == 0){
        Log.info("[%s] Timezone not requested yet",
            Time.format(TIME_FORMAT_ISO8601_FULL).c_str());
        Log.info("[%s] Requesting local UTC offset from server ...",
            Time.format(TIME_FORMAT_ISO8601_FULL).c_str());

        timezone.request();
    }

    if (timezone.requestPending()) {
        Log.info("[%s] Timezone was requested at %lu, waiting for server call-back",
            Time.format(TIME_FORMAT_ISO8601_FULL).c_str(),
            timezone.requestedLast());
    }

    if (timezone.requestDone())
    {
        Log.info("[%s] Timezone request at %lu completed",
            Time.format(TIME_FORMAT_ISO8601_FULL).c_str(),
            timezone.requestedLast());
    }

    if ((timezone.requestDone() && !timezone.isValid()) ||
        (!timezone.requestPending() && !timezone.isValid()))
    {
        Log.info("[%s] Something went wrong with last request!",
            Time.format(TIME_FORMAT_ISO8601_FULL).c_str());
        Log.info("[%s] Requesting local UTC offset from server ...",
            Time.format(TIME_FORMAT_ISO8601_FULL).c_str());

        timezone.request();
    }

    // If we synced the time and we have a valid timezone
    if (Time.isValid() && timezone.isValid()){

    	Log.info("[%s] The current time is: %s",
            Time.format(TIME_FORMAT_ISO8601_FULL).c_str(),
            Time.format(TIME_FORMAT_ISO8601_FULL).c_str());

		Log.info("[%s] Raw UTC offset: %.1f",
            Time.format(TIME_FORMAT_ISO8601_FULL).c_str(),
            timezone.rawOffset);
    	Log.info("[%s] Daylight savings time offset: %.1f",
            Time.format(TIME_FORMAT_ISO8601_FULL).c_str(),
            timezone.dstOffset);

    	if (Time.isDST())
    		Log.info("[%s] DST is in effect!",
                Time.format(TIME_FORMAT_ISO8601_FULL).c_str());
    	else
    		Log.info("[%s] DST is not in effect.",
                Time.format(TIME_FORMAT_ISO8601_FULL).c_str());

	    Log.info("[%s] UTC offset: %.1f",
            Time.format(TIME_FORMAT_ISO8601_FULL).c_str(),
            timezone.utcOffset); //rawOffset + dstOffset
	    Log.info("------------------------------");

        done = true; // we're done here

    } else {

    	Log.info("[%s] Waiting for time fix or timezone update...",
            Time.format(TIME_FORMAT_ISO8601_FULL).c_str());
        Log.info("------------------------------");
    }

    delay(1000);
}
