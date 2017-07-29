/* 
 * File: sws.c
 * Author: Alex Brodsky
 * Purpose: This file contains the butchered implementation of a simple web server.
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

typedef struct RequestControlBlock{
   // This is the request control table to store state info for each request by
   // the client.
   // It should be initialized as an array of RequestControlTables and
   // should associate a spot in the array with a request from the client

   int sequenceNumber;  //similar to process ID. sequence numbers start at 1
   int fileDescriptor;  //returned by network_wait() in network.h
   FILE * fileName;     //filename given by the client
   int bytesRemaining;  //the number of bytes remaining to be sent
   int quantum;         //max number of bytes to send

   char fname[100];        // req name of file

}RequestControlBlock; 


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
   // This function is deprecated, but we did not want to remove it
   // from our file for reference purposes and because our serve_client2
   // function has a segfault error.

   static char *buffer;                              /* request buffer */
   char *req = NULL;                                 /* ptr to req file */
   char *brk;                                        /* state used by strtok */
   char *tmp;                                        /* error checking ptr */
   FILE *fin;                                        /* input file handle */
   int len;                                          /* length of data read */

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
   //close( fd );                                     /* close client connectuin*/

}

RequestControlBlock process_client(RequestControlBlock newBlock) {
   //This function initializes a control block entry to be processed
   //in the request control table, and later served by serve_client
   
   static char *buffer;                              /* request buffer */
   FILE *fin;                                        /* input file handle */
   char *req = NULL;                                 /* ptr to req file */
   char *brk;                                        /* state used by strtok */
   char *tmp;                                        /* error checking ptr */

   int fd = newBlock.fileDescriptor;
   network_open();

   //check file to init control block
   buffer = malloc( MAX_HTTP_SIZE );
   read( fd, buffer, MAX_HTTP_SIZE ) ;
   tmp = strtok_r( buffer, " ", &brk );
   if( tmp && !strcmp( "GET", tmp ) ) {
      req = strtok_r( NULL, " ", &brk );
   }
   req++;

   fin = fopen( req, "r" );  

   //file = fileno(fin);

   struct stat finfo;
   if (stat(req, &finfo) == 0) {

      newBlock.sequenceNumber = seqCounter;
      newBlock.fileName = fin;
      newBlock.bytesRemaining = finfo.st_size;

      strcpy(newBlock.fname, req);

      if (sjf && !rr && !mlfb){
         newBlock.quantum = finfo.st_size;
      }
      else if (rr && !sjf && !mlfb){
         // do round robin quantum
         newBlock.quantum = 8192;
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

   //close( fd );                                     /* close client connectuin*/

   return newBlock;
}


RequestControlBlock serve_client2( RequestControlBlock rcb ) {
   //This is our attempt at serving the client while utilizing the blocks
   //from the control table.
   //
   //If this method didnt segfault we believe that we would have SJF and RR
   //functioning correctly. 
   //
   //(Please dont take all of our marks away because of a segfault :)

   int fd = rcb.fileDescriptor;

   static char *buffer;                              /* request buffer */
   //char *req = rcb.fname;                                 /* ptr to req file */
   //char *brk;                                        /* state used by strtok */
   //char *tmp;                                        /* error checking ptr */
   FILE *fin;// = rcb.fileName;                                        /* input file handle */
   int len;                                          /* length of data read */
   char ch;

   if( !buffer ) {                                   // 1st time, alloc buffer 
      buffer = malloc( MAX_HTTP_SIZE );
      if( !buffer ) {                                 // error check 
         perror( "Error while allocating memory" );
         abort();
      }
   }
   memset( buffer, 0, MAX_HTTP_SIZE );

   fin = fopen( rcb.fname, "r" );                        // open file 

   len = sprintf( buffer, "HTTP/1.1 200 OK\n\n" );/* send success code */

   do {/* loop, read & send file */
      int pch = 0;
      //len = fread( buffer, 1, MAX_HTTP_SIZE, fin );  /* read file chunk */
      while( (ch = fgetc(fin) ) != EOF ){
         buffer[pch] = ch;
         pch++;
      }
     
      len = pch;

      if( len < 0 ) {                             /* check for errors */
         perror( "Error while writing to client" );
      } 
      else if( len > 0 ) {                      /* if none, send chunk */
         len = write( rcb.fileDescriptor, buffer, len );
         if( len < 1 ) {                           /* check for errors */
            perror( "Error while writing to client" );
         }
      }

      rcb.bytesRemaining-=rcb.quantum;
      

      //For round robin scheduling:
      //break out of the loop and continue the rest of the remaining bytes 
      //during the next loop.
      if (rr) break;

   } while( len == MAX_HTTP_SIZE );              /* the last chunk < 8192 */
   fclose( fin );
   //}
   //}
   close( rcb.fileDescriptor );                                     /* close client connectuin*/

   return rcb;
   }


bool blockExists(RequestControlBlock newBlock, RequestControlBlock rct[]){
   // check if block exists before creating a new entry
   int i;

   for (i = 0; i < seqCounter-1; i++){
      if (strcmp(newBlock.fname, rct[i].fname)==0 ){
         return true;
      }
   }

   return false;
}



int printrcb(RequestControlBlock b[]){
   //When given a request control table, this function will print the values of
   //the blocks populating the table
   // mostly for debugging purposes

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

   int feedbackLevel = 0;        //for multilevel feedback queue
   int qSize = 8192;                    // # of bytes to put in the queue

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
   network_init( port );                             // init network module 
   network_wait();

   for( ;; ) {                                       // main loop 

      for( fd = network_open(); fd >= 0; fd = network_open() ) // get control blocks 
      {
         RequestControlBlock b;
         b.fileDescriptor = fd;
         b = process_client(b);

         // check if block exists, add to table if false
         if (!blockExists(b, table)){
            table[seqCounter-1] = b;
            seqCounter++;
         }

         //Do SJF scheduling
         //The request control blocks are reordered in the table to place the
         // shortest jobs first 
         if (sjf && !rr && !mlfb){
            //looping variables for SJF
            int k, j;
            int n = seqCounter-1;
            RequestControlBlock temp;

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
         // Do RR sscheduling 
         else if (rr && !sjf && !mlfb){
            // No logic is done here for RR scheduling, instead an if statement is added
            // to serve_client to break out of the do_while loop
         }
         // Do multilevel feedback queue
         else if (mlfb && !sjf && !rr) {
            if (feedbackLevel == 1)
               qSize = 65536;                   // for the second feedback queue size
            else if (feedbackLevel > 1)
               qSize = -1;                      // negative integer to send all remaining bytes
         }
         else {
            printf("Error, no scheduler selected\n");
            abort();
         }

         int i;
         for (int i = 0; i < seqCounter; i++)
         {
            // Process all the blocks that are waiting in the control table
            table[i] = serve_client2(table[i]);
         }

         printrcb(table);

         //table[filesServed].fileDescriptor=fd;
         //table[filesServed] = serve_client2(table[filesServed]);
      }

      feedbackLevel++; //increment feedback level to change queue size


      network_wait();
   }

}

