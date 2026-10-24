----------------------------------------------------------------
--  ZLib for Ada thick binding.                               --
--                                                            --
--  Copyright (C) 2002-2003 Dmitriy Anisimkov                 --
--                                                            --
--  Open source license information is in the zlib.ads file.  --
----------------------------------------------------------------



with Interfaces.C.Strings;

with System;

private package ZLib.Thin is



   MAX_MEM_LEVEL : constant := 9;          

   MAX_WBITS : constant := 15;       


   SEEK_SET : constant := 8#0000#;   


   SEEK_CUR : constant := 1;         


   SEEK_END : constant := 2;         



   type Byte is new Interfaces.C.unsigned_char;  

   type UInt is new Interfaces.C.unsigned;       

   type Int is new Interfaces.C.int;

   type ULong is new Interfaces.C.unsigned_long;      

   subtype Chars_Ptr is Interfaces.C.Strings.chars_ptr;

   type ULong_Access is access ULong;
   type Int_Access is access Int;

   subtype Voidp is System.Address;             

   subtype Byte_Access is Voidp;

   Nul : constant Voidp := System.Null_Address;


   Z_NO_FLUSH : constant := 8#0000#;    

   Z_PARTIAL_FLUSH : constant := 1;        



   Z_SYNC_FLUSH : constant := 2;        

   Z_FULL_FLUSH : constant := 3;        

   Z_FINISH : constant := 4;         

   Z_OK : constant := 8#0000#;    

   Z_STREAM_END : constant := 1;        

   Z_NEED_DICT : constant := 2;         

   Z_ERRNO : constant := -1;         

   Z_STREAM_ERROR : constant := -2;        

   Z_DATA_ERROR : constant := -3;       

   Z_MEM_ERROR : constant := -4;        

   Z_BUF_ERROR : constant := -5;        

   Z_VERSION_ERROR : constant := -6;       

   Z_NO_COMPRESSION : constant := 8#0000#;    

   Z_BEST_SPEED : constant := 1;        

   Z_BEST_COMPRESSION : constant := 9;        

   Z_DEFAULT_COMPRESSION : constant := -1;       

   Z_FILTERED : constant := 1;       

   Z_HUFFMAN_ONLY : constant := 2;         

   Z_DEFAULT_STRATEGY : constant := 8#0000#;  

   Z_BINARY : constant := 8#0000#;   

   Z_ASCII : constant := 1;       

   Z_UNKNOWN : constant := 2;        

   Z_DEFLATED : constant := 8;       

   Z_NULL : constant := 8#0000#;  


   type gzFile is new Voidp;                   

   type Z_Stream is private;

   type Z_Streamp is access all Z_Stream;      

   type alloc_func is access function
     (Opaque : Voidp;
      Items  : UInt;
      Size   : UInt)
      return Voidp;  

   type free_func is access procedure (opaque : Voidp; address : Voidp);

   function zlibVersion return Chars_Ptr;

   function Deflate (strm : Z_Streamp; flush : Int) return Int;

   function DeflateEnd (strm : Z_Streamp) return Int;

   function Inflate (strm : Z_Streamp; flush : Int) return Int;

   function InflateEnd (strm : Z_Streamp) return Int;

   function deflateSetDictionary
     (strm       : Z_Streamp;
      dictionary : Byte_Access;
      dictLength : UInt)
      return       Int;

   function deflateCopy (dest : Z_Streamp; source : Z_Streamp) return Int;


   function deflateReset (strm : Z_Streamp) return Int;  

   function deflateParams
     (strm     : Z_Streamp;
      level    : Int;
      strategy : Int)
      return     Int;        

   function inflateSetDictionary
     (strm       : Z_Streamp;
      dictionary : Byte_Access;
      dictLength : UInt)
      return       Int;  

   function inflateSync (strm : Z_Streamp) return Int;   

   function inflateReset (strm : Z_Streamp) return Int;  

   function compress
     (dest      : Byte_Access;
      destLen   : ULong_Access;
      source    : Byte_Access;
      sourceLen : ULong)
      return      Int;            

   function compress2
     (dest      : Byte_Access;
      destLen   : ULong_Access;
      source    : Byte_Access;
      sourceLen : ULong;
      level     : Int)
      return      Int;           

   function uncompress
     (dest      : Byte_Access;
      destLen   : ULong_Access;
      source    : Byte_Access;
      sourceLen : ULong)
      return      Int;

   function gzopen (path : Chars_Ptr; mode : Chars_Ptr) return gzFile;

   function gzdopen (fd : Int; mode : Chars_Ptr) return gzFile;

   function gzsetparams
     (file     : gzFile;
      level    : Int;
      strategy : Int)
      return     Int;

   function gzread
     (file : gzFile;
      buf  : Voidp;
      len  : UInt)
      return Int;

   function gzwrite
     (file : in gzFile;
      buf  : in Voidp;
      len  : in UInt)
      return Int;

   function gzprintf (file : in gzFile; format : in Chars_Ptr) return Int;

   function gzputs (file : in gzFile; s : in Chars_Ptr) return Int;

   function gzgets
     (file : gzFile;
      buf  : Chars_Ptr;
      len  : Int)
      return Chars_Ptr;

   function gzputc (file : gzFile; char : Int) return Int;

   function gzgetc (file : gzFile) return Int;

   function gzflush (file : gzFile; flush : Int) return Int;

   function gzseek
     (file   : gzFile;
      offset : Int;
      whence : Int)
      return   Int;

   function gzrewind (file : gzFile) return Int;

   function gztell (file : gzFile) return Int;

   function gzeof (file : gzFile) return Int;

   function gzclose (file : gzFile) return Int;

   function gzerror (file : gzFile; errnum : Int_Access) return Chars_Ptr;

   function adler32
     (adler : ULong;
      buf   : Byte_Access;
      len   : UInt)
      return  ULong;

   function crc32
     (crc  : ULong;
      buf  : Byte_Access;
      len  : UInt)
      return ULong;

   function deflateInit
     (strm        : Z_Streamp;
      level       : Int;
      version     : Chars_Ptr;
      stream_size : Int)
      return        Int;

   function deflateInit2
     (strm        : Z_Streamp;
      level       : Int;
      method      : Int;
      windowBits  : Int;
      memLevel    : Int;
      strategy    : Int;
      version     : Chars_Ptr;
      stream_size : Int)
      return        Int;

   function Deflate_Init
     (strm       : Z_Streamp;
      level      : Int;
      method     : Int;
      windowBits : Int;
      memLevel   : Int;
      strategy   : Int)
      return       Int;
   pragma Inline (Deflate_Init);

   function inflateInit
     (strm        : Z_Streamp;
      version     : Chars_Ptr;
      stream_size : Int)
      return        Int;

   function inflateInit2
     (strm        : in Z_Streamp;
      windowBits  : in Int;
      version     : in Chars_Ptr;
      stream_size : in Int)
      return      Int;

   function inflateBackInit
     (strm        : in Z_Streamp;
      windowBits  : in Int;
      window      : in Byte_Access;
      version     : in Chars_Ptr;
      stream_size : in Int)
      return      Int;


   function Inflate_Init (strm : Z_Streamp; windowBits : Int) return Int;
   pragma Inline (Inflate_Init);

   function zError (err : Int) return Chars_Ptr;

   function inflateSyncPoint (z : Z_Streamp) return Int;

   function get_crc_table return ULong_Access;







   procedure Set_In
     (Strm   : in out Z_Stream;
      Buffer : in Voidp;
      Size   : in UInt);
   pragma Inline (Set_In);

   procedure Set_Out
     (Strm   : in out Z_Stream;
      Buffer : in Voidp;
      Size   : in UInt);
   pragma Inline (Set_Out);

   procedure Set_Mem_Func
     (Strm   : in out Z_Stream;
      Opaque : in Voidp;
      Alloc  : in alloc_func;
      Free   : in free_func);
   pragma Inline (Set_Mem_Func);

   function Last_Error_Message (Strm : in Z_Stream) return String;
   pragma Inline (Last_Error_Message);

   function Avail_Out (Strm : in Z_Stream) return UInt;
   pragma Inline (Avail_Out);

   function Avail_In (Strm : in Z_Stream) return UInt;
   pragma Inline (Avail_In);

   function Total_In (Strm : in Z_Stream) return ULong;
   pragma Inline (Total_In);

   function Total_Out (Strm : in Z_Stream) return ULong;
   pragma Inline (Total_Out);

   function inflateCopy
     (dest   : in Z_Streamp;
      Source : in Z_Streamp)
      return Int;

   function compressBound (Source_Len : in ULong) return ULong;

   function deflateBound
     (Strm       : in Z_Streamp;
      Source_Len : in ULong)
      return     ULong;

   function gzungetc (C : in Int; File : in  gzFile) return Int;

   function zlibCompileFlags return ULong;

private

   type Z_Stream is record             
      Next_In   : Voidp      := Nul;   
      Avail_In  : UInt       := 0;     
      Total_In  : ULong      := 0;     
      Next_Out  : Voidp      := Nul;   
      Avail_Out : UInt       := 0;     
      Total_Out : ULong      := 0;     
      msg       : Chars_Ptr;           
      state     : Voidp;               
      zalloc    : alloc_func := null;  
      zfree     : free_func  := null;  
      opaque    : Voidp;               

      data_type : Int;                 

      adler     : ULong;               

      reserved  : ULong;               
   end record;

   pragma Convention (C, Z_Stream);

   pragma Import (C, zlibVersion, "zlibVersion");
   pragma Import (C, Deflate, "deflate");
   pragma Import (C, DeflateEnd, "deflateEnd");
   pragma Import (C, Inflate, "inflate");
   pragma Import (C, InflateEnd, "inflateEnd");
   pragma Import (C, deflateSetDictionary, "deflateSetDictionary");
   pragma Import (C, deflateCopy, "deflateCopy");
   pragma Import (C, deflateReset, "deflateReset");
   pragma Import (C, deflateParams, "deflateParams");
   pragma Import (C, inflateSetDictionary, "inflateSetDictionary");
   pragma Import (C, inflateSync, "inflateSync");
   pragma Import (C, inflateReset, "inflateReset");
   pragma Import (C, compress, "compress");
   pragma Import (C, compress2, "compress2");
   pragma Import (C, uncompress, "uncompress");
   pragma Import (C, gzopen, "gzopen");
   pragma Import (C, gzdopen, "gzdopen");
   pragma Import (C, gzsetparams, "gzsetparams");
   pragma Import (C, gzread, "gzread");
   pragma Import (C, gzwrite, "gzwrite");
   pragma Import (C, gzprintf, "gzprintf");
   pragma Import (C, gzputs, "gzputs");
   pragma Import (C, gzgets, "gzgets");
   pragma Import (C, gzputc, "gzputc");
   pragma Import (C, gzgetc, "gzgetc");
   pragma Import (C, gzflush, "gzflush");
   pragma Import (C, gzseek, "gzseek");
   pragma Import (C, gzrewind, "gzrewind");
   pragma Import (C, gztell, "gztell");
   pragma Import (C, gzeof, "gzeof");
   pragma Import (C, gzclose, "gzclose");
   pragma Import (C, gzerror, "gzerror");
   pragma Import (C, adler32, "adler32");
   pragma Import (C, crc32, "crc32");
   pragma Import (C, deflateInit, "deflateInit_");
   pragma Import (C, inflateInit, "inflateInit_");
   pragma Import (C, deflateInit2, "deflateInit2_");
   pragma Import (C, inflateInit2, "inflateInit2_");
   pragma Import (C, zError, "zError");
   pragma Import (C, inflateSyncPoint, "inflateSyncPoint");
   pragma Import (C, get_crc_table, "get_crc_table");



   pragma Import (C, inflateCopy, "inflateCopy");
   pragma Import (C, compressBound, "compressBound");
   pragma Import (C, deflateBound, "deflateBound");
   pragma Import (C, gzungetc, "gzungetc");
   pragma Import (C, zlibCompileFlags, "zlibCompileFlags");

   pragma Import (C, inflateBackInit, "inflateBackInit_");












end ZLib.Thin;
