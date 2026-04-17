/* controlctl.c - v1.0
   Copyright (C) 2026 <NopAngel> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>

#define SOCKET_PATH "/tmp/controld.sock"
#define BUFFER_SIZE 2048

int
main (int argc, char *argv[])
{
  int sock;
  struct sockaddr_un server_addr;
  char buffer[BUFFER_SIZE];

  if (argc < 2)
    {
      fprintf (stderr, "Usage: %s <status|list|start <name>|stop <name>>\n", argv[0]);
      return EXIT_FAILURE;
    }

  /* 1. Create the Unix Domain Socket. */
  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      return EXIT_FAILURE;
    }

  memset (&server_addr, 0, sizeof (struct sockaddr_un));
  server_addr.sun_family = AF_UNIX;
  strncpy (server_addr.sun_path, SOCKET_PATH, sizeof (server_addr.sun_path) - 1);

  /* 2. Connect to the running daemon. */
  if (connect (sock, (struct sockaddr *) &server_addr, sizeof (struct sockaddr_un)) < 0)
    {
      perror ("Connection failed. Is controld running?");
      close (sock);
      return EXIT_FAILURE;
    }

  /* 3. Prepare and send the command. 
     If the user sends multiple arguments (like 'stop impresora'), 
     we join them into a single string. */
  memset (buffer, 0, BUFFER_SIZE);
  for (int i = 1; i < argc; i++)
    {
      strcat (buffer, argv[i]);
      if (i < argc - 1) strcat (buffer, " ");
    }

  if (send (sock, buffer, strlen (buffer), 0) < 0)
    {
      perror ("send");
      close (sock);
      return EXIT_FAILURE;
    }

  /* 4. Receive and print the response (Crucial for the 'list' command). */
  ssize_t n;
  bool received_anything = false;
  
  while ((n = read (sock, buffer, sizeof (buffer) - 1)) > 0)
    {
      buffer[n] = '\0';
      printf ("%s", buffer);
      received_anything = true;
    }

  if (!received_anything && strcmp(argv[1], "list") != 0)
    {
      printf ("[controlctl] Command '%s' sent successfully.\n", argv[1]);
    }

  close (sock);
  return EXIT_SUCCESS;
}