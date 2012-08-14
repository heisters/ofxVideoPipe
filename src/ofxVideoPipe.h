#pragma once

#include "ofMain.h"

class ofxVideoPipe : public ofThread {
    class PPMHeader {
    public:
        PPMHeader();
        PPMHeader(PPMHeader &other);
        int dataSize();
        bool good();
        void reset();
        string type;
        int width;
        int height;
        int depth;
        int channels;
        stringstream error;
    };
    
    class PPMFrame : public ofBuffer {
    public:
        PPMFrame(){};
        void reset();
        void set(const char * data){ ofBuffer::set(data, dataSize()); }
        void writeTo(ofPixels & pixels);
        int dataSize(){ return header.dataSize(); }
        bool good(){ return header.good(); }
        string errors(){ return header.error.str(); }
        int getWidth(){ return header.width; }
        int getHeight(){ return header.height; }
        PPMHeader header;
    };


public:
    ofxVideoPipe() : isFrameChanged(false) {};
    
    void open(string _filename);
    void close();
    void draw(int x, int y);
    void threadedFunction();
    void update();
    
    bool isFrameNew();
    ofPixelsRef getPixelsRef();
    void setFrameRate(float targetRate);
    
    int getWidth(){ return currentFrame.getWidth(); }
    int getHeight(){ return currentFrame.getHeight(); }
    
private:
    string readLine();
    void readFrame();
    void readHeader();
    void idle();
    
    PPMFrame currentFrame;
    ofPixels pixels;
    ofImage frameImage;
    ofFile pipe;
    string filename;
    bool isFrameChanged;
    bool isFrameRateSet;
    int prevMillis, millisForFrame;
};