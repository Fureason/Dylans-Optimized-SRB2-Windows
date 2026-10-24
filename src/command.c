// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------









#include "doomdef.h"
#include "doomstat.h"
#include "command.h"
#include "console.h"
#include "z_zone.h"
#include "m_menu.h"
#include "m_misc.h"
#include "m_fixed.h"
#include "m_argv.h"
#include "byteptr.h"
#include "p_saveg.h"
#include "g_game.h"  
#include "netcode/d_netcmd.h"
#include "netcode/net_command.h"
#include "hu_stuff.h"
#include "p_setup.h"
#include "lua_script.h"
#include "netcode/d_netfil.h"  
#include "r_data.h"  
#include "d_main.h"  




static boolean COM_Exists(const char *com_name);
static void COM_ExecuteString(char *com_text);

static void COM_Alias_f(void);
static void COM_Echo_f(void);
static void COM_CEcho_f(void);
static void COM_CEchoFlags_f(void);
static void COM_CEchoDuration_f(void);
static void COM_Exec_f(void);
static void COM_Wait_f(void);
static void COM_Help_f(void);
static void COM_Find_f(void);
static void COM_Toggle_f(void);
static void COM_Add_f(void);
static void COM_Sudo_f(void);


static void CV_EnforceExecVersion(void);
static boolean CV_FilterVarByVersion(consvar_t *v, const char *valstr);
static boolean CV_Command(void);
consvar_t *CV_FindVar(const char *name);
static const char *CV_StringValue(const char *var_name);
static boolean CV_Immutable(const consvar_t *var);

static consvar_t *consvar_vars;  
static UINT16     consvar_number_of_netids = 0;

#ifdef OLD22DEMOCOMPAT
static old_demo_var_t *consvar_old_demo_vars;
#endif

static char com_token[1024];
static char *COM_Parse(char *data);

static char * COM_Purge (char *text, int *lenp);

CV_PossibleValue_t CV_OnOff[] = {{0, "Off"}, {1, "On"}, {0, NULL}};
CV_PossibleValue_t CV_YesNo[] = {{0, "No"}, {1, "Yes"}, {0, NULL}};
CV_PossibleValue_t CV_Unsigned[] = {{0, "MIN"}, {999999999, "MAX"}, {0, NULL}};
CV_PossibleValue_t CV_Natural[] = {{1, "MIN"}, {999999999, "MAX"}, {0, NULL}};
CV_PossibleValue_t CV_TrueFalse[] = {{0, "False"}, {1, "True"}, {0, NULL}};




static boolean execversion_enabled = false;
consvar_t cv_execversion = CVAR_INIT ("execversion","25",CV_CALL,CV_Unsigned, CV_EnforceExecVersion);


static boolean joyaxis_default = false;
static boolean joyaxis2_default = false;
static INT32 joyaxis_count = 0;
static INT32 joyaxis2_count = 0;

#define COM_BUF_SIZE (32<<10)  
#define MAX_ALIAS_RECURSION 100  

static INT32 com_wait;  

boolean com_sudo = false;



typedef struct cmdalias_s
{
	struct cmdalias_s *next;
	char *name;
	char *value;  
} cmdalias_t;

static cmdalias_t *com_alias;  





static vsbuf_t com_text;  
static com_flags_t com_flags = 0;

                                               
   
                      
                                                                      
                     
                         
    
static char *
COM_Purge (char *s, int *np)
{
	char *t;
	char *p;
	int n;
	n = strlen(s);
	t = s + n + 1;
	p = s;
	while (( p = strchr(p, '\033') ))
	{
		memmove(p, &p[1], t - p - 1);
		n--;
	}
	if (np)
		(*np) = n;
	return s;
}

                                                          
   
                                 
                           
    
void COM_BufAddTextEx(const char *ptext, com_flags_t flags)
{
	int l;
	char *text;

	text = COM_Purge(Z_StrDup(ptext), &l);

	if (com_text.cursize + 2 + l >= com_text.maxsize)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Command buffer full!\n"));
		return;
	}

	VS_WriteEx(&com_text, text, l, flags);

	Z_Free(text);
}

                                                  
   
                                                                       
                        
    
void COM_BufInsertTextEx(const char *ptext, com_flags_t flags)
{
	const INT32 old_wait = com_wait;

	char *temp = NULL;
	size_t templen;


	templen = com_text.cursize;
	if (templen)
	{
		temp = M_Memcpy(ZZ_Alloc(templen), com_text.data, templen);
		VS_Clear(&com_text);
	}

	com_wait = 0;


	COM_BufAddTextEx(ptext, flags);
	COM_BufExecute();  

	com_wait += old_wait;


	if (templen)
	{
		VS_Write(&com_text, temp, templen);
		Z_Free(temp);
	}
}

                                                                          
    
void
COM_BufTicker(void)
{
	if (com_wait)
	{
		com_wait--;
		return;
	}

	COM_BufExecute();
}

                                                      
    
void COM_BufExecute(void)
{
	size_t i;
	char *ptext;
	char line[1024] = "";
	INT32 quotes;

	while (com_text.cursize)
	{

		ptext = (char *)com_text.data;

		quotes = 0;
		for (i = 0; i < com_text.cursize; i++)
		{
			if (ptext[i] == '\"' && !quotes && i > 0 && ptext[i-1] != ' ')  
				break;
			if (ptext[i] == '\"')
				quotes++;
			if (!(quotes & 1) && ptext[i] == ';')
				break;  
			if (ptext[i] == '\n' || ptext[i] == '\r')
				break;
		}

		M_Memcpy(line, ptext, i);
		line[i] = 0;






		if (i == com_text.cursize)

			com_text.cursize = 0;
		else
		{
			i++;
			com_text.cursize -= i;

			memmove(ptext, ptext+i, com_text.cursize);
		}


		COM_ExecuteString(line);


		if (com_wait)
		{
			com_wait--;
			break;
		}
	}

	com_flags = 0;
}

                                                                           
    
void COM_ImmedExecute(const char *ptext)
{
	size_t i = 0, j = 0;
	char line[1024] = "";
	INT32 quotes;

	while (i < strlen(ptext))
	{

		quotes = 0;
		for (j = 0; i < strlen(ptext); i++,j++)
		{
			if (ptext[i] == '\"' && !quotes && i > 0 && ptext[i-1] != ' ')  
				return;
			if (ptext[i] == '\"')
				quotes++;

			if ((!(quotes & 1) && ptext[i] == ';') || ptext[i] == '\n' || ptext[i] == '\r')
				break;
		}

		memcpy(line, ptext+(i-j), j);
		line[j] = 0;


		COM_ExecuteString(line);

		i++;  
	}
}





typedef struct xcommand_s
{
	const char *name;
	struct xcommand_s *next;
	com_func_t function;
	com_flags_t flags;
} xcommand_t;

static xcommand_t *com_commands = NULL;  

#define MAX_ARGS 80
static size_t com_argc;
static char *com_argv[MAX_ARGS];
static const char *com_null_string = "";
static char *com_args = NULL;  

static void Got_NetVar(UINT8 **p, INT32 playernum);

                                                       
    
void COM_Init(void)
{

	VS_Alloc(&com_text, COM_BUF_SIZE);


	COM_AddCommand("alias", COM_Alias_f, 0);
	COM_AddCommand("echo", COM_Echo_f, COM_LUA);
	COM_AddCommand("cecho", COM_CEcho_f, COM_LUA);
	COM_AddCommand("cechoflags", COM_CEchoFlags_f, COM_LUA);
	COM_AddCommand("cechoduration", COM_CEchoDuration_f, COM_LUA);
	COM_AddCommand("exec", COM_Exec_f, 0);
	COM_AddCommand("wait", COM_Wait_f, 0);
	COM_AddCommand("help", COM_Help_f, COM_LUA);
	COM_AddCommand("find", COM_Find_f, COM_LUA);
	COM_AddCommand("toggle", COM_Toggle_f, COM_LUA);
	COM_AddCommand("add", COM_Add_f, COM_LUA);
	COM_AddCommand("sudo", COM_Sudo_f, 0);
	RegisterNetXCmd(XD_NETVAR, Got_NetVar);
}

                                          
   
                                                     
                
    
size_t COM_Argc(void)
{
	return com_argc;
}

                                    
   
                                                           
                                                     
                          
    
const char *COM_Argv(size_t arg)
{
	if (arg >= com_argc || (signed)arg < 0)
		return com_null_string;
	return com_argv[arg];
}

                                       
   
                                                                 
                
    
char *COM_Args(void)
{
	return com_args;
}

                                                          
   
                                                            
                                                               
                                                
    
size_t COM_CheckParm(const char *check)
{
	size_t i;

	for (i = 1; i < com_argc; i++)
		if (!strcasecmp(check, com_argv[i]))
			return i;
	return 0;
}

                                                                     
                                                              
    
size_t COM_CheckPartialParm(const char *check)
{
	int  len;
	size_t i;

	len = strlen(check);

	for (i = 1; i < com_argc; i++)
	{
		if (strncasecmp(check, com_argv[i], len) == 0)
			return i;
	}
	return 0;
}

                                                          
                                           
                                           
    
size_t COM_FirstOption(void)
{
	size_t i;

	for (i = 1; i < com_argc; i++)
	{
		if (com_argv[i][0] == '-') 
			return i;
	}
	return 0;
}

                                             
   
                                                              
                                   
    
static void COM_TokenizeString(char *ptext)
{
	size_t i;


	for (i = 0; i < com_argc; i++)
		Z_Free(com_argv[i]);

	com_argc = 0;
	com_args = NULL;

	while (com_argc < MAX_ARGS)
	{

		while (*ptext != '\0' && *ptext <= ' ' && *ptext != '\n')
		{
			if (ptext[0] == '\033')
			{
				com_flags = (unsigned)ptext[1];
				ptext += 2;
			}
			else
				ptext++;
		}



		if (*ptext == '\n' || *ptext == '\0')
			break;

		if (com_argc == 1)
			com_args = ptext;

		ptext = COM_Parse(ptext);
		if (ptext == NULL)
			break;

		com_argv[com_argc] = Z_StrDup(com_token);
		com_argc++;
	}
}

                           
   
                                    
                                                        
    
void COM_AddCommand(const char *name, com_func_t func, com_flags_t flags)
{
	xcommand_t *cmd;


	if (CV_StringValue(name)[0] != '\0')
	{
		I_Error("%s is a variable name\n", name);
		return;
	}


	for (cmd = com_commands; cmd; cmd = cmd->next)
	{
		if (!stricmp(name, cmd->name))  
		{




			if (cmd->function != COM_Lua_f)
				I_Error("Command %s already exists\n", name);

			return;
		}
	}

	cmd = ZZ_Alloc(sizeof *cmd);
	cmd->name = name;
	cmd->function = func;
	cmd->flags = flags;
	cmd->next = com_commands;
	com_commands = cmd;
}

                                   
                                                        
   
                                    
    
int COM_AddLuaCommand(const char *name)
{
	xcommand_t *cmd;


	if (CV_StringValue(name)[0] != '\0')
		return -1;


	for (cmd = com_commands; cmd; cmd = cmd->next)
	{
		if (!stricmp(name, cmd->name))  
		{

			cmd->function = COM_Lua_f;
			return 1;
		}
	}


	cmd = ZZ_Alloc(sizeof *cmd);
	cmd->name = name;
	cmd->function = COM_Lua_f;
	cmd->flags = COM_LUA;
	cmd->next = com_commands;
	com_commands = cmd;
	return 0;
}

                              
   
                                     
                                                       
    
static boolean COM_Exists(const char *com_name)
{
	xcommand_t *cmd;

	for (cmd = com_commands; cmd; cmd = cmd->next)
		if (!stricmp(com_name, cmd->name))
			return true;

	return false;
}

                                            
   
                                                                 
                                              
                                               
                                        
    
const char *COM_CompleteCommand(const char *partial, INT32 skips)
{
	xcommand_t *cmd;
	size_t len;

	len = strlen(partial);

	if (!len)
		return NULL;


	for (cmd = com_commands; cmd; cmd = cmd->next)
		if (!strncmp(partial, cmd->name, len))
			if (!skips--)
				return cmd->name;

	return NULL;
}

                                   
   
                                                               
                                             
                                             
                                          
    
const char *COM_CompleteAlias(const char *partial, INT32 skips)
{
	cmdalias_t *a;
	size_t len;

	len = strlen(partial);

	if (!len)
		return NULL;


	for (a = com_alias; a; a = a->next)
		if (!strncmp(partial, a->name, len))
			if (!skips--)
				return a->name;

	return NULL;
}

                                                                        
                                                                         
   
                                       
    
static void COM_ExecuteString(char *ptext)
{
	xcommand_t *cmd;
	cmdalias_t *a;
	static INT32 recursion = 0;  

	COM_TokenizeString(ptext);


	if (COM_Argc() == 0)
		return;  


	for (cmd = com_commands; cmd; cmd = cmd->next)
	{
		if (!stricmp(com_argv[0], cmd->name))  
		{
			if ((com_flags & COM_LUA) && !(cmd->flags & COM_LUA))
			{
				CONS_Alert(CONS_WARNING, "Command '%s' cannot be run from a script.\n", cmd->name);
				return;
			}

			cmd->function();
			return;
		}
	}


	for (a = com_alias; a; a = a->next)
	{
		if (!stricmp(com_argv[0], a->name))
		{
			if (recursion > MAX_ALIAS_RECURSION)
				CONS_Alert(CONS_WARNING, M_GetText("Alias recursion cycle detected!\n"));
			else
			{  
				recursion++;
				COM_BufInsertTextEx(a->value, com_flags);
				recursion--;
			}
			return;
		}
	}




	if (!CV_Command() && (con_destlines || dedicated))
		CONS_Printf(M_GetText("Unknown command '%s'\n"), COM_Argv(0));
}





static void print_alias(void)
{
	cmdalias_t *a;

	CONS_Printf("\x82""Current alias commands:\n");
	for (a = com_alias; a; a = a->next)
	{
		CONS_Printf("%s : %s", a->name, a->value);
	}
}

static void add_alias(char *newname, char *newcmd)
{
	cmdalias_t *a;


	for (a = com_alias; a; a = a->next)
	{
		if (!stricmp(newname, a->name))
		{
			Z_Free(a->value);  
			a->value = newcmd;
			return;
		}
	}


	a = ZZ_Alloc(sizeof *a);
	a->next = com_alias;
	com_alias = a;

	a->name = newname;
	a->value = newcmd;
}

                                                         
    
static void COM_Alias_f(void)
{
	char *name;
	char *zcmd;
	char cmd[1024];
	size_t i, c;

	if (COM_Argc() < 3)
	{
		CONS_Printf(M_GetText("alias <name> <command>: create a shortcut command that executes other command(s)\n"));
		print_alias();
		return;
	}

	name = Z_StrDup(COM_Argv(1));


	cmd[0] = 0;  
	c = COM_Argc();
	for (i = 2; i < c; i++)
	{
		strcat(cmd, COM_Argv(i));
		if (i != c)
			strcat(cmd, " ");
	}
	strcat(cmd, "\n");
	zcmd = Z_StrDup(cmd);
	add_alias(name, zcmd);
}

                                         
    
static void COM_Echo_f(void)
{
	size_t i;

	for (i = 1; i < COM_Argc(); i++)
		CONS_Printf("%s ", COM_Argv(i));
	CONS_Printf("\n");
}

                                                               
    
static void COM_CEcho_f(void)
{
	size_t i;
	char cechotext[1024] = "";

	for (i = 1; i < COM_Argc(); i++)
	{
		strncat(cechotext, COM_Argv(i), sizeof(cechotext)-1);
		strncat(cechotext, " ", sizeof(cechotext)-1);
	}

	cechotext[sizeof(cechotext) - 1] = '\0';

	HU_DoCEcho(cechotext);
}

                                             
    
static void COM_CEchoFlags_f(void)
{
	if (COM_Argc() > 1)
	{
		const char *arg = COM_Argv(1);

		if (arg[0] && arg[0] == '0' &&
			arg[1] && arg[1] == 'x')  
			HU_SetCEchoFlags(axtoi(arg+2));
		else
			HU_SetCEchoFlags(atoi(arg));
	}
	else
		CONS_Printf(M_GetText("cechoflags <flags>: set CEcho flags, prepend with 0x to use hexadecimal\n"));
}

                                                              
    
static void COM_CEchoDuration_f(void)
{
	if (COM_Argc() > 1)
		HU_SetCEchoDuration(atoi(COM_Argv(1)));
}

                           
    
boolean COM_ExecFile(const char *scriptname, com_flags_t flags, boolean silent)
{
	UINT8 *buf = NULL;
	char filename[256];

	if (!D_CheckPathAllowed(scriptname, "tried to exec"))
		return false;



	FIL_ReadFile(scriptname, &buf);

	if (!buf)
	{


		strlcpy(filename, scriptname, sizeof(filename));
		if (findfile(filename, NULL, true) != FS_NOTFOUND)
			FIL_ReadFile(filename, &buf);

		if (!buf)
			return false;
	}

	if (!silent)
		CONS_Printf(M_GetText("Executing %s\n"), scriptname);


	COM_BufAddTextEx((char *)buf, flags);
	COM_BufAddTextEx("\n", flags);


	Z_Free(buf);

	return true;
}

static void COM_Exec_f(void)
{
	boolean silent;

	if (COM_Argc() < 2 || COM_Argc() > 3)
	{
		CONS_Printf(M_GetText("exec <filename>: run a script file\n"));
		return;
	}

	silent = COM_CheckParm("-silent");

	if (COM_ExecFile(COM_Argv(1), com_flags, silent))
		return;

	if (!COM_CheckParm("-noerror"))
		CONS_Printf(M_GetText("Couldn't execute file %s\n"), COM_Argv(1));
}

                                                                      
                                                             
    
static void COM_Wait_f(void)
{
	if (COM_Argc() > 1)
		com_wait = atoi(COM_Argv(1));
	else
		com_wait = 1;  
}

                                          
    
static void COM_Help_f(void)
{
	xcommand_t *cmd;
	consvar_t *cvar;
	INT32 i = 0;

	if (COM_Argc() > 1)
	{
		const char *help = COM_Argv(1);
		cvar = CV_FindVar(help);
		if (cvar)
		{
			boolean floatmode = false;
			const char *cvalue = NULL;
			CONS_Printf("\x82""Variable %s:\n", cvar->name);
			CONS_Printf(M_GetText("  flags: "));
			if (cvar->flags & CV_SAVE)
				CONS_Printf("AUTOSAVE ");
			if (cvar->flags & CV_FLOAT)
			{
				CONS_Printf("FLOAT ");
				floatmode = true;
			}
			if (cvar->flags & CV_NETVAR)
				CONS_Printf("NETVAR ");
			if (cvar->flags & CV_CALL)
				CONS_Printf("ACTION ");
			if (cvar->flags & CV_CHEAT)
				CONS_Printf("CHEAT ");
			CONS_Printf("\n");
			if (cvar->PossibleValue)
			{
				CONS_Printf(" Possible values:\n");
				if (cvar->PossibleValue == CV_YesNo)
					CONS_Printf("  Yes or No (On or Off, True or False, 1 or 0)\n");
				else if (cvar->PossibleValue == CV_OnOff)
					CONS_Printf("  On or Off (Yes or No, True or False, 1 or 0)\n");
				else if (cvar->PossibleValue == CV_TrueFalse)
					CONS_Printf("  True or False (On or Off, Yes or No, 1 or 0)\n");
				else if (cvar->PossibleValue == Color_cons_t)
				{
					for (i = 1; i < numskincolors; ++i)
					{
						if (skincolors[i].accessible)
						{
							CONS_Printf("  %-2d : %s\n", i, skincolors[i].name);
							if (i == cvar->value)
								cvalue = skincolors[i].name;
						}
					}
				}
				else
				{
#define MINVAL 0
#define MAXVAL 1
					if (!stricmp(cvar->PossibleValue[MINVAL].strvalue, "MIN"))
					{
						if (floatmode)
						{
							float fu = FIXED_TO_FLOAT(cvar->PossibleValue[MINVAL].value);
							float ck = FIXED_TO_FLOAT(cvar->PossibleValue[MAXVAL].value);
							CONS_Printf("  range from %ld%s to %ld%s\n",
									(long)fu, M_Ftrim(fu),
									(long)ck, M_Ftrim(ck));
						}
						else
							CONS_Printf("  range from %d to %d\n", cvar->PossibleValue[MINVAL].value,
								cvar->PossibleValue[MAXVAL].value);
						i = MAXVAL+1;
					}
#undef MINVAL
#undef MAXVAL


					while (cvar->PossibleValue[i].strvalue)
					{
						if (floatmode)
							CONS_Printf("  %-2f : %s\n", FIXED_TO_FLOAT(cvar->PossibleValue[i].value),
								cvar->PossibleValue[i].strvalue);
						else
							CONS_Printf("  %-2d : %s\n", cvar->PossibleValue[i].value,
								cvar->PossibleValue[i].strvalue);
						if (cvar->PossibleValue[i].value == cvar->value)
							cvalue = cvar->PossibleValue[i].strvalue;
						i++;
					}
				}
			}

			if (cvalue)
				CONS_Printf(" Current value: %s\n", cvalue);
			else if (cvar->string)
				CONS_Printf(" Current value: %s\n", cvar->string);
			else
				CONS_Printf(" Current value: %d\n", cvar->value);

			if (cvar->revert.v.string != NULL && strcmp(cvar->revert.v.string, cvar->string) != 0)
				CONS_Printf(" Value before netgame: %s\n", cvar->revert.v.string);
		}
		else
		{
			for (cmd = com_commands; cmd; cmd = cmd->next)
			{
				if (strcmp(cmd->name, help))
					continue;

				CONS_Printf("\x82""Command %s:\n", cmd->name);
				CONS_Printf("  help is not available for commands");
				CONS_Printf("\x82""\nCheck wiki.srb2.org for more or try typing <name> without arguments\n");
				return;
			}

			CONS_Printf("No variable or command named %s", help);
			CONS_Printf("\x82""\nCheck wiki.srb2.org for more or try typing help without arguments\n");
		}
		return;
	}
	else
	{

		CONS_Printf("\x82""Variables:\n");
		for (cvar = consvar_vars; cvar; cvar = cvar->next)
		{
			if (cvar->flags & CV_NOSHOWHELP)
				continue;
			CONS_Printf("%s ", cvar->name);
			i++;
		}


		CONS_Printf("\x82""\nCommands:\n");
		for (cmd = com_commands; cmd; cmd = cmd->next)
		{
			CONS_Printf("%s ",cmd->name);
			i++;
		}

		CONS_Printf("\x82""\nCheck wiki.srb2.org for more or type help <command or variable>\n");

		CONS_Debug(DBG_GAMELOGIC, "\x82Total : %d\n", i);
	}
}

static void COM_Find_f(void)
{
	static char prefix[80];
	xcommand_t *cmd;
	consvar_t *cvar;
	cmdalias_t *alias;
	const char *match;
	const char *help;
	size_t helplen;
	boolean matchesany;

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("find <text>: Search for variables, commands and aliases containing <text>\n"));
		return;
	}

	help = COM_Argv(1);
	helplen = strlen(help);
	CONS_Printf("\x82""Variables:\n");
	matchesany = false;
	for (cvar = consvar_vars; cvar; cvar = cvar->next)
	{
		if (cvar->flags & CV_NOSHOWHELP)
			continue;
		match = strstr(cvar->name, help);
		if (match != NULL)
		{
			memcpy(prefix, cvar->name, match - cvar->name);
			prefix[match - cvar->name] = '\0';
			CONS_Printf("  %s\x83%s\x80%s\n", prefix, help, &match[helplen]);
			matchesany = true;
		}
	}
	if (!matchesany)
		CONS_Printf("  (none)\n");

	CONS_Printf("\x82""Commands:\n");
	matchesany = false;
	for (cmd = com_commands; cmd; cmd = cmd->next)
	{
		match = strstr(cmd->name, help);
		if (match != NULL)
		{
			memcpy(prefix, cmd->name, match - cmd->name);
			prefix[match - cmd->name] = '\0';
			CONS_Printf("  %s\x83%s\x80%s\n", prefix, help, &match[helplen]);
			matchesany = true;
		}
	}
	if (!matchesany)
		CONS_Printf("  (none)\n");

	CONS_Printf("\x82""Aliases:\n");
	matchesany = false;
	for (alias = com_alias; alias; alias = alias->next)
	{
		match = strstr(alias->name, help);
		if (match != NULL)
		{
			memcpy(prefix, alias->name, match - alias->name);
			prefix[match - alias->name] = '\0';
			CONS_Printf("  %s\x83%s\x80%s\n", prefix, help, &match[helplen]);
			matchesany = true;
		}
	}
	if (!matchesany)
		CONS_Printf("  (none)\n");
}

                                                         
   
                                            
    
static void COM_Toggle_f(void)
{
	consvar_t *cvar;

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("Toggle <cvar_name>: Toggle the value of a cvar\n"));
		return;
	}
	cvar = CV_FindVar(COM_Argv(1));
	if (!cvar)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("%s is not a cvar\n"), COM_Argv(1));
		return;
	}

	if (CV_Immutable(cvar))
		return;

	if (!(cvar->PossibleValue == CV_YesNo || cvar->PossibleValue == CV_OnOff || cvar->PossibleValue == CV_TrueFalse))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("%s is not a boolean value\n"), COM_Argv(1));
		return;
	}


	cvar->flags |= CV_SHOWMODIFONETIME;
	CV_AddValue(cvar, +1);
}

                                  
    
static void COM_Add_f(void)
{
	consvar_t *cvar;

	if (COM_Argc() != 3)
	{
		CONS_Printf(M_GetText("Add <cvar_name> <value>: Add to the value of a cvar. Negative values work too!\n"));
		return;
	}
	cvar = CV_FindVar(COM_Argv(1));
	if (!cvar)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("%s is not a cvar\n"), COM_Argv(1));
		return;
	}

	if (CV_Immutable(cvar))
		return;

	if (( cvar->flags & CV_FLOAT ))
	{
		float n =FIXED_TO_FLOAT (cvar->value) + atof(COM_Argv(2));
		CV_Set(cvar, va("%ld%s", (long)n, M_Ftrim(n)));
	}
	else
		CV_AddValue(cvar, atoi(COM_Argv(2)));
}

static void COM_Sudo_f(void)
{
	boolean mp = multiplayer;
	boolean ng = netgame;
	boolean dp = devparm;
	xcommand_t *cmd;
	boolean found_cmd = false;
	boolean cmd_is_devmode;
	size_t i;

	if (COM_Argc() < 2) {
		CONS_Printf("Usage: sudo <command>\n");
		return;
	}

	cmd_is_devmode = !stricmp(com_argv[0], "devmode");

	com_sudo = true;
	multiplayer = netgame = false;
	if (!cmd_is_devmode)
		devparm = true;

	--com_argc;
	for (i=0; i<com_argc; ++i)
		com_argv[i] = com_argv[i+1];

	for (cmd = com_commands; cmd; cmd = cmd->next) {
		if (!stricmp(com_argv[0], cmd->name)) {
			cmd->function();
			found_cmd = true;
			break;
		}
	}
	if (!found_cmd && !CV_Command() && con_destlines)
		CONS_Printf(M_GetText("Unknown command '%s'\n"), COM_Argv(0));
	
	com_sudo = false;
	multiplayer = mp;
	netgame = ng;
	if (!cmd_is_devmode)
		devparm = dp;
}






                                       
   
                                         
                                                
    
void VS_Alloc(vsbuf_t *buf, size_t initsize)
{
#define VSBUFMINSIZE 256
	if (initsize < VSBUFMINSIZE)
		initsize = VSBUFMINSIZE;
	buf->data = Z_Malloc(initsize, PU_STATIC, NULL);
	buf->maxsize = initsize;
	buf->cursize = 0;
#undef VSBUFMINSIZE
}

                                 
   
                              
    
void VS_Free(vsbuf_t *buf)
{
	buf->cursize = 0;
}

                                  
   
                               
    
void VS_Clear(vsbuf_t *buf)
{
	buf->cursize = 0;
}

                                                                    
                   
   
                                                          
                                                    
                                                 
    
void *VS_GetSpace(vsbuf_t *buf, size_t length)
{
	void *data;

	if (buf->cursize + length > buf->maxsize)
	{
		if (!buf->allowoverflow)
			I_Error("overflow 111");

		if (length > buf->maxsize)
			I_Error("overflow l%s 112", sizeu1(length));

		buf->overflowed = true;
		CONS_Printf("VS buffer overflow");
		VS_Clear(buf);
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;

	return data;
}

                                                     
   
                             
                                   
                                         
                
    
void VS_Write(vsbuf_t *buf, const void *data, size_t length)
{
	M_Memcpy(VS_GetSpace(buf, length), data, length);
}

void VS_WriteEx(vsbuf_t *buf, const void *data, size_t length, com_flags_t flags)
{
	char *p;
	p = VS_GetSpace(buf, 2 + length);
	p[0] = '\033';
	p[1] = flags;
	M_Memcpy(&p[2], data, length);
}

                                                            
                 
   
                           
                                          
                
    
void VS_Print(vsbuf_t *buf, const char *data)
{
	size_t len;

	len = strlen(data) + 1;

	if (buf->data[buf->cursize-1])
		M_Memcpy((UINT8 *)VS_GetSpace(buf, len), data, len);  
	else
		M_Memcpy((UINT8 *)VS_GetSpace(buf, len-1) - 1, data, len);  
}













static const char *cv_null_string = "";

                                               
   
                                       
                                                      
                     
    
consvar_t *CV_FindVar(const char *name)
{
	consvar_t *cvar;

	for (cvar = consvar_vars; cvar; cvar = cvar->next)
		if (!stricmp(name,cvar->name))
			return cvar;

	return NULL;
}

#ifdef OLD22DEMOCOMPAT
                                                                  
                                                          
   
                                                                         
   
                                  
                                    
    
static inline UINT16 CV_ComputeOldDemoID(const char *s)
{
	UINT16 ret = 0, i = 0;
	static UINT16 premiers[16] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53};

	while (*s)
	{
		ret = (UINT16)(ret + (*s)*premiers[i]);
		s++;
		i = (UINT16)((i+1) % 16);
	}
	return ret;
}

                                                                           
                                                      
   
                                             
                                                               
    
static old_demo_var_t *CV_FindOldDemoVar(UINT16 chk)
{
	old_demo_var_t *demovar;

	for (demovar = consvar_old_demo_vars; demovar; demovar = demovar->next)
	{
		if (demovar->checksum == chk)
		{
			if (demovar->collides)
			{
				CONS_Alert(CONS_WARNING,
						"Old demo netvar id %hu is a collision\n", chk);
				return NULL;
			}

			return demovar;
		}
	}

	return NULL;
}
#endif 

                                                        
   
                                                  
                                                               
    
static consvar_t *CV_FindNetVar(UINT16 netid)
{
	consvar_t *cvar;

	if (netid > consvar_number_of_netids)
		return NULL;

	for (cvar = consvar_vars; cvar; cvar = cvar->next)
		if (cvar->netid == netid)
			return cvar;

	return NULL;
}

static void Setvalue(consvar_t *var, const char *valstr, boolean stealth);

#ifdef OLD22DEMOCOMPAT
 
static void CV_RegisterOldDemoVar(consvar_t *variable)
{
	old_demo_var_t *demovar;
	UINT16 old_demo_id;

	old_demo_id = CV_ComputeOldDemoID(variable->name);

	demovar = CV_FindOldDemoVar(old_demo_id);

	if (demovar)
		demovar->collides = true;
	else
	{
		demovar = ZZ_Calloc(sizeof *demovar);

		demovar->checksum = old_demo_id;
		demovar->cvar = variable;

		demovar->next = consvar_old_demo_vars;
		consvar_old_demo_vars = demovar;
	}
}
#endif

                                                        
   
                                             
    
void CV_RegisterVar(consvar_t *variable)
{

	if (CV_FindVar(variable->name))
	{
		CONS_Printf(M_GetText("Variable %s is already defined\n"), variable->name);
		return;
	}


	if (COM_Exists(variable->name))
	{
		CONS_Printf(M_GetText("%s is a command name\n"), variable->name);
		return;
	}


	if (variable->flags & CV_NETVAR)
	{
		 
		if (consvar_number_of_netids == UINT16_MAX)
			I_Error("Way too many netvars");

		variable->netid = ++consvar_number_of_netids;

#ifdef OLD22DEMOCOMPAT
		CV_RegisterOldDemoVar(variable);
#endif
	}


	if (!(variable->flags & CV_HIDEN))
	{
		variable->next = consvar_vars;
		consvar_vars = variable;
	}
	variable->string = variable->zstring = NULL;
	memset(&variable->revert, 0, sizeof variable->revert);
	variable->changed = 0;  

#ifdef PARANOIA
	if ((variable->flags & CV_NOINIT) && !(variable->flags & CV_CALL))
		I_Error("variable %s has CV_NOINIT without CV_CALL\n", variable->name);
	if ((variable->flags & CV_CALL) && !(variable->func || variable->can_change))
		I_Error("variable %s has CV_CALL without any callbacks\n", variable->name);
#endif

	if (variable->flags & CV_NOINIT)
		variable->flags &= ~CV_CALL;

	Setvalue(variable, variable->defaultvalue, false);

	if (variable->flags & CV_NOINIT)
		variable->flags |= CV_CALL;


	variable->flags &= ~CV_MODIFIED;
}

                                                 
   
                                        
                                                                
    
static const char *CV_StringValue(const char *var_name)
{
	consvar_t *var;

	var = CV_FindVar(var_name);
	if (!var)
		return cv_null_string;
	return var->string;
}

                                             
   
                                                                  
                                               
                                                
                                             
    
const char *CV_CompleteVar(char *partial, INT32 skips)
{
	consvar_t *cvar;
	size_t len;

	len = strlen(partial);

	if (!len)
		return NULL;


	for (cvar = consvar_vars; cvar; cvar = cvar->next)
		if (!strncmp(partial, cvar->name, len))
			if (!skips--)
				return cvar->name;

	return NULL;
}

                                                                         
   
                                  
                                                
    
static void Setvalue(consvar_t *var, const char *valstr, boolean stealth)
{
	boolean override = false;
	INT32 overrideval = 0;


	LUA_CVarChanged(var);  
	if (var->flags & CV_CALL && var->can_change && !stealth)
	{
		if (!var->can_change(valstr))
		{


			if (var->string == NULL)
			{
				if (var->PossibleValue)
				{

					valstr = var->PossibleValue[0].strvalue;
				}
				else
				{

					valstr = "";
				}
			}
			else
			{


				return;
			}
		}
	}

	if (var->PossibleValue)
	{
		INT32 v;

		if (var->flags & CV_FLOAT)
		{
			double d = atof(valstr);
			if (fpclassify(d) == FP_ZERO && valstr[0] != '0')
				v = INT32_MIN;
			else
				v = (INT32)(d * FRACUNIT);
		}
		else
		{
			v = atoi(valstr);
			if (!v && valstr[0] != '0')
				v = INT32_MIN;  
		}

		if (var->PossibleValue[0].strvalue && !stricmp(var->PossibleValue[0].strvalue, "MIN"))  
		{
#define MINVAL 0
#define MAXVAL 1
			INT32 i;
#ifdef PARANOIA
			if (!var->PossibleValue[MAXVAL].strvalue)
				I_Error("Bounded cvar \"%s\" without maximum!\n", var->name);
#endif


			for (i = MAXVAL+1; var->PossibleValue[i].strvalue; i++)
				if (v == var->PossibleValue[i].value || !stricmp(var->PossibleValue[i].strvalue, valstr))
				{
					if (client && execversion_enabled)
					{
						if (var->revert.allocated)
						{
							Z_Free(var->revert.v.string);
							var->revert.allocated = false;  
						}

						var->revert.v.const_munge = var->PossibleValue[i].strvalue;

						return;
					}


					Z_Free(var->zstring);
					var->zstring = NULL;

					var->value = var->PossibleValue[i].value;
					var->string = var->PossibleValue[i].strvalue;
					goto finish;
				}


			if ((v != INT32_MIN && v < var->PossibleValue[MINVAL].value) || !stricmp(valstr, "MIN"))
			{
				v = var->PossibleValue[MINVAL].value;
				valstr = var->PossibleValue[MINVAL].strvalue;
				override = true;
				overrideval = v;
			}
			else if ((v != INT32_MIN && v > var->PossibleValue[MAXVAL].value) || !stricmp(valstr, "MAX"))
			{
				v = var->PossibleValue[MAXVAL].value;
				valstr = var->PossibleValue[MAXVAL].strvalue;
				override = true;
				overrideval = v;
			}
			if (v == INT32_MIN)
				goto badinput;
#undef MINVAL
#undef MAXVAL
		}
		else
		{
			INT32 i;


			for (i = 0; var->PossibleValue[i].strvalue; i++)
				if (!stricmp(var->PossibleValue[i].strvalue, valstr))
					goto found;
			if (v != INT32_MIN)
			{

				for (i = 0; var->PossibleValue[i].strvalue; i++)
					if (v == var->PossibleValue[i].value)
						goto found;
			}

			if (var->PossibleValue == CV_OnOff || var->PossibleValue == CV_YesNo || var->PossibleValue == CV_TrueFalse)
			{
				overrideval = -1;
				if (!stricmp(valstr, "on") || !stricmp(valstr, "yes") || !stricmp(valstr, "true"))
					overrideval = 1;
				else if (!stricmp(valstr, "off") || !stricmp(valstr, "no") || !stricmp(valstr, "false"))
					overrideval = 0;

				if (overrideval != -1)
				{
					for (i = 0; var->PossibleValue[i].strvalue; i++)
						if (overrideval == var->PossibleValue[i].value)
							goto found;
				}
			}


			goto badinput;
found:
			if (client && execversion_enabled)
			{
				var->revert.v.const_munge = var->PossibleValue[i].strvalue;
				return;
			}

			var->value = var->PossibleValue[i].value;
			var->string = var->PossibleValue[i].strvalue;
			goto finish;
		}
	}

	if (client && execversion_enabled)
	{
		if (var->revert.allocated)
		{
			Z_Free(var->revert.v.string);

		}

		var->revert.v.string = Z_StrDup(valstr);

		return;
	}


	Z_Free(var->zstring);

	var->string = var->zstring = Z_StrDup(valstr);

	if (override)
		var->value = overrideval;
	else if (var->flags & CV_FLOAT)
	{
		double d = atof(var->string);
		var->value = (INT32)(d * FRACUNIT);
	}
	else
	{
		if (var == &cv_forceskin)
		{
			var->value = R_SkinAvailable(var->string);
			if (!R_SkinUsable(-1, var->value))
				var->value = -1;
		}
		else
			var->value = atoi(var->string);
	}

finish:

	if (var->flags & CV_SHOWMODIFONETIME || var->flags & CV_SHOWMODIF)
	{
		CONS_Printf(M_GetText("%s set to %s\n"), var->name, var->string);
		var->flags &= ~CV_SHOWMODIFONETIME;
	}
	else  
	{
		DEBFILE(va("%s set to %s\n", var->name, var->string));
	}
	var->flags |= CV_MODIFIED;

	if (var->flags & CV_CALL && var->func && !stealth)
		var->func();

	return;


badinput:

	if (var != &cv_nextmap)  
		CONS_Printf(M_GetText("\"%s\" is not a possible value for \"%s\"\n"), valstr, var->name);


	if (var->defaultvalue == valstr)
		I_Error("Variable %s default value \"%s\" is not a possible value\n", var->name, var->defaultvalue);
}







static boolean serverloading = false;

static consvar_t *ReadNetVar(save_t *p, char **return_value, boolean *return_stealth)
{
	UINT16  netid;
	char   *val;
	boolean stealth;

	consvar_t *cvar;

	netid   = P_ReadUINT16(p);
	val     = (char *)&p->buf[p->pos];
	P_SkipString(p);
	stealth = P_ReadUINT8(p);

	cvar = CV_FindNetVar(netid);

	if (cvar)
	{
		(*return_value)   = val;
		(*return_stealth) = stealth;

		DEBFILE(va("Netvar received: %s [netid=%d] value %s\n", cvar->name, netid, val));
	}
	else
		CONS_Alert(CONS_WARNING, "Netvar not found with netid %hu\n", netid);

	return cvar;
}

#ifdef OLD22DEMOCOMPAT
static consvar_t *ReadOldDemoVar(save_t *p, char **return_value, boolean *return_stealth)
{
	UINT16  id;
	char   *val;
	boolean stealth;

	old_demo_var_t *demovar;

	id      = P_ReadUINT16(p);
	val     = (char *)&p->buf[p->pos];
	P_SkipString(p);
	stealth = P_ReadUINT8(p);

	demovar = CV_FindOldDemoVar(id);

	if (demovar)
	{
		(*return_value)   = val;
		(*return_stealth) = stealth;

		return demovar->cvar;
	}
	else
	{
		CONS_Alert(CONS_WARNING, "Netvar not found with old demo id %hu\n", id);
		return NULL;
	}
}
#endif 

static consvar_t *ReadDemoVar(save_t *p, char **return_value, boolean *return_stealth)
{
	char   *name;
	char   *val;
	boolean stealth;

	consvar_t *cvar;

	name    = (char *)&p->buf[p->pos];
	P_SkipString(p);
	val     = (char *)&p->buf[p->pos];
	P_SkipString(p);
	stealth = P_ReadUINT8(p);

	cvar = CV_FindVar(name);

	if (cvar)
	{
		(*return_value)   = val;
		(*return_stealth) = stealth;
	}
	else
		CONS_Alert(CONS_WARNING, "Netvar not found with name %s\n", name);

	return cvar;
}

static void Got_NetVar(UINT8 **p, INT32 playernum)
{
	consvar_t *cvar;
	char *svalue;
	boolean stealth;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum) && !serverloading)
	{

		CONS_Alert(CONS_WARNING, M_GetText("Illegal netvar command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	save_t save_p;
	save_p.buf = *p;
	save_p.size = MAXTEXTCMD;
	save_p.pos = 0;
	cvar = ReadNetVar(&save_p, &svalue, &stealth);
	*p = &save_p.buf[save_p.pos];

	if (cvar)
	{
		if (cv_commandwatch_originals.value && netgame && !demoplayback
		&& !serverloading && playernum != consoleplayer)
		{
			const char *actor = player_names[playernum][0] ? player_names[playernum]
				: ((playernum == serverplayer) ? "SERVER" : va("Player %d", playernum + 1));
			CONS_Printf("\x82%s: %s %s\x80\n", actor, cvar->name, svalue);
		}
		Setvalue(cvar, svalue, stealth);
	}
}

void CV_SaveVars(save_t *p, boolean in_demo)
{
	consvar_t *cvar;
	UINT8 *count_p = &p->buf[p->pos];
	UINT16 count = 0;



	P_WriteUINT16(p, 0x0000);
	for (cvar = consvar_vars; cvar; cvar = cvar->next)
		if ((cvar->flags & CV_NETVAR) && !CV_IsSetToDefault(cvar))
		{
			if (in_demo)
			{
				P_WriteString(p, cvar->name);
			}
			else
			{
				P_WriteUINT16(p, cvar->netid);
			}
			P_WriteString(p, cvar->string);
			P_WriteUINT8(p, false);
			++count;
		}
	WRITEUINT16(count_p, count);
}

static void CV_LoadVars(save_t *p,
		consvar_t *(*got)(save_t *p, char **ret_value, boolean *ret_stealth))
{
	const boolean store = (client || demoplayback);

	consvar_t *cvar;
	UINT16 count;

	char *val;
	boolean stealth;


	serverloading = true;

	for (cvar = consvar_vars; cvar; cvar = cvar->next)
	{
		if (cvar->flags & CV_NETVAR)
		{
			if (store && cvar->revert.v.string == NULL)
			{
				cvar->revert.v.const_munge = cvar->string;
				cvar->revert.allocated = ( cvar->zstring != NULL );
				cvar->zstring = NULL; 
			}

			Setvalue(cvar, cvar->defaultvalue, true);
		}
	}

	count = P_ReadUINT16(p);
	while (count--)
	{
		cvar = (*got)(p, &val, &stealth);

		if (cvar)
			Setvalue(cvar, val, stealth);
	}

	serverloading = false;
}

void CV_RevertNetVars(void)
{
	consvar_t * cvar;

	for (cvar = consvar_vars; cvar; cvar = cvar->next)
	{
		if (cvar->revert.v.string != NULL)
		{
			Setvalue(cvar, cvar->revert.v.string, false);

			if (cvar->revert.allocated)
			{
				Z_Free(cvar->revert.v.string);
				cvar->revert.allocated = false;  
			}

			cvar->revert.v.string = NULL;
		}
	}
}

void CV_LoadNetVars(save_t *p)
{
	CV_LoadVars(p, ReadNetVar);
}

#ifdef OLD22DEMOCOMPAT
void CV_LoadOldDemoVars(save_t *p)
{
	CV_LoadVars(p, ReadOldDemoVar);
}
#endif

void CV_LoadDemoVars(save_t *p)
{
	CV_LoadVars(p, ReadDemoVar);
}

static void CV_SetCVar(consvar_t *var, const char *value, boolean stealth);

void CV_ResetCheatNetVars(void)
{
	consvar_t *cvar;


	for (cvar = consvar_vars; cvar; cvar = cvar->next)
		if (cvar->flags & CV_CHEAT)
			CV_SetCVar(cvar, cvar->defaultvalue, true);
}


boolean CV_IsSetToDefault(consvar_t *v)
{
	return (!(strcmp(v->defaultvalue, v->string)));
}





UINT8 CV_CheatsEnabled(void)
{
	consvar_t *cvar;

	for (cvar = consvar_vars; cvar; cvar = cvar->next)
		if ((cvar->flags & CV_CHEAT) && strcmp(cvar->defaultvalue, cvar->string))
			return 1;
	return 0;
}

                                                                      
                                      
                                                             
   
                              
                                  
                                  
    
static void CV_SetCVar(consvar_t *var, const char *value, boolean stealth)
{
#ifdef PARANOIA
	if (!var)
		I_Error("CV_Set: no variable\n");
	if (!var->string)
		I_Error("CV_Set: %s no string set!\n", var->name);
#endif
	if (!var || !var->string || !value || (var->can_change == NULL && !stricmp(var->string, value)))
		return;  

	if (var->flags & CV_NETVAR)
	{

		UINT8 buf[512];
		UINT8 *p = buf;


		if (client && execversion_enabled)
		{
			Setvalue(var, value, true);
			return;
		}

		if (!(server || (addedtogame && IsPlayerAdmin(consoleplayer))))
		{
			CONS_Printf(M_GetText("Only the server or admin can change: %s %s\n"), var->name, var->string);
			return;
		}

		if (var == &cv_forceskin)
		{
			INT32 skin = R_SkinAvailable(value);
			if ((stricmp(value, "None")) && ((skin == -1) || !R_SkinUsable(-1, skin)))
			{
				CONS_Printf("Please provide a valid skin name (\"None\" disables).\n");
				return;
			}
		}

		                                                                      
                                                                          
                                                                            
                                                                       
                                                                       
                                                    
		if ((netgame || multiplayer) && (client || serverrunning))
		{
			WRITEUINT16(p, var->netid);
			WRITESTRING(p, value);
			WRITEUINT8(p, stealth);

			SendNetXCmd(XD_NETVAR, buf, p-buf);
		}
		else
			Setvalue(var, value, stealth);
	}
	else
		if ((var->flags & CV_NOTINNET) && netgame)
		{
			CONS_Printf(M_GetText("This variable can't be changed while in netgame: %s %s\n"), var->name, var->string);
			return;
		}
		else
			Setvalue(var, value, stealth);
}

                                                                     
   
                              
                                  
                                  
    
void CV_StealthSet(consvar_t *var, const char *value)
{
	CV_SetCVar(var, value, true);
}

                                                                      
             
   
                              
                                                                         
                                                           
    
static void CV_SetValueMaybeStealth(consvar_t *var, INT32 value, boolean stealth)
{
	char val[SKINNAMESIZE+1] = "None";

	if (var == &cv_forceskin)  
	{
		const char *tmpskin = NULL;
		if (value >= 0 && value < numskins)
			tmpskin = skins[value]->name;
		if (tmpskin)
			memcpy(val, tmpskin, SKINNAMESIZE);
	}
	else
		sprintf(val, "%d", value);

	CV_SetCVar(var, val, stealth);
}

                                                                   
             
   
                              
                                                                         
                                  
    
void CV_StealthSetValue(consvar_t *var, INT32 value)
{
	CV_SetValueMaybeStealth(var, value, true);
}


void CV_Set(consvar_t *var, const char *value)
{
	CV_SetCVar(var, value, false);
}

                                                                  
                                                  
   
                              
                                                                         
                                  
    
void CV_SetValue(consvar_t *var, INT32 value)
{
	CV_SetValueMaybeStealth(var, value, false);
}

                                       
                                                            
                                                                          
                                     
   
                                            
                                                                      
                               
                   
    
void CV_AddValue(consvar_t *var, INT32 increment)
{
	INT32 newvalue, max;

	if (!increment)
		return;


	if (var == &cv_pointlimit && (gametype == GT_MATCH))
		increment *= 50;

	if (var == &cv_forceskin)  
	{
		INT32 oldvalue = var->value;
		newvalue = oldvalue;
		do
		{
			newvalue += increment;
			if (newvalue < -1)
				newvalue = (numskins - 1);
			else if (newvalue >= numskins)
				newvalue = -1;
		} while ((oldvalue != newvalue)
				&& !(R_SkinUsable(-1, newvalue)));
	}
	else
		newvalue = var->value + increment;

	if (var->PossibleValue)
	{
		if (var == &cv_nextmap)
		{

			INT32 oldvalue = var->value - 1, gt;
			gt = cv_newgametype.value;
			{
				newvalue = var->value - 1;
				do
				{
					if(increment > 0)  
					{
						newvalue++;
						if (newvalue == NUMMAPS)
							newvalue = 0;
					}
					else  
					{
						newvalue--;
						if (newvalue == -1)
							newvalue = NUMMAPS-1;
					}

					if (newvalue == oldvalue)
						gt = -1;  

					if(!mapheaderinfo[newvalue])
						continue;  

				} while (newvalue != oldvalue && !M_CanShowLevelInList(newvalue, gt));

				var->value = newvalue + 1;
				var->func();
				return;
			}
		}
#define MINVAL 0
#define MAXVAL 1
		else if (var->PossibleValue[MINVAL].strvalue && !strcmp(var->PossibleValue[MINVAL].strvalue, "MIN"))
		{
#ifdef PARANOIA
			if (!var->PossibleValue[MAXVAL].strvalue)
				I_Error("Bounded cvar \"%s\" without maximum!\n", var->name);
#endif

			if (newvalue < var->PossibleValue[MINVAL].value || newvalue > var->PossibleValue[MAXVAL].value)
			{
				INT32 currentindice = -1, newindice;
				for (max = MAXVAL+1; var->PossibleValue[max].strvalue; max++)
				{
					if (var->PossibleValue[max].value == newvalue)
					{
						increment = 0;
						currentindice = max;
						break;  
					}
					else if (var->PossibleValue[max].value == var->value)
						currentindice = max;  
				}

				if (increment)
				{
					increment = (increment > 0) ? 1 : -1;
					if (currentindice == -1 && max != MAXVAL+1)
						newindice = ((increment > 0) ? MAXVAL : max) + increment;
					else
						newindice = currentindice + increment;

					if (newindice >= max || newindice <= MAXVAL)
					{
						if (var == &cv_pointlimit && (gametype == GT_MATCH) && increment > 0)
							CV_SetValue(var, 50);
						else
						{
							newvalue = var->PossibleValue[((increment > 0) ? MINVAL : MAXVAL)].value;
							CV_SetValue(var, newvalue);
						}
					}
					else
						CV_Set(var, var->PossibleValue[newindice].strvalue);
				}
				else
					CV_Set(var, var->PossibleValue[currentindice].strvalue);
			}
			else
				CV_SetValue(var, newvalue);
		}
#undef MINVAL
#undef MAXVAL
		else
		{
			INT32 currentindice = -1, newindice;


			for (max = 0; var->PossibleValue[max].strvalue; max++)
				if (var->PossibleValue[max].value == var->value)
					currentindice = max;

			if (var == &cv_chooseskin)
			{

				newvalue = var->value - 1;
				do
				{
					if (increment > 0)  
					{
						newvalue++;
						if (newvalue == MAXSKINS)
							newvalue = 0;
					}
					else if (increment < 0)  
					{
						newvalue--;
						if (newvalue == -1)
							newvalue = MAXSKINS-1;
					}
				} while (var->PossibleValue[newvalue].strvalue == NULL);

				var->value = newvalue + 1;
				var->string = var->PossibleValue[newvalue].strvalue;
				var->func();
				return;
			}
#ifdef PARANOIA
			if (currentindice == -1)
				I_Error("CV_AddValue: current value %d not found in possible value\n",
					var->value);
#endif

			newindice = (currentindice + increment + max) % max;
			CV_Set(var, var->PossibleValue[newindice].strvalue);
		}
	}
	else
		CV_SetValue(var, newvalue);

	var->changed = 1;  
}

void CV_InitFilterVar(void)
{
	joyaxis_default = joyaxis2_default = true;
	joyaxis_count = joyaxis2_count = 0;
}

void CV_ToggleExecVersion(boolean enable)
{
	execversion_enabled = enable;
}

static void CV_EnforceExecVersion(void)
{
	if (!execversion_enabled)
		CV_StealthSetValue(&cv_execversion, EXECVERSION);
}

static boolean CV_FilterJoyAxisVars(consvar_t *v, const char *valstr)
{



	if (joyaxis_default)
	{
		if (!stricmp(v->name, "joyaxis_turn"))
		{
			if (joyaxis_count > 6) return false;

			else if (joyaxis_count == 6) return true;

			if (!stricmp(valstr, "X-Axis")) joyaxis_count++;
			else joyaxis_default = false;
		}
		if (!stricmp(v->name, "joyaxis_move"))
		{
			if (joyaxis_count > 6) return false;
			else if (joyaxis_count == 6) return true;

			if (!stricmp(valstr, "Y-Axis")) joyaxis_count++;
			else joyaxis_default = false;
		}
		if (!stricmp(v->name, "joyaxis_side"))
		{
			if (joyaxis_count > 6) return false;
			else if (joyaxis_count == 6) return true;

			if (!stricmp(valstr, "Z-Axis")) joyaxis_count++;
			else joyaxis_default = false;
		}
		if (!stricmp(v->name, "joyaxis_look"))
		{
			if (joyaxis_count > 6) return false;
			else if (joyaxis_count == 6) return true;

			if (!stricmp(valstr, "None")) joyaxis_count++;
			else joyaxis_default = false;
		}
		if (!stricmp(v->name, "joyaxis_fire")
			|| !stricmp(v->name, "joyaxis_firenormal"))
		{
			if (joyaxis_count > 6) return false;
			else if (joyaxis_count == 6) return true;

			if (!stricmp(valstr, "None")) joyaxis_count++;
			else joyaxis_default = false;
		}

		if (joyaxis_count == 6)
		{
			COM_BufInsertText(va("%s \"%s\"\n", cv_turnaxis.name, cv_turnaxis.defaultvalue));
			COM_BufInsertText(va("%s \"%s\"\n", cv_moveaxis.name, cv_moveaxis.defaultvalue));
			COM_BufInsertText(va("%s \"%s\"\n", cv_sideaxis.name, cv_sideaxis.defaultvalue));
			COM_BufInsertText(va("%s \"%s\"\n", cv_lookaxis.name, cv_lookaxis.defaultvalue));
			COM_BufInsertText(va("%s \"%s\"\n", cv_fireaxis.name, cv_fireaxis.defaultvalue));
			COM_BufInsertText(va("%s \"%s\"\n", cv_firenaxis.name, cv_firenaxis.defaultvalue));
			joyaxis_count++;
			return false;
		}
	}

	if (joyaxis2_default)
	{
		if (!stricmp(v->name, "joyaxis2_turn"))
		{
			if (joyaxis2_count > 6) return false;

			else if (joyaxis2_count == 6) return true;

			if (!stricmp(valstr, "X-Axis")) joyaxis2_count++;
			else joyaxis2_default = false;
		}
		if (!stricmp(v->name, "joyaxis2_move"))
		{
			if (joyaxis2_count > 6) return false;
			else if (joyaxis2_count == 6) return true;

			if (!stricmp(valstr, "Y-Axis")) joyaxis2_count++;
			else joyaxis2_default = false;
		}
		if (!stricmp(v->name, "joyaxis2_side"))
		{
			if (joyaxis2_count > 6) return false;
			else if (joyaxis2_count == 6) return true;

			if (!stricmp(valstr, "Z-Axis")) joyaxis2_count++;
			else joyaxis2_default = false;
		}
		if (!stricmp(v->name, "joyaxis2_look"))
		{
			if (joyaxis2_count > 6) return false;
			else if (joyaxis2_count == 6) return true;

			if (!stricmp(valstr, "None")) joyaxis2_count++;
			else joyaxis2_default = false;
		}
		if (!stricmp(v->name, "joyaxis2_fire")
			|| !stricmp(v->name, "joyaxis2_firenormal"))
		{
			if (joyaxis2_count > 6) return false;
			else if (joyaxis2_count == 6) return true;

			if (!stricmp(valstr, "None")) joyaxis2_count++;
			else joyaxis2_default = false;
		}


		if (joyaxis2_count == 6)
		{
			COM_BufInsertText(va("%s \"%s\"\n", cv_turnaxis2.name, cv_turnaxis2.defaultvalue));
			COM_BufInsertText(va("%s \"%s\"\n", cv_moveaxis2.name, cv_moveaxis2.defaultvalue));
			COM_BufInsertText(va("%s \"%s\"\n", cv_sideaxis2.name, cv_sideaxis2.defaultvalue));
			COM_BufInsertText(va("%s \"%s\"\n", cv_lookaxis2.name, cv_lookaxis2.defaultvalue));
			COM_BufInsertText(va("%s \"%s\"\n", cv_fireaxis2.name, cv_fireaxis2.defaultvalue));
			COM_BufInsertText(va("%s \"%s\"\n", cv_firenaxis2.name, cv_firenaxis2.defaultvalue));
			joyaxis2_count++;
			return false;
		}
	}


	return true;
}

static boolean CV_FilterVarByVersion(consvar_t *v, const char *valstr)
{




	if (!(v->flags & CV_SAVE))
		return true;

	if (GETMAJOREXECVERSION(cv_execversion.value) < 26)  
	{



		if (!stricmp(v->name, "alwaysmlook")
			|| !stricmp(v->name, "alwaysmlook2")
			|| !stricmp(v->name, "mousemove")
			|| !stricmp(v->name, "mousemove2"))
			return false;


		if ((!stricmp(v->name, "mousesens")
			|| !stricmp(v->name, "mousesens2")
			|| !stricmp(v->name, "mouseysens")
			|| !stricmp(v->name, "mouseysens2"))
			&& atoi(valstr) == 35)
			return false;



#if defined(HAVE_SDL) || defined(_WINDOWS)
		if ((!stricmp(v->name, "use_joystick")
			|| !stricmp(v->name, "use_joystick2"))
			&& atoi(valstr) == 0)
			return false;
#endif



		if (!CV_FilterJoyAxisVars(v, valstr))
			return false;
	}
	return true;
}

                                                    
                                                                
                                                                 
   
                                                                   
                                     
                            
    
static boolean CV_Command(void)
{
	consvar_t *v;


	v = CV_FindVar(COM_Argv(0));
	if (!v)
		return false;

	if (CV_Immutable(v))
	{
		CONS_Alert(CONS_WARNING, "Variable '%s' cannot be changed from a script.\n", v->name);
		return true;
	}


	if (COM_Argc() == 1)
	{
		CONS_Printf(M_GetText("\"%s\" is \"%s\" default is \"%s\"\n"), v->name, v->string, v->defaultvalue);
		return true;
	}

	if (!(v->flags & CV_SAVE) || CV_FilterVarByVersion(v, COM_Argv(1)))
	{
		CV_Set(v, COM_Argv(1));
		v->changed = 1;  
	}
	return true;
}

                                                                         
                          
   
                  
                                       
    
void CV_ClearChangedFlags(void)
{
	consvar_t *cvar;

	for (cvar = consvar_vars; cvar; cvar = cvar->next)
		cvar->changed = 0;
}

                                                                
             
   
                             
    
void CV_SaveVariables(FILE *f)
{
	consvar_t *cvar;

	for (cvar = consvar_vars; cvar; cvar = cvar->next)
		if (cvar->flags & CV_SAVE)
		{
			char stringtowrite[MAXTEXTCMD+1];

			const char * string;

			if (cvar->revert.v.string != NULL)
			{
				string = cvar->revert.v.string;
			}
			else
			{
				string = cvar->string;
			}


#define MINVAL 0
#define MAXVAL 1
			if (
					cvar->PossibleValue != NULL &&
					cvar->PossibleValue[0].strvalue &&
					stricmp(cvar->PossibleValue[0].strvalue, "MIN") == 0
			){  
				int which = stricmp(string, "MAX") == 0;

				if (which || stricmp(string, "MIN") == 0)
				{
					INT32 value = cvar->PossibleValue[which].value;

					if (cvar->flags & CV_FLOAT)
						sprintf(stringtowrite, "%f", FIXED_TO_FLOAT(value));
					else
						sprintf(stringtowrite, "%d", value);

					string = stringtowrite;
				}
			}
#undef MINVAL
#undef MAXVAL

			fprintf(f, "%s \"%s\"\n", cvar->name, string);
		}
}



static boolean CV_Immutable(const consvar_t *var)
{

	if (com_flags & COM_LUA)
	{
		if (!(var->flags & CV_ALLOWLUA))
		{
			return true;
		}
	}

	return false;
}





                                                             
   
                                
                                                                     
    
static char *COM_Parse(char *data)
{
	char c;
	size_t len = 0;

	com_token[0] = 0;

	if (data == NULL)
		return NULL;


skipwhite:
	while ((c = *data) <= ' ')
	{
		if (c == '\0')
			return NULL;  
		data++;
	}


	if (c == '/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}


	if (c == '\"')
	{
		data++;
		for (;;)
		{
			c = *data++;
			if (c == '\"' || c == '\0')
			{
				com_token[len] = 0;
				return data;
			}
			if (c == '\033')
				data++;
			else
			{
				com_token[len] = c;
				len++;
			}
		}
	}


	do
	{
		if (c == '\033')
		{
			do
			{
				data += 2;
				c = *data;
			}
			while (c == '\033') ;
		}
		else
		{
			com_token[len] = c;
			data++;
			len++;
			c = *data;
		}
	} while (c > 32);

	com_token[len] = 0;
	return data;
}
