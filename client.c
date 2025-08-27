#include <pandora.h>

void handle(PND_MESSAGE *msg)
{
  pandora.emit("world!");
}

int loop = 1;
void stopDigesting(PND_MESSAGE *msg)
{
  printf("server says stop\n");
  loop = 0;
}

int main(int argc, char *argv[])
{
  printf("using %s\n\n", pandora.version);

  // 1. Connect to the server.
  if (pandora.connect("localhost", 1337) < 0)
  {
    fprintf(stderr, "Failed to connect to the server.\n");
    return 1;
  }

  // 2. Register event handlers for messages from the server.
  pandora.on("connection", handle);
  pandora.on("stop", stopDigesting);

  // 3. Enter the main loop to listen for messages.
  while (loop)
    pandora.digest();

  pandora.close(PND_OK);
  return 0;
}
