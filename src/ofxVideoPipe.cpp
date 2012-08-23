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
 *
 * much of this section is cribbed from OF core's ofSetFrameRate
 */

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
    openPipe();
    startThread();
}

void ofxVideoPipe::openPipe(){
    if (isPipeOpen) return;
    
    // lots of voodoo to avoid deadlocks when the fifo writer goes away
    // temporarily.
    
    int fd_pipe = ::open(ofToDataPath(filename).c_str(), O_RDONLY | O_NONBLOCK);
    if (fd_pipe < 0) {
        ofLogError() << "Error opening pipe " << filename << ": " << errno;
        return;
    }
    
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd_pipe, &set);
    
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    int ready = select(fd_pipe + 1, &set, NULL, NULL, &timeout);
    ::close(fd_pipe);
    
    if (ready < 0) {
        ofLogError() << "Error waiting for pipe to be ready for reading.";
        return;
    } else if (ready == 0) {
        // timeout
        return;
    }
    
    pipe.open(filename, ofFile::ReadOnly, true);
    isPipeOpen = true;
}

void ofxVideoPipe::closePipe(){
    if (!isPipeOpen) return;
    
    pipe.close();
    isPipeOpen = false;
}

void ofxVideoPipe::close(){
    waitForThread(true);
    closePipe();
}

bool ofxVideoPipe::tryOpenPipe(){
    if (filename.empty() || !frameImage.bAllocated()) {
        return false;
    }
    
    openPipe();
    return true;
}

/*******************************************************************************
 * Reading data
 */

void ofxVideoPipe::readFrame(){
    if(!isPipeOpen && !tryOpenPipe()){
        ofLogError() << "Attempting to read, but the pipe has not been properly initialized.";
    }

    lock();
    
    currentFrame.reset();
    
    readHeader();
    
    if (pipe.good() && currentFrame.good()) {        
        char * buffer = new char[currentFrame.dataSize()];
        pipe.read(buffer, currentFrame.dataSize());
        currentFrame.set(buffer);
        
        isFrameChanged = true;
    } else if (!pipe.good()) {
        ofLogNotice() << "Attempting to reopen stream.";
        closePipe();
    } else {
        ofLogError() << "Error opening PPM stream for reading: " << currentFrame.errors();
    }
    unlock();
}

string ofxVideoPipe::readLine(){
    string buffer;
    getline(pipe, buffer);
    
    if(!pipe.good()){
        if(pipe.eofbit){
            ofLogError() << "Pipe has been closed.";
        } else if (pipe.failbit) {
            ofLogError() << "Reading from pipe failed.";
        } else if (pipe.badbit) {
            ofLogError() << "Pipe in error state.";
        }
    }
    
    return buffer;
}

void ofxVideoPipe::readHeader(){
    PPMHeader & header = currentFrame.header;
    
    header.type = readLine();
    if(!pipe.good()) return;
    
    if(header.type != "P6"){
        header.error << "PPM type identifier not found in header.";
        return;
    }
    
    istringstream dimensions(readLine());
    if(!pipe.good()) return;

    dimensions >> header.width;
    dimensions >> header.height;
    if(header.width == 0 || header.height == 0){
        header.error << "Invalid dimensions for PPM stream. " << header.width << header.height;
        return;
    }
    
    header.depth = ofToInt(readLine());
    if(!pipe.good()) return;

    return;
}