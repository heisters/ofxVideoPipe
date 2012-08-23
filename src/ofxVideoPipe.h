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
    struct onSizeChangedData {
        onSizeChangedData(int w, int h) : width(w), height(h) {};
        int width;
        int height;
    };
    
    ofxVideoPipe() : isFrameChanged(false), isPipeOpen(false), filename("") {};
    
    void open(string _filename);
    void close();
    void draw(int x, int y);
    void threadedFunction();
    void update();
    
    bool isFrameNew();
    ofPixelsRef getPixelsRef();
    void updatePixels();
    void setFrameRate(float targetRate);
    
    int getWidth(){ return currentFrame.getWidth(); }
    int getHeight(){ return currentFrame.getHeight(); }
    
    ofEvent< onSizeChangedData > onSizeChanged;
    
private:
    string readLine();
    void readFrame();
    void readHeader();
    void idle();
    void openPipe();
    void closePipe();
    bool tryOpenPipe();
    
    PPMFrame currentFrame;
    ofPixels pixels;
    ofImage frameImage;
    ofFile pipe;
    string filename;
    bool isFrameChanged;
    bool isPixelsChanged;
    bool isFrameRateSet;
    bool isPipeOpen;
    int prevMillis, millisForFrame;
};