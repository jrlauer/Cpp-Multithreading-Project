// John Lauer
// Homework 6; Due: Sunday, April 29th @ 11:59PM
// Four types of parts, 16 partworkers who produce 3 parts at a time
// 12 Product Workers, Each take 4 parts at a time but only 2 types of parts
// 5 Simulation Requests/Iterations
#include <iostream>
#include <random>
#include <mutex>
#include <thread>
#include <vector>
#include <stdlib.h>
#include <condition_variable>

using namespace std;
mutex m;                                                        //Define the mutex
vector<int> bCapacity = { 6, 5, 4, 3};                          //Define the buffer capacity; with four parts of capacity A=6, B=5, C=4, and D=3
vector<int> bState(4);                                          //Define the buffer state as four because there are four parts
/* In the next line, we define 8 condition variables, one for each part for both the Part Workers and the Product Workers
   This enables us to wake up only the part workers/product workers who currently need to either place or pick up a part
*/
vector<pair<condition_variable, condition_variable>> neededWorker(4);
void PartWorker(int id) {                                   //The Function for the PartWorkers, who produce 3 parts at a time
    int iteration;                                          //Define the integer iteration, which will keep track of which simulation is running
    unique_lock<mutex> l(m);                              //Define the mutex
    vector<int> placeRequest(4);                            //Define a vector of 4 place requests (for the 4 types of parts)
    vector<bool> updatePlace(4);                            //Define a vector of places which need to be updated (for the 4 types of parts)
    for(iteration = 1; iteration <= 5; iteration++){        //Simulate the PartWorker function 5 times
        for (int x = 0; x < 3; x++) {                       //Create a request for 3 parts of random type
            placeRequest[rand() % 4]++;                     //Set to 4 so it picks one of the four types each iteration
        }
        for (int x = 0; x < 4; x++) {                       //Iterate through the four types
            if (placeRequest[x] != 0){                      //If a place request is not equal to 0
                updatePlace[x] = true;                      //Set updatePlace to true because it needs to be updated
            }
        }
        while (!(placeRequest[0] == 0 && placeRequest[1] == 0 && placeRequest[2] == 0 && placeRequest[3] == 0)) { //While none of the placeRequests = 0 (Until your parts have been made)
            //Output all of the current information about the parts being made
            cout << "PartWorker ID: " << id << endl;
            cout << "Iteration: " << iteration << endl;
            cout << "Buffer State: (" << bState[0] << "," << bState[1] << "," << bState[2] << "," << bState[3] << ")" << endl;
            cout << "Place Request: (" << placeRequest[0] << "," << placeRequest[1] << "," << placeRequest[2] << "," << placeRequest[3] << ")" << endl;
            for (int x = 0; x < 4; x++) { //Go through the 4 types of parts
                if (bState[x] == bCapacity[x]){ //If the state of the buffer is equal to the capacity of the buffer
                    continue;                   //Go to the end of the for loop because we cannot make any more parts
                }
                if (!updatePlace[x]){     //If nothing needs to be updated:
                    continue;             //Go to the end of the for loop because we are done
                }
                if ((bState[x] + placeRequest[x]) <= bCapacity[x]) {   //If the current amount of parts plus the amount being created is less than or equal to the capacity
                    bState[x] += placeRequest[x];                      //Add/Create the new parts
                    placeRequest[x] = 0;                               //Set the request equal to 0 because we are done
                }
                else {                                                 //If the current amount plus the amount being created is greater than the capacity
                    placeRequest[x] -= (bCapacity[x] - bState[x]);     //Make the place request the amount of parts that need to still be created
                    bState[x] = bCapacity[x];                          //Make all the parts that can possibly be made
                }
                neededWorker[x].second.notify_one();                   //Make note of which worker is making the parts
            }
            //Output the current information after the parts have been made (or attempted to be made)
            cout << "Updated Buffer State: (" << bState[0] << "," << bState[1] << "," << bState[2] << "," << bState[3] << ")" << endl;
            cout << "Updated Place Request: (" << placeRequest[0] << "," << placeRequest[1] << "," << placeRequest[2] << "," << placeRequest[3] << ")" << endl;
            cout << endl;                   //Output another line to separate each iteration so it is easy to read
            /*
             This following section updates information/variables now that the parts have been made/attempted to be made
            */
            for (int x = 0; x < 4; x++) {
                if (placeRequest[x] != 0) {             //If parts still need to be made for one of the types of parts
                    neededWorker[x].first.wait(l);    //Have this part worker busy wait until the buffer state is not equal to the buffer capacity
                    for (int x = 0; x < 4; x++) {       //For each part
                        if (placeRequest[x] != 0 && bState[x] != bCapacity[x]){ //If parts still need to be made and the capacity has not been reached
                            updatePlace[x] = true;     //Then we say that the place still needs to be updated
                        }
                        else {                         //If this is not the case
                            updatePlace[x] = false;    //Set updatePlace to false because we are done
                        }
                    }
                    break;  //Stop the for loop
                }
            }
        }
    }
}
/*
 The ProductWorker function is similar to the PartWorker function;
 Essentially just reversed because we are taking parts and removing them from the buffer instead of adding them
*/
void ProductWorker(int id) {
    unique_lock<mutex> l(m);                                  //Define the mutex
    vector<int> pickUpRequest(4);                               //Define a vector of 4 pick up requests (one for each type of part)
    vector<bool> updatePickUp(4);                               //Define a vector of pick ups which need to be updated (one for each type of part)
    int iteration;                                              //Define the integer iteratrion, which will keep track of which simulation is running
    for (iteration = 1; iteration <= 5; iteration++){           //Simulate the ProductWorker function 5 times
        int dont = rand() % 4;                            //We can only pick up two types of parts so we have to select a type of part not to pick up
        int dont1 = rand() % 4;                                        //We have to select another that is not the same as the first one we chose not to pick up
        while (dont1 == dont){
            dont1 = rand() % 4;
        }
        for (int x = 0; x < 4; x++) {
            int pickUpType = rand() % 4;                    //Select one of the types of parts
            if (pickUpType == dont || pickUpType == dont1) {                 //If we selected a type of part that we said we were not going to pick up
                x--;                                  //Go to a different type of part
                continue;                             //Go to the end of the for loop and try again
            }
            while (pickUpRequest[pickUpType] == 4) { //If the request of the part type is equal to 4
                pickUpType = rand() % 4;             //Pick a new type of part becuase we need two types of parts
            }
            pickUpRequest[pickUpType]++;            //Update the request because now the worker wants to pick up this type of part
        }
        for (int x = 0; x < 4; x++) {           //Go through each type of part
            if (pickUpRequest[x] != 0){         //If the request is not equal to 0
                updatePickUp[x] = true;         //Set updatePickUp to true because parts still need to be picked up
            }
        }
        while (!(pickUpRequest[0] == 0 && pickUpRequest[1] == 0 && pickUpRequest[2] == 0 && pickUpRequest[3] == 0)) { //Until all of the parts have been picked up:
            /*
             Output all of the current information about the parts needed to be picked up
            */
            cout << "ProductWorker ID: " << id << endl;
            cout << "Buffer State: (" << bState[0] << "," << bState[1] << "," << bState[2] << "," << bState[3] << ")" << endl;
            cout << "Pickup Request: (" << pickUpRequest[0] << "," << pickUpRequest[1] << "," << pickUpRequest[2] << "," << pickUpRequest[3] << ")" << endl;
            cout << "Iteration number:" << iteration << endl;
            for (int x = 0; x < 4; x++) {   //For each of the types of parts
                if (bState[x] == 0){        //If the buffer state for the specified part is equal to zero
                    continue;               //Go to the end of the for loop because we are not able to pick up any parts
                }
                if (!updatePickUp[x]){      //If no parts need to be updated
                    continue;               //Go to the end of the for loop because we are done
                }
                if (bState[x] >= pickUpRequest[x]) {    //If the amount of parts of a specific type is greater than or equal to the amount needed to be picked up
                    bState[x] = bState[x] - pickUpRequest[x]; //Reduce the amount of parts in the buffer by the amount that were picked up
                    pickUpRequest[x] = 0;                     //Set the pick up request to 0 because we picked up all the parts that were requested
                }b
                else {                                  //If this is not the case
                    pickUpRequest[x] = pickUpRequest[x] - bState[x]; //Reduce the pick up request by the amount of parts that were available in the buffer
                    bState[x] = 0;                      //Set the number of parts in the buffer to 0 because the worker picked them all up
                }
                neededWorker[x].first.notify_one();     //Make note of the product worker that was attempting to pick up parts
            }
            /*
             The following section will update information/variables now that the parts have been picked up (or attempted to be picked up)
             We begin by outputting the updated information
            */
            cout << "Updated Buffer State: (" << bState[0] << "," << bState[1] << "," << bState[2] << "," << bState[3] << ")" << endl;
            cout << "Updated Pickup Request: (" << pickUpRequest[0] << "," << pickUpRequest[1] << "," << pickUpRequest[2] << "," << pickUpRequest[3] << ")" << endl;
            cout << endl;       //Output an extra line to separate each iteration so it is easy to read
            for (int x = 0; x < 4; x++) {           //For each part
                if (pickUpRequest[x] != 0) {        //If parts still need to be picked up
                    neededWorker[x].second.wait(l);   //Make note of the worker that still needs to pick up parts after new ones have been produced
                    for (int x = 0; x < 4; x++) {       //Go through each type of part
                        if (pickUpRequest[x] != 0 && bState[x] != 0){   //If parts still need to be picked up and the buffer state is not 0 (there are still parts)
                            updatePickUp[x] = true;                     //Set updatePickUp to true because parts still need to be picked up
                        }
                        else{                                           //If this is not the case
                            updatePickUp[x] = false;                    //Set updatePickUp to false because parts do not need to be picked up
                        }
                    }
                    break;      //Exit the foor loop
                }
            }
        }
    }
}
// The main function of the program
int main(){
    const int m = 16, n = 12; //m: number of Part Workers
    //n: number of Product Workers
    thread partW[m];
    thread prodW[n];
    for (int i = 0; i < n; i++){
        partW[i] = thread(PartWorker, i);
        prodW[i] = thread(ProductWorker, i);
    }
    for (int i = n; i<m; i++) {
        partW[i] = thread(PartWorker, i);
    }
    /* Join the threads to the main threads */
    for (int i = 0; i < n; i++) {
        partW[i].join();
        prodW[i].join();
    }
    for (int i = n; i<m; i++) {
        partW[i].join();
    }
    cout << "Finish!" << endl;
    getchar();
    getchar();
    return 0;
}
