# Particle Timezone Library

This is a Particle library that sets the correct timezone on your Particle
device based on its location.

## How it works

The Particle device collects the mac addresses of the local WiFi networks and
sends them via a Particle webhook to your [Google Cloud App
Engine](https://cloud.google.com/appengine/) flex server. The server sends the
WiFi-network data to the [Google Maps Geolocation
API](https://developers.google.com/maps/documentation/geolocation/intro). Once
the API determined the location of the device, the `lat` and `long` is sent to
the [Google Maps Time Zone
API](https://developers.google.com/maps/documentation/timezone/start). This API
in turn responds with the correct UTC offset and daylight savings-time data of
the device. The offsets are send back, via the Particle webhook, to the Particle
device, which sets the correct timezone and DST data.


## How to set it up

### Setup the App Engine server

You can find the source code of the App Engine server in the
[timezone-server](timezone-server) folder.

Before you deploy your server you need to add your [Google Maps API
key](https://developers.google.com/maps/documentation/geolocation/get-api-key)
to `config.py` (you can duplicate and rename `config.py.template`).

Then you can deploy the server with:
```
gcloud app deploy
```

Take note of the URL of your server (you can visit it in a browser to see if
it's running).

### Setup the webhook

The configuration of the webhook is described in
[`timezone-webhook.json`](timezone-webhook.json).

Change the `url` value in the JSON file to your server URL.

You can deploy the webhook via the
[Particle-cli](https://docs.particle.io/tutorials/developer-tools/cli/) with:
```
particle webhook create timezone-webhook.json
```

Visit https://console.particle.io/integrations to verify or setup the webhook
manually.

### Run the library on your device

```
#include "timezone.h"

Timezone timezone;

void setup() {
  timezone.begin();
}

void loop() {
  if (Particle.connected() && !timezone.isValid())
    timezone.request();
}
```

See the [examples](examples) folder for more details.

## Firmware Library API

### Creating an object

You normally create an timezone object as a global variable in your program:

```
Timezone timezone;
```

### Setup

To start the library in setup() you use the begin() function:
```
timezone.begin();
```
This will setup a subscribtion to the correct Particle event.

### Customizing the event name

The default event name is **timezone**. You can change that in setup() by using:

```
timezone.withEventName("test/custom_timezone_event").begin();
```

### Requesting the timezone

The timezone can be requested with:

```
if (Particle.connected() && !timezone.isValid())
	timezone.request();
```
This will first check if the device is connected to the Particle cloud and if
the timezone was not already set. Then the library will publish an event which
fires the webhook and requests the timezone.

When the server answers the subscriptionHandler() function is run and the
timezone is update. You can check if the timezone is set with:

```
timezone.isValid()
```

### Debugging

The library uses the logging feature of system firmware 0.6.0 or later when
building for 0.6.0 or later. Adding this line to the top of your .ino file will
enable debugging messages to the serial port.

```
SerialLogHandler logHandler;
```

## Contributing

This library can be imporved in a lot of ways. [GitHub pull
request](https://help.github.com/articles/about-pull-requests/) are highly
appreciated.

## Related Libraries

This library is heavily based on the [Google Maps Device Locator
library](https://github.com/particle-iot/google-maps-device-locator) by Rick
Kaseguma.

Another timezone library for Particle devices is
[TzCfg](https://github.com/rwpalmer/TzCfg) by Rodney Palmer. This library does
not use Particle webhooks and uses timezonedb.com and the local ip to determine
timezone info.

## LICENSE

Based on [Google Maps Device Locator
library](https://github.com/particle-iot/google-maps-device-locator) by Rick
Kaseguma licensed under MIT License.

Copyright 2019 Jelmer Tiete <jelmer@tiete.be>

Licensed under the MIT License
