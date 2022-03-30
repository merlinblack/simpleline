#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

#define BUFFERSIZE 1024
#define MAX_BRANCHNAME_LEN 256
#define PATH_SHORTEN_LENGTH 35

#define REPO_BRANCH "\uE0A0"
#define REPO_STAGED "\u2714"
#define REPO_NOT_STAGED "\u270E"
#define REPO_UNTRACKED "+"
#define REPO_CONFLICTED "\u273C"
#define REPO_STASHED "\u2691"

#define BEERMUG "\U0001F37A"
#define PENGUIN "\U0001F427"
#define PATH_ELLIPSIS "\u2026"

#define SEGMENT "\uE0B0"
#define SEGMENT_THIN "\uE0B1"

#define RESET_COLOR "\\[\x1b[0m\\]"

typedef struct Segment {
	char text[MAX_BRANCHNAME_LEN];
	unsigned char fore_color;
	unsigned char back_color;
	bool bold;
	bool raw;
	struct Segment *next;
} Segment;

Segment *addSegment(Segment *prev, char *text, unsigned char fore_color, unsigned char back_color, bool bold)
{
	Segment *segment = malloc(sizeof *segment);

	strcpy(segment->text, text);
	segment->fore_color = fore_color;
	segment->back_color = back_color;
	segment->bold = bold;
	segment->raw = false;
	segment->next = NULL;

	if (prev)
	{
		prev->next = segment;
	}

	return segment;
}

void freeSegments(Segment *head)
{
	Segment *current = head;

	while (current)
	{
		current = current->next;
		free(head);
		head = current;
	}
}


Segment *git_segments(Segment *current)
{
	bool isGit = true;
	char buffer[BUFFERSIZE];
	char branch[MAX_BRANCHNAME_LEN] = { 0 };
	unsigned modified = 0;
	unsigned untracked = 0;
	unsigned staged = 0;

	FILE *git = popen("git status --porcelain=v1 --branch --ignore-submodules 2>&1", "r");

	while (fgets(buffer, BUFFERSIZE, git) != NULL)
	{
		if (strncmp(buffer, "fatal", 5) == 0)
		{
			isGit = false;
			break;
		}
		switch (buffer[0])
		{
			case '#':
        {
          char* end = strchr(&buffer[3], '.');
          if (!end)
          {
            end = strchr(&buffer[3], '\n');
          }
          if (end)
          {
            strncpy(branch, buffer + 3, end - &buffer[3]);
          }
          else 
          {
            strcpy(branch, &buffer[3]);
          }
        }
				break;
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
		switch (buffer[1])
		{
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

	if (isGit)
	{
		char buffer[256];

		sprintf(buffer, "%s %s", REPO_BRANCH, branch);
		current = addSegment( current, buffer, 231, 52, false);

		sprintf(buffer, "%s %u %s %u %s%d",
				REPO_NOT_STAGED, modified,
				REPO_STAGED, staged,
				REPO_UNTRACKED, untracked);
		current = addSegment( current, buffer, 231, 32, false);

		sprintf(buffer, "%s\r\n", RESET_COLOR);
		current = addSegment( current, buffer, 0, 0, false);
		current->raw = true;
	}

	return current;
}

Segment *notice_segment(Segment *current)
{
    if (getenv("PROMPT_NOTICE"))
    {
        current = addSegment(current, getenv("PROMPT_NOTICE"), 231, 38, false);
        current = addSegment(current, RESET_COLOR "\r\n", 231, 22, false);
        current->raw = true;
    }
	return current;
}

Segment *user_segment(Segment *current)

{
	return addSegment(current, getenv("USER"), 231, 22, true);
}

Segment *host_segment(Segment *current)
{
  if (getenv("SSH_CLIENT") != NULL)
  {
    char buffer[HOST_NAME_MAX+1];
    gethostname(buffer, HOST_NAME_MAX+1);
    current = addSegment( current, buffer, 231, 89, false );
  }
  return current;
}

Segment *current_dir_segments(Segment *current)
{
	char *seperator = "/";
	char *homedir = getenv("HOME");
	char *pwd = getenv("PWD");
	char path[4096];

	current = addSegment(current, "~", 231, 238, false);
	if (strstr(pwd, homedir) == pwd)
	{
		strcpy(path, pwd + strlen(homedir));
	}
	else
	{
		current->text[0] = '/';
		strcpy(path, pwd);
	}

	int path_len = strlen(path);
	char *folder = strtok(path, seperator);

	if (path_len < PATH_SHORTEN_LENGTH)
	{
		while (folder)
		{
			current = addSegment(current, folder, 231, 238, false);
			folder = strtok(NULL, seperator);
		}
	}
	else {
		// Only add first, '...' and last current path folder.
		current = addSegment(current, folder, 231, 238, false);
		folder = strtok(NULL, seperator);
		current = addSegment(current, PATH_ELLIPSIS, 231, 238, false);
		char *prev_folder = NULL;
		while (folder)
		{
			prev_folder = folder;
			folder = strtok(NULL, seperator);
		}
		if (prev_folder)
		{
			current = addSegment(current, prev_folder, 231, 238, false);
		}
	}

	current->bold = true;

	return current;
}

Segment *friday_icon_segment(Segment *current)
{
	char* which = PENGUIN;

	time_t unixtime;

	time(&unixtime);

	struct tm *lt = localtime(&unixtime);

	if (lt->tm_wday == 5)
	{
		which = BEERMUG;
	}

	return addSegment(current, which, 231, 238, false);
}

void print_segments(Segment *head)
{
	Segment *current = head;
	unsigned char last_back_color;
	while (current)
	{
		if (current->raw)
		{
			printf("%s", current->text);
			current = current->next;
			continue;
		}
		if (current->bold)
		{
			printf("%s", "\\[\x1b[1m\\]");
		}
		printf("\\[\x1b[38;5;%dm\x1b[48;5;%dm\\] %s \\[\x1b[0m\\]", current->fore_color, current->back_color, current->text);
		if (current->next)
		{
			if (current->back_color == current->next->back_color)
			{
				printf("\\[\x1b[38;5;245m\x1b[48;5;%dm\\]%s", current->next->back_color, SEGMENT_THIN);
			}
			else
			{
				printf("\\[\x1b[38;5;%dm\x1b[48;5;%dm\\]%s", current->back_color, current->next->back_color, SEGMENT);
			}
		}

		last_back_color = current->back_color;
		current = current->next;
	}

	printf("\\[\x1b[38;5;%dm\x1b[48;5;0m\\]%s%s ", last_back_color, SEGMENT, RESET_COLOR);

}

int main(int argc, char*argv[])
{
	Segment *head = addSegment(NULL, "", 0, 0, false);
	head->raw = true;

	Segment *current;

	current = git_segments(head);
	current = notice_segment(current);
	current = user_segment(current);
	current = host_segment(current);
	current = current_dir_segments(current);
	current = friday_icon_segment(current);

	print_segments(head);

	freeSegments(head);

	return EXIT_SUCCESS;
}
