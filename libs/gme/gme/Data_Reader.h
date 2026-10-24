


#ifndef DATA_READER_H
#define DATA_READER_H

#include "blargg_common.h"


class Data_Reader {
public:
	virtual ~Data_Reader() { }
	
	static const char eof_error [];  
	

	virtual long read_avail( void*, long n ) = 0;
	

	virtual blargg_err_t read( void*, long count );
	

	virtual long remain() const = 0;
	

	virtual blargg_err_t skip( long count );
	
public:
	Data_Reader() { }
	typedef blargg_err_t error_t;  
private:

	Data_Reader( const Data_Reader& );
	Data_Reader& operator = ( const Data_Reader& );
};


class File_Reader : public Data_Reader {
public:

	virtual long size() const = 0;
	

	virtual long tell() const = 0;
	

	virtual blargg_err_t seek( long ) = 0;
	
	long remain() const;
	blargg_err_t skip( long n );
};


class Std_File_Reader : public File_Reader {
public:
	blargg_err_t open( const char* path );
	void close();
	
public:
	Std_File_Reader();
	~Std_File_Reader();
	long size() const;
	blargg_err_t read( void*, long );
	long read_avail( void*, long );
	long tell() const;
	blargg_err_t seek( long );
private:
	void* file_;
};


class Mem_File_Reader : public File_Reader {
public:
	Mem_File_Reader( const void*, long size );
	
public:
	long size() const;
	long read_avail( void*, long );
	long tell() const;
	blargg_err_t seek( long );
private:
	const char* const begin;
	const long size_;
	long pos;
};


class Subset_Reader : public Data_Reader {
public:
	Subset_Reader( Data_Reader*, long count );

public:
	long remain() const;
	long read_avail( void*, long );
private:
	Data_Reader* in;
	long remain_;
};


class Remaining_Reader : public Data_Reader {
public:
	Remaining_Reader( void const* header, long size, Data_Reader* );

public:
	long remain() const;
	long read_avail( void*, long );
	blargg_err_t read( void*, long );
private:
	char const* header;
	char const* header_end;
	Data_Reader* in;
	long read_first( void* out, long count );
};


class Callback_Reader : public Data_Reader {
public:
	typedef const char* (*callback_t)( void* data, void* out, int count );
	Callback_Reader( callback_t, long size, void* data = 0 );
public:
	long read_avail( void*, long );
	blargg_err_t read( void*, long );
	long remain() const;
private:
	callback_t const callback;
	void* const data;
	long remain_;
};

#ifdef HAVE_ZLIB_H
#include <zlib.h>


class Gzip_File_Reader : public File_Reader {
public:
	blargg_err_t open( const char* path );
	void close();
	
public:
	Gzip_File_Reader();
	~Gzip_File_Reader();
	long size() const;
	long read_avail( void*, long );
	long tell() const;
	blargg_err_t seek( long );
private:
	gzFile file_;
	long size_;
};
#endif

#endif
