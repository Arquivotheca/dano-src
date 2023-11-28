#ifndef EXPD_UTILS
#define EXPD_UTILS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Alert.h>
#include <Entry.h>
#include <Mime.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>

void 	dirname(const char *path, char *newpath);
void 	path_from_entry_ref(const entry_ref *ref,char *p);

bool	TraverseSymLink(entry_ref *ref);

void	What(char *msg);

#endif
