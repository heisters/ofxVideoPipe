ofxVideoPipe
============

An OpenFrameworks addon for reading from a video pipe/FIFO created by FFMPEG,
or, potentially, anything that can write a PPM image stream.

Usage
-----

First, setup your OF app to use ofxVideoPipe.

in +testApp.h+:
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

in +testApp.cpp+:

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

    cd <application directory>
    mkfifo bin/data/fifo.ppm
    ffmpeg -i "rtmp://example.com/live/ playpath=foo.sdp" \
    -an -f image2pipe -vcodec ppm -r 20 -y bin/data/fifo.ppm

The second line of the FFMPEG command is the important bit, as it ensures
FFMPEG outputs in the format ofxVideoPipe expects. You should set +-r+ (frame
rate) to whatever you pass to +ofxVideoPipe::setFrameRate+ in your app.
