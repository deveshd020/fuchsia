// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <errno.h>
#include <fcntl.h>
#include <lib/syslog/global.h>
#include <lib/zx/socket.h>
#include <poll.h>
#include <unistd.h>

#include <unittest/unittest.h>

__BEGIN_CDECLS

// This does not come from header file as this function should only be used in
// tests and is not for general use.
void fx_log_reset_global_for_testing(void);

__END_CDECLS

static bool ends_with(const char* str, const char* suffix) {
  if (strlen(str) < strlen(suffix)) {
    return false;
  }
  size_t l = strlen(suffix);
  str += strlen(str) - l;
  return strcmp(str, suffix) == 0;
}

bool test_log_init(void) {
  BEGIN_TEST;
  fx_log_reset_global_for_testing();
#ifdef SYSLOG_STATIC
  // Init without config does not work with syslog-static
  zx_status_t expected = ZX_ERR_INVALID_ARGS;
#else
  zx_status_t expected = ZX_OK;
#endif
  EXPECT_EQ(expected, fx_log_init(), "");
  END_TEST;
}

bool test_log_init_with_socket(void) {
  BEGIN_TEST;
  zx::socket socket0, socket1;
  EXPECT_EQ(ZX_OK, zx::socket::create(0, &socket0, &socket1));
  fx_logger_config_t config = {.min_severity = FX_LOG_INFO,
                               .console_fd = -1,
                               .log_service_channel = socket1.release(),
                               .tags = nullptr,
                               .num_tags = 0};
  fx_log_reset_global_for_testing();
  EXPECT_EQ(ZX_OK, fx_log_init_with_config(&config), "");
  END_TEST;
}

bool test_log_enabled_macro(void) {
  BEGIN_TEST;
  fx_log_reset_global_for_testing();
  zx::socket socket0, socket1;
  EXPECT_EQ(ZX_OK, zx::socket::create(0, &socket0, &socket1));
  fx_logger_config_t config = {.min_severity = FX_LOG_INFO,
                               .console_fd = -1,
                               .log_service_channel = socket1.release(),
                               .tags = nullptr,
                               .num_tags = 0};
  EXPECT_EQ(ZX_OK, fx_log_init_with_config(&config), "");
  if (FX_VLOG_IS_ENABLED(1)) {
    EXPECT_TRUE(false, "control should not reach this line");
  }
  if (!FX_LOG_IS_ENABLED(INFO)) {
    EXPECT_TRUE(false, "control should not reach this line");
  }
  if (!FX_LOG_IS_ENABLED(ERROR)) {
    EXPECT_TRUE(false, "control should not reach this line");
  }
  fx_log_reset_global_for_testing();
  END_TEST;
}

static inline zx_status_t init_helper(int fd, const char** tags, size_t ntags) {
  fx_logger_config_t config = {.min_severity = FX_LOG_INFO,
                               .console_fd = fd,
                               .log_service_channel = ZX_HANDLE_INVALID,
                               .tags = tags,
                               .num_tags = ntags};

  return fx_log_init_with_config(&config);
}

bool test_log_simple_write(void) {
  BEGIN_TEST;
  fx_log_reset_global_for_testing();
  int pipefd[2];
  EXPECT_NE(pipe2(pipefd, O_NONBLOCK), -1, "");
  EXPECT_EQ(ZX_OK, init_helper(pipefd[0], NULL, 0), "");
  FX_LOG(INFO, NULL, "test message");
  char buf[256];
  size_t n = read(pipefd[1], buf, sizeof(buf));
  EXPECT_GT(n, 0u, "");
  buf[n] = 0;
  EXPECT_TRUE(ends_with(buf, "test message\n"), buf);
  close(pipefd[1]);
  fx_log_reset_global_for_testing();
  END_TEST;
}

bool test_log_write(void) {
  BEGIN_TEST;
  fx_log_reset_global_for_testing();
  int pipefd[2];
  EXPECT_NE(pipe2(pipefd, O_NONBLOCK), -1, "");
  EXPECT_EQ(ZX_OK, init_helper(pipefd[0], NULL, 0), "");
  FX_LOGF(INFO, NULL, "%d, %s", 10, "just some number");
  char buf[256];
  size_t n = read(pipefd[1], buf, sizeof(buf));
  EXPECT_GT(n, 0u, "");
  buf[n] = 0;
  EXPECT_TRUE(ends_with(buf, "INFO: 10, just some number\n"), buf);
  close(pipefd[1]);
  fx_log_reset_global_for_testing();
  END_TEST;
}

bool test_log_preprocessed_message(void) {
  BEGIN_TEST;
  fx_log_reset_global_for_testing();
  int pipefd[2];
  EXPECT_NE(pipe2(pipefd, O_NONBLOCK), -1, "");
  EXPECT_EQ(ZX_OK, init_helper(pipefd[0], NULL, 0), "");
  FX_LOG(INFO, NULL, "%d, %s");
  char buf[256];
  size_t n = read(pipefd[1], buf, sizeof(buf));
  EXPECT_GT(n, 0u, "");
  buf[n] = 0;
  EXPECT_TRUE(ends_with(buf, "INFO: %d, %s\n"), buf);
  close(pipefd[1]);
  fx_log_reset_global_for_testing();
  END_TEST;
}

bool test_log_severity(void) {
  struct pollfd fd;
  BEGIN_TEST;
  fx_log_reset_global_for_testing();
  int pipefd[2];
  EXPECT_NE(pipe2(pipefd, O_NONBLOCK), -1, "");
  EXPECT_EQ(ZX_OK, init_helper(pipefd[0], NULL, 0), "");
  FX_LOG_SET_SEVERITY(WARNING);
  FX_LOGF(INFO, NULL, "%d, %s", 10, "just some number");
  fd.fd = pipefd[1];
  fd.events = POLLIN;
  EXPECT_EQ(poll(&fd, 1, 1), 0, "");
  close(pipefd[1]);
  fx_log_reset_global_for_testing();
  END_TEST;
}

bool test_log_write_with_tag(void) {
  BEGIN_TEST;
  int pipefd[2];
  fx_log_reset_global_for_testing();
  EXPECT_NE(pipe2(pipefd, O_NONBLOCK), -1, "");
  EXPECT_EQ(ZX_OK, init_helper(pipefd[0], NULL, 0), "");
  FX_LOGF(INFO, "tag", "%d, %s", 10, "just some string");
  char buf[256];
  size_t n = read(pipefd[1], buf, sizeof(buf));
  EXPECT_GT(n, 0u, "");
  buf[n] = 0;
  EXPECT_TRUE(ends_with(buf, "[tag] INFO: 10, just some string\n"), buf);
  close(pipefd[1]);
  fx_log_reset_global_for_testing();
  END_TEST;
}

bool test_log_write_with_global_tag(void) {
  BEGIN_TEST;
  int pipefd[2];
  fx_log_reset_global_for_testing();
  EXPECT_NE(pipe2(pipefd, O_NONBLOCK), -1, "");
  const char* tags[] = {"gtag"};
  EXPECT_EQ(ZX_OK, init_helper(pipefd[0], tags, 1), "");
  FX_LOGF(INFO, "tag", "%d, %s", 10, "just some string");
  char buf[256];
  size_t n = read(pipefd[1], buf, sizeof(buf));
  EXPECT_GT(n, 0u, "");
  buf[n] = 0;
  EXPECT_TRUE(ends_with(buf, "[gtag, tag] INFO: 10, just some string\n"), buf);
  close(pipefd[1]);
  fx_log_reset_global_for_testing();
  END_TEST;
}

bool test_log_write_with_multi_global_tag(void) {
  BEGIN_TEST;
  int pipefd[2];
  fx_log_reset_global_for_testing();
  EXPECT_NE(pipe2(pipefd, O_NONBLOCK), -1, "");
  const char* tags[] = {"gtag", "gtag2"};
  EXPECT_EQ(ZX_OK, init_helper(pipefd[0], tags, 2), "");
  FX_LOGF(INFO, "tag", "%d, %s", 10, "just some string");
  char buf[256];
  size_t n = read(pipefd[1], buf, sizeof(buf));
  EXPECT_GT(n, 0u, "");
  buf[n] = 0;
  EXPECT_TRUE(ends_with(buf, "[gtag, gtag2, tag] INFO: 10, just some string\n"), buf);
  close(pipefd[1]);
  fx_log_reset_global_for_testing();
  END_TEST;
}

bool test_global_tag_limit(void) {
  BEGIN_TEST;
  fx_log_reset_global_for_testing();
  EXPECT_NE(ZX_OK, init_helper(-1, NULL, FX_LOG_MAX_TAGS + 1), "");
  fx_log_reset_global_for_testing();
  END_TEST;
}

bool test_msg_length_limit(void) {
  BEGIN_TEST;
  fx_log_reset_global_for_testing();
  int pipefd[2];
  EXPECT_NE(pipe2(pipefd, O_NONBLOCK), -1, "");
  EXPECT_EQ(ZX_OK, init_helper(pipefd[0], NULL, 0), "");
  char msg[2048] = {0};
  char buf[2048] = {0};
  memset(msg, 'a', sizeof(msg) - 1);
  FX_LOGF(INFO, NULL, "%s", msg);
  size_t n = read(pipefd[1], buf, sizeof(buf));
  EXPECT_GT(n, 0u, "");
  msg[n] = 0;
  EXPECT_TRUE(ends_with(buf, "a...\n"), buf);

  msg[0] = '%';
  msg[1] = 's';
  FX_LOG(INFO, NULL, msg);
  n = read(pipefd[1], buf, sizeof(buf));
  EXPECT_GT(n, 0u, "");
  msg[n] = 0;
  EXPECT_TRUE(ends_with(buf, "a...\n"), buf);
  close(pipefd[1]);
  fx_log_reset_global_for_testing();
  END_TEST;
}

bool test_vlog_simple_write(void) {
  BEGIN_TEST;
  fx_log_reset_global_for_testing();
  int pipefd[2];
  EXPECT_NE(pipe2(pipefd, O_NONBLOCK), -1, "");
  EXPECT_EQ(ZX_OK, init_helper(pipefd[0], NULL, 0), "");
  FX_LOG_SET_VERBOSITY(1);
  FX_VLOG(1, NULL, "test message");
  char buf[256];
  size_t n = read(pipefd[1], buf, sizeof(buf));
  EXPECT_GT(n, 0u, "");
  buf[n] = 0;
  EXPECT_TRUE(ends_with(buf, "VLOG(1): test message\n"), buf);
  close(pipefd[1]);
  fx_log_reset_global_for_testing();
  END_TEST;
}

bool test_vlog_write(void) {
  BEGIN_TEST;
  fx_log_reset_global_for_testing();
  int pipefd[2];
  EXPECT_NE(pipe2(pipefd, O_NONBLOCK), -1, "");
  EXPECT_EQ(ZX_OK, init_helper(pipefd[0], NULL, 0), "");
  FX_LOG_SET_VERBOSITY(1);
  FX_VLOGF(1, NULL, "%d, %s", 10, "just some number");
  char buf[256];
  size_t n = read(pipefd[1], buf, sizeof(buf));
  EXPECT_GT(n, 0u, "");
  buf[n] = 0;
  EXPECT_TRUE(ends_with(buf, "VLOG(1): 10, just some number\n"), buf);
  close(pipefd[1]);
  fx_log_reset_global_for_testing();
  END_TEST;
}

BEGIN_TEST_CASE(syslog_tests)
RUN_TEST(test_log_init)
RUN_TEST(test_log_init_with_socket)
RUN_TEST(test_log_simple_write)
RUN_TEST(test_log_write)
RUN_TEST(test_log_preprocessed_message)
RUN_TEST(test_log_severity)
RUN_TEST(test_log_write_with_tag)
RUN_TEST(test_log_write_with_global_tag)
RUN_TEST(test_log_write_with_multi_global_tag)
RUN_TEST(test_log_enabled_macro)
RUN_TEST(test_vlog_simple_write)
RUN_TEST(test_vlog_write)
END_TEST_CASE(syslog_tests)

BEGIN_TEST_CASE(syslog_tests_edge_cases)
RUN_TEST(test_global_tag_limit)
RUN_TEST(test_msg_length_limit)
END_TEST_CASE(syslog_tests_edge_cases)
