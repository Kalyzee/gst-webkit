# Gst Webkit

Easiest way to build overlays with Gstreamer

Test pipeline

```
gst-launch-1.0 videotestsrc ! video/x-raw, format=ARGB, width=1280, height=720 ! videoconvert ! webkitoverlayfilter url=http://www.google.com ! videoconvert ! xvimagesink
```
