#define MAXSIZE 100
#include <stdint.h>
#if defined(ARDUINO)
    #include <Arduino.h>
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
    //#define try if (true)
    //#define catch else if (false)
    #include <Wire.h>
    #include "MAX30105.h"
    MAX30105 particleSensor;
#else
    #include <iostream>
    #include <chrono>
    #define LOG(x) (std::cout << (x))
    #define LOGLN(x) (std::cout << (x) << std::endl)
    #define LOGERR(x) (std::cerr << (x) << std::endl)
#endif
using u16 = uint16_t;
using u32 = uint32_t;
using u8 = uint8_t;
/* 200Hz sample rate = 12000 samples per minute
400Hz sample rate = 24000 samples per minute */
// Provide a host implementation of micros() / micros64() when not building for Arduino.
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

// return tenths of a millisecond (100 us) rounded to nearest, using integer arithmetic
inline unsigned long int currentTime(){
    return static_cast<u32>((MICROS() + 50u) / 100u); // returns time in tenths of a ms since program launch
}

template <typename timeType, typename waveType>
struct wavelet{
    timeType offset;
    waveType wave;
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
    LOG("Allocating new node ->");
    startNode = new waveNode<timeType,  waveType>;
    LOG(" done.");
    if (startNode == NULL){
        LOGERR("Fatal: unable to allocate start node\n");
        exit(-1);
    }
    currentNode = startNode;
    wavePtr = currentNode->wave;
    timePtr = currentNode->offset;
    currentNode->baseTime = currentTime();
}

void enqueue(waveType waveIn){
    if (isFull()){
        LOG("Allocating new node ->");
        currentNode->next = new waveNode<timeType,  waveType>;
        LOG(" done.");
        if (currentNode->next == NULL){
        LOGERR("Fatal: unable to allocate start node\n");
        exit(-1);
    }
        length = 1;
        currentNode = currentNode->next;
        *(currentNode->wave) = waveIn;
        *(currentNode->offset) = currentTime() - baseTime;
        currentNode->baseTime = currentTime();
        wavePtr = currentNode->wave;
        timePtr = currentNode->offset;
    }
    else{
        unsigned long int timeIn = currentTime();
        *(currentNode->wave+length) = waveIn;
        *(currentNode->offset+length) = timeIn;
        length++;
        
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
        item.offset = *(startNode->offset+cursor);
        cursor++;
        return item;
    }
    else{
        LOGERR("Error: Dequeued an empty structure: 1");
        exit(-1);
    }
    
}

timeType getBaseTime(){
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
/*
int getLength(){
    return length;
}
    */
private:
waveNode<timeType,  waveType>* startNode = NULL;
waveNode<timeType,  waveType>* currentNode = NULL;
int length = 0;
waveType * wavePtr = NULL;
timeType baseTime;
timeType * timePtr = NULL;;
int cursor = 0;



};

void testdriver1(){LOG("\n\n now enqueueing \n\n");
    waveformPkg <int, int>pk1;
    wavelet<int, int> item;
    for(int i = 0; i<1010;i++){
        pk1.enqueue(i);
        item = pk1.peek();
        
        LOG(" Item: ");
        LOG(item.wave);
        LOGLN(" Time: ");
        LOGLN(item.offset);
    }
    
    LOG("\n\n now Dequeing\n\n");

    while(pk1.isListEmpty() == false){
        item = pk1.dequeue();
        LOG(" Item: ");
        LOG(item.wave);
        LOG(" Time: ");
        LOG(item.offset);
    }
    



    return;
}