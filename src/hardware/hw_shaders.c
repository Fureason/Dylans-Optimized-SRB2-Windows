// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2021-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifdef HWRENDER

#include "hw_glob.h"
#include "hw_drv.h"
#include "hw_shaders.h"
#include "../z_zone.h"





static struct {
	const char *vertex;
	const char *fragment;
} const gl_shadersources[] = {

	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_FLOOR_FRAGMENT_SHADER},


	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_WALL_FRAGMENT_SHADER},


	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_WALL_FRAGMENT_SHADER},


	{GLSL_MODEL_VERTEX_SHADER, GLSL_MODEL_FRAGMENT_SHADER},


	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_WATER_FRAGMENT_SHADER},


	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_FOG_FRAGMENT_SHADER},


	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_SKY_FRAGMENT_SHADER},


	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_PALETTE_POSTPROCESS_FRAGMENT_SHADER},


	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_UI_COLORMAP_FADE_FRAGMENT_SHADER},


	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_UI_TINTED_WIPE_FRAGMENT_SHADER},

	{NULL, NULL},
};

typedef struct
{
	int base_shader;  
	int custom_shader;  
} shadertarget_t;

typedef struct
{
	char *vertex;
	char *fragment;
	boolean compiled;
} shader_t;  




static shader_t gl_shaders[NUMSHADERTARGETS*2];

static shadertarget_t gl_shadertargets[NUMSHADERTARGETS];

#define WHITESPACE_CHARS " \t"

#define MODEL_LIGHTING_DEFINE "#define SRB2_MODEL_LIGHTING"
#define PALETTE_RENDERING_DEFINE "#define SRB2_PALETTE_RENDERING"



boolean HWR_InitShaders(void)
{
	int i;

	if (!HWD.pfnInitShaders())
		return false;

	for (i = 0; i < NUMSHADERTARGETS; i++)
	{

		gl_shaders[i].vertex = Z_StrDup(gl_shadersources[i].vertex);
		gl_shaders[i].fragment = Z_StrDup(gl_shadersources[i].fragment);

		gl_shadertargets[i].base_shader = i;
		gl_shadertargets[i].custom_shader = -1;
	}

	HWR_CompileShaders();

	return true;
}



static INT32 strstr_int(const char *str1, const char *str2)
{
	char *location = strstr(str1, str2);
	if (location)
		return location - str1;
	else
		return INT32_MAX;
}




static char *HWR_PreprocessShader(char *original)
{
	const char *line_ending = "\n";
	int line_ending_len;
	char *read_pos = original;
	int original_len = strlen(original);
	int distance_to_end = original_len;
	int new_len;
	char *new_shader;
	char *write_pos;
	char shader_glsl_version[3];
	int version_pos = -1;
	int version_len = 0;

	if (strstr(original, "\r\n"))
	{
		line_ending = "\r\n";

		while ((read_pos = strchr(read_pos, '\n')))
		{
			read_pos--;
			if (*read_pos != '\r')
			{




				line_ending = "\n";
				break;
			}
			read_pos += 2;
		}
		read_pos = original;
	}

	line_ending_len = strlen(line_ending);





#define STARTSWITH(str, with_what) !strncmp(str, with_what, sizeof(with_what)-1)
#define ADVANCE(amount) read_pos += (amount); distance_to_end -= (amount);
	while (true)
	{


		int whitespace_len = strspn(read_pos, WHITESPACE_CHARS);
		if (whitespace_len == distance_to_end)
			break;  
		ADVANCE(whitespace_len)

		if (STARTSWITH(read_pos, "#version"))
		{


			int version_number_len;
			version_pos = read_pos - original;
			ADVANCE(sizeof("#version") - 1)
			whitespace_len = strspn(read_pos, WHITESPACE_CHARS);
			if (!whitespace_len)
			{
				CONS_Alert(CONS_ERROR, "HWR_PreprocessShader: Syntax error in #version. Expected space after #version, but got other text.\n");
				return NULL;
			}
			else if (whitespace_len == distance_to_end)
			{
				CONS_Alert(CONS_ERROR, "HWR_PreprocessShader: Syntax error in #version. Expected version number, but got end of file.\n");
				return NULL;
			}
			ADVANCE(whitespace_len)
			version_number_len = strspn(read_pos, "0123456789");
			if (!version_number_len)
			{
				CONS_Alert(CONS_ERROR, "HWR_PreprocessShader: Syntax error in #version. Expected version number, but got other text.\n");
				return NULL;
			}
			else if (version_number_len != 3)
			{
				CONS_Alert(CONS_ERROR, "HWR_PreprocessShader: Syntax error in #version. Expected version with 3 digits, but got %d digits.\n", version_number_len);
				return NULL;
			}
			M_Memcpy(shader_glsl_version, read_pos, 3);
			ADVANCE(version_number_len)
			version_len = (read_pos - original) - version_pos;
			whitespace_len = strspn(read_pos, WHITESPACE_CHARS);
			ADVANCE(whitespace_len)
			if (STARTSWITH(read_pos, "es"))
			{
				CONS_Alert(CONS_ERROR, "HWR_PreprocessShader: Support for ES shaders is not implemented.\n");
				return NULL;
			}
			break;
		}
		else
		{


			INT32 newline_pos = strstr_int(read_pos, line_ending);
			INT32 line_comment_pos;
			INT32 block_comment_pos;


			if (newline_pos != INT32_MAX)
				read_pos[newline_pos] = '\0';
			line_comment_pos = strstr_int(read_pos, "//");
			block_comment_pos = strstr_int(read_pos, "/*");

			if (newline_pos != INT32_MAX)
				read_pos[newline_pos] = line_ending[0];
			if (line_comment_pos < block_comment_pos)
			{

				if (newline_pos != INT32_MAX)
				{
					ADVANCE(newline_pos + line_ending_len)
				}
				else
				{

					break;
				}
			}
			else if (block_comment_pos < line_comment_pos)
			{

				INT32 block_comment_end;
				ADVANCE(block_comment_pos + 2)
				block_comment_end = strstr_int(read_pos, "*/");
				if (block_comment_end == INT32_MAX)
				{


					CONS_Alert(CONS_ERROR, "HWR_PreprocessShader: Encountered unclosed block comment in shader.\n");
					return NULL;
				}
				ADVANCE(block_comment_end + 2)
			}
			else if (newline_pos == INT32_MAX)
			{

				break;
			}
			else
			{

				ADVANCE(newline_pos + line_ending_len)
			}
		}
	}
#undef STARTSWITH
#undef ADVANCE

#define ADD_TO_LEN(def) new_len += sizeof(def) - 1 + line_ending_len;


	new_len = original_len;
	if (cv_glmodellighting.value)
		ADD_TO_LEN(MODEL_LIGHTING_DEFINE)
	if (cv_glpaletterendering.value)
		ADD_TO_LEN(PALETTE_RENDERING_DEFINE)

#undef ADD_TO_LEN

#define VERSION_PART "#version "

	if (new_len != original_len)
	{
		if (version_pos != -1)
			new_len += sizeof(VERSION_PART) - 1 + 3 + line_ending_len;
		new_len += sizeof("#line 0") - 1 + line_ending_len;
	}


	new_shader = Z_Malloc(new_len + 1, PU_STATIC, NULL);

	read_pos = original;
	write_pos = new_shader;

	if (new_len != original_len && version_pos != -1)
	{
		strcpy(write_pos, VERSION_PART);
		write_pos += sizeof(VERSION_PART) - 1;
		M_Memcpy(write_pos, shader_glsl_version, 3);
		write_pos += 3;
		strcpy(write_pos, line_ending);
		write_pos += line_ending_len;
	}

#undef VERSION_PART

#define WRITE_DEFINE(define) \
	{ \
		strcpy(write_pos, define); \
		write_pos += sizeof(define) - 1; \
		strcpy(write_pos, line_ending); \
		write_pos += line_ending_len; \
	}


	if (cv_glmodellighting.value)
		WRITE_DEFINE(MODEL_LIGHTING_DEFINE)
	if (cv_glpaletterendering.value)
		WRITE_DEFINE(PALETTE_RENDERING_DEFINE)

#undef WRITE_DEFINE



	if (new_len != original_len)
	{

		if (version_pos == -1 || shader_glsl_version[0] == '1')
			strcpy(write_pos, "#line 0");
		else
			strcpy(write_pos, "#line 1");
		write_pos += sizeof("#line 0") - 1;
		strcpy(write_pos, line_ending);
		write_pos += line_ending_len;
	}


	M_Memcpy(write_pos, read_pos, original_len);


	if (new_len != original_len && version_pos != -1)
		memset(write_pos + version_pos, ' ', version_len);


	new_shader[new_len] = '\0';

	return new_shader;
}


static void HWR_CompileShader(int index)
{
	char *vertex_source = gl_shaders[index].vertex;
	char *fragment_source = gl_shaders[index].fragment;

	if (vertex_source)
	{
		char *preprocessed = HWR_PreprocessShader(vertex_source);
		if (!preprocessed) return;
		HWD.pfnLoadShader(index, preprocessed, HWD_SHADERSTAGE_VERTEX);
	}
	if (fragment_source)
	{
		char *preprocessed = HWR_PreprocessShader(fragment_source);
		if (!preprocessed) return;
		HWD.pfnLoadShader(index, preprocessed, HWD_SHADERSTAGE_FRAGMENT);
	}

	gl_shaders[index].compiled = HWD.pfnCompileShader(index);
}


void HWR_CompileShaders(void)
{
	int i;

	for (i = 0; i < NUMSHADERTARGETS; i++)
	{
		int custom_index = gl_shadertargets[i].custom_shader;
		HWR_CompileShader(i);
		if (!gl_shaders[i].compiled)
			CONS_Alert(CONS_ERROR, "HWR_CompileShaders: Compilation failed for base %s shader!\n", shaderxlat[i].type);
		if (custom_index != -1)
		{
			HWR_CompileShader(custom_index);
			if (!gl_shaders[custom_index].compiled)
				CONS_Alert(CONS_ERROR, "HWR_CompileShaders: Recompilation failed for the custom %s shader! See the console messages above for more information.\n", shaderxlat[i].type);
		}
	}
}

int HWR_GetShaderFromTarget(int shader_target)
{
	int custom_shader = gl_shadertargets[shader_target].custom_shader;





	if (custom_shader != -1 && gl_shaders[custom_shader].compiled &&
		cv_glshaders.value == 1 && cv_glallowshaders.value)
		return custom_shader;
	else
		return gl_shadertargets[shader_target].base_shader;
}

static inline UINT16 HWR_FindShaderDefs(UINT16 wadnum)
{
	UINT16 i;
	lumpinfo_t *lump_p;

	lump_p = wadfiles[wadnum]->lumpinfo;
	for (i = 0; i < wadfiles[wadnum]->numlumps; i++, lump_p++)
		if (memcmp(lump_p->name, "SHADERS", 7) == 0)
			return i;

	return INT16_MAX;
}

customshaderxlat_t shaderxlat[] =
{
	{"Flat", SHADER_FLOOR},
	{"WallTexture", SHADER_WALL},
	{"Sprite", SHADER_SPRITE},
	{"Model", SHADER_MODEL},
	{"WaterRipple", SHADER_WATER},
	{"Fog", SHADER_FOG},
	{"Sky", SHADER_SKY},
	{"PalettePostprocess", SHADER_PALETTE_POSTPROCESS},
	{"UIColormapFade", SHADER_UI_COLORMAP_FADE},
	{"UITintedWipe", SHADER_UI_TINTED_WIPE},
	{NULL, 0},
};

void HWR_LoadAllCustomShaders(void)
{
	INT32 i;


	for (i = 0; i < numwadfiles; i++)
		HWR_LoadCustomShadersFromFile(i, W_FileHasFolders(wadfiles[i]));
}

static const char version_directives[][14] = {
	"#version 330\n",
	"#version 150\n",
	"#version 140\n",
	"#version 130\n",
	"#version 120\n",
	"#version 110\n",
};

static boolean HWR_VersionDirectiveExists(const char* source)
{
    return strncmp(source, "#version", 8) == 0;
}

static char* HWR_PrependVersionDirective(const char* source, UINT32 version_index)
{
	const UINT32 version_len = sizeof(version_directives[version_index]) - 1;
	const UINT32 source_len = strlen(source);

	char* result = Z_Malloc(source_len + version_len + 1, PU_STATIC, NULL);
	strcpy(result, version_directives[version_index]);
	strcpy(result + version_len, source);

	return result;
}

static void HWR_ReplaceVersionInplace(char* shader, UINT32 version_index)
{
	shader[9] = version_directives[version_index][9];
	shader[10] = version_directives[version_index][10];
	shader[11] = version_directives[version_index][11];
}

static boolean HWR_CheckVersionDirectives(const char* vert, const char* frag)
{
	return HWR_VersionDirectiveExists(vert) && HWR_VersionDirectiveExists(frag);
}

static void HWR_TryToCompileShaderWithImplicitVersion(INT32 shader_index, INT32 shaderxlat_id)
{
	char* vert_shader = gl_shaders[shader_index].vertex;
	char* frag_shader = gl_shaders[shader_index].fragment;

	boolean vert_shader_version_exists = HWR_VersionDirectiveExists(vert_shader);
	boolean frag_shader_version_exists = HWR_VersionDirectiveExists(frag_shader);

	if(!vert_shader_version_exists) {
		CONS_Alert(CONS_WARNING, "HWR_LoadCustomShadersFromFile: vertex shader '%s' is missing a #version directive\n", HWR_GetShaderName(shaderxlat_id));
	}

	if(!frag_shader_version_exists) {
		CONS_Alert(CONS_WARNING, "HWR_LoadCustomShadersFromFile: fragment shader '%s' is missing a #version directive\n", HWR_GetShaderName(shaderxlat_id));
	}


	HWR_CompileShader(shader_index);
	if (gl_shaders[shader_index].compiled)
		return;


	for(UINT32 i = 0; i < sizeof(version_directives) / sizeof(version_directives[0]); ++i) {
		CONS_Alert(CONS_NOTICE, "HWR_TryToCompileShaderWithImplicitVersion: Trying %s\n", version_directives[i]);

		if(!vert_shader_version_exists) {


			if(i == 0) {
				void* old = (void*)gl_shaders[shader_index].vertex;
				vert_shader = gl_shaders[shader_index].vertex = HWR_PrependVersionDirective(vert_shader, i);
				Z_Free(old);
			} else {
				HWR_ReplaceVersionInplace(vert_shader, i);
			}
		}

		if(!frag_shader_version_exists) {
			if(i == 0) {
				void* old = (void*)gl_shaders[shader_index].fragment;
				frag_shader = gl_shaders[shader_index].fragment = HWR_PrependVersionDirective(frag_shader, i);
				Z_Free(old);
			} else {
				HWR_ReplaceVersionInplace(frag_shader, i);
			}
		}

		HWR_CompileShader(shader_index);
		if (gl_shaders[shader_index].compiled) {
			CONS_Alert(CONS_NOTICE, "HWR_TryToCompileShaderWithImplicitVersion: Compiled with %s\n",
					   version_directives[i]);
			CONS_Alert(CONS_WARNING, "Implicit GLSL version is used. Correct behavior is not guaranteed\n");
			return;
		}
	}
}

void HWR_LoadCustomShadersFromFile(UINT16 wadnum, boolean PK3)
{
	UINT16 lump;
	char *shaderdef, *line;
	char *stoken;
	char *value;
	size_t size;
	int linenum = 1;
	int shadertype = 0;
	int i;
	boolean modified_shaders[NUMSHADERTARGETS] = {0};

	if (!gl_shadersavailable)
		return;

	lump = HWR_FindShaderDefs(wadnum);
	if (lump == INT16_MAX)
		return;

	shaderdef = W_CacheLumpNumPwad(wadnum, lump, PU_CACHE);
	size = W_LumpLengthPwad(wadnum, lump);

	line = Z_Malloc(size+1, PU_STATIC, NULL);
	M_Memcpy(line, shaderdef, size);
	line[size] = '\0';

	stoken = strtok(line, "\r\n ");
	while (stoken)
	{
		if ((stoken[0] == '/' && stoken[1] == '/')
			|| (stoken[0] == '#')) 
		{
			stoken = strtok(NULL, "\r\n");
			goto skip_field;
		}

		if (!stricmp(stoken, "GLSL"))
		{
			value = strtok(NULL, "\r\n ");
			if (!value)
			{
				CONS_Alert(CONS_WARNING, "HWR_LoadCustomShadersFromFile: Missing shader type (file %s, line %d)\n", wadfiles[wadnum]->filename, linenum);
				stoken = strtok(NULL, "\r\n");  
				goto skip_lump;
			}

			if (!stricmp(value, "VERTEX"))
				shadertype = 1;
			else if (!stricmp(value, "FRAGMENT"))
				shadertype = 2;

skip_lump:
			stoken = strtok(NULL, "\r\n ");
			linenum++;
		}
		else
		{
			value = strtok(NULL, "\r\n= ");
			if (!value)
			{
				CONS_Alert(CONS_WARNING, "HWR_LoadCustomShadersFromFile: Missing shader target (file %s, line %d)\n", wadfiles[wadnum]->filename, linenum);
				stoken = strtok(NULL, "\r\n");  
				goto skip_field;
			}

			if (!shadertype)
			{
				CONS_Alert(CONS_ERROR, "HWR_LoadCustomShadersFromFile: Missing shader type (file %s, line %d)\n", wadfiles[wadnum]->filename, linenum);
				Z_Free(line);
				return;
			}

			for (i = 0; shaderxlat[i].type; i++)
			{
				if (!stricmp(shaderxlat[i].type, stoken))
				{
					size_t shader_string_length;
					char *shader_source;
					char *shader_lumpname;
					UINT16 shader_lumpnum;
					int shader_index;  

					if (PK3)
					{
						shader_lumpname = Z_Malloc(strlen(value) + 12, PU_STATIC, NULL);
						strcpy(shader_lumpname, "Shaders/sh_");
						strcat(shader_lumpname, value);
						shader_lumpnum = W_CheckNumForFullNamePK3(shader_lumpname, wadnum, 0);
					}
					else
					{
						shader_lumpname = Z_Malloc(strlen(value) + 4, PU_STATIC, NULL);
						strcpy(shader_lumpname, "SH_");
						strcat(shader_lumpname, value);
						shader_lumpnum = W_CheckNumForNamePwad(shader_lumpname, wadnum, 0);
					}

					if (shader_lumpnum == INT16_MAX)
					{
						CONS_Alert(CONS_ERROR, "HWR_LoadCustomShadersFromFile: Missing shader source %s (file %s, line %d)\n", shader_lumpname, wadfiles[wadnum]->filename, linenum);
						Z_Free(shader_lumpname);
						continue;
					}

					shader_string_length = W_LumpLengthPwad(wadnum, shader_lumpnum) + 1;
					shader_source = Z_Malloc(shader_string_length, PU_STATIC, NULL);
					W_ReadLumpPwad(wadnum, shader_lumpnum, shader_source);
					shader_source[shader_string_length-1] = '\0';

					shader_index = shaderxlat[i].id + NUMSHADERTARGETS;
					if (!modified_shaders[shaderxlat[i].id])
					{


						Z_Free(gl_shaders[shader_index].vertex);
						gl_shaders[shader_index].vertex = NULL;
						Z_Free(gl_shaders[shader_index].fragment);
						gl_shaders[shader_index].fragment = NULL;
					}
					modified_shaders[shaderxlat[i].id] = true;

					if (shadertype == 1)
					{
						if (gl_shaders[shader_index].vertex)
						{
							CONS_Alert(CONS_WARNING, "HWR_LoadCustomShadersFromFile: %s is overwriting another %s vertex shader from the same addon! (file %s, line %d)\n", shader_lumpname, shaderxlat[i].type, wadfiles[wadnum]->filename, linenum);
							Z_Free(gl_shaders[shader_index].vertex);
						}
						gl_shaders[shader_index].vertex = shader_source;
					}
					else
					{
						if (gl_shaders[shader_index].fragment)
						{
							CONS_Alert(CONS_WARNING, "HWR_LoadCustomShadersFromFile: %s is overwriting another %s fragment shader from the same addon! (file %s, line %d)\n", shader_lumpname, shaderxlat[i].type, wadfiles[wadnum]->filename, linenum);
							Z_Free(gl_shaders[shader_index].fragment);
						}
						gl_shaders[shader_index].fragment = shader_source;
					}

					Z_Free(shader_lumpname);
				}
			}

skip_field:
			stoken = strtok(NULL, "\r\n= ");
			linenum++;
		}
	}

	for (i = 0; i < NUMSHADERTARGETS; i++)
	{
		if (modified_shaders[i])
		{
			int shader_index = i + NUMSHADERTARGETS;  
			gl_shadertargets[i].custom_shader = shader_index;


			if (!gl_shaders[shader_index].fragment)
				gl_shaders[shader_index].fragment = Z_StrDup(gl_shadersources[i].fragment);
			if (!gl_shaders[shader_index].vertex)
				gl_shaders[shader_index].vertex = Z_StrDup(gl_shadersources[i].vertex);

			if(!HWR_CheckVersionDirectives(gl_shaders[shader_index].vertex, gl_shaders[shader_index].fragment)) {
				HWR_TryToCompileShaderWithImplicitVersion(shader_index, i);
			} else {
				HWR_CompileShader(shader_index);
			}

			if (!gl_shaders[shader_index].compiled)
				CONS_Alert(CONS_ERROR, "HWR_LoadCustomShadersFromFile: A compilation error occured for the %s shader in file %s. See the console messages above for more information.\n", shaderxlat[i].type, wadfiles[wadnum]->filename);
		}
	}

	Z_Free(line);
	return;
}

const char *HWR_GetShaderName(INT32 shader)
{
	INT32 i;

	for (i = 0; shaderxlat[i].type; i++)
	{
		if (shaderxlat[i].id == shader)
			return shaderxlat[i].type;
	}

	return "Unknown";
}

#endif  
