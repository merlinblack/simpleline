#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

// For Darwin
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 128
#endif

#define MAX_SEGMENTS 20
#define PROC_BUFFER_SIZE 1024
#define MAX_BRANCHNAME_LEN 256
#define MAX_SEGMENT_TEXT 256
#define BUFFER_SIZE MAX_SEGMENT_TEXT - 1  // -1 keeps strncat warnings away
#define PATH_SHORTEN_LENGTH 35
#define REPO_BRANCH "\uE0A0"      // 
#define REPO_STAGED "\u2714"      // ✔
#define REPO_NOT_STAGED "\u270E"  // ✎
#define REPO_UNTRACKED "+"
#define REPO_CONFLICTED "\u273C"  // ✼
#define REPO_STASHED "\u2691"     // ⚑

#define BEERMUG "\U0001F37A"    // 🍺
#define PENGUIN "\U0001F427"    // 🐧
#define TOOLBOX "\U0001F6E0"    // 🛠
#define PATH_ELLIPSIS "\u2026"  // …

#define SEGMENT "\uE0B0"       // 
#define SEGMENT_THIN "\uE0B1"  // 

#define RESET_COLOR "\\[\e[0m\\]"

// See 'man console_codes' and search for 'ECMA-48 Select Graphic Rendition'
// for ANSI escape codes for colours

// Command line parameter values
int last_command_exit_code = 0;
int number_of_jobs_running = 0;

typedef struct Segment {
  char text[MAX_BRANCHNAME_LEN];
  unsigned char fore_color;
  unsigned char back_color;
  bool bold;
  bool italics;
  bool raw;
} Segment;

Segment segments[MAX_SEGMENTS];
int last_used_segment = 0;

Segment* addSegment(char* text,
                    unsigned char fore_color,
                    unsigned char back_color)
{
  if (last_used_segment == MAX_SEGMENTS) {
    fprintf(stderr, "ERROR! Maximum segments reached!\n");
    exit(EXIT_FAILURE);
  }
  Segment* segment = &segments[last_used_segment++];

  segment->text[0] = 0;
  strncat(segment->text, text, MAX_BRANCHNAME_LEN - 1);
  segment->fore_color = fore_color;
  segment->back_color = back_color;
  segment->bold = false;
  segment->italics = false;
  segment->raw = false;

  return segment;
}

void parse_arguments(int argc, char* argv[])
{
  struct option long_options[] = {{"exitcode", required_argument, 0, 'e'},
                                  {"jobs", required_argument, 0, 'j'},
                                  {0, 0, 0, 0}};
  int option_index = 0;

  // Keep going unill we break out
  while (true) {
    int c = getopt_long(argc, argv, "e:j:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 'e':
        last_command_exit_code = atoi(optarg);
        break;
      case 'j':
        number_of_jobs_running = atoi(optarg);
        break;
    }
  }
}

void git_segments()
{
  bool isGit = true;
  char buffer[PROC_BUFFER_SIZE];
  char branch[MAX_BRANCHNAME_LEN] = {0};
  unsigned modified = 0;
  unsigned untracked = 0;
  unsigned staged = 0;

  FILE* git =
      popen("git status --porcelain=v1 --branch --ignore-submodules 2>&1", "r");

  while (fgets(buffer, PROC_BUFFER_SIZE, git) != NULL) {
    if (strncmp(buffer, "fatal", 5) == 0) {
      isGit = false;
      break;
    }
    switch (buffer[0]) {
      case '#': {
        char* end = strchr(&buffer[3], '.');
        if (!end) {
          end = strchr(&buffer[3], '\n');
        }
        if (end) {
          strncpy(branch, &buffer[3], end - &buffer[3]);
        } else {
          strcpy(branch, &buffer[3]);
        }
        break;
      }
      case 'A':
      case 'M':
      case 'C':
      case 'R':
      case 'D':
        staged++;
        break;
      case '?':
        untracked++;
        break;
    }
    switch (buffer[1]) {
      case 'A':
      case 'M':
      case 'C':
      case 'R':
      case 'D':
        modified++;
        break;
    }
  }

  pclose(git);

  if (isGit) {
    char buffer[BUFFER_SIZE];

    snprintf(buffer, BUFFER_SIZE, "%s %s", REPO_BRANCH, branch);
    Segment* segment = addSegment(buffer, 231, 52);
    segment->italics = true;

    snprintf(buffer, BUFFER_SIZE, "%s %u %s %u %s%d", REPO_NOT_STAGED, modified,
             REPO_STAGED, staged, REPO_UNTRACKED, untracked);
    addSegment(buffer, 231, 32);

    segment = addSegment(RESET_COLOR "\r\n", 0, 0);
    segment->raw = true;
  }
}

void notice_segment()
{
  if (getenv("PROMPT_NOTICE")) {
    addSegment(getenv("PROMPT_NOTICE"), 231, 38);
    Segment* segment = addSegment(RESET_COLOR "\r\n", 0, 0);
    segment->raw = true;
  }
}

void user_segment()
{
  Segment* segment = addSegment(getenv("USER"), 231, 22);
  segment->bold = true;
}

void host_segment()
{
  if (getenv("SSH_CONNECTION") != NULL) {
    char buffer[HOST_NAME_MAX + 1];
    gethostname(buffer, HOST_NAME_MAX);
    buffer[HOST_NAME_MAX] = 0;  // Incase of truncation

    // Only display up to the frist '.' if any.
    char* dot = strchr(buffer, '.');
    if (dot) {
      *dot = 0;
    }

    addSegment(buffer, 231, 89);
  }
}

void python_virtual_env_segment()
{
  char* env = "VIRTUAL_ENV";
  if (getenv(env) != NULL) {
    char* seperator = "/";
    char buffer[PATH_MAX + 1];
    strncpy(buffer, getenv(env), PATH_MAX);
    char* folder = strtok(buffer, seperator);
    char* prev = NULL;
    char* prev2 = NULL;
    while (folder) {
      prev2 = prev;
      prev = folder;
      folder = strtok(NULL, seperator);
    }
    if (prev2) {
      addSegment(prev2, 231, 38);
      Segment* segment = addSegment(RESET_COLOR "\r\n", 231, 0);
      segment->raw = true;
    }
  }
}

void aws_awsume_profile_segment()
{
  char* profile = getenv("AWSUME_PROFILE");
  if (profile) {
    if (strcmp(profile, "connect_dev") == 0) {
      // Default - dont show
      return;
    }

    char buffer[BUFFER_SIZE];

    snprintf(buffer, BUFFER_SIZE, "AWS: %s", profile);

    addSegment(buffer, 231, 20);
    Segment* segment = addSegment(RESET_COLOR "\r\n", 231, 0);
    segment->raw = true;
  }
}

void jobs_running_segment()
{
  if (number_of_jobs_running) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "%d Job%s", number_of_jobs_running,
             number_of_jobs_running == 1 ? "" : "s");
    addSegment(buffer, 231, 22);
  }
}

void exitcode_segment()
{
  if (last_command_exit_code) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "%d", last_command_exit_code);
    Segment* segment = addSegment(buffer, 231, 3);
    segment->bold = true;
  }
}

void current_dir_segments()
{
  char* seperator = "/";
  char* homedir = getenv("HOME");
  char* pwd = getenv("PWD");
  char path[4096];

  Segment* segment = addSegment("~", 231, 238);
  if (strstr(pwd, homedir) == pwd) {
    strcpy(path, pwd + strlen(homedir));
  } else {
    segment->text[0] = '/';
    strcpy(path, pwd);
  }

  int path_len = strlen(path);
  char* folder = strtok(path, seperator);

  if (path_len < PATH_SHORTEN_LENGTH) {
    while (folder) {
      segment = addSegment(folder, 231, 238);
      segment->italics = true;
      folder = strtok(NULL, seperator);
    }
  } else {
    // Only add first, '...' and last current path folder.
    segment = addSegment(folder, 231, 238);
    segment->italics = true;
    folder = strtok(NULL, seperator);
    segment = addSegment(PATH_ELLIPSIS, 231, 238);
    char* prev_folder = NULL;
    while (folder) {
      prev_folder = folder;
      folder = strtok(NULL, seperator);
    }
    if (prev_folder) {
      segment = addSegment(prev_folder, 231, 238);
      segment->italics = true;
    }
  }

  segment->bold = true;
}

void inside_toolbx_segment()
{
  if (access("/run/.containerenv", F_OK) == 0) {
    addSegment(TOOLBOX, 231, 52);
  }
}

void friday_icon_segment()
{
  char* which = PENGUIN;

  time_t unixtime;

  time(&unixtime);

  struct tm* lt = localtime(&unixtime);

  if (lt->tm_wday == 5) {
    which = BEERMUG;
  }

  addSegment(which, 231, 238);
}

void print_segments()
{
  unsigned char last_back_color = 0;
  int index = 0;

  while (index < last_used_segment) {
    Segment* current = &segments[index];
    if (current->raw) {
      printf("%s", current->text);
      index++;
      continue;
    }
    if (current->bold) {
      printf("%s", "\\[\e[1m\\]");
    }
    if (current->italics) {
      printf("%s", "\\[\e[3m\\]");
    }
    printf("\\[\e[38;5;%d;48;5;%dm\\] %s \\[\e[0m\\]", current->fore_color,
           current->back_color, current->text);
    if ((index + 1) < last_used_segment) {
      Segment* next = &segments[index + 1];
      if (current->back_color == next->back_color) {
        printf("\\[\e[38;5;245;48;5;%dm\\]%s", next->back_color, SEGMENT_THIN);
      } else if (next->back_color == 0) {
        printf("\\[\e[38;5;%d;49m\\]%s", current->back_color, SEGMENT);
      } else {
        printf("\\[\e[38;5;%d;48;5;%dm\\]%s", current->back_color,
               next->back_color, SEGMENT);
      }
    }

    last_back_color = current->back_color;
    index++;
  }

  printf("\\[\e[38;5;%d;49m\\]%s%s ", last_back_color, SEGMENT, RESET_COLOR);
}

int main(int argc, char* argv[])
{
  parse_arguments(argc, argv);

  Segment* head = addSegment(RESET_COLOR, 0, 0);
  head->raw = true;

  notice_segment();
  git_segments();
  python_virtual_env_segment();
  aws_awsume_profile_segment();
  user_segment();
  host_segment();
  current_dir_segments();
  jobs_running_segment();
  friday_icon_segment();
  inside_toolbx_segment();
  exitcode_segment();

  print_segments();

  return EXIT_SUCCESS;
}
