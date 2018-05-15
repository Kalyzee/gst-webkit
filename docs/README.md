# Gst Webkit

Easiest way to build overlays with Gstreamer

Test pipeline

```
gst-launch-1.0 videotestsrc ! videoconvert ! webkitoverlayfilter silent=true ! videoconvert ! xvimagesink

```
