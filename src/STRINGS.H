#ifndef _STRINGS_H_
#define _STRINGS_H_
extern int add_to_string(char **str,const char *fstr,...);
extern const char *get_string(unsigned short n);
extern int parse_language_file(const char *name,int really);
extern void free_language_table(void);
extern int make_language_table(void);
extern unsigned int language_info(char **code,char **name,char **filename,unsigned int i);
extern unsigned int language_codetofile(const char *code);
#endif /*!_STRINGS_H_*/
