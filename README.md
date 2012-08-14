ofxVideoPipe
============

An OpenFrameworks addon for reading from a video pipe/FIFO created by FFMPEG,
or, potentially, anything that can write a PPM image stream.

Usage
-----

First, setup your OF app to use ofxVideoPipe.

in `testApp.h`:
```c++
#pragma once

#include "ofMain.h"
#include "ofxVideoPipe.h"

class testApp : public ofBaseApp{
public:
    void setup();
    void update();
    void draw();
    void exit();

private:
    ofxVideoPipe video;
};
```

in `testApp.cpp`:

```c++
#include "NewsApp.h"

void NewsApp::setup(){
    video.open("fifo.ppm"); // <---- remember this for FFMPEG
    video.setFrameRate(20);
}

void NewsApp::update(){
    video.update();
}

void NewsApp::draw(){
    video.draw(0, 0);
}

void NewsApp::exit(){
    video.close();
}

```

Now create a named pipe and start FFMPEG writing a stream to the pipe. In this
case, we'll watch an RTMP stream, but it could be any input format FFMPEG
supports.

```bash
cd <application directory>
mkfifo bin/data/fifo.ppm
ffmpeg -i "rtmp://example.com/live/ playpath=foo.sdp" \
-an -f image2pipe -vcodec ppm -r 20 -y bin/data/fifo.ppm
```

The second line of the FFMPEG command is the important bit, as it ensures
FFMPEG outputs in the format ofxVideoPipe expects. You should set `-r` (frame
rate) to whatever you pass to `ofxVideoPipe::setFrameRate` in your app.

Once FFMPEG is ready, it should look like this:

```
# ...
Input #0, flv, from 'rtmp://example.com/live/ playpath=foo.sdp':
  Metadata:
    frameWidth      : 480
    frameHeight     : 272
    displayWidth    : 480
    displayHeight   : 272
    audiochannels   : 1
  Duration: N/A, start: 0.000000, bitrate: 335 kb/s
    Stream #0:0: Video: h264 (Constrained Baseline), yuv420p, 480x272, 294 kb/s, 25 tbr, 1k tbn, 50 tbc
    Stream #0:1: Audio: aac, 48000 Hz, stereo, s16, 40 kb/s
```

It won't start writing frames until your app is running. So...

Finally, build and run your app.

License
-------

Copyright (C) 2012 Ian Smith-Heisters

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
