#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

extern int errno;

static const char * const SOCKET_PATH = "server.sock";

static int create_server() {
  int sock = -1;
  struct sockaddr_un addr;

  sock = socket(PF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    fprintf(stderr, "socket failed\n");
    goto EXIT;
  }

  unlink(SOCKET_PATH);
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, SOCKET_PATH);

  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    fprintf(stderr, "bind failed\n");
    goto ERR_BIND;
  }

  if (listen(sock, 5) < 0) {
    fprintf(stderr, "listen failed\n");
    goto ERR_LISTEN;
  }

  goto EXIT;

ERR_LISTEN:
ERR_BIND:
  close(sock);
  sock = -1;
EXIT:
  return sock;
}

static send_file_descriptor(int sock, int fd) {
  struct msghdr message;
  struct iovec iov;
  struct cmsghdr *control_message = NULL;
  char ctrl_buf[CMSG_SPACE(sizeof(fd))];
  char data[1];

  memset(&message, 0, sizeof(message));
  memset(&iov, 0, sizeof(iov));
  memset(ctrl_buf, 0, sizeof(ctrl_buf));

  data[0] = '\0';
  iov.iov_base = data;
  iov.iov_len = sizeof(data);

  message.msg_name = NULL;
  message.msg_namelen = 0;
  message.msg_iov = &iov;
  message.msg_iovlen = 1;
  message.msg_controllen = sizeof(ctrl_buf);
  message.msg_control = ctrl_buf;

  control_message = CMSG_FIRSTHDR(&message);
  control_message->cmsg_level = SOL_SOCKET;
  control_message->cmsg_type = SCM_RIGHTS;
  control_message->cmsg_len = CMSG_LEN(sizeof(fd));

  *((int*)CMSG_DATA(control_message)) = fd;

  return sendmsg(sock, &message, 0);
}

int main() {
  int fd = -1, server = -1, client = -1;
  FILE *fp = NULL;

  fp = fopen("a.txt", "w");
  if (fp == NULL) {
    fprintf(stderr, "fopen(a.txt) failed: %s\n", strerror(errno));
    goto ERR_FOPEN;
  }
  fd = fileno(fp);
  printf("a.txt:%d\n", fd);

  server = create_server();
  if (server < 0) {
    fprintf(stderr, "create_server failed\n");
    goto ERR_CREATE_SERVER;
  }
  while(1) {
    client = accept(server, NULL, NULL);
    fprintf(stderr, "client accept %d\n", client);
    printf("a.txt:%d\n", fd);
    if (client < 0) {
      fprintf(stderr, "accept failed\n");
      goto ERR_ACCEPT;
    }

    if (send_file_descriptor(client, fd) < 0) {
      fprintf(stderr, "send_file_descriptor failed: %s\n", strerror(errno));
      goto ERR_SEND_FILE_DESCRIPTOR;
    }

    if (shutdown(client, SHUT_RDWR) < 0) {
      fprintf(stderr, "shutdown failed: %s\n", strerror(errno));
      goto ERR_SHUTDOWN;
    }
  }

ERR_SHUTDOWN:
ERR_SEND_FILE_DESCRIPTOR:
  close(client);
ERR_ACCEPT:
  close(server);
ERR_CREATE_SERVER:
  fclose(fp);
ERR_FOPEN:
  return 0;
}
