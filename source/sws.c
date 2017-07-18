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

#include "network.h"

#define MAX_HTTP_SIZE 8192                 /* size of buffer to allocate */
#define MAX_INPUT_LINE 255                //max line length to accept as input

struct RequestControlTable {
   // This is the request control table to store state info for each request by
   // the client.
   // It should be initialized as an array of RequestControlTables and
   // should associate a spot in the array with a request from the client
   // - Matt
   
   int sequenceNumber;  //similar to process ID. sequence numbers start at 1
   int fileDescriptor;  //returned by network_wait() in network.c
   FILE * fileName;     //filename given by the client
   int bytesRemaining;  //the number of bytes remaining to be sent
   int quantum;         //max number of bytes to send
}



int scheduler(char * schedulerType){
   // get console command args
   
   if (strcmp(schedulerType, "RR")== 0 ){
     // do round robin code
     printf("Round Robin scheduler selected\n");
   }
   else if (strcmp(schedulerType, "SJF") == 0)
   {
      // do shortest job first code
     printf("Shortest Job First scheduler selected\n");
   }
   else if (strcmp(schedulerType, "MLFB") == 0)
   {
      // do mulitlevel feedback queue
     printf("Multilevel Feedback Queue scheduler selected\n");
   }
   else 
   {
     printf("Error: unknown scheduler selected.\n");
     printf("Please select one of 'RR', 'SJF', or 'MLFB'\n"); 
   }

   return 0;
}

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
      write( fd, buffer, len );

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
  int i; //used in the loop later, do not delete

  // check for and process parameters 

  if( ( argc < 3 ) || ( sscanf( argv[1], "%d", &port ) < 1 ) ) {
    printf( "usage: sws <port> <scheduler>\n" );
    return 0;
  }
  /*
  network_init( port );                             // init network module 

  for( ;; ) {                                       // main loop 
    network_wait();                                 // wait for clients 

    for( fd = network_open(); fd >= 0; fd = network_open() ) { // get clients 
      serve_client( fd );                           // process each client 
    }
  }
  */

  if (argc > 3)
  {                                                 // append any extra args to string
    printf("WARNING: Extra arguments encountered:\n");
    for (i = 3; i < argc; i++)
    {
      printf("argv[%d]: %s\n", i, argv[i]);  
    }
  }

  scheduler(schedulerType);

}

