#include "pandora.h"

struct Pandora pandora = {
    "0.0.2",   // version (char*)
    0,         // status (int) 0 disconnected, 1 connected, 2 listening
    "0.0.0.0", // host (char*)
    0,         // port (int)
    -1,        // socket (int)
    0,         // listenersc (int)
    0,         // clientsc (int)
    NULL,      // server (struct hostent*)

    _info,    // void _info
    _check,   // int _check
    _listen,  // int _listen
    _connect, // int _connect
    _on,      // void _on
    _emit,    // void _emit
    _digest,  // void _digest
    _close,   // int _close
};

void *_hostRuntime()
{
  pandora.clients = malloc(1 * sizeof(int));
  int maxsd;
  // int activity;
  int activesocket;
  socklen_t socketlen;
  int valread;
  char buffer[256];

  printf("Waiting for connections...\n");
  while (pandora.status == PND_STATUS_LISTENING)
  {
    FD_ZERO(&pandora.fdclients);
    FD_SET(pandora.socket, &pandora.fdclients);
    maxsd = pandora.socket;

    int c = pandora.clientsc;
    while (c-- > 0)
      if (pandora.clients[c] > 0)
      {
        FD_SET(pandora.clients[c], &pandora.fdclients);
        if (pandora.clients[c] > maxsd)
          maxsd = pandora.clients[c];
      }

    int activity = select(maxsd + 1, &pandora.fdclients, NULL, NULL, NULL);

    // Main socket
    if (FD_ISSET(pandora.socket, &pandora.fdclients))
    {
      socketlen = sizeof(pandora.serv_addr);
      if ((activesocket = accept(pandora.socket, (struct sockaddr *)&pandora.serv_addr, (socklen_t *)&socketlen)) < 0)
        printf("ERROR cant accept connection\n");

      printf("New client connected - fd: %d, ip: %s, port: %d\n", activesocket,
             inet_ntoa(pandora.serv_addr.sin_addr),
             ntohs(pandora.serv_addr.sin_port));

      // emit "connection" event
      if (send(activesocket, "connection", strlen("connection"), 0) < 0)
        printf("ERROR handshake failed\n");

      // push client
      pandora.clientsc++;
      pandora.clients = realloc(pandora.clients, pandora.clientsc * sizeof(int));
      pandora.clients[pandora.clientsc - 1] = activesocket;
    }

    // clients sockets
    c = pandora.clientsc;
    int needs_compact = 0;
    while (c-- > 0)
    {
      activesocket = pandora.clients[c];
      if (FD_ISSET(activesocket, &pandora.fdclients))
      {
        memset(buffer, 0, sizeof(buffer));
        valread = read(activesocket, buffer, sizeof(buffer) - 1);

        // Check if disconnection
        if (valread == 0)
        {
          socketlen = sizeof(pandora.serv_addr);
          getpeername(activesocket, (struct sockaddr *)&pandora.serv_addr, (socklen_t *)&socketlen);
          printf("Host disconnected , ip %s , port %d \n",
                 inet_ntoa(pandora.serv_addr.sin_addr),
                 ntohs(pandora.serv_addr.sin_port));

          close(activesocket);
          pandora.clients[c] = 0; // Mark for removal
          needs_compact = 1;
        }
        else
        { // if not, its a input
          printf("Message received: %s\n", buffer);
          PND_MESSAGE msg = {buffer};
          int cev = pandora.listenersc;
          while (cev--)
          {
            if (strcmp(buffer, pandora.listeners[cev].ev) == 0)
            {
              pandora.listeners[cev].callback(&msg);
            }
          }
        }
      }
    }

    if (needs_compact)
    {
      int new_c = 0;
      for (int i = 0; i < pandora.clientsc; i++)
      {
        if (pandora.clients[i] != 0)
        {
          pandora.clients[new_c++] = pandora.clients[i];
        }
      }
      pandora.clientsc = new_c;
      pandora.clients = realloc(pandora.clients, pandora.clientsc * sizeof(int));
    }
  }
  return NULL;
}

int _listen(int port)
{
  if (pandora.status > PND_STATUS_DISCONNECTED)
  {
    printf("ERROR pandora is currently %s\n", pandora.status == PND_STATUS_CONNECTED ? "conceted" : "listening");
    return PND_ERROR_INVOCATION;
  }
  if (!port)
  {
    printf("Error connecting\n");
    return PND_ERROR_INVOCATION;
  }
  printf("Opening port %d to listen...", port);

  pandora.port = port;
  pandora.socket = socket(AF_INET, SOCK_STREAM, 0);
  if (pandora.socket < 0)
  {
    printf("ERROR\ncant get sockfd\n");
    return PND_ERROR_CONNECTION;
  }
  printf("OPEN\n");

  int opt = 1;
  if (setsockopt(pandora.socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
  {
    printf("ERROR\ncant configure socket\n");
    return PND_ERROR_SOCKET;
  }

  bzero((char *)&pandora.serv_addr, sizeof(pandora.serv_addr));
  pandora.serv_addr.sin_family = AF_INET;
  pandora.serv_addr.sin_addr.s_addr = INADDR_ANY;
  pandora.serv_addr.sin_port = htons(pandora.port);

  if (bind(pandora.socket, (struct sockaddr *)&pandora.serv_addr, sizeof(pandora.serv_addr)) < 0 || listen(pandora.socket, 5) < 0)
  {
    printf("ERROR\ncant bind or listen socket\n");
    return 5;
  }

  pandora.status = PND_STATUS_LISTENING;
  int running = pthread_create(&pandora.hostRuntime, NULL, _hostRuntime, NULL);
  if (running == 0)
    printf("Listener thread is up\n");
  else
  {
    printf("ERROR setting up listener thread, server is not running\n");
    pandora.status = PND_STATUS_DISCONNECTED;
  }

  return pandora.socket;
}

int _connect(char *host, int port)
{
  if (pandora.status > 0)
  {
    printf("ERROR pandora is currently %s", pandora.status == 1 ? "connected" : "listening\n");
    return PND_ERROR_INVOCATION;
  }
  if (!strlen(host) || !port)
  {
    printf("Error connecting\n");
    return PND_ERROR_INVOCATION;
  }
  printf("creating connection with %s:%d...\n", host, port);
  pandora.host = host;
  pandora.port = port;
  pandora.socket = socket(AF_INET, SOCK_STREAM, 0);
  pandora.server = gethostbyname(pandora.host);
  if (pandora.socket < 0 || pandora.server < 0)
  {
    printf("ERROR\ncant get sockfd or can't reach host\n");
    return PND_ERROR_CONNECTION;
  }

  bzero((char *)&pandora.serv_addr, sizeof(pandora.serv_addr));
  pandora.serv_addr.sin_family = AF_INET;
  bcopy((char *)pandora.server->h_addr_list[0],
        (char *)&pandora.serv_addr.sin_addr,
        pandora.server->h_length); // h_addr_list[0] is the first address
  pandora.serv_addr.sin_port = htons(pandora.port);

  if (connect(pandora.socket, (struct sockaddr *)&pandora.serv_addr, sizeof(pandora.serv_addr)) < 0)
  {
    printf("ERROR cant connect\n");
    pandora.close(PND_ERROR_CONNECTION);
  }

  pandora.status = 1;
  printf("SUCCESS\n");
  return pandora.socket;
}
void _info(void)
{
  printf("CONFIGURATION \t\n \
          host: %s\n \
          port: %d\n",
         pandora.host,
         pandora.port);
  printf("STATUS\n \
          %s\n",
         pandora.status == PND_STATUS_LISTENING ? "listening" : (pandora.status ? "connected" : "disconnected"));
}
int _check(void)
{
  return pandora.status > PND_STATUS_DISCONNECTED;
}

void _on(char *ev, PND_HANDLER_FUNCTION *callback)
{
  pandora.listenersc++;
  pandora.listeners = realloc(pandora.listeners, sizeof(PND_LISTENER) * pandora.listenersc);
  // TODO: check realloc failure
  PND_LISTENER listener = {ev, callback};
  pandora.listeners[pandora.listenersc - 1] = listener;
}

int osocket;
void _send(int client, char *msg)
{
  osocket = write(client, msg, strlen(msg));
  if (osocket < 0)
  {
    printf("ERROR writing in socket\n");
    /* pandora.close(PND_SOCKET_ERROR); */
  }
  else
    printf("i say %s to %d\n", msg, client);
}
void _broadcast(char *msg)
{
  int c = pandora.clientsc;
  while (c--)
  {
    _send(pandora.clients[c], msg);
  }
}
void _emit(char *msg)
{
  if (pandora.status == PND_STATUS_LISTENING)
    _broadcast(msg);
  else if (pandora.status == PND_STATUS_CONNECTED)
    _send(pandora.socket, msg);
}

void _digest()
{
  if (pandora.status != PND_STATUS_CONNECTED)
    return;

  char buffer[256];
  memset(buffer, 0, sizeof(buffer));

  int n = read(pandora.socket, buffer, sizeof(buffer) - 1);
  if (n < 0)
  {
    perror("ERROR reading from socket");
    pandora.close(PND_ERROR_SOCKET);
    exit(PND_ERROR_SOCKET);
  }
  if (n == 0)
  {
    printf("Server disconnected.\n");
    pandora.close(PND_OK);
    exit(PND_OK);
  }

  PND_MESSAGE msg = {buffer};

  int c = pandora.listenersc;
  while (c--)
  {
    if (strcmp(buffer, pandora.listeners[c].ev) == 0)
    {
      pandora.listeners[c].callback(&msg);
    }
  }
}

void _close(int err)
{
  if (pandora.clients)
  {
    for (int i = 0; i < pandora.clientsc; i++)
      close(pandora.clients[i]);
    free(pandora.clients);
  }
  if (pandora.socket > -1)
    close(pandora.socket);
  if (pandora.listeners)
    free(pandora.listeners);
  pandora.status = PND_STATUS_DISCONNECTED;
}
