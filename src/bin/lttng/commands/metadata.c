/*
 * Copyright (C) 2015 - Julien Desfossez <jdesfossez@efficios.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2 only,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define _LGPL_SOURCE
#include <assert.h>
#include <ctype.h>
#include <popt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <common/mi-lttng.h>

#include "../command.h"

static char *opt_session_name;
static char *session_name = NULL;

static int metadata_regenerate(int argc, const char **argv);

enum {
	OPT_HELP = 1,
	OPT_LIST_OPTIONS,
	OPT_LIST_COMMANDS,
};

static struct poptOption long_options[] = {
	/* { longName, shortName, argInfo, argPtr, value, descrip, argDesc, } */
	{ "help",		'h', POPT_ARG_NONE, 0, OPT_HELP, 0, 0, },
	{ "session",		's', POPT_ARG_STRING, &opt_session_name, 0, 0, 0},
	{ "list-options",	0, POPT_ARG_NONE, NULL, OPT_LIST_OPTIONS, 0, 0, },
	{ "list-commands",  	0, POPT_ARG_NONE, NULL, OPT_LIST_COMMANDS},
	{ 0, 0, 0, 0, 0, 0, 0, },
};

static struct cmd_struct actions[] = {
	{ "regenerate", metadata_regenerate },
	{ NULL, NULL }	/* Array closure */
};

/*
 * usage
 */
static void usage(FILE *ofp)
{
	fprintf(ofp, "usage: lttng metadata [OPTION] ACTION\n");
	fprintf(ofp, "\n");
	fprintf(ofp, "Actions:\n");
	fprintf(ofp, "   regenerate\n");
	fprintf(ofp, "      Regenerate and overwrite the metadata of the session.\n");
	fprintf(ofp, "Options:\n");
	fprintf(ofp, "  -h, --help               Show this help.\n");
	fprintf(ofp, "      --list-options       Simple listing of options.\n");
	fprintf(ofp, "  -s, --session NAME       Apply to session name.\n");
	fprintf(ofp, "\n");
}

/*
 * Count and return the number of arguments in argv.
 */
static int count_arguments(const char **argv)
{
	int i = 0;

	assert(argv);

	while (argv[i] != NULL) {
		i++;
	}

	return i;
}

static int metadata_regenerate(int argc, const char **argv)
{
	int ret;

	ret = lttng_metadata_regenerate(session_name);
	if (ret == 0) {
		MSG("Metadata successfully regenerated for session %s", session_name);
	}
	return ret;
}

static int handle_command(const char **argv)
{
	struct cmd_struct *cmd;
	int ret = CMD_SUCCESS, i = 0, argc, command_ret = CMD_SUCCESS;

	if (argv == NULL) {
		usage(stderr);
		command_ret = CMD_ERROR;
		goto end;
	}

	argc = count_arguments(argv);

	cmd = &actions[i];
	while (cmd->func != NULL) {
		/* Find command */
		if (strcmp(argv[0], cmd->name) == 0) {
			command_ret = cmd->func(argc, argv);
			goto end;
		}

		cmd = &actions[i++];
	}

	ret = CMD_UNDEFINED;

end:
	/* Overwrite ret if an error occurred in cmd->func() */
	ret = command_ret ? command_ret : ret;
	return ret;
}

/*
 * Metadata command handling.
 */
int cmd_metadata(int argc, const char **argv)
{
	int opt, ret = CMD_SUCCESS, command_ret = CMD_SUCCESS;
	static poptContext pc;

	if (argc < 1) {
		usage(stderr);
		ret = CMD_ERROR;
		goto end;
	}

	pc = poptGetContext(NULL, argc, argv, long_options, 0);
	poptReadDefaultConfig(pc, 0);

	while ((opt = poptGetNextOpt(pc)) != -1) {
		switch (opt) {
		case OPT_HELP:
			usage(stdout);
			goto end;
		case OPT_LIST_OPTIONS:
			list_cmd_options(stdout, long_options);
			goto end;
		case OPT_LIST_COMMANDS:
			list_commands(actions, stdout);
			goto end;
		default:
			usage(stderr);
			ret = CMD_UNDEFINED;
			goto end;
		}
	}

	if (!opt_session_name) {
		session_name = get_session_name();
		if (session_name == NULL) {
			ret = CMD_ERROR;
			goto end;
		}
	} else {
		session_name = opt_session_name;
	}

	command_ret = handle_command(poptGetArgs(pc));
	if (command_ret) {
		switch (-command_ret) {
		default:
			ERR("%s", lttng_strerror(command_ret));
			break;
		}
	}

end:
	if (!opt_session_name) {
		free(session_name);
	}

	/* Overwrite ret if an error occurred during handle_command() */
	ret = command_ret ? command_ret : ret;

	poptFreeContext(pc);
	return ret;
}