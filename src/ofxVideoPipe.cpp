#include "ofxVideoPipe.h"

/*******************************************************************************
 * Headers
 ******************************************************************************/

ofxVideoPipe::PPMHeader::PPMHeader() :
channels(3), width(0), height(0), depth(0)
{}

ofxVideoPipe::PPMHeader::PPMHeader(PPMHeader &mom){
    type = mom.type;
    width = mom.width;
    height = mom.height;
    channels = mom.channels;
    depth = mom.depth;
    error << mom.error.str();
}

void ofxVideoPipe::PPMHeader::reset(){
    type = "";
    width = 0;
    height = 0;
    channels = 3;
    depth = 0;
    error.str("");
}

int ofxVideoPipe::PPMHeader::dataSize(){
    return width * height * channels;
}

bool ofxVideoPipe::PPMHeader::good(){
    return error.str().empty();
}

/*******************************************************************************
 * Frames
 ******************************************************************************/

void ofxVideoPipe::PPMFrame::writeTo(ofPixels & pixels){
    pixels.setFromPixels(
                         (unsigned char *)getBinaryBuffer(),
                         header.width,
                         header.height,
                         header.channels
                         );
}

void ofxVideoPipe::PPMFrame::reset(){
    header.reset();
}

/*******************************************************************************
 * Video Pipe Class
 ******************************************************************************/

/*******************************************************************************
 * Threading
 */

void ofxVideoPipe::threadedFunction(){
    while(isThreadRunning()){
        readFrame();
        idle();
    }
}

/*******************************************************************************
 * Timing
 */

// cribbed from OF core
void ofxVideoPipe::idle(){
    if (ofGetFrameNum() != 0 && isFrameRateSet == true){
		int diffMillis = ofGetElapsedTimeMillis() - prevMillis;
		if (diffMillis > millisForFrame){
			; // we do nothing, we are already slower than target frame
		} else {
			int waitMillis = millisForFrame - diffMillis;
#ifdef TARGET_WIN32
            Sleep(waitMillis);         //windows sleep in milliseconds
#else
            usleep(waitMillis * 1000);   //mac sleep in microseconds - cooler :)
#endif
		}
	}
	prevMillis = ofGetElapsedTimeMillis(); // you have to measure here
}

// cribbed from OF core
void ofxVideoPipe::setFrameRate(float targetRate){
    // given this FPS, what is the amount of millis per frame
    // that should elapse?
    
    // --- > f / s
    
    if (targetRate == 0){
        isFrameRateSet = false;
        return;
    }
    
    isFrameRateSet 			= true;
    float durationOfFrame 	= 1.0f / (float)targetRate;
    millisForFrame 			= (int)(1000.0f * durationOfFrame);
}

/*******************************************************************************
 * Update
 */

void ofxVideoPipe::update(){
    if(!isFrameNew()) return;
    
    int oldWidth = pixels.getWidth();
    int oldHeight = pixels.getHeight();
    
    updatePixels();
    
    if(oldWidth != pixels.getWidth() || oldHeight != pixels.getHeight()){
        onSizeChangedData data(pixels.getWidth(), pixels.getHeight());
        ofNotifyEvent(onSizeChanged, data, this);
    }
    
    frameImage.setFromPixels(getPixelsRef());
}

ofPixelsRef ofxVideoPipe::getPixelsRef(){
    if(isFrameNew()){
        ofLogWarning() << "Returning pixels from ofxVideoPipe::getPixelsRef, but they are not up to date. Call updatePixels first.";
    }
    
    return pixels;
}

void ofxVideoPipe::updatePixels(){
    lock();
    if(isFrameChanged){
        currentFrame.writeTo(pixels);
        isFrameChanged = false;
    }
    unlock();
}

bool ofxVideoPipe::isFrameNew(){
    lock();
    bool changed = isFrameChanged;
    unlock();
    return changed;
}

/*******************************************************************************
 * Draw
 */

void ofxVideoPipe::draw(int x, int y){
    frameImage.draw(x, y);
}


/*******************************************************************************
 * Opening, closing
 */

void ofxVideoPipe::open(string _filename){
    filename = _filename;
    frameImage.allocate(1, 1, OF_IMAGE_COLOR);
    
    pipe.open(filename, ofFile::ReadOnly, true);
    startThread();
}

void ofxVideoPipe::close(){
    waitForThread(true);
    pipe.close();
}

/*******************************************************************************
 * Reading data
 */

string ofxVideoPipe::readLine(){
    string buffer;
    getline(pipe, buffer);
    return buffer;
}

void ofxVideoPipe::readHeader(){
    PPMHeader & header = currentFrame.header;
    
    header.type = readLine();
    if(header.type != "P6"){
        header.error << "PPM type identifier not found in header.";
        return;
    }
    
    istringstream dimensions(readLine());
    dimensions >> header.width;
    dimensions >> header.height;
    if(header.width == 0 || header.height == 0){
        header.error << "Invalid dimensions for PPM stream. " << header.width << header.height;
        return;
    }
    
    header.depth = ofToInt(readLine());
    return;
}

void ofxVideoPipe::readFrame(){
    lock();
    
    currentFrame.reset();
    
    readHeader();
    
    if(currentFrame.good()){        
        char * buffer = new char[currentFrame.dataSize()];
        pipe.read(buffer, currentFrame.dataSize());
        currentFrame.set(buffer);
        
        isFrameChanged = true;
    } else {
        ofLogError() << "Error opening PPM stream for reading:";
        ofLogError() << currentFrame.errors();
   }
    
    unlock();
}
