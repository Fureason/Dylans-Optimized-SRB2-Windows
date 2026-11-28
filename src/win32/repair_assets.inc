
#include <wincrypt.h>
#include <curl/curl.h>
#include <zlib.h>

#define REPAIR_ARCHIVE_SIZE 179033270u
static const char repair_archive_url[] =
	"https://github.com/STJr/SRB2/releases/download/SRB2_release_2.2.15/SRB2-v2215-Full.zip";
static const BYTE repair_archive_sha256[32] = {
	0x3e, 0xda, 0x30, 0x80, 0xab, 0x87, 0x94, 0x0f,
	0xca, 0x5e, 0x4d, 0xcd, 0x22, 0xb8, 0xe0, 0x41,
	0xdc, 0x1d, 0x9e, 0x65, 0xbf, 0xe9, 0x17, 0x7b,
	0xff, 0x68, 0x38, 0x3e, 0x9b, 0xd5, 0x8a, 0x10
};

typedef struct {
	const char *name;
	long offset;
	size_t compressed_size;
	size_t size;
} repair_asset_t;



static const repair_asset_t repair_assets[] = {
	{"srb2.pk3",       110627220L, 47506310u, 50115266u},
	{"zones.pk3",      160607870L, 18398791u, 18503511u},
	{"characters.pk3",   4765545L,  1765769u,  2044315u}
};

typedef struct {
	FILE *file;
	HCRYPTHASH hash;
	size_t received;
} repair_download_t;

static size_t I_WriteRepairArchive(char *data, size_t unit, size_t count, void *context)
{
	repair_download_t *download = context;
	if (unit == 0 || count > (REPAIR_ARCHIVE_SIZE - download->received) / unit)
		return 0;
	const size_t length = unit * count;

	if (fwrite(data, 1, length, download->file) != length
		|| !CryptHashData(download->hash, (const BYTE *)data, (DWORD)length, 0))
		return 0;
	download->received += length;
	return length;
}

static boolean I_DownloadRepairArchive(FILE *archive)
{
	repair_download_t download = {archive, 0, 0};
	HCRYPTPROV provider = 0;
	CURL *request = NULL;
	boolean verified = false;
	BYTE digest[sizeof repair_archive_sha256];
	DWORD digest_length = sizeof digest;

	if (!CryptAcquireContextA(&provider, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)
		|| !CryptCreateHash(provider, CALG_SHA_256, 0, 0, &download.hash)
		|| !(request = curl_easy_init()))
	{
		I_OutputMsg("Game data repair could not initialize the download or SHA-256.\n");
		goto done;
	}

	curl_easy_setopt(request, CURLOPT_URL, repair_archive_url);
#if LIBCURL_VERSION_NUM >= 0x075500
	curl_easy_setopt(request, CURLOPT_PROTOCOLS_STR, "https");
	curl_easy_setopt(request, CURLOPT_REDIR_PROTOCOLS_STR, "https");
#else
	curl_easy_setopt(request, CURLOPT_PROTOCOLS, (long)CURLPROTO_HTTPS);
	curl_easy_setopt(request, CURLOPT_REDIR_PROTOCOLS, (long)CURLPROTO_HTTPS);
#endif
	curl_easy_setopt(request, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(request, CURLOPT_MAXREDIRS, 5L);
	curl_easy_setopt(request, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(request, CURLOPT_CONNECTTIMEOUT, 15L);
	curl_easy_setopt(request, CURLOPT_TIMEOUT, 180L);
	curl_easy_setopt(request, CURLOPT_MAXFILESIZE_LARGE, (curl_off_t)REPAIR_ARCHIVE_SIZE);
	curl_easy_setopt(request, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(request, CURLOPT_USERAGENT, "Sonic Robo Blast 2/" SRB2VERSION);
	curl_easy_setopt(request, CURLOPT_WRITEFUNCTION, I_WriteRepairArchive);
	curl_easy_setopt(request, CURLOPT_WRITEDATA, &download);

	I_OutputMsg("Downloading the official SRB2 2.2.15 archive to restore missing game data...\n");
	const CURLcode result = curl_easy_perform(request);
	if (result != CURLE_OK)
		I_OutputMsg("Game data repair download failed: %s\n", curl_easy_strerror(result));
	else if (download.received == REPAIR_ARCHIVE_SIZE && fflush(archive) == 0
		&& CryptGetHashParam(download.hash, HP_HASHVAL, digest, &digest_length, 0)
		&& digest_length == sizeof digest && !memcmp(digest, repair_archive_sha256, sizeof digest))
		verified = true;
	else
		I_OutputMsg("Game data repair rejected the archive because verification failed.\n");

done:
	if (request)
		curl_easy_cleanup(request);
	if (download.hash)
		CryptDestroyHash(download.hash);
	if (provider)
		CryptReleaseContext(provider, 0);
	return verified;
}

static boolean I_ExtractRepairAsset(FILE *archive, FILE *output, const repair_asset_t *asset)
{
	BYTE in[32768], out[32768];
	z_stream stream = {0};
	size_t compressed = asset->compressed_size;
	size_t remaining = asset->size;
	int result = Z_OK;

	if (fseek(archive, asset->offset, SEEK_SET) != 0 || inflateInit2(&stream, -MAX_WBITS) != Z_OK)
		return false;

	do
	{
		if (stream.avail_in == 0 && compressed)
		{
			const size_t bytes = min(compressed, sizeof in);
			if (fread(in, 1, bytes, archive) != bytes)
				break;
			stream.next_in = in;
			stream.avail_in = (uInt)bytes;
			compressed -= bytes;
		}
		stream.next_out = out;
		stream.avail_out = sizeof out;
		result = inflate(&stream, Z_NO_FLUSH);

		const size_t bytes = sizeof out - stream.avail_out;
		if (bytes > remaining || fwrite(out, 1, bytes, output) != bytes)
		{
			result = Z_ERRNO;
			break;
		}
		remaining -= bytes;
	} while (result == Z_OK);

	const boolean complete = result == Z_STREAM_END && !compressed && !stream.avail_in && !remaining;
	inflateEnd(&stream);
	return complete;
}

static boolean I_RepairAssetPath(char path[MAX_PATH], const char *dir, const char *name)
{
	const int length = snprintf(path, MAX_PATH, "%s" PATHSEP "%s", dir, name);
	return length > 0 && length < MAX_PATH;
}

static boolean I_RequiredGameDataPresent(const char *dir)
{
	char path[MAX_PATH];
	for (size_t i = 0; i < sizeof repair_assets / sizeof *repair_assets; ++i)
		if (!I_RepairAssetPath(path, dir, repair_assets[i].name) || !FIL_ReadFileOK(path))
			return false;
	return true;
}

static FILE *I_OpenRepairTemp(const char *dir, char path[MAX_PATH])
{
	if (!GetTempFileNameA(dir, "srb", 0, path))
		return NULL;
	FILE *file = fopen(path, "w+b");
	if (!file)
		DeleteFileA(path);
	return file;
}

static boolean I_RestoreRepairAsset(FILE *archive, const char *dir,
	const char *path, const repair_asset_t *asset)
{
	char temporary[MAX_PATH];
	FILE *file = I_OpenRepairTemp(dir, temporary);
	if (!file)
		return false;
	const boolean extracted = I_ExtractRepairAsset(archive, file, asset);
	const int closed = fclose(file);
	if (extracted && closed == 0 && MoveFileExA(temporary, path, MOVEFILE_WRITE_THROUGH))
	{
		I_OutputMsg("Restored %s next to srb2win.exe.\n", asset->name);
		return true;
	}
	DeleteFileA(temporary);
	return FIL_ReadFileOK(path);
}

static const char *I_RepairGameDirectory(void)
{
	static char directory[MAX_PATH];
	char paths[sizeof repair_assets / sizeof *repair_assets][MAX_PATH];
	boolean missing[sizeof repair_assets / sizeof *repair_assets] = {0};
	boolean needed = false;
	DWORD length = GetModuleFileNameA(NULL, directory, sizeof directory);
	if (!length || length >= sizeof directory || strcmp(SRB2VERSION, "2.2.15") != 0)
		return NULL;

	char *last = strrchr(directory, '\\');
	if (!last)
		return NULL;
	*last = '\0';

	for (size_t i = 0; i < sizeof repair_assets / sizeof *repair_assets; ++i)
	{
		if (!I_RepairAssetPath(paths[i], directory, repair_assets[i].name))
			return NULL;
		if (FIL_ReadFileOK(paths[i]))
			continue;
		if (GetFileAttributesA(paths[i]) != INVALID_FILE_ATTRIBUTES
			|| GetLastError() != ERROR_FILE_NOT_FOUND)
			return NULL;
		missing[i] = needed = true;
	}
	if (!needed)
		return directory;

	char archive_path[MAX_PATH];
	FILE *archive = I_OpenRepairTemp(directory, archive_path);
	if (!archive)
	{
		I_OutputMsg("Game data repair could not write to %s.\n", directory);
		return NULL;
	}

	boolean ready = I_DownloadRepairArchive(archive);
	for (size_t i = 0; ready && i < sizeof repair_assets / sizeof *repair_assets; ++i)
		if (missing[i] && !I_RestoreRepairAsset(archive, directory, paths[i], &repair_assets[i]))
		{
			I_OutputMsg("Game data repair could not restore %s in %s.\n", repair_assets[i].name, directory);
			ready = false;
		}

	fclose(archive);
	DeleteFileA(archive_path);
	return ready && I_RequiredGameDataPresent(directory) ? directory : NULL;
}
