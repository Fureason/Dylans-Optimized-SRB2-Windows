------------------------------------------------------------------------------
--                      ZLib for Ada thick binding.                         --
--                                                                          --
--              Copyright (C) 2002-2004 Dmitriy Anisimkov                   --
--                                                                          --
--  This library is free software; you can redistribute it and/or modify    --
--  it under the terms of the GNU General Public License as published by    --
--  the Free Software Foundation; either version 2 of the License, or (at   --
--  your option) any later version.                                         --
--                                                                          --
--  This library is distributed in the hope that it will be useful, but     --
--  WITHOUT ANY WARRANTY; without even the implied warranty of              --
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       --
--  General Public License for more details.                                --
--                                                                          --
--  You should have received a copy of the GNU General Public License       --
--  along with this library; if not, write to the Free Software Foundation, --
--  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.          --
--                                                                          --
--  As a special exception, if other files instantiate generics from this   --
--  unit, or you link this unit with other files to produce an executable,  --
--  this  unit  does not  by itself cause  the resulting executable to be   --
--  covered by the GNU General Public License. This exception does not      --
--  however invalidate any other reasons why the executable file  might be  --
--  covered by the  GNU Public License.                                     --
------------------------------------------------------------------------------



with Ada.Streams;

with Interfaces;

package ZLib is

   ZLib_Error   : exception;
   Status_Error : exception;

   type Compression_Level is new Integer range -1 .. 9;

   type Flush_Mode is private;

   type Compression_Method is private;

   type Window_Bits_Type is new Integer range 8 .. 15;

   type Memory_Level_Type is new Integer range 1 .. 9;

   type Unsigned_32 is new Interfaces.Unsigned_32;

   type Strategy_Type is private;

   type Header_Type is (None, Auto, Default, GZip);



   subtype Count is Ada.Streams.Stream_Element_Count;

   Default_Memory_Level : constant Memory_Level_Type := 8;
   Default_Window_Bits  : constant Window_Bits_Type  := 15;





   Deflated : constant Compression_Method;






   No_Compression      : constant Compression_Level := 0;
   Best_Speed          : constant Compression_Level := 1;
   Best_Compression    : constant Compression_Level := 9;
   Default_Compression : constant Compression_Level := -1;





   No_Flush      : constant Flush_Mode;


   Partial_Flush : constant Flush_Mode;


   Sync_Flush    : constant Flush_Mode;







   Block_Flush   : constant Flush_Mode;







   Full_Flush    : constant Flush_Mode;





   Finish        : constant Flush_Mode;








   Filtered         : constant Strategy_Type;
   Huffman_Only     : constant Strategy_Type;
   RLE              : constant Strategy_Type;
   Default_Strategy : constant Strategy_Type;

   Default_Buffer_Size : constant := 4096;

   type Filter_Type is tagged limited private;



   function Version return String;
   pragma Inline (Version);


   procedure Deflate_Init
     (Filter       : in out Filter_Type;
      Level        : in     Compression_Level  := Default_Compression;
      Strategy     : in     Strategy_Type      := Default_Strategy;
      Method       : in     Compression_Method := Deflated;
      Window_Bits  : in     Window_Bits_Type   := Default_Window_Bits;
      Memory_Level : in     Memory_Level_Type  := Default_Memory_Level;
      Header       : in     Header_Type        := Default);







   procedure Inflate_Init
     (Filter      : in out Filter_Type;
      Window_Bits : in     Window_Bits_Type := Default_Window_Bits;
      Header      : in     Header_Type      := Default);












   function Is_Open (Filter : in Filter_Type) return Boolean;
   pragma Inline (Is_Open);


   procedure Close
     (Filter       : in out Filter_Type;
      Ignore_Error : in     Boolean := False);




   generic
      with procedure Data_In
        (Item : out Ada.Streams.Stream_Element_Array;
         Last : out Ada.Streams.Stream_Element_Offset);
      with procedure Data_Out
        (Item : in Ada.Streams.Stream_Element_Array);
   procedure Generic_Translate
     (Filter          : in out Filter_Type;
      In_Buffer_Size  : in     Integer := Default_Buffer_Size;
      Out_Buffer_Size : in     Integer := Default_Buffer_Size);





   function Total_In (Filter : in Filter_Type) return Count;
   pragma Inline (Total_In);


   function Total_Out (Filter : in Filter_Type) return Count;
   pragma Inline (Total_Out);


   function CRC32
     (CRC    : in Unsigned_32;
      Data   : in Ada.Streams.Stream_Element_Array)
      return Unsigned_32;
   pragma Inline (CRC32);


   procedure CRC32
     (CRC  : in out Unsigned_32;
      Data : in     Ada.Streams.Stream_Element_Array);
   pragma Inline (CRC32);






   procedure Translate
     (Filter    : in out Filter_Type;
      In_Data   : in     Ada.Streams.Stream_Element_Array;
      In_Last   :    out Ada.Streams.Stream_Element_Offset;
      Out_Data  :    out Ada.Streams.Stream_Element_Array;
      Out_Last  :    out Ada.Streams.Stream_Element_Offset;
      Flush     : in     Flush_Mode);






   function Stream_End (Filter : in Filter_Type) return Boolean;
   pragma Inline (Stream_End);


   procedure Flush
     (Filter    : in out Filter_Type;
      Out_Data  :    out Ada.Streams.Stream_Element_Array;
      Out_Last  :    out Ada.Streams.Stream_Element_Offset;
      Flush     : in     Flush_Mode);
   pragma Inline (Flush);


   generic
      with procedure Write
        (Item : in Ada.Streams.Stream_Element_Array);



      Buffer_Size : in Ada.Streams.Stream_Element_Offset
         := Default_Buffer_Size;


   procedure Write
     (Filter  : in out Filter_Type;
      Item    : in     Ada.Streams.Stream_Element_Array;
      Flush   : in     Flush_Mode := No_Flush);



   generic
      with procedure Read
        (Item : out Ada.Streams.Stream_Element_Array;
         Last : out Ada.Streams.Stream_Element_Offset);



      Buffer : in out Ada.Streams.Stream_Element_Array;



      Rest_First, Rest_Last : in out Ada.Streams.Stream_Element_Offset;




      Allow_Read_Some : in Boolean := False;


   procedure Read
     (Filter : in out Filter_Type;
      Item   :    out Ada.Streams.Stream_Element_Array;
      Last   :    out Ada.Streams.Stream_Element_Offset;
      Flush  : in     Flush_Mode := No_Flush);





private

   use Ada.Streams;

   pragma Assert (Ada.Streams.Stream_Element'Size    =    8);
   pragma Assert (Ada.Streams.Stream_Element'Modulus = 2**8);

   type Flush_Mode is new Integer range 0 .. 5;

   type Compression_Method is new Integer range 8 .. 8;

   type Strategy_Type is new Integer range 0 .. 3;

   No_Flush      : constant Flush_Mode := 0;
   Partial_Flush : constant Flush_Mode := 1;
   Sync_Flush    : constant Flush_Mode := 2;
   Full_Flush    : constant Flush_Mode := 3;
   Finish        : constant Flush_Mode := 4;
   Block_Flush   : constant Flush_Mode := 5;

   Filtered         : constant Strategy_Type := 1;
   Huffman_Only     : constant Strategy_Type := 2;
   RLE              : constant Strategy_Type := 3;
   Default_Strategy : constant Strategy_Type := 0;

   Deflated : constant Compression_Method := 8;

   type Z_Stream;

   type Z_Stream_Access is access all Z_Stream;

   type Filter_Type is tagged limited record
      Strm        : Z_Stream_Access;
      Compression : Boolean;
      Stream_End  : Boolean;
      Header      : Header_Type;
      CRC         : Unsigned_32;
      Offset      : Stream_Element_Offset;

   end record;

end ZLib;
