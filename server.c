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

static send_file_descriptor(int sock, int fd[2]) {

  struct msghdr message;
  struct iovec iov;
  struct cmsghdr *control_message = NULL;

  char ctrl_buf[CMSG_SPACE(sizeof(fd[0]) * 2)];
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
  control_message->cmsg_len = CMSG_LEN(sizeof(fd[0]) * 2); //!

  int* dataBuff = ((int*)CMSG_DATA(control_message));
  memcpy(dataBuff, fd, 2 * sizeof(int));
  return sendmsg(sock, &message, 0);
}

int main() {
  int fd[2] = {-1, -1};
  int server = -1, client = -1;
  FILE *fp = NULL;
  FILE *fp2 = NULL;
  fp = fopen("a.txt", "w");
  if (fp == NULL) {
    fprintf(stderr, "fopen(a.txt) failed: %s\n", strerror(errno));
    goto ERR_FOPEN;
  }
  fd[0] = fileno(fp);
  printf("a.txt:%d\n", fd[0]);

  fp2 = fopen("aa.txt", "w");
  if (fp2 == NULL) {
    fprintf(stderr, "fopen(aa.txt) failed: %s\n", strerror(errno));
    goto ERR_FOPEN;
  }
  fd[1] = fileno(fp2);
  printf("aa.txt:%d\n", fd[1]);
  server = create_server();
  if (server < 0) {
    fprintf(stderr, "create_server failed\n");
    goto ERR_CREATE_SERVER;
  }
  while(1) {
    client = accept(server, NULL, NULL);
    fprintf(stderr, "client accept %d\n", client);
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
  fclose(fp2);
ERR_FOPEN:
  return 0;
}
