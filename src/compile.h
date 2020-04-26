#ifndef COMPILE_H
#define COMPILE_H

extern int numscripts;
extern unsigned int scriptofstbl[1024];
extern char *code, *cpos, *src;
extern char token[2048];

extern void err(const char *str);
extern void EmitOperand();
extern void ProcessGoto();
extern void EmitD(int w);
extern void EmitC(char c);
extern void Expect(const char *str);
extern char NextIs(const char *str);
extern void GetString();
extern void EmitString(const char *str);

#endif // COMPILE_H