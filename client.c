#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

extern int errno;

static const char * const SOCKET_PATH = "server.sock";
static const char * const HELLO_WORLD = "Hello, world!\n";

static recv_file_descriptor(int sock) {
  int fd;
  struct msghdr message;
  struct iovec iov;
  struct cmsghdr *control_message = NULL;
  char ctrl_buf[CMSG_SPACE(sizeof(fd))];
  char data[1];
  int res = -1;

  memset(&message, 0, sizeof(message));
  memset(&iov, 0, sizeof(iov));
  memset(ctrl_buf, 0, sizeof(ctrl_buf));

  data[0] = '\0';
  iov.iov_base = data;
  iov.iov_len = sizeof(data);

  message.msg_name = NULL;
  message.msg_namelen = 0;
  message.msg_control = ctrl_buf;
  message.msg_controllen = sizeof(ctrl_buf);
  message.msg_iov = &iov;
  message.msg_iovlen = 1;

  res = recvmsg(sock, &message, 0);
  if (res < 0) {
    fprintf(stderr, "recvmsg failed: %s\n", strerror(errno));
    goto ERR_RECVMSG;
  }

  for (control_message = CMSG_FIRSTHDR(&message);
      control_message != NULL;
      control_message = CMSG_NXTHDR(&message, control_message)) {
    if ((control_message->cmsg_level == SOL_SOCKET)
        && (control_message->cmsg_type == SCM_RIGHTS)) {
      fd = *((int *)CMSG_DATA(control_message));
      break;
    }
  }

ERR_RECVMSG:
EXIT:
  return fd;
}

int main() {
  int fd = -1, sock = -1;
  FILE *fp = NULL;
  struct sockaddr_un addr;

  fp = fopen("b.txt", "w");
  if (fp == NULL) {
    fprintf(stderr, "fopen(b.txt) failed: %s\n", strerror(errno));
    goto ERR_FOPEN;
  }
  fd = fileno(fp);
  printf("b.txt:%d\n", fd);

  sock = socket(PF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    fprintf(stderr, "socket failed: %s\n", strerror(errno));
    goto ERR_SOCKET;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, SOCKET_PATH);

  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    fprintf(stderr, "connect failed: %s\n", strerror(errno));
    goto ERR_CONNECT;
  }

  fd = recv_file_descriptor(sock);
  if (fd < 0) {
    fprintf(stderr, "recv_file_descriptro failed\n");
    goto ERR_RECV_FILE_DESCRIPTOR;
  }

  printf("fd:%d\n", fd);
  if (write(fd, HELLO_WORLD, strlen(HELLO_WORLD)) < 0) {
    fprintf(stderr, "write failed: %s\n", strerror(errno));
    goto ERR_WRITE;
  }

ERR_WRITE:
  close(fd);
ERR_RECV_FILE_DESCRIPTOR:
  if (shutdown(sock, SHUT_RDWR) < 0) {
    fprintf(stderr, "shutdown failed: %s\n", strerror(errno));
  }
ERR_CONNECT:
  close(sock);
ERR_SOCKET:
  fclose(fp);
ERR_FOPEN:
  return 0;
}
