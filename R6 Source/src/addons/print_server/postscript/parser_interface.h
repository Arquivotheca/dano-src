#ifndef _PARSER_INTERFACE_H_
#define _PARSER_INTERFACE_H_

#ifndef PARSETEST

extern	void open_group(char *name, char *translation);
extern	void close_group(char *name);
extern	void open_ui(char *name, char *translation, char *type);
extern 	void start_invocation(char *name, char *option, char *translation);
extern	void add_invocation_line(char *invoke);
extern	void close_ui(char *name);
extern	void print_everything();
extern	void imageable_area(char *name, char *translation, char *box);
extern	void paper_dimension(char *name, char *size);
extern  void set_language_level(char *lev);
extern 	void set_default(char *name, char *option, char *translation);
extern	void add_stringval(char *name, char *option, char *translation, char *string);
extern	void add_ui_constraint(char *key1, char *opt1, char *key2, char *opt2);
extern	void jcl_open_ui(char *name, char *translation, char *type);
extern	void jcl_close_ui(char *name);

#else

void open_group(char *name, char *translation) { }
void close_group(char *name) { }
void open_ui(char *name, char *translation, char *type) { }
void start_invocation(char *name, char *option, char *translation) {}
void add_invocation_line(char *invoke) { }
void close_ui(char *name) { }
void print_everything() { }
void imageable_area(char *name, char *translation, char *box) { }
void paper_dimension(char *name, char *size) { }
void set_language_level(char *lev) { }
void set_default(char *name, char *option, char *translation) { }
void add_stringval(char *name, char *option, char *translation, char *string) { }
void add_ui_constraint(char *key1, char *opt1, char *key2, char *opt2) { }
void jcl_open_ui(char *name, char *translation, char *type) { }
void jcl_close_ui(char *name) { }

#endif /* parsetest */



#endif /* _PARSER_INTERFACE_H_ */
