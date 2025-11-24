%option noyywrap
%{
#include <err.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "idblex.h"

struct idbline_s line = {0,};
idblex_cb_func_t cb_func = NULL;
void *cb_data = NULL;

unsigned colNum = 0;
unsigned lineNum = 1;

bool dump_line = false;

static void printLine(struct idbline_s *line);
static void freeLine(struct idbline_s *line);
static char *getBody(const char *s);
static unsigned long getNum(const char *s);

%}

%s various
%s config

%%

\n {
	int rc;
	if (dump_line) {
		printLine(&line);
		dump_line = false;
	}

	rc = cb_func(&line, cb_data);
	switch (rc) {
	case IDBLEX_CONTINUE:
		break;
	case IDBLEX_STOP:
	default:
		{
		yyterminate();
		}
		break;
	}

	freeLine(&line);
	colNum = 0;
	lineNum++;
	BEGIN(INITIAL);
}

[\t ]+ {
	colNum++;
}

<various>cmpsize\(.*\) {
	line.cmpsize_present = true;
	line.cmpsize = getNum(yytext);
}

<various>config\([^ ]*\) {
	line.config1 = getBody(yytext);
}

<various>delhist {
	line.flags |= FLAG_DELHIST;
}

<various>dev\(.*\) {
	/* TODO: get major and minor? */
}

<various>exitop\(\'(\\.|[^'\\])*\'\) {
	line.exitop = getBody(yytext);
}

<various>exitop\(\"(\\.|[^"\\])*\"\) {
	line.exitop = getBody(yytext);
}

<various>exitop\([^\)]*\) {
	line.exitop = getBody(yytext);
}

<various>f\(.*\) {
	line.f_present = true;
	line.f = getNum(yytext);
}

<various>mac\([^\)]+\) {
	line.mac = getBody(yytext);
}

<various>mach\([^\)]+\) {
	line.mach = getBody(yytext);
}

<various>needrqs {
	line.flags |= FLAG_NEEDRQS;
}

<various>nohist {
	line.flags |= FLAG_NOHIST;
}

<various>norqs {
	line.flags |= FLAG_NORQS;
}

<various>noshare {
	line.flags |= FLAG_NOSHARE;
}

<various>nostrip {
	line.flags |= FLAG_NOSTRIP;
}

<various>off\([0-9]+\) {
	line.off_present = true;
	line.off = getNum(yytext);
}

<various>postop\(\'(\\.|[^'\\])*\'\) {
	line.postop = getBody(yytext);
}

<various>postop\(\"(\\.|[^"\\])*\"\) {
	line.postop = getBody(yytext);
}

<various>postop\([^\)]*\) {
	line.postop = getBody(yytext);
}

<various>preop\(\'(\\.|[^'\\])*\'\) {
	line.preop = getBody(yytext);
}

<various>preop\(\"(\\.|[^"\\])*\"\) {
	line.preop = getBody(yytext);
}

<various>preop\([^\)]*\) {
	line.preop = getBody(yytext);
}

<various>removeop\(\'(\\.|[^'\\])*\'\) {
	line.removeop = getBody(yytext);
}

<various>removeop\(\"(\\.|[^"\\])*\"\) {
	line.removeop = getBody(yytext);
}

<various>removeop\([^\)]+\) {
	line.removeop = getBody(yytext);
}

<various>shadow {
	line.flags |= FLAG_SHADOW;
}

<various>size\([0-9]+\) {
	line.size_present = true;
	line.size = getNum(yytext);
}

<various>stripdso {
	line.flags |= FLAG_STRIPDSO;
}

<various>sum\([0-9]+\) {
	line.sum_present = true;
	line.sum = getNum(yytext);
}

<various>symval\([^)]*\) {
	line.symval = getBody(yytext);
}

<various>[^\t\n ]+ {
	/* catch-all, but also grab subsystem */
	if (!line.subsystem) {
		line.subsystem = strdup(yytext);
	} else {
		printf("token: '%s'\n", yytext);
		warnx("unknown token on line %u", lineNum);
		dump_line = true;
	}
}

[^\t\n ]+ {
	switch (colNum) {
	case 0:
		line.line_num = lineNum;
		line.type = yytext[0];
		break;
	case 1:
		{
		unsigned long temp;
		char *ptr = NULL;
		temp = strtoul(yytext, &ptr, 8);
		if (ptr && (*ptr != '\0')) errx(1, "problem in mode strtoul: '%s'", ptr);
		line.mode = temp;
		}
		break;
	case 2:
		line.userName = strdup(yytext);
		break;
	case 3:
		line.groupName = strdup(yytext);
		break;
	case 4:
		line.installPath = strdup(yytext);
		break;
	case 5:
		line.sourcePath = strdup(yytext);
		BEGIN(various);
		break;
	default:
		errx(1, "shouldn't reach here");
	}
}
%%
static void printLine(struct idbline_s *line)
{
	printf("type: %c\n", line->type);
	printf("mode: %o\n", line->mode);
	printf("userName: '%s'\n", line->userName);
	printf("groupName: '%s'\n", line->groupName);
	printf("installPath: '%s'\n", line->installPath);
	printf("sourcePath: '%s'\n", line->sourcePath);
	printf("subsystem: '%s'\n", line->subsystem);
	printf("offset: %u\n", line->off);
	printf("size: %u\n", line->size);
	printf("cmpsize: %u\n", line->cmpsize);
	printf("preop: '%s'\n", line->preop);
	/* TODO */
	printf("\n");
}

static void freeLine(struct idbline_s *line)
{
	free(line->userName);
	free(line->groupName);
	free(line->installPath);
	free(line->sourcePath);
	free(line->subsystem);
	
	free(line->config1);
	free(line->exitop);
	free(line->mach);
	free(line->preop);
	free(line->postop);
	free(line->removeop);
	free(line->symval);
	free(line->mac);
	
	memset(line, 0, sizeof(*line));
}

static char *getBody(const char *s)
{
	__label__ out_error, out_ok;
	char *retval = NULL;
	char *tmp, *ptr, *mydup = NULL;
	
	if (!s) goto out_error;
	
	mydup = strdup(s);
	if (!mydup) goto out_error;
	
	tmp = strchr(mydup, '(');
	if (!tmp) goto out_error;
	*tmp = '\0';
	ptr = tmp + 1;
	if (*ptr == '\0') goto out_error;
	
	tmp = strrchr(ptr, ')');
	if (!tmp) goto out_error;
	*tmp = '\0';
	
	retval = strdup(ptr);
	free(mydup);
	goto out_ok;
	
out_error:
	free(mydup);
	retval = NULL;
out_ok:
	return retval;
}

static unsigned long getNum(const char *s)
{
	__label__ out_error, out_ok;
	unsigned long retval = 0;
	char *tmp = NULL;
	char *ptr;
	
	ptr = getBody(s);
	if (!ptr) goto out_error;
	
	retval = strtoul(ptr, &tmp, 10);
	if ((*tmp != '\0') && (*tmp != ')')) goto out_error;
	goto out_ok;
	
out_error:
	printf("idblex getNum failed");
	retval = -1;
out_ok:
	free(ptr);
	return retval;
}

void idblex(char *buf, size_t len, idblex_cb_func_t func, void *data)
{
	YY_BUFFER_STATE state;

	memset(&line, 0, sizeof(line));
	colNum = 0;
	lineNum = 1;
	cb_func = func;
	cb_data = data;

	state = yy_scan_buffer(buf, len);
	yylex();
	yy_delete_buffer(state);
	yylex_destroy();

}
