# Gst Webkit

Easiest way to build overlays with Gstreamer

![](imgs/principes.png)

Our goal is to develop a Webkit mixer providing javascript interactions.

![](imgs/principes-surface.png)


Using webkit you can use all css animations, all javascript interactions. You don't need to code complex pipelines in c / python to do that.


You can use this gstreamer element to do :

![](imgs/usages.png)

## Installation

### Requirements

* libwebkit
* libcairo
* Gstreamer-1.0

### Compilation

```
apt-get install libwebkit-dev libcairo-dev
```

```
./autogen.sh
```

```
make
```

```
sudo make install
```

## Usage

```
Factory Details:
  Rank                     none (0)
  Long-name                WebkitOverlayFilter
  Klass                    FIXME:Generic
  Description              FIXME:Generic Template Element
  Author                   Ludovic <ludovic.bouguerra@kalyzee.com>

Plugin Details:
  Name                     webkitoverlayfilter
  Description              webkitoverlayfilter
  Filename                 /usr/local/lib/gstreamer-1.0/libgstwebkitoverlay.so
  Version                  1.0.0
  License                  LGPL
  Source module            gst-webkit
  Binary package           GStreamer
  Origin URL               http://gstreamer.net/

GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstWebkitOverlayFilter

Pad Templates:
  SINK template: 'sink'
    Availability: Always
    Capabilities:
      video/x-raw
                 format: { (string)ARGB }
                  width: [ 1, 2147483647 ]
                 height: [ 1, 2147483647 ]
              framerate: [ 0/1, 2147483647/1 ]

  SRC template: 'src'
    Availability: Always
    Capabilities:
      video/x-raw
                 format: { (string)ARGB }
                  width: [ 1, 2147483647 ]
                 height: [ 1, 2147483647 ]
              framerate: [ 0/1, 2147483647/1 ]


Element Flags:
  no flags set

Element Implementation:
  Has change_state() function: gst_element_change_state_func

Element has no clocking capabilities.
Element has no URI handling capabilities.

Pads:
  SINK: 'sink'
    Pad Template: 'sink'
  SRC: 'src'
    Pad Template: 'src'

Element Properties:
  name                : The name of the object
                        flags: accès en lecture, accès en écriture
                        String. Default: "webkitoverlayfilter0"
  parent              : The parent of the object
                        flags: accès en lecture, accès en écriture
                        Object of type "GstObject"
  url                 : url page
                        flags: accès en lecture, accès en écriture
                        String. Default: "http://www.google.com"

```


Test pipeline

```
gst-launch-1.0 videotestsrc ! video/x-raw, format=ARGB, width=1280, height=720 ! videoconvert ! webkitoverlayfilter url=http://www.google.com ! videoconvert ! xvimagesink
```


```
gst-launch-1.0 webkitsrc ! video/x-raw, format=ARGB, width=1280, height=720 ! videoconvert ! videoconvert ! xvimagesink
```
