#include <string>
#include <Arduino.h>
#define MAXSIZE 100
#include <stdint.h>
#define LOG(x) (Serial.print(x))
#define LOGLN(x) (Serial.println(x))
#define LOGERR(x) (Serial.println(x))
    // Arduino provides micros() which returns unsigned long; expose a no-arg macro for parity with host
#define MICROS() (micros())
const byte ledBrightness = 100; //Options: 0=Off to 255=50mA
const byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
const byte ledMode = 3; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
const int sampleRate = 400; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
const int pulseWidth = 411; //Options: 69, 118, 215, 411
const int adcRange = 16384; //Options: 2048, 4096, 8192, 16384
int x = 0;
int secondsperphase = 10;
#include <Wire.h>
#include "MAX30105.h"
MAX30105 particleSensor;                                                                                                                                                                                                                                        
using u16 = uint16_t;
using u32 = uint32_t;
using u8 = uint8_t;
/* 200Hz sample rate = 12000 samples per minute
400Hz sample rate = 24000 samples per minute */
// Provide a host implementation of micros() / micros64() when not building for Arduino.

/*
#if !defined(ARDUINO)
// steady_clock-based microsecond timer (64-bit) for host builds
inline uint64_t micros64() {
    static const auto start = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(now - start).count()
    );
}
// Arduino-compatible 32-bit micros() wrapper (wraps like unsigned long)
inline unsigned long micros() {
    return static_cast<unsigned long>(micros64() & 0xFFFFFFFFu);
}
// Use the same macro name used on Arduino so call sites can use MICROS()
#define MICROS() (micros())
#endif
*/

// return tenths of a millisecond (100 us) rounded to nearest, using integer arithmetic
inline unsigned long int currentTime(){
    return static_cast<u32>((MICROS() + 50u) / 100u); // returns time in tenths of a ms since program launch
}


template <typename timeType, typename waveType>
struct wavelet{
    unsigned long int time;
    waveType wave;
    timeType offset;
};

template <typename timeType, typename waveType>
struct waveNode{
    timeType baseTime;          // offset from which the time of each item will be calculated
    timeType offset[MAXSIZE];
    waveType wave[MAXSIZE];
    waveNode<timeType,  waveType>* next  = NULL;
};

template <class timeType, class waveType>
class waveformPkg{

public:
    waveformPkg(){
        //LOG("Allocating new node ->");
        startNode = new waveNode<timeType,  waveType>;
        realTime = currentTime();
        length = 0;
        //LOG(" done.");
        if (startNode == NULL){
            LOGERR("Fatal: unable to allocate start node\n");
            exit(-1);
        }
        currentNode = startNode;
        wavePtr = currentNode->wave;
        timePtr = currentNode->offset;
        currentNode->baseTime = static_cast<unsigned int>(currentTime()-realTime);
    }

    void enqueue(waveType waveIn){
        unsigned long int timeIn = currentTime() - realTime;
        if (isFull()){
            //LOG("Allocating new node ->");
            currentNode->next = new waveNode<timeType,  waveType>;
            //LOGLN(" done.");
            if (currentNode->next == NULL){
            LOGERR("Fatal: unable to allocate start node\n");
            exit(-1);
        }
            length = 1;
            currentNode = currentNode->next;
            wavePtr = currentNode->wave;
            timePtr = currentNode->offset;
            *(wavePtr) = waveIn;
            currentNode->baseTime = timeIn;
            *(timePtr) = static_cast<timeType>(timeIn - static_cast<unsigned int>(currentNode->baseTime));
        }
        else{
            *(wavePtr+length) = waveIn;
            *(timePtr+length) = static_cast<timeType>(timeIn - static_cast<unsigned int>(currentNode->baseTime));
            length = length + 1;
            
        }
    }

    ~waveformPkg(){
        while (startNode != NULL){
            waveNode<timeType,  waveType>* tempNode = startNode;
            startNode = startNode->next;
            delete tempNode;
        }
    }

    wavelet<timeType, waveType> peek(){
        if (isArrEmpty()){
            LOGERR("Error: peeked an empty structure");
            exit(-1);
        }
        else{
            wavelet<timeType, waveType> item;
            item.wave = *(wavePtr+length-1);
            item.offset = *(timePtr+length-1);
            return item;
        }
    }

    /*
    wavelet<timeType, waveType> pop(){
        if (isArrEmpty()){
            LOGERR("Error: popped an empty structure");
            exit(-1);
        }
        else{
            wavelet<timeType, waveType> item;
            item.wave = *(wavePtr+length-1);
            item.offset = *(timePtr+length-1);
            length--;
            return item;
            
        }
        
    }
    */
    wavelet<timeType, waveType> dequeue(){
        if (isArrEmpty()){
            LOGERR("Error: Dequeued an empty structure");
            exit(-1);
        }
        
        if (startNode == NULL){
            LOGERR("Error: Dequeued an empty structure: 3");
            exit(-1);
        }
        else if (startNode != NULL && cursor == MAXSIZE){
            if (startNode->next != NULL){
                waveNode<timeType,  waveType>* temp = startNode->next;
                delete startNode;
                startNode = temp;
                cursor = 0; 
            }
            else{
                LOGERR("Error: Dequeued an empty structure: 2");
                exit(-1);
            }
        }
        if (startNode != NULL){
            wavelet<timeType, waveType> item;
            item.wave = *(startNode->wave+cursor);
            item.time = static_cast<unsigned long int>(*(startNode->offset+cursor)+startNode->baseTime)+realTime;
            cursor++;
            return item;
        }
        else{
            LOGERR("Error: Dequeued an empty structure: 1");
            exit(-1);
        }
        
    }
    void dequeueToCSV(){
        wavelet<u32, u16> item;
        LOGLN("Time, Waveform");
        int g = 0;
        while(startNode->next != NULL || cursor < length){
            item = dequeue();
            LOG(item.time);
            LOG(",");
            LOGLN(item.wave);
            g++;
            if (g > 10101){
                break;
            }
        }
    }

    unsigned long int getBaseTime(){
        return baseTime;
    }
    bool isArrEmpty(){
        return (length == 0);
    }
    bool isListEmpty(){
        return (cursor == MAXSIZE && startNode->next == NULL);
    }
    bool isFull(){
        return (length == MAXSIZE);
    }
    void setSampleRate(int rate){ // set sample rate in Hz for residual calculation
        differential = (1/rate)*10000;
    }
private:
    waveNode<timeType,  waveType>* startNode = NULL;
    waveNode<timeType,  waveType>* currentNode = NULL;
    int length;
    waveType * wavePtr = NULL;
    unsigned int baseTime; // This is the base time reference for an individual node 
    timeType * timePtr = NULL;;
    int cursor = 0;
    unsigned int differential;
    unsigned long int realTime; // This is the base time reference for all nodes in the array
    //unsigned long int expectedTime;
};

/* 

--- The remainder of this code is dedicated to testing functions for development --- 

*/

waveformPkg<u32, u16> pk1;

bool testruns (waveformPkg<u32, u16>* pk1){
  float adjusted = micros()/1000/secondsperphase - x;
  if (adjusted > 1 && x < 26){
    particleSensor.setup(10*x, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
    LOG("Brightness: ");
    LOGLN(10*x);
    if (x >= 1){
    pk1->dequeueToCSV();}
    x++;
  }
  else if (x >= 26 && x < 31){
    particleSensor.setup(ledBrightness, static_cast<int>(pow(2, x-26)), ledMode, sampleRate, pulseWidth, adcRange);
    LOG("Sampling Average: ");
    LOGLN(static_cast<int>(pow(2, x-26)));
    pk1->dequeueToCSV();
    x++;
  }
  else if (x >= 31 && x < 39){
    particleSensor.setup(ledBrightness, sampleAverage, ledMode, static_cast<int>(50*pow(2, x-32)), pulseWidth, adcRange);
    LOG("Sample Rate: ");
    LOGLN(static_cast<int>(50*pow(2, x-32)));
    pk1->dequeueToCSV();
    x++;
  }
  else if (x >= 39 && x < 43){
    switch (x-39){
      case 0:
        particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, 69, adcRange);
        LOG("Pulse Width: ");
        LOGLN(69);
        pk1->dequeueToCSV();
        break;
      case 1:
        particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, 118, adcRange);
        LOG("Pulse Width: ");
        LOGLN(118);
        pk1->dequeueToCSV();
        break;
      case 2:
        particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, 215, adcRange);
        LOG("Pulse Width: ");
        LOGLN(215);
        pk1->dequeueToCSV();
        break;
      case 3:
        particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, 411, adcRange);
        LOG("Pulse Width: ");
        LOGLN(411);
        pk1->dequeueToCSV();
        break;
    }
    x++;
  }
  else if (x >= 43){
    return true;
  }
  return false;
}

void testdriver1(){LOG("\n\n now enqueueing \n\n");
    waveformPkg <u32, u16>pk1;
    wavelet<u32, u16> item;
    for(int i = 0; i<1010;i++){
        pk1.enqueue(u16(i));
        item = pk1.peek();
        LOG("..");
        LOG(" Item: ");
        LOG(item.wave);
        LOG(" Time: ");
        LOGLN(item.offset);
    }
    LOG("\n\n now Dequeing\n\n");
    pk1.dequeueToCSV();
    return;
}

  void testDriver2(){
    u16 in = u16(particleSensor.getIR());
    bool stop = false;
    if (millis()/1000/secondsperphase - x > 1){
      stop = testruns(&pk1);
    }
    if (stop == true){
      LOGLN("Done!");
      while (!Serial.available()){
      }
      Serial.read();
    }
    pk1.enqueue(in);
  }
