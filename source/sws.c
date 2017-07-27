/* 
 * File: sws.c
 * Author: Alex Brodsky
 * Purpose: This file contains the implementation of a simple web server.
 *          It consists of two functions: main() which contains the main 
 *          loop accept client connections, and serve_client(), which
 *          processes each client request.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "network.h"

#include <sys/stat.h>
#include <fcntl.h>

#define MAX_HTTP_SIZE 8192                 /* size of buffer to allocate */
#define MAX_INPUT_LINE 255                //max line length to accept as input

typedef struct RequestControlBlock{
   // This is the request control table to store state info for each request by
   // the client.
   // It should be initialized as an array of RequestControlTables and
   // should associate a spot in the array with a request from the client
   // - Matt

   int sequenceNumber;  //similar to process ID. sequence numbers start at 1
   int fileDescriptor;  //returned by network_wait() in network.h
   FILE * fileName;     //filename given by the client
   int bytesRemaining;  //the number of bytes remaining to be sent
   int quantum;         //max number of bytes to send

   char fname[100];        // req name of file
}RequestControlBlock; 

//typedef struct RequestControlBlock * rcbPtr;

bool rr = false;
bool sjf = false;
bool mlfb = false;

int seqCounter = 1; //sequence counter. increments for each request

/* This function takes a file handle to a client, reads in the request, 
 *    parses the request, and sends back the requested file.  If the
 *    request is improper or the file is not available, the appropriate
 *    error is sent back.
 * Parameters: 
 *             fd : the file descriptor to the client connection
 * Returns: None
 */
static void serve_client( int fd ) {
   static char *buffer;                              /* request buffer */
   char *req = NULL;                                 /* ptr to req file */
   char *brk;                                        /* state used by strtok */
   char *tmp;                                        /* error checking ptr */
   FILE *fin;                                        /* input file handle */
   int len;                                          /* length of data read */

   // TODO: add 8kb, 64kb, and RR buffer queues for MLFB scheduling
   if( !buffer ) {                                   /* 1st time, alloc buffer */

      buffer = malloc( MAX_HTTP_SIZE );

      if( !buffer ) {                                 /* error check */
         perror( "Error while allocating memory" );
         abort();
      }
   }

   memset( buffer, 0, MAX_HTTP_SIZE );
   if( read( fd, buffer, MAX_HTTP_SIZE ) <= 0 ) {    /* read req from client */
      perror( "Error while reading request" );
      abort();
   } 

   /* standard requests are of the form
    *   GET /foo/bar/qux.html HTTP/1.1
    * We want the second token (the file path).
    */
   tmp = strtok_r( buffer, " ", &brk );              /* parse request */
   if( tmp && !strcmp( "GET", tmp ) ) {
      req = strtok_r( NULL, " ", &brk );
   }


   if( !req ) {                                      /* is req valid? */
      len = sprintf( buffer, "HTTP/1.1 400 Bad request\n\n" );
      write( fd, buffer, len );                       /* if not, send err */
   } else {                                          /* if so, open file */
      req++;                                          /* skip leading / */
      fin = fopen( req, "r" );                        /* open file */
      if( !fin ) {                                    /* check if successful */
         len = sprintf( buffer, "HTTP/1.1 404 File not found\n\n" );  
         write( fd, buffer, len );                     /* if not, send err */
      } else {                                        /* if so, send file */
         len = sprintf( buffer, "HTTP/1.1 200 OK\n\n" );/* send success code */


         do {                                          /* loop, read & send file */
            len = fread( buffer, 1, MAX_HTTP_SIZE, fin );  /* read file chunk */
            if( len < 0 ) {                             /* check for errors */
               perror( "Error while writing to client" );
            } else if( len > 0 ) {                      /* if none, send chunk */
               len = write( fd, buffer, len );
               if( len < 1 ) {                           /* check for errors */
                  perror( "Error while writing to client" );
               }
            }
         } while( len == MAX_HTTP_SIZE );              /* the last chunk < 8192 */
         fclose( fin );
      }
   }
   close( fd );                                     /* close client connectuin*/
}

bool blockExists(RequestControlBlock newBlock, RequestControlBlock rct[]){
   int i;

   for (i = 0; i < seqCounter-1; i++){
      if (strcmp(newBlock.fname, rct[i].fname)==0 ){
         return true;
         //    printf("Block exists\n");
      }
   }

   return false;
}

RequestControlBlock process_client( int fd, RequestControlBlock rct[] ) {
   static char *buffer;                              /* request buffer */
   char *req = NULL;                                 /* ptr to req file */
   char *brk;                                        /* state used by strtok */
   char *tmp;                                        /* error checking ptr */
   FILE *fin;                                        /* input file handle */
   int len;                                          /* length of data read */

   RequestControlBlock newBlock;
   char fnametemp[100];

   // TODO: add 8kb, 64kb, and RR buffer queues for MLFB scheduling
   if( !buffer ) {                                   /* 1st time, alloc buffer */

      buffer = malloc( MAX_HTTP_SIZE );

      if( !buffer ) {                                 /* error check */
         perror( "Error while allocating memory" );
         abort();
      }
   }

   memset( buffer, 0, MAX_HTTP_SIZE );
   if( read( fd, buffer, MAX_HTTP_SIZE ) <= 0 ) {    /* read req from client */
      perror( "Error while reading request" );
      abort();
   } 

   /* standard requests are of the form
    *   GET /foo/bar/qux.html HTTP/1.1
    * We want the second token (the file path).
    */
   tmp = strtok_r( buffer, " ", &brk );              /* parse request */
   if( tmp && !strcmp( "GET", tmp ) ) {
      req = strtok_r( NULL, " ", &brk );
   }


   if( !req ) {                                      /* is req valid? */
      len = sprintf( buffer, "HTTP/1.1 400 Bad request\n\n" );
      write( fd, buffer, len );                       /* if not, send err */
   } else {                                          /* if so, open file */
      req++;                                          /* skip leading / */
      strcpy(fnametemp, req);
      fin = fopen( req, "r" );                        /* open file */
      if( !fin ) {                                    /* check if successful */
         len = sprintf( buffer, "HTTP/1.1 404 File not found\n\n" );  
         write( fd, buffer, len );                     /* if not, send err */
      } 
      else {                                        /* if so, send file */
         len = sprintf( buffer, "HTTP/1.1 200 OK\n\n" );/* send success code */

         // Read file size
         write( fd, buffer, len );
         struct stat finfo;
         int file = 0;
         file = fileno(fin);
         if (fstat(file, &finfo) == 0) {

            newBlock.sequenceNumber = seqCounter;
            newBlock.fileDescriptor = fd;
            newBlock.fileName = fin;
            newBlock.bytesRemaining = finfo.st_size;

            strcpy(newBlock.fname, fnametemp);

            if (sjf && !rr && !mlfb){
               newBlock.quantum = finfo.st_size;
            }
            else if (rr && !sjf && !mlfb){
               // do round robin quantum
            }
            else if (mlfb && !sjf && !rr) {
               // do mlfb quantum
            }

         }
         else {
            printf("Error accessing file :(\n");
            abort();
         }

         fclose( fin );
      }
   }

   close( fd );                                     /* close client connectuin*/

   return newBlock;
}

int printrcb(RequestControlBlock b[]){
   printf("Number of control blocks:\t%d\n", (seqCounter-1));

   int i;
   for (i = 0; i < seqCounter-1; i++){
      printf("File Name:\t%s\n", b[i].fname);
      printf("SequenceNumber\t%d\n", b[i].sequenceNumber);
      printf("fileDescriptor\t%d\n", b[i].fileDescriptor);
      printf("bytes remaining\t%d\n", b[i].bytesRemaining);
      printf("quantum\t\t%d\n", b[i].quantum);
      printf("\n");
   }
   printf("=====================================================\n");
   return 0;

}


RequestControlBlock *scheduler(RequestControlBlock * rct){
   int i, j, pos;
   int n = seqCounter;
   RequestControlBlock temp;

   if (sjf && !rr && !mlfb){
      for (i = 0; i < n; i++){
         pos = i;
         for (j = i+1; j<n; j++){
            if (rct[j].bytesRemaining<rct[pos].bytesRemaining){
               pos = j;
            }
         }
         temp = rct[i];
         rct[i]=rct[pos];
         rct[pos] = temp;

      }
   }
   else if (rr && !sjf && !mlfb){
      // do round robin quantum
   }
   else if (mlfb && !sjf && !rr) {
      // do mlfb quantum
   }
   else {
      printf("Some weird error occured, no scheduler selected\n");
      abort();
   }

   return rct;
}

void scheduler2(RequestControlBlock * rct){
   int i, j, pos;
   int n = seqCounter;
   RequestControlBlock temp;

   if (sjf && !rr && !mlfb){
      for (i = 0; i < n; i++){
         pos = i;
         for (j = i+1; j<n; j++){
            if (rct[j].bytesRemaining<rct[pos].bytesRemaining){
               pos = j;
            }
         }
         temp = rct[i];
         rct[i]=rct[pos];
         rct[pos] = temp;

      }
   }
   else if (rr && !sjf && !mlfb){
      // do round robin quantum
   }
   else if (mlfb && !sjf && !rr) {
      // do mlfb quantum
   }
   else {
      printf("Some weird error occured, no scheduler selected\n");
      abort();
   }

}


/* This function is where the program starts running.
 *    The function first parses its command line parameters to determine port #
 *    Then, it initializes, the network and enters the main loop.
 *    The main loop waits for a client (1 or more to connect, and then processes
 *    all clients by calling the seve_client() function for each one.
 * Parameters: 
 *             argc : number of command line parameters (including program name
 *             argv : array of pointers to command line parameters
 * Returns: an integer status code, 0 for success, something else for error.
 */
int main( int argc, char **argv ) {

   int port = -1;                                    // server port # 
   int fd;                                           // client file descriptor 
   char * schedulerType = malloc(sizeof(argv[2]) * sizeof(char));
   if (argc >= 3) {
      strcpy(schedulerType, argv[2]);                   //scheduler type as argv
   }

   // check for and process parameters 

   if( ( argc < 3 ) || ( sscanf( argv[1], "%d", &port ) < 1 ) ) {
      printf( "usage: sws <port> <scheduler>\n" );
      return 0;
   }
   else if (argc > 3)
   {
      //This code exists for verbose error checking at runtime
      printf("WARNING: Extra arguments encountered:\n");
      int i; 
      for (i = 3; i < argc; i++)
      {
         printf("argv[%d]: %s\n", i, argv[i]);  
      }
   }

   network_init( port );                             // init network module 

   if (strcmp(schedulerType, "RR")== 0 ){
      // do round robin code
      printf("Round Robin scheduler selected\n");
      rr = true;
   }
   else if (strcmp(schedulerType, "SJF") == 0)
   {
      // do shortest job first code
      printf("Shortest Job First scheduler selected\n");
      sjf = true;
   }
   else if (strcmp(schedulerType, "MLFB") == 0)
   {
      // do mulitlevel feedback queue
      printf("Multilevel Feedback Queue scheduler selected\n");
      mlfb = true;
   }
   else 
   {
      printf("Error: unknown scheduler selected.\n");
      printf("Please select one of 'RR', 'SJF', or 'MLFB'\n"); 
      abort();
   }

   RequestControlBlock table[64];
   //RequestControlBlock *table = malloc (64 * sizeof(RequestControlBlock));

   network_wait();

   for( ;; ) {                                       // main loop 
      //network_wait();                                 // wait for clients 
      for( fd = network_open(); fd >= 0; fd = network_open() ) // get control blocks 
      {
         // create new block
         RequestControlBlock b;
         b = process_client(fd, table);

         // check if block exists, add to table if false
         if (!blockExists(b, table)){
            table[seqCounter-1] = b;
            seqCounter++;
         }
         // print block table
         //table = scheduler(table);




         int k, j;
         int n = seqCounter-1;
         RequestControlBlock temp;

         if (sjf && !rr && !mlfb && seqCounter > 1){
            for (k = 0; k < n; k++){
               for (j = k; j<n; j++){
                  if (table[j].bytesRemaining < table[k].bytesRemaining ){
                     temp = table[k];
                     table[k]=table[j];
                     table[j] = temp;
                  }
               }
            }
         }
         else if (rr && !sjf && !mlfb){
            // do round robin quantum
         }
         else if (mlfb && !sjf && !rr) {
            // do mlfb quantum
         }
         else {
            printf("Some weird error occured, no scheduler selected\n");
            abort();
         }











         printrcb(table);

      }

      for( fd = network_open(); fd >= 0; fd = network_open() ) // get clients 
      {
         serve_client( fd );            // process each client 
      }
      network_wait();
   }

}

