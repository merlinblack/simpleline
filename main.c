#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <getopt.h>

// For Darwin
#ifndef HOST_NAME_MAX
#  define HOST_NAME_MAX 128
#endif

#define PROC_BUFFER_SIZE 1024
#define MAX_BRANCHNAME_LEN 256
#define BUFFER_SIZE 256
#define PATH_SHORTEN_LENGTH 35

#define REPO_BRANCH "\uE0A0" // 
#define REPO_STAGED "\u2714" // ✔
#define REPO_NOT_STAGED "\u270E" // ✎
#define REPO_UNTRACKED "+"
#define REPO_CONFLICTED "\u273C" // ✼
#define REPO_STASHED "\u2691" // ⚑

#define BEERMUG "\U0001F37A" // 🍺
#define PENGUIN "\U0001F427" // 🐧
#define PATH_ELLIPSIS "\u2026" // …

#define SEGMENT "\uE0B0" // 
#define SEGMENT_THIN "\uE0B1" // 

#define RESET_COLOR "\\[\e[0m\\]"

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
	struct Segment *next;
} Segment;

Segment *addSegment(Segment *prev, char *text, unsigned char fore_color, unsigned char back_color)
{
	Segment *segment = malloc(sizeof *segment);

	segment->text[0] = 0;
	strncat(segment->text, text, MAX_BRANCHNAME_LEN-1);
	segment->fore_color = fore_color;
	segment->back_color = back_color;
	segment->bold = false;
	segment->italics = false;
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

void parse_arguments(int argc, char *argv[])
{
	struct option long_options[] =
	{
		{"exitcode", required_argument, 0, 'e'},
		{"jobs", required_argument, 0, 'j'},
		{0, 0, 0, 0}
	};
	int option_index = 0;

	// Keep going unill we break out
	while(true)
	{
		int c = getopt_long(argc, argv, "e:j:", long_options, &option_index);

		if (c == -1)
			break;

		switch (c)
		{
			case 'e':
				last_command_exit_code = atoi(optarg);
				break;
			case 'j':
				number_of_jobs_running = atoi(optarg);
				break;
		}
	}
}

Segment* git_segments(Segment *current)
{
	bool isGit = true;
	char buffer[PROC_BUFFER_SIZE];
	char branch[MAX_BRANCHNAME_LEN] = { 0 };
	unsigned modified = 0;
	unsigned untracked = 0;
	unsigned staged = 0;

	FILE *git = popen("git status --porcelain=v1 --branch --ignore-submodules 2>&1", "r");

	while (fgets(buffer, PROC_BUFFER_SIZE, git) != NULL)
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
				char *end = strchr(&buffer[3], '.');
				if (!end)
				{
					end = strchr(&buffer[3], '\n');
				}
				if (end)
				{
					strncpy(branch, &buffer[3], end - &buffer[3]);
				}
				else
				{
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
		char buffer[BUFFER_SIZE];

		snprintf(buffer, BUFFER_SIZE, "%s %s", REPO_BRANCH, branch);
		current = addSegment( current, buffer, 231, 52);
        current->italics = true;

		snprintf(buffer, BUFFER_SIZE, "%s %u %s %u %s%d",
				REPO_NOT_STAGED, modified,
				REPO_STAGED, staged,
				REPO_UNTRACKED, untracked);
		current = addSegment( current, buffer, 231, 32);

		current = addSegment( current, RESET_COLOR "\r\n", 0, 0);
		current->raw = true;
	}

	return current;
}

Segment* notice_segment(Segment *current)
{
	if (getenv("PROMPT_NOTICE"))
	{
		current = addSegment(current, getenv("PROMPT_NOTICE"), 231, 38);
		current = addSegment(current, RESET_COLOR "\r\n", 0, 0);
		current->raw = true;
	}
	return current;
}

Segment* user_segment(Segment *current)
{
    current = addSegment(current, getenv("USER"), 231, 22);
    current->bold = true;
	return current; 
}

Segment* host_segment(Segment *current)
{
	if (getenv("SSH_CONNECTION") != NULL)
	{
		char buffer[HOST_NAME_MAX+1];
		gethostname(buffer, HOST_NAME_MAX);
        buffer[HOST_NAME_MAX] = 0; // Incase of truncation

        // Only display up to the frist '.' if any.
        char *dot = strchr(buffer, '.');
        if (dot) {
            *dot = 0;
        }

		current = addSegment( current, buffer, 231, 89 );
	}
	return current;
}

Segment* python_virtual_env_segment(Segment* current)
{
	char *env = "VIRTUAL_ENV";
	if (getenv(env) != NULL)
	{
		char *seperator = "/";
		char buffer[PATH_MAX];
		strncpy(buffer, getenv(env), PATH_MAX);
		char *folder = strtok(buffer, seperator);
		char *prev = NULL;
		char *prev2 = NULL;
		while (folder)
		{
			prev2 = prev;
			prev = folder;
			folder = strtok(NULL, seperator);
		}
		if (prev2) {
			current = addSegment(current, prev2, 231, 38 );
			current = addSegment(current, RESET_COLOR "\r\n", 231, 0);
			current->raw = true;
		}
	}
	return current;
}

Segment* aws_awsume_profile_segment(Segment* current)
{
    char *profile = getenv("AWSUME_PROFILE");
    if (profile) {
        if (strcmp(profile, "connect_dev") == 0) {
            // Default - dont show
            return current;
        }

        char buffer[BUFFER_SIZE];

        snprintf(buffer, BUFFER_SIZE, "AWS: %s", profile);

        current = addSegment(current, buffer, 231, 20 );
        current = addSegment(current, RESET_COLOR "\r\n", 231, 0);
        current->raw = true;
    }
    return current;
}

Segment* jobs_running_segment(Segment* current)
{
	if (number_of_jobs_running)
	{
		char buffer[BUFFER_SIZE];
		snprintf(buffer, BUFFER_SIZE, "%d Job%s", number_of_jobs_running, number_of_jobs_running == 1 ? "": "s" );
		current = addSegment(current, buffer, 231, 22);
	}
	return current;
}

Segment* exitcode_segment(Segment* current)
{
	if (last_command_exit_code)
	{
		char buffer[BUFFER_SIZE];
		snprintf(buffer, BUFFER_SIZE, "%d", last_command_exit_code);
		current = addSegment(current, buffer, 231, 3);
        current->bold = true;
	}

	return current;
}

Segment* current_dir_segments(Segment *current)
{
	char *seperator = "/";
	char *homedir = getenv("HOME");
	char *pwd = getenv("PWD");
	char path[4096];

	current = addSegment(current, "~", 231, 238);
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
			current = addSegment(current, folder, 231, 238);
            current->italics = true;
			folder = strtok(NULL, seperator);
		}
	}
	else {
		// Only add first, '...' and last current path folder.
		current = addSegment(current, folder, 231, 238);
        current->italics = true;
		folder = strtok(NULL, seperator);
		current = addSegment(current, PATH_ELLIPSIS, 231, 238);
		char *prev_folder = NULL;
		while (folder)
		{
			prev_folder = folder;
			folder = strtok(NULL, seperator);
		}
		if (prev_folder)
		{
			current = addSegment(current, prev_folder, 231, 238);
            current->italics = true;
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

	return addSegment(current, which, 231, 238);
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
			printf("%s", "\\[\e[1m\\]");
		}
		if (current->italics)
		{
			printf("%s", "\\[\e[3m\\]");
		}
		printf("\\[\e[38;5;%dm\e[48;5;%dm\\] %s \\[\e[0m\\]", current->fore_color, current->back_color, current->text);
		if (current->next)
		{
			if (current->back_color == current->next->back_color)
			{
				printf("\\[\e[38;5;245m\e[48;5;%dm\\]%s", current->next->back_color, SEGMENT_THIN);
			}
            else if (current->next->back_color == 0)
            {
				printf("\\[\e[38;5;%dm\e[49m\\]%s", current->back_color, SEGMENT);
            }
			else
			{
				printf("\\[\e[38;5;%dm\e[48;5;%dm\\]%s", current->back_color, current->next->back_color, SEGMENT);
			}
		}

		last_back_color = current->back_color;
		current = current->next;
	}

	printf("\\[\e[38;5;%dm\e[49m\\]%s%s ", last_back_color, SEGMENT, RESET_COLOR);

}

int main(int argc, char*argv[])
{
	parse_arguments(argc, argv);

	Segment *head = addSegment(NULL, RESET_COLOR, 0, 0);
	head->raw = true;

	Segment *current;

	current = notice_segment(head);
	current = git_segments(current);
	current = python_virtual_env_segment(current);
	current = aws_awsume_profile_segment(current);
	current = user_segment(current);
	current = host_segment(current);
	current = current_dir_segments(current);
	current = jobs_running_segment(current);
	current = friday_icon_segment(current);
	current = exitcode_segment(current);

	print_segments(head);

	freeSegments(head);

	return EXIT_SUCCESS;
}
