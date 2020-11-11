#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

// Size of the buffer
#define SIZE 1000

// Counter for num lines read
int countB1 = 0;
// Index where the producer will put the next item
int prod1_idx = 0;
// Index where the consumer will pick up the next item
int con1_idx = 0;

int countB2 = 0;
// Index where the producer will put the next item
int prod2_idx = 0;
// Index where the consumer will pick up the next item
int con2_idx = 0;

int countB3 = 0;
// Index where the producer will put the next item
int prod3_idx = 0;
// Index where the consumer will pick up the next item
int con3_idx = 0;

int outputPosition = 0;

// Buffer between Input and Line Separator
char buffer1[SIZE];

// Buffer between Line Separator and Plus Sign
char buffer2[SIZE];

// Buffer between Plus Sign and Output
char buffer3[SIZE];


// Initialize mutexes
pthread_mutex_t mutexBuffer1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBuffer2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBuffer3 = PTHREAD_MUTEX_INITIALIZER;

// Initialize condition variables
pthread_cond_t fullB1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t emptyB1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t fullB2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t emptyB2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t fullB3 = PTHREAD_COND_INITIALIZER;
pthread_cond_t emptyB3 = PTHREAD_COND_INITIALIZER;

/*********************************************************************
** Description: Function reads chars from input to buffer1
** Returns: Void
** Arguments: None
*********************************************************************/
void* getInput(void* args) {
    
    int loop = 1;
    int i;
    char input[SIZE];
    int sizeInput;
    

    // Read chars from stdin until termination
    while (loop == 1) {

        // Reset and get user input and size
        memset(input, '\0', SIZE);
        fgets(input, SIZE, stdin);
        sizeInput = strlen(input);
        
        // Lock use of buff1
        pthread_mutex_lock(&mutexBuffer1);

        // While there are max chars in the buffer wait to get next input
        while ( countB1 + sizeInput >= SIZE) {
            pthread_cond_wait(&emptyB1, &mutexBuffer1);
        }


        for(i = 0; i < sizeInput; i++){
            // Copy chars into buffer and inc prod1_idx & count
            buffer1[prod1_idx] = input[i];
            prod1_idx = (prod1_idx + 1) % SIZE;
            countB1++;
        }
        

        //check if line is termination
        if (strstr(buffer1, "DONE\n") != NULL) {
            loop = 0;
        }

        // Signal that buffer1 has chars and unlock
        pthread_cond_signal(&fullB1);
        pthread_mutex_unlock(&mutexBuffer1);
        

    }
    
    return NULL;
    
}


/*********************************************************************
** Description: Function replaces newline char with space
** Returns: Void
** Arguments: None
*********************************************************************/
void* lineSeparator(void* args) {
    
    int loop = 1;
    int i;
    char* donePtr;

    while(loop == 1){

        // Lock buffer1 to copy chars
        pthread_mutex_lock(&mutexBuffer1);

        // Wait until signal if buffer1 is empty
        while(countB1 <= 0) {
            pthread_cond_wait(&fullB1, &mutexBuffer1);
        }
        
        // Once buffer1 has chars lock buffer2
        pthread_mutex_lock(&mutexBuffer2);

        //keep track of starting index to replace with spaces
        int sIdx = prod2_idx;

        // Copy buffer1 chars into buffer2 keeping track of indices 
        while(countB1 > 0){
            buffer2[prod2_idx] = buffer1[con1_idx];
            prod2_idx = (prod2_idx + 1) % SIZE;
            con1_idx = (con1_idx + 1) % SIZE;
            countB2++;
            countB1--;
        }

        // Signal buffer1 has room for chars and unlock buffer1
        pthread_cond_signal(&emptyB1);
        pthread_mutex_unlock(&mutexBuffer1);

       
        // If buffer2 has "DONE\n" change end index to just before \n to not change it
        if((donePtr = strstr(buffer2, "DONE\n")) != NULL){
            prod2_idx = prod2_idx - 1;
            loop = 0;
        }

        // Change the rest of the \n to spaces
        int j;
        for(j = sIdx; j < prod2_idx; j++) {
            if (buffer2[j] == '\n') {
                //replace \n with space
                buffer2[j] = 32;
            }
        }

        // Unlock buffer2 which changed chars
        pthread_cond_signal(&fullB2);
        pthread_mutex_unlock(&mutexBuffer2);
        
    }
    
    return NULL;
    
}


/*********************************************************************
** Description: Function replaces pairs of + with pairs of ^
** Returns: Void
** Arguments: None
*********************************************************************/
void* plusSign(void* args) {
    
    int loop = 1;
    char* donePtr;
      
    while (loop == 1) {
        
        // Lock buffer2 to copy into buffer3
        pthread_mutex_lock(&mutexBuffer2);

        // Wait until there are chars in buffer2 
        while(countB2 <= 0) {
            pthread_cond_wait(&fullB2, &mutexBuffer2);
        }

        // Once chars in buffer2 lock buffer3 to copy
        pthread_mutex_lock(&mutexBuffer3);

        // Keep track of starting index
        int sIdx = prod3_idx;

        // While there are chars, copy them to buffer3
        while(countB2 > 0){
            buffer3[prod3_idx] = buffer2[con2_idx];
            prod3_idx = (prod3_idx + 1) % SIZE;
            con2_idx = (con2_idx + 1) % SIZE;
            countB3++;
            countB2--;
        }

        // Signal buffer2 has space and unlock buffer2
        pthread_cond_signal(&emptyB2);
        pthread_mutex_unlock(&mutexBuffer2);

        // If buffer 3 has "DONE\n", change end index to just before
        if((donePtr = strstr(buffer3, "DONE\n")) != NULL){
            prod3_idx = donePtr - buffer3;
            loop = 0;
        }

        // Convert pairs of + to ^
        int j;
        for (j = sIdx; j < prod3_idx - 1; j++) {
            if (buffer3[j] == '+' && buffer3[j + 1] == '+') {
                buffer3[j] = '^';
                buffer3[j+1] = '^';
            }
        }
        
        
        // signal buffer3 has chars and unlock buffer3
        pthread_cond_signal(&fullB3);
        pthread_mutex_unlock(&mutexBuffer3);

        
    }
    
    return NULL;
}


/*********************************************************************
** Description: Function prints buffer to stdout
** Returns: Void
** Arguments: None
*********************************************************************/
void* output(void* args) {
    
    int countOp = 0;
    int outIdx = 0;
    int con4_idx = 0;
    int loop = 1;
    int i;
    char* donePtr;

    char output1[SIZE];
    memset(output1, '\0', SIZE);
    
    
    while (loop == 1) {
        // Lock buffer3 to copy
        pthread_mutex_lock(&mutexBuffer3);

        // Wait until buffer3 has chars to copy
        while(countB3 <= 0) {
            pthread_cond_wait(&fullB3, &mutexBuffer3);
        }

        
        // While chars in buffer3 copy to output array
        while(countB3 > 0){
            output1[outIdx] = buffer3[con3_idx];
            con3_idx = (con3_idx + 1) % SIZE;
            outIdx = (outIdx + 1) % SIZE;
            countB3--;
            countOp++;
        }

        // Signal buffer3 is empty and unlock
        pthread_cond_signal(&emptyB3);
        pthread_mutex_unlock(&mutexBuffer3);

        // Check if output array has "DONE\n" and subtract output to just before DONE
        if((donePtr = strstr(output1, "DONE\n")) != NULL){
            int newIdx = donePtr - output1;
            // Dec count to just before "DONE"
            countOp = countOp - (outIdx - newIdx);
            loop = 0;
        }

        // While there are 80 chars in output array, print them
        // Keep track of indices
        while(countOp >= 80){
            int j;
            for(j = 0; j < 80; j++){
                printf("%c", output1[con4_idx]);
                con4_idx = (con4_idx + 1) % SIZE;
                countOp--;
            }
            printf("\n");
            
        }

        
          
        
    }

    return NULL;
}

int main(int argc, const char * argv[]) {

    memset(buffer1, '\0', SIZE);
    memset(buffer2, '\0', SIZE);
    memset(buffer3, '\0', SIZE);
    
    
    pthread_t inputThread, separateThread, plusThread, outputThread;
    pthread_create(&inputThread, NULL, getInput, NULL);
    pthread_create(&separateThread, NULL, lineSeparator, NULL);
    pthread_create(&plusThread, NULL, plusSign, NULL);
    pthread_create(&outputThread, NULL, output, NULL);
    
    pthread_join(inputThread, NULL);
    pthread_join(separateThread, NULL);
    pthread_join(plusThread, NULL);
    pthread_join(outputThread, NULL);
    
    return 0;
}
