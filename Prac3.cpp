//==============================================================================
// Copyright (C) John-Philip Taylor
// tyljoh010@myuct.ac.za
//
// This file is part of the EEE4120F Course
//
// This file is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>
//
// This is an adaptition of The "Hello World" example avaiable from
// https://en.wikipedia.org/wiki/Message_Passing_Interface#Example_program
//==============================================================================

//---------- STUDENT NUMBERS --------------------------------------------------
// EDWIAN004
// SPRAGO001
//-----------------------------------------------------------------------------


// Includes needed for the program
#include "Prac3.h"
#include <iostream>

unsigned char* getMedian(unsigned char *){
 return 0;
};
//implements a method to send image data in chunks to slave processes to median filter an image - useful as a preprocessing technique to remove salt and pepper noise etc.
void Master () {
 int  j;             //! j: Loop counter
 MPI_Status stat;    //! stat: Status of the MPI application

 printf("0: We have %d processors\n", numprocs);
 // Read the input image
 if(!Input.Read("Data/greatwall.jpg")){
  printf("Cannot read image\n");
  return;
 }

 int chunkSize = Input.Height*Input.Width*Input.Components/(numprocs-1); //total size of chunks sent to each process.
 int chunkHeight = Input.Height/(numprocs-1); //height of chunk - total image height divided by number of slaves.
 int totalSize = Input.Height*Input.Width*Input.Components; //Total image size including components.
 int chunkWidth = Input.Width * Input.Components; //width of chunk - same as original image.
 int windowHeight = 10; //Declare window dimensions for median filter here.
 int windowWidth = 10;
 int startRow = 0; //loop variable
 unsigned char * linearChunk = new unsigned char[chunkSize]; //linear array to store 2D image array in chunks sent to each process.


printf("Sending...\n");
//loops to nr of slaves and creates linear chunk array from 2D image, sends in chunk dimensinos, window dimensions and the chunk.
for(int j = 1; j < numprocs; j++){
     //flatten 2D array chunk:
    int counter = 0;
    for (int y = startRow; y < startRow + chunkHeight; y++){
        for (int x = 0; x < Input.Width*Input.Components; x++, counter++){
            linearChunk[counter] = Input.Rows[y][x];
        }
    }
    MPI_Send(&chunkSize, 32, MPI_BYTE, j, TAG, MPI_COMM_WORLD);
    MPI_Send(&chunkHeight, 32, MPI_BYTE, j, TAG, MPI_COMM_WORLD);
    MPI_Send(&chunkWidth, 32, MPI_BYTE, j, TAG, MPI_COMM_WORLD);
    MPI_Send(&windowHeight, 32, MPI_BYTE, j, TAG, MPI_COMM_WORLD);
    MPI_Send(&windowWidth, 32, MPI_BYTE, j, TAG, MPI_COMM_WORLD);
    MPI_Send(linearChunk, chunkSize, MPI_CHAR, j, TAG, MPI_COMM_WORLD);
    startRow += chunkHeight;
}

 // Allocated RAM for the output image
 if(!Output.Allocate(Input.Width, Input.Height, Input.Components)) return;

printf("Receiving...\n");
//loop through all slaves receiving data into master process and add it to the output image array. Chunk is received as a 1D array and loaded into the 2D image array - Output.
for (int j=1; j < numprocs; j++){
    MPI_Recv(linearChunk, chunkSize, MPI_CHAR, j, TAG, MPI_COMM_WORLD, &stat);
    int counter = 0;
    for (int y = (j-1)*chunkHeight; y < j*chunkHeight; y++){
        for (int x = 0; x< Input.Width*Input.Components; x++, counter++){
            Output.Rows[y][x] = linearChunk[counter];
        }
    }
}

 // Write the output image
 if(!Output.Write("Data/Output.jpg")){
  printf("Cannot write image\n");
  return;
 }


printf("End of Experiment code code...\n\n");
}
//------------------------------------------------------------------------------

/** This is the Slave function, the workers of this MPI application. */
void Slave(int ID){
    tic(); //time each slave's functioning.

    MPI_Status stat; //Represents status of MPI functioning.
    
    //initialize variables to store received data.
    int chunkSize = 0;
    int chunkHeight = 0;
    int chunkWidth = 0;
    int windowHeight = 0;
    int windowWidth = 0;
    
    //Various receive lines to receive data into variables declared above.
    MPI_Recv(&chunkSize, 32, MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);
    MPI_Recv(&chunkHeight, 32, MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);
    MPI_Recv(&chunkWidth, 32, MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);
    MPI_Recv(&windowHeight, 32, MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);
    MPI_Recv(&windowWidth, 32, MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);
    //Create linear chunk from data received, and receive a linear chunk into it.
    unsigned char * linearChunk = new unsigned char[chunkSize];
    MPI_Recv(linearChunk, chunkSize, MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);

    //Dynamically allocate 2D arrays onto the heap to allow us to process data in a 2D form for median filtering.
    unsigned char ** image = new unsigned char*[chunkHeight];
    unsigned char ** output = new unsigned char*[chunkHeight];
    //Create a window 1D array in which neighbour values around a point are stored and ordered and the median is found.
    unsigned char * window= new unsigned char[windowHeight*windowWidth];

    //represents the edge cases so that when pulling values into the window array, out of bounds errors are avoided.
    int edgex = windowHeight/2;
    int edgey = windowWidth/2;
    
    int counter = 0;
    int pos = 0;
    //instantiates the dynamic arrays.
    for (int y = 0; y < chunkHeight; y++){
        image[y] = new unsigned char[chunkWidth];
        output[y] = new unsigned char[chunkWidth];
        for (int x = 0; x< chunkWidth; x++, counter++){
            image[y][x] = linearChunk[counter];
            output[y][x] = linearChunk[counter];
        }
    }

    //Start at edgex and edgey and go to the corresponding edges on the image.
    for (int x = edgex; x < chunkHeight - edgex; x++){ //start at half the window size across and down so we can get median values around it.
        for (int y = edgey; y < chunkWidth - edgey; y++){
            counter = 0;

            //edgex - windowHeight/2 = 0 therefore one can loop in a window around the specific value being worked on i.e. image[x][y].
            for (int k = -windowHeight/2; k <= windowHeight/2; k++){ //get all values in a box around a central point - the value in question.
                for (int j =-windowWidth/2*3; j<=windowWidth/2*3; j+=3){ //move 3 along each time to get corresponding R,G and B values.
                    window[counter] = image[x+k][y+j];
                    ++counter;
                }
            }

            //Loop through window and reorder - linear sort.
            for (int i = 0; i<windowWidth*windowHeight; i++){
                for (int j=i+1; j<windowWidth*windowHeight; j++){
                    if (window[i]>window[j]){
                        unsigned char temp = window[i];
                        window[i] = window[j];
                        window[j] = temp;
                    }
                }
            }
            //output image position is loaded with median value.
            output[x][y] = window[windowHeight*windowWidth/2];
        }
    }
    //Convert 2D image array back into 1D array to be sent back to master.
    counter = 0;
    for (int y = 0; y < chunkHeight; y++){
        for (int x = 0; x< chunkWidth; x++, counter++){
            linearChunk[counter] = output[y][x];
        }
    }

    
    MPI_Send(linearChunk, chunkSize, MPI_CHAR, 0, TAG, MPI_COMM_WORLD); //send computed median filtered chunk back to master.
    std::cout << "Process: " << ID << " | " << "time: " << (double)toc()/1e-3 << " ms" << std::endl; //Send timing data to output.
}
//------------------------------------------------------------------------------

/** This is the entry point to the program. */
int main(int argc, char** argv){
 int myid;

 // MPI programs start with MPI_Init
 MPI_Init(&argc, &argv);

 // find out how big the world is
 MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

 // and this processes' rank is
 MPI_Comm_rank(MPI_COMM_WORLD, &myid);

 // At this point, all programs are running equivalently, the rank
 // distinguishes the roles of the programs, with
 // rank 0 often used as the "master".
 if(myid == 0) Master();
 else Slave (myid);

 // MPI programs end with MPI_Finalize
 MPI_Finalize();
 return 0;
}
//------------------------------------------------------------------------------
