#include <pandora.h>

void world_handler(PND_MESSAGE *msg)
{
  printf("client said: %s\n", msg->msg);
  pandora.emit("stop");
}
/*  */
/* void* list(void* arg) { */
/*   pandora.listen(1337); */
/* } */

int main()
{
  printf("Starting Pandora Server...\n");
  printf("Using Pandora version: %s\n\n", pandora.version);

  pandora.on("world!", world_handler);

  if (pandora.listen(1337) < 0)
  {
    fprintf(stderr, "Failed to start listener.\n");
    return 1;
  }

  // Wait for the listener thread to finish, keeping the server alive.
  pthread_join(pandora.hostRuntime, NULL);

  pandora.close(0);
  return 0;
}
