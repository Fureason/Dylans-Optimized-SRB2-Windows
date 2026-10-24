 
 
 
 
 
{       Copyright (c) 1997,99 Borland Corporation       }
 
 

 

unit ZLib;

interface

uses SysUtils, Classes;

type
  TAlloc = function (AppData: Pointer; Items, Size: Integer): Pointer; cdecl;
  TFree = procedure (AppData, Block: Pointer); cdecl;


  TZStreamRec = packed record
    next_in: PChar;        
    avail_in: Integer;     
    total_in: Longint;     

    next_out: PChar;       
    avail_out: Integer;    
    total_out: Longint;    

    msg: PChar;            
    internal: Pointer;     

    zalloc: TAlloc;        
    zfree: TFree;          
    AppData: Pointer;      

    data_type: Integer;    
    adler: Longint;        
    reserved: Longint;     
  end;


  TCustomZlibStream = class(TStream)
  private
    FStrm: TStream;
    FStrmPos: Integer;
    FOnProgress: TNotifyEvent;
    FZRec: TZStreamRec;
    FBuffer: array [Word] of Char;
  protected
    procedure Progress(Sender: TObject); dynamic;
    property OnProgress: TNotifyEvent read FOnProgress write FOnProgress;
    constructor Create(Strm: TStream);
  end;

                                                                             
                                               

                                                                            
                                                                       
                          

                                                                          
                                                                         
                               

                                                                   
                                                   

                                                                         
                                                                              
                                                                           
        

                                                                          
                                                                       
                                                                         
                           


  TCompressionLevel = (clNone, clFastest, clDefault, clMax);

  TCompressionStream = class(TCustomZlibStream)
  private
    function GetCompressionRate: Single;
  public
    constructor Create(CompressionLevel: TCompressionLevel; Dest: TStream);
    destructor Destroy; override;
    function Read(var Buffer; Count: Longint): Longint; override;
    function Write(const Buffer; Count: Longint): Longint; override;
    function Seek(Offset: Longint; Origin: Word): Longint; override;
    property CompressionRate: Single read GetCompressionRate;
    property OnProgress;
  end;

                                                                            

                                                                            
                                                                              
                                                                        
                                                                             
                                                                              
                                                                             
                                     

                                                                             
                                        

                                                                       
                                                                                
                                                                         
                                                                      

  TDecompressionStream = class(TCustomZlibStream)
  public
    constructor Create(Source: TStream);
    destructor Destroy; override;
    function Read(var Buffer; Count: Longint): Longint; override;
    function Write(const Buffer; Count: Longint): Longint; override;
    function Seek(Offset: Longint; Origin: Word): Longint; override;
    property OnProgress;
  end;



                                                             
                                     
                                         
                                                                          
                                               
procedure CompressBuf(const InBuf: Pointer; InBytes: Integer;
                      out OutBuf: Pointer; out OutBytes: Integer);


                                                                 
                                     
                                         
                                                                
                                                                          
                                               
procedure DecompressBuf(const InBuf: Pointer; InBytes: Integer;
 OutEstimate: Integer; out OutBuf: Pointer; out OutBytes: Integer);

                                                                       
                                     
                                         
                                                                         
                                              
procedure DecompressToUserBuf(const InBuf: Pointer; InBytes: Integer;
  const OutBuf: Pointer; BufSize: Integer);

const
  zlib_version = '1.3.1';

type
  EZlibError = class(Exception);
  ECompressionError = class(EZlibError);
  EDecompressionError = class(EZlibError);

implementation

uses ZLibConst;

const
  Z_NO_FLUSH      = 0;
  Z_PARTIAL_FLUSH = 1;
  Z_SYNC_FLUSH    = 2;
  Z_FULL_FLUSH    = 3;
  Z_FINISH        = 4;

  Z_OK            = 0;
  Z_STREAM_END    = 1;
  Z_NEED_DICT     = 2;
  Z_ERRNO         = (-1);
  Z_STREAM_ERROR  = (-2);
  Z_DATA_ERROR    = (-3);
  Z_MEM_ERROR     = (-4);
  Z_BUF_ERROR     = (-5);
  Z_VERSION_ERROR = (-6);

  Z_NO_COMPRESSION       =   0;
  Z_BEST_SPEED           =   1;
  Z_BEST_COMPRESSION     =   9;
  Z_DEFAULT_COMPRESSION  = (-1);

  Z_FILTERED            = 1;
  Z_HUFFMAN_ONLY        = 2;
  Z_RLE                 = 3;
  Z_DEFAULT_STRATEGY    = 0;

  Z_BINARY   = 0;
  Z_ASCII    = 1;
  Z_UNKNOWN  = 2;

  Z_DEFLATED = 8;


 
 
 
 
 
 
 
 
 
 
 

procedure adler32; external;
procedure compressBound; external;
procedure crc32; external;
procedure deflateInit2_; external;
procedure deflateParams; external;

function _malloc(Size: Integer): Pointer; cdecl;
begin
  Result := AllocMem(Size);
end;

procedure _free(Block: Pointer); cdecl;
begin
  FreeMem(Block);
end;

procedure _memset(P: Pointer; B: Byte; count: Integer); cdecl;
begin
  FillChar(P^, count, B);
end;

procedure _memcpy(dest, source: Pointer; count: Integer); cdecl;
begin
  Move(source^, dest^, count);
end;




function deflateInit_(var strm: TZStreamRec; level: Integer; version: PChar;
  recsize: Integer): Integer; external;
function deflate(var strm: TZStreamRec; flush: Integer): Integer; external;
function deflateEnd(var strm: TZStreamRec): Integer; external;


function inflateInit_(var strm: TZStreamRec; version: PChar;
  recsize: Integer): Integer; external;
function inflate(var strm: TZStreamRec; flush: Integer): Integer; external;
function inflateEnd(var strm: TZStreamRec): Integer; external;
function inflateReset(var strm: TZStreamRec): Integer; external;


function zlibAllocMem(AppData: Pointer; Items, Size: Integer): Pointer; cdecl;
begin

  Result := AllocMem(Items * Size);
end;

procedure zlibFreeMem(AppData, Block: Pointer); cdecl;
begin
  FreeMem(Block);
end;

                                            
     
                 
                  
                                             
     

function CCheck(code: Integer): Integer;
begin
  Result := code;
  if code < 0 then
    raise ECompressionError.Create('error');  
end;

function DCheck(code: Integer): Integer;
begin
  Result := code;
  if code < 0 then
    raise EDecompressionError.Create('error');   
end;

procedure CompressBuf(const InBuf: Pointer; InBytes: Integer;
                      out OutBuf: Pointer; out OutBytes: Integer);
var
  strm: TZStreamRec;
  P: Pointer;
begin
  FillChar(strm, sizeof(strm), 0);
  strm.zalloc := zlibAllocMem;
  strm.zfree := zlibFreeMem;
  OutBytes := ((InBytes + (InBytes div 10) + 12) + 255) and not 255;
  GetMem(OutBuf, OutBytes);
  try
    strm.next_in := InBuf;
    strm.avail_in := InBytes;
    strm.next_out := OutBuf;
    strm.avail_out := OutBytes;
    CCheck(deflateInit_(strm, Z_BEST_COMPRESSION, zlib_version, sizeof(strm)));
    try
      while CCheck(deflate(strm, Z_FINISH)) <> Z_STREAM_END do
      begin
        P := OutBuf;
        Inc(OutBytes, 256);
        ReallocMem(OutBuf, OutBytes);
        strm.next_out := PChar(Integer(OutBuf) + (Integer(strm.next_out) - Integer(P)));
        strm.avail_out := 256;
      end;
    finally
      CCheck(deflateEnd(strm));
    end;
    ReallocMem(OutBuf, strm.total_out);
    OutBytes := strm.total_out;
  except
    FreeMem(OutBuf);
    raise
  end;
end;


procedure DecompressBuf(const InBuf: Pointer; InBytes: Integer;
  OutEstimate: Integer; out OutBuf: Pointer; out OutBytes: Integer);
var
  strm: TZStreamRec;
  P: Pointer;
  BufInc: Integer;
begin
  FillChar(strm, sizeof(strm), 0);
  strm.zalloc := zlibAllocMem;
  strm.zfree := zlibFreeMem;
  BufInc := (InBytes + 255) and not 255;
  if OutEstimate = 0 then
    OutBytes := BufInc
  else
    OutBytes := OutEstimate;
  GetMem(OutBuf, OutBytes);
  try
    strm.next_in := InBuf;
    strm.avail_in := InBytes;
    strm.next_out := OutBuf;
    strm.avail_out := OutBytes;
    DCheck(inflateInit_(strm, zlib_version, sizeof(strm)));
    try
      while DCheck(inflate(strm, Z_NO_FLUSH)) <> Z_STREAM_END do
      begin
        P := OutBuf;
        Inc(OutBytes, BufInc);
        ReallocMem(OutBuf, OutBytes);
        strm.next_out := PChar(Integer(OutBuf) + (Integer(strm.next_out) - Integer(P)));
        strm.avail_out := BufInc;
      end;
    finally
      DCheck(inflateEnd(strm));
    end;
    ReallocMem(OutBuf, strm.total_out);
    OutBytes := strm.total_out;
  except
    FreeMem(OutBuf);
    raise
  end;
end;

procedure DecompressToUserBuf(const InBuf: Pointer; InBytes: Integer;
  const OutBuf: Pointer; BufSize: Integer);
var
  strm: TZStreamRec;
begin
  FillChar(strm, sizeof(strm), 0);
  strm.zalloc := zlibAllocMem;
  strm.zfree := zlibFreeMem;
  strm.next_in := InBuf;
  strm.avail_in := InBytes;
  strm.next_out := OutBuf;
  strm.avail_out := BufSize;
  DCheck(inflateInit_(strm, zlib_version, sizeof(strm)));
  try
    if DCheck(inflate(strm, Z_FINISH)) <> Z_STREAM_END then
      raise EZlibError.CreateRes(@sTargetBufferTooSmall);
  finally
    DCheck(inflateEnd(strm));
  end;
end;



constructor TCustomZLibStream.Create(Strm: TStream);
begin
  inherited Create;
  FStrm := Strm;
  FStrmPos := Strm.Position;
  FZRec.zalloc := zlibAllocMem;
  FZRec.zfree := zlibFreeMem;
end;

procedure TCustomZLibStream.Progress(Sender: TObject);
begin
  if Assigned(FOnProgress) then FOnProgress(Sender);
end;




constructor TCompressionStream.Create(CompressionLevel: TCompressionLevel;
  Dest: TStream);
const
  Levels: array [TCompressionLevel] of ShortInt =
    (Z_NO_COMPRESSION, Z_BEST_SPEED, Z_DEFAULT_COMPRESSION, Z_BEST_COMPRESSION);
begin
  inherited Create(Dest);
  FZRec.next_out := FBuffer;
  FZRec.avail_out := sizeof(FBuffer);
  CCheck(deflateInit_(FZRec, Levels[CompressionLevel], zlib_version, sizeof(FZRec)));
end;

destructor TCompressionStream.Destroy;
begin
  FZRec.next_in := nil;
  FZRec.avail_in := 0;
  try
    if FStrm.Position <> FStrmPos then FStrm.Position := FStrmPos;
    while (CCheck(deflate(FZRec, Z_FINISH)) <> Z_STREAM_END)
      and (FZRec.avail_out = 0) do
    begin
      FStrm.WriteBuffer(FBuffer, sizeof(FBuffer));
      FZRec.next_out := FBuffer;
      FZRec.avail_out := sizeof(FBuffer);
    end;
    if FZRec.avail_out < sizeof(FBuffer) then
      FStrm.WriteBuffer(FBuffer, sizeof(FBuffer) - FZRec.avail_out);
  finally
    deflateEnd(FZRec);
  end;
  inherited Destroy;
end;

function TCompressionStream.Read(var Buffer; Count: Longint): Longint;
begin
  raise ECompressionError.CreateRes(@sInvalidStreamOp);
end;

function TCompressionStream.Write(const Buffer; Count: Longint): Longint;
begin
  FZRec.next_in := @Buffer;
  FZRec.avail_in := Count;
  if FStrm.Position <> FStrmPos then FStrm.Position := FStrmPos;
  while (FZRec.avail_in > 0) do
  begin
    CCheck(deflate(FZRec, 0));
    if FZRec.avail_out = 0 then
    begin
      FStrm.WriteBuffer(FBuffer, sizeof(FBuffer));
      FZRec.next_out := FBuffer;
      FZRec.avail_out := sizeof(FBuffer);
      FStrmPos := FStrm.Position;
      Progress(Self);
    end;
  end;
  Result := Count;
end;

function TCompressionStream.Seek(Offset: Longint; Origin: Word): Longint;
begin
  if (Offset = 0) and (Origin = soFromCurrent) then
    Result := FZRec.total_in
  else
    raise ECompressionError.CreateRes(@sInvalidStreamOp);
end;

function TCompressionStream.GetCompressionRate: Single;
begin
  if FZRec.total_in = 0 then
    Result := 0
  else
    Result := (1.0 - (FZRec.total_out / FZRec.total_in)) * 100.0;
end;




constructor TDecompressionStream.Create(Source: TStream);
begin
  inherited Create(Source);
  FZRec.next_in := FBuffer;
  FZRec.avail_in := 0;
  DCheck(inflateInit_(FZRec, zlib_version, sizeof(FZRec)));
end;

destructor TDecompressionStream.Destroy;
begin
  FStrm.Seek(-FZRec.avail_in, 1);
  inflateEnd(FZRec);
  inherited Destroy;
end;

function TDecompressionStream.Read(var Buffer; Count: Longint): Longint;
begin
  FZRec.next_out := @Buffer;
  FZRec.avail_out := Count;
  if FStrm.Position <> FStrmPos then FStrm.Position := FStrmPos;
  while (FZRec.avail_out > 0) do
  begin
    if FZRec.avail_in = 0 then
    begin
      FZRec.avail_in := FStrm.Read(FBuffer, sizeof(FBuffer));
      if FZRec.avail_in = 0 then
      begin
        Result := Count - FZRec.avail_out;
        Exit;
      end;
      FZRec.next_in := FBuffer;
      FStrmPos := FStrm.Position;
      Progress(Self);
    end;
    CCheck(inflate(FZRec, 0));
  end;
  Result := Count;
end;

function TDecompressionStream.Write(const Buffer; Count: Longint): Longint;
begin
  raise EDecompressionError.CreateRes(@sInvalidStreamOp);
end;

function TDecompressionStream.Seek(Offset: Longint; Origin: Word): Longint;
var
  I: Integer;
  Buf: array [0..4095] of Char;
begin
  if (Offset = 0) and (Origin = soFromBeginning) then
  begin
    DCheck(inflateReset(FZRec));
    FZRec.next_in := FBuffer;
    FZRec.avail_in := 0;
    FStrm.Position := 0;
    FStrmPos := 0;
  end
  else if ( (Offset >= 0) and (Origin = soFromCurrent)) or
          ( ((Offset - FZRec.total_out) > 0) and (Origin = soFromBeginning)) then
  begin
    if Origin = soFromBeginning then Dec(Offset, FZRec.total_out);
    if Offset > 0 then
    begin
      for I := 1 to Offset div sizeof(Buf) do
        ReadBuffer(Buf, sizeof(Buf));
      ReadBuffer(Buf, Offset mod sizeof(Buf));
    end;
  end
  else
    raise EDecompressionError.CreateRes(@sInvalidStreamOp);
  Result := FZRec.total_out;
end;


end.
