#define MAXSIZE 100
#include <iostream>
#include <stdint.h>
#include <math.h>
/*
200Hz sample rate = 12000 samples per minute
400Hz sample rate = 24000 samples per minute
*/
using u16 = uint16_t;
using u32 = uint32_t;
using u8 = uint8_t;
// placeholder function
unsigned int micros(){
    return 1;
}

inline unsigned long int currentTime(){
    return round(micros()/100); // returns time in tenths of a ms since program launch
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
    try{
        startNode = new waveNode<timeType,  waveType>;
    }
    catch(const std::bad_alloc& e){
        std::cerr << "Error: Ran out of heap memory for waveformPkg: " << e.what() << '\n';
    }
    currentNode = startNode;
    wavePtr = currentNode->wave;
    timePtr = currentNode->offset;
    currentNode->baseTime = currentTime();
    dequeueNode = startNode; 
}

void push(waveType waveIn){
    if (isFull()){
        try{
            currentNode->next = new waveNode<timeType,  waveType>;
        }
        catch(const std::bad_alloc& e){
            std::cerr << "Error: Ran out of heap memory for waveform: " << e.what() << '\n';
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
    if (isEmpty()){
        std::cerr<<"Error: peeked an empty structure";
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
    if (isEmpty()){
        std::cerr<<"Error: popped an empty structure";
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
    if (isEmpty()){
        std::cerr<<"Error: Dequeued an empty structure";
        exit(-1);
    }
    static int cursor = 0;
    if (dequeueNode == NULL){
        std::cerr<<"Error: Dequeued an empty structure";
        exit(-1);
    }
    else if (dequeueNode != NULL && cursor == MAXSIZE){
        if (dequeueNode->next != NULL){
            waveNode<timeType,  waveType>* temp = dequeueNode->next;
            delete dequeueNode;
            dequeueNode = temp;
            cursor = 0; 
        }
        else{
            return {10,10};
        }
    }
    if (dequeueNode != NULL){
        wavelet<timeType, waveType> item;
        item.wave = *(dequeueNode->wave+cursor);
        item.offset = *(dequeueNode->offset+cursor);
        cursor++;
        return item;
    }
    
}

timeType getBaseTime(){
    return baseTime;
}
bool isEmpty(){
    return (length == 0);
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
waveNode<timeType,  waveType>* dequeueNode = NULL;
int length = 0;
waveType * wavePtr = NULL;
timeType baseTime;
timeType * timePtr = NULL;;
};



int main()
{
    std::cout<<"Hello World";
    waveformPkg <int, u16>pk1;
    wavelet<int, u16> item;

    // std::cout << item.wave << " at " << item.offset << "\n";
    std::cout<<"\n\n now Pushing \n\n";
    for(int i = 0; i<10100;i++){
        pk1.push(u16(i));
        item = pk1.peek();
        std::cout << item.wave << " ";
    }
    
    std::cout<<"\n\n now Dequeing\n\n";
    for(int i = 0; i<10100;i++){
        item = pk1.dequeue();
        std::cout << item.wave << " ";
    }
    



    return 0;
}